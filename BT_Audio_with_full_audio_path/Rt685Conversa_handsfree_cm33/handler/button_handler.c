/*
 * button_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#include "button_handler.h"
#include "i2c_component_handler.h"
#include "spi_handler.h"
#include "WorkStateManager.h"

/* ===== external SPI handlers ===== */
extern QueueHandle_t      spi_request_queue;
extern EventGroupHandle_t spi_event_group;
extern SemaphoreHandle_t  spi_semaphore;

/* ===== unified event bits for a single task ===== */
#define FUNK_NOTIFY_EDGE  (1UL << 0)  /* FunKey edge from PINT ISR */
#define FUNK_NOTIFY_DBL   (1UL << 1)  /* FunKey double-click timer expiry */
#define PWR_NOTIFY_EDGE   (1UL << 2)  /* PowerKey edge from PINT ISR */

/* === 單一任務的 handle 與 FunKey 的軟體定時器 === */
static TaskHandle_t  sKeysTaskHandle = NULL;   /* unified task handle (button_key) */
static TimerHandle_t sBtnDblTimer    = NULL;   /* FunKey only */

/* 簡單阻塞式 delay：使用 NXP SDK，依核心時脈做最少延遲 */
static inline void delay_ms(uint32_t ms)
{
    SDK_DelayAtLeastUs(ms * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk));
}

/* 檢查按鍵是否連續維持低電位（active-low）達指定毫秒數：for pca9422 ship mode */
bool power_key_low_for_ms(uint32_t ms)
{
    uint32_t elapsed = 0U;
    while (elapsed < ms)
    {
        /* 低電位＝按下（active-low 假設） */
        if (GPIO_PinRead(GPIO, NXP_BQ_MR_N_PORT, NXP_BQ_MR_N_PIN) != 0U)
        {
            return false; /* 中途釋放，長按失敗 */
        }
        delay_ms(SAMPLE_MS);
        elapsed += SAMPLE_MS;
    }
    return true; /* 全程保持低電位達指定時間 */
}

/* PINT ISR 回呼：將不同來源的中斷以不同 bit 通知同一個任務 */
void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status)
{
    BaseType_t xHPW = pdFALSE;
    /* FUN_KEY1：通知 unified 任務 */
    if ((pintr == FUN_KEY1_PINT_CH) && (sKeysTaskHandle != NULL))
    {
        (void)xTaskNotifyFromISR(sKeysTaskHandle, FUNK_NOTIFY_EDGE, eSetBits, &xHPW);
    }
    /* PowerKey：通知 unified 任務 */
    if ((pintr == POWER_KEY_PINT_CH) && (sKeysTaskHandle != NULL))
    {
        (void)xTaskNotifyFromISR(sKeysTaskHandle, PWR_NOTIFY_EDGE, eSetBits, &xHPW);
    }
    portYIELD_FROM_ISR(xHPW);
}

/* 讀腳位：PowerKey */
static inline uint8_t pwr_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, NXP_BQ_MR_N_PORT, NXP_BQ_MR_N_PIN);
}

/* 讀腳位：FunKey */
static inline uint8_t btn_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, FUN_KEY_PORT, FUN_KEY_PIN);
}

/* FunKey 雙擊計時器回呼：只通知 unified 任務 */
static void vBtnDblTimerCb(TimerHandle_t xTimer)
{
    if (sKeysTaskHandle)
    {
        (void)xTaskNotify(sKeysTaskHandle, FUNK_NOTIFY_DBL, eSetBits);
    }
}

/**
 * @brief Unified button_key task (FunKey + PowerKey)
 * @details 按鍵處理任務（生產者）：
 * - FunKey：短按、長按、雙擊（雙擊以單次 Timer 視窗判定）。
 * - PowerKey：短按、長按（含最小短按門檻）。
 * - SPI 事件沿用既有佇列，LED/AMP 行為保留。
 */
void button_task(void *pvParameters)
{
    /* -------- FunKey thresholds -------- */
    const TickType_t btn_debounceTicks = pdMS_TO_TICKS(BTN_DEBOUNCE_MS);
    const TickType_t btn_longTicks     = pdMS_TO_TICKS(BTN_LONG_MS);

    sKeysTaskHandle = xTaskGetCurrentTaskHandle();

    /* FunKey 的雙擊單次定時器 */
    sBtnDblTimer = xTimerCreate("btn_dbl",
                                pdMS_TO_TICKS(BTN_DBLCLICK_GAP_MS),
                                pdFALSE,
                                NULL,
                                vBtnDblTimerCb);
    configASSERT(sBtnDblTimer);

    /* -------- FunKey state -------- */
    uint8_t    btn_stable_level      = btn_raw_read();
    bool       btn_is_pressed        = (btn_stable_level == BTN_ACTIVE_LEVEL);
    bool       btn_dbl_pending       = false; /* 是否正在等待第二次短按 */
    TickType_t btn_press_start_tick  = 0;
    if (btn_is_pressed)
    {
        /* 上電時剛好按住：當作剛按下 */
        btn_press_start_tick = xTaskGetTickCount();
    }

    /* -------- PowerKey thresholds & state -------- */
    const TickType_t pwr_debounceTicks = pdMS_TO_TICKS(PWR_DEBOUNCE_MS);
    const TickType_t pwr_longTicks     = pdMS_TO_TICKS(PWR_LONG_MS);
    const TickType_t pwr_minShortTicks = pdMS_TO_TICKS(PWR_MIN_SHORT_MS);

    uint8_t    pwr_stable_level      = pwr_raw_read();
    bool       pwr_is_pressed        = (pwr_stable_level == PWR_ACTIVE_LEVEL);
    TickType_t pwr_press_start_tick  = 0;
    if (pwr_is_pressed)
    {
        /* 上電時剛好被按住：當作剛按下，等待放開後再判斷是否為長按 */
        pwr_press_start_tick = xTaskGetTickCount();
    }

    for (;;)
    {
        uint32_t notifyBits = 0;
        (void)xTaskNotifyWait(0, 0xFFFFFFFFu, &notifyBits, portMAX_DELAY);

        /* A) FunKey 雙擊窗口到期：若仍在等待第二下且目前沒有按住 → 單擊成立 */
        if (notifyBits & FUNK_NOTIFY_DBL)
        {
            if (btn_dbl_pending && !btn_is_pressed)
            {
                btn_dbl_pending = false;
                PRINTF("[Button] Short Press detected.\r\n");
                PRINTF("[Button] Short Press detected. Sending 0x%02X\r\n", SHORT_PRESS_HEX_VALUE);
#if SOC_SPI_ENABLE
                send_spi_request(SHORT_PRESS_HEX_VALUE);
#endif
                /* amp_post_event(AMP_EVT_MUSIC_START); // test amp */
            }
            /* 若此時已經在第二次按壓中（btn_is_pressed==true），不回報短按，
             * 等放開時再判斷是雙擊或長按（第二次按壓可能超過長按門檻）。 */
        }

        /* B) FunKey 邊緣事件：去抖後判斷按下/放開 */
        if (notifyBits & FUNK_NOTIFY_EDGE)
        {
            (void)btn_raw_read(); /* l1 可視需要使用 */
            vTaskDelay(btn_debounceTicks);
            uint8_t new_level = btn_raw_read();
            if (new_level != btn_stable_level)
            {
                btn_stable_level = new_level;
                bool       now_pressed = (btn_stable_level == BTN_ACTIVE_LEVEL);
                TickType_t now         = xTaskGetTickCount();

                if (now_pressed && !btn_is_pressed)
                {
                    /* 放開 -> 按下：記錄開始時間 */
                    btn_is_pressed       = true;
                    btn_press_start_tick = now;
                    /* 不動 btn_dbl_pending，讓第二次按壓可以覆蓋成雙擊或長按 */
                }
                else if (!now_pressed && btn_is_pressed)
                {
                    /* 按下 -> 放開：在此刻才判定長按/短按/雙擊 */
                    btn_is_pressed = false;
                    TickType_t press_dur = now - btn_press_start_tick;

                    if (press_dur >= btn_longTicks)
                    {
                        /* 長按（放開才觸發）→ 最高優先權 */
                        PRINTF("[Button] Long Press (on-release) detected. \r\n");
                        PRINTF("[Button] Long Press (on-release) detected. Sending 0x%02X\r\n",
                               LONG_PRESS_HEX_VALUE);
#if SOC_SPI_ENABLE
                        send_spi_request(LONG_PRESS_HEX_VALUE);
#endif
                        /* 任何待定的單擊作廢 */
                        btn_dbl_pending = false;
                        (void)xTimerStop(sBtnDblTimer, 0);
                    }
                    else
                    {
                        /* 未達長按：處理短按/雙擊 */
                        if (btn_dbl_pending)
                        {
                            /* 第二次在時間窗內完成 → 雙擊 */
                            btn_dbl_pending = false;
                            (void)xTimerStop(sBtnDblTimer, 0);
                            PRINTF("[Button] Double Click detected.\r\n");
                            /* 如需送 SPI：解開下面三行
                             * PRINTF("[Button] Double Click detected. Sending 0x%02X\r\n", DOUBLE_CLICK_HEX_VALUE);
                             * uint8_t vv = DOUBLE_CLICK_HEX_VALUE;
                             * (void)xQueueSend(spi_request_queue, &vv, 0);
                             */
                            /* amp_post_event(AMP_EVT_RECEIVER_START); // test amp */
                        }
                        else
                        {
                            /* 第一次短按：開窗等第二下 */
                            btn_dbl_pending = true;
                            (void)xTimerStop(sBtnDblTimer, 0);
                            (void)xTimerStart(sBtnDblTimer, 0);
                        }
                    }
                }
            }
        } /* FunKey EDGE */

        /* C) PowerKey 邊緣事件：去抖後判斷按下/放開 */
        if (notifyBits & PWR_NOTIFY_EDGE)
        {
            (void)pwr_raw_read(); /* l1 可視需要使用 */
            vTaskDelay(pwr_debounceTicks);
            uint8_t new_level = pwr_raw_read();
            if (new_level != pwr_stable_level)
            {
                pwr_stable_level = new_level;
                bool       now_pressed = (pwr_stable_level == PWR_ACTIVE_LEVEL);
                TickType_t now         = xTaskGetTickCount();

                if (now_pressed && !pwr_is_pressed)
                {
                    /* 放開 -> 按下：記錄開始時間 */
                    pwr_is_pressed       = true;
                    pwr_press_start_tick = now;
                }
                else if (!now_pressed && pwr_is_pressed)
                {
                    /* 按下 -> 放開：在放開此刻判斷短按或長按 */
                    pwr_is_pressed = false;
                    TickType_t press_dur = now - pwr_press_start_tick;

                    if (press_dur >= pwr_longTicks)
                    {
                        /* 長按（放開才觸發） */
                        PRINTF("[PWR] Long Press (>=%ums) detected.\r\n", (unsigned)PWR_LONG_MS);
#if SOC_SPI_ENABLE
                        send_spi_request(POWER_LONG_PRESS_HEX_VALUE);
#endif
                        led_post_event(LED_EVT_POWER_OFF_PROGRESS);
                        /* amp_post_event(AMP_EVT_MUSIC_START); // test amp */
                    }
                    else if (press_dur >= pwr_minShortTicks)
                    {
                        /* 短按 */
                        PRINTF("[PWR] Short Press detected.\r\n");
#if SOC_SPI_ENABLE
#if 1
                        send_spi_request(POWER_SHORT_PRESS_HEX_VALUE);
#else // use button to switch state
//                        RequestToGetIntoMediaPlayer = 1;
                        RequestToGetIntoTranslation = 1;
#endif
#endif
                        /* reg_led++; */
                        /* led_post_event(reg_led); // test led */
                        /* amp_post_event(AMP_EVT_STOP); // test amp */
                    }
                    else
                    {
                        /* 小於最小短按時間：視為抖動/誤觸，忽略 */
                        /* no-op */
                    }
                }
            }
        } /* PowerKey EDGE */
    } /* for (;;) */
}

