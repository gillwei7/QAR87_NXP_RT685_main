/*
 * button_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */


#include "button_handler.h"
#include "i2c_component_handler.h"
#include "spi_handler.h"

/* ===== external SPI handlers ===== */
extern QueueHandle_t spi_request_queue ;
extern EventGroupHandle_t spi_event_group ;
extern SemaphoreHandle_t spi_semaphore ;

/* event bits */
#define BTN_NOTIFY_EDGE   (1UL << 0)  // In sButtonTaskHandle, double edge event from FunKey PINT ISR,
#define BTN_NOTIFY_DBL    (1UL << 1)  // In sButtonTaskHandle, double-click the timer when it expires.
#define PWR_NOTIFY_EDGE   (1UL << 0)  // In sPowerKeyTaskHandle, edge events from the PowerKey PINT ISR

/* === 按鈕任務的 handle 與軟體定時器 === */
static TaskHandle_t   sButtonTaskHandle = NULL;
static TimerHandle_t  sBtnDblTimer  = NULL;
static TaskHandle_t sPowerKeyTaskHandle = NULL;

/* 簡單阻塞式 delay：使用 NXP SDK，依核心時脈做最少延遲 */
static inline void delay_ms(uint32_t ms)
{
    SDK_DelayAtLeastUs(ms * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk));
}

/* 檢查按鍵是否連續維持低電位（active-low）達指定毫秒數 */
bool power_key_low_for_ms(uint32_t ms) //for pca9422 ship mode
{
    uint32_t elapsed = 0U;

    while (elapsed < ms)
    {
        /* 低電位＝按下（active-low 假設） */
        if (GPIO_PinRead(GPIO, POWER_KEY_PORT, POWER_KEY_PIN) != 0U)
        {
            return false;  // 中途釋放，長按失敗
        }
        delay_ms(SAMPLE_MS);
        elapsed += SAMPLE_MS;
    }
    return true; // 全程保持低電位達指定時間
}

void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status)
{

    BaseType_t xHPW = pdFALSE;

    /* FUN_KEY1：通知 button 任務 */
    if ((pintr == FUN_KEY1_PINT_CH) && (sButtonTaskHandle != NULL)) {
        (void)xTaskNotifyFromISR(sButtonTaskHandle, BTN_NOTIFY_EDGE, eSetBits, &xHPW);
    }

    /* PowerKey：通知 power key 任務 */
    if ((pintr == POWER_KEY_PINT_CH) && (sPowerKeyTaskHandle != NULL)) {
        (void)xTaskNotifyFromISR(sPowerKeyTaskHandle, PWR_NOTIFY_EDGE, eSetBits, &xHPW);
    }

    portYIELD_FROM_ISR(xHPW);


	/*
    PRINTF("\f\r\nPINT Pin Interrupt %d event detected.\r\n", pintr);
    uint8_t pin_state;
    if(pintr==0)
    {
    	pin_state = GPIO_PinRead(GPIO,POWER_KEY_PORT,POWER_KEY_PIN);

    }
    else if(pintr==1)
    {
    	pin_state = GPIO_PinRead(GPIO,FUN_KEY1_N_PORT,FUN_KEY1_N_PIN);
    }
    PRINTF(" pin_state:%d \r\n",pin_state);
    */
}

/* 讀腳位 */
static inline uint8_t pwr_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, POWER_KEY_PORT, POWER_KEY_PIN);
}

/* 讀腳位 */
static inline uint8_t btn_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, FUN_KEY1_N_PORT, FUN_KEY1_N_PIN);
}

/* 雙擊計時器回呼：只通知任務 */
static void vBtnDblTimerCb(TimerHandle_t xTimer)
{
    if (sButtonTaskHandle) {
        (void)xTaskNotify(sButtonTaskHandle, BTN_NOTIFY_DBL, eSetBits);
    }
}

/**
 * @brief 按鈕處理任務 (生產者)。
 * @details 此任務持續監控一個 GPIO 按鈕的狀態，並能區分短按和長按。
 * 當檢測到有效的按鍵事件時，它會將對應的十六進制值 (SHORT_PRESS_HEX_VALUE 或 LONG_PRESS_HEX_VALUE)
 * 作為一個請求發送到 `spi_request_queue` 消息佇列中，以觸發 SPI 傳輸。
 */
void button_task(void *pvParameters)
{
	   const TickType_t debounceTicks   = pdMS_TO_TICKS(BTN_DEBOUNCE_MS);
	   const TickType_t longTicks       = pdMS_TO_TICKS(BTN_LONG_MS);

	    sButtonTaskHandle = xTaskGetCurrentTaskHandle();

	    /* 只需要「雙擊」單次定時器 */
	    sBtnDblTimer  = xTimerCreate("btn_dbl", pdMS_TO_TICKS(BTN_DBLCLICK_GAP_MS),
	                                 pdFALSE, NULL, vBtnDblTimerCb);
	    configASSERT(sBtnDblTimer);

	    /* 初始狀態 */
	    uint8_t stable_level = btn_raw_read();
	    bool    is_pressed   = (stable_level == BTN_ACTIVE_LEVEL);
	    bool    dbl_pending  = false;            // 是否正在等待第二次短按
	    TickType_t press_start_tick = 0;

	    if (is_pressed) {
	        // 上電時剛好按住：當作剛按下
	        press_start_tick = xTaskGetTickCount();
	    }

	    for (;;)
	    {
	        uint32_t notifyBits = 0;
	        (void)xTaskNotifyWait(0, 0xFFFFFFFFu, &notifyBits, portMAX_DELAY);

	        /* A) 雙擊窗口到期：若仍在等待第二下且目前沒有按住 → 單擊成立 */
	        if (notifyBits & BTN_NOTIFY_DBL) {
	            if (dbl_pending && !is_pressed) {
	                dbl_pending = false;
	                PRINTF("[Button] Short Press detected.\r\n");
	                PRINTF("[Button] Short Press detected. Sending 0x%02X\r\n", SHORT_PRESS_HEX_VALUE);
	                uint8_t v = SHORT_PRESS_HEX_VALUE;
	                (void)xQueueSend(spi_request_queue, &v, 0);

	                //amp_post_event(AMP_EVT_MUSIC_START); test amp
	            }
	            // 若此時已經在第二次按壓中（is_pressed==true），不回報短按，
	            // 等放開時再判斷是雙擊或長按（第二次按壓可能超過 1 秒而成為長按）
	        }

	        /* B) 來自 PINT 的邊緣事件：去抖後判斷按下/放開 */
	        if (notifyBits & BTN_NOTIFY_EDGE) {
	            uint8_t l1 = btn_raw_read();
	            vTaskDelay(debounceTicks);
	            uint8_t l2 = btn_raw_read();
	            uint8_t new_level = l2;

	            if (new_level != stable_level) {
	                stable_level = new_level;
	                bool now_pressed = (stable_level == BTN_ACTIVE_LEVEL);
	                TickType_t now = xTaskGetTickCount();

	                if (now_pressed && !is_pressed) {
	                    /* 放開 -> 按下：記錄開始時間 */
	                    is_pressed = true;
	                    press_start_tick = now;
	                    // 不動 dbl_pending，讓第二次按壓可以覆蓋成「雙擊或長按」
	                }
	                else if (!now_pressed && is_pressed) {
	                    /* 按下 -> 放開：在此刻才判定長按/短按/雙擊 */
	                    is_pressed = false;
	                    TickType_t press_dur = now - press_start_tick;

	                    if (press_dur >= longTicks) {
	                        /* 長按（放開才觸發）→ 最高優先權 */
	                        PRINTF("[Button] Long Press (on-release) detected. \r\n");
	                        PRINTF("[Button] Long Press (on-release) detected. Sending 0x%02X\r\n",
	                               LONG_PRESS_HEX_VALUE);
	                        uint8_t v = LONG_PRESS_HEX_VALUE;
	                        (void)xQueueSend(spi_request_queue, &v, 0);

	                        /* 任何待定的單擊作廢 */
	                        dbl_pending = false;
	                        (void)xTimerStop(sBtnDblTimer, 0);
	                    } else {
	                        /* 未達長按：處理短按/雙擊 */
	                        if (dbl_pending) {
	                            /* 第二次在時間窗內完成 → 雙擊 */
	                            dbl_pending = false;
	                            (void)xTimerStop(sBtnDblTimer, 0);
	                            PRINTF("[Button] Double Click detected.\r\n");
	                            //PRINTF("[Button] Double Click detected. Sending 0x%02X\r\n",
	                            //      DOUBLE_CLICK_HEX_VALUE);
	                            //uint8_t v = DOUBLE_CLICK_HEX_VALUE;
	                            //(void)xQueueSend(spi_request_queue, &v, 0);
	                            //amp_post_event(AMP_EVT_RECEIVER_START); //test amp
	                        } else {
	                            /* 第一次短按：開窗等第二下 */
	                            dbl_pending = true;
	                            (void)xTimerStop(sBtnDblTimer, 0);
	                            (void)xTimerStart(sBtnDblTimer, 0);
	                        }
	                    }
	                }
	            } // level changed
	        } // EDGE
	    } // for

}


void power_key_task(void *pvParameters)
{

    const TickType_t debounceTicks = pdMS_TO_TICKS(PWR_DEBOUNCE_MS);
    const TickType_t longTicks     = pdMS_TO_TICKS(PWR_LONG_MS);
    const TickType_t minShortTicks = pdMS_TO_TICKS(PWR_MIN_SHORT_MS);

    sPowerKeyTaskHandle = xTaskGetCurrentTaskHandle();

    /* 取當前穩定狀態 */
    uint8_t stable_level = pwr_raw_read();
    bool    is_pressed   = (stable_level == PWR_ACTIVE_LEVEL);
    TickType_t press_start_tick = 0;

    if (is_pressed) {
        /* 上電時剛好被按住：當作剛按下，等待放開後再判斷是否為長按 */
        press_start_tick = xTaskGetTickCount();
    }

    for (;;)
    {
        uint32_t notifyBits = 0;
        (void)xTaskNotifyWait(0, 0xFFFFFFFFu, &notifyBits, portMAX_DELAY);

        if (notifyBits & PWR_NOTIFY_EDGE) {
            /* 軟體去抖：延遲後再讀取一次，採用第二次結果 */
            uint8_t l1 = pwr_raw_read();
            (void)l1; // 可視需要使用
            vTaskDelay(debounceTicks);
            uint8_t l2 = pwr_raw_read();
            uint8_t new_level = l2;

            if (new_level != stable_level) {
                stable_level = new_level;
                bool now_pressed = (stable_level == PWR_ACTIVE_LEVEL);
                TickType_t now = xTaskGetTickCount();

                if (now_pressed && !is_pressed) {
                    /* 放開 -> 按下：記錄開始時間 */
                    is_pressed = true;
                    press_start_tick = now;
                }
                else if (!now_pressed && is_pressed) {
                    /* 按下 -> 放開：在放開此刻判斷短按或長按 */
                    is_pressed = false;

                    TickType_t press_dur = now - press_start_tick;

                    if (press_dur >= longTicks) {
                        /* 長按（放開才觸發） */
                        PRINTF("[PWR] Long Press (>=%ums) detected.\r\n",(unsigned)PWR_LONG_MS);

                        led_post_event(LED_EVT_POWER_OFF_PROGRESS);
                        //amp_post_event(AMP_EVT_MUSIC_START); //test amp

                    } else if (press_dur >= minShortTicks) {
                        /* 短按 */
                        PRINTF("[PWR] Short Press detected.\r\n");
                        //reg_led++;
                        //led_post_event(reg_led);      //test led
                        //amp_post_event(AMP_EVT_STOP); //test amp

                    } else {
                        /* 小於最小短按時間：視為抖動/誤觸，忽略 */
                        // no-op
                    }
                }
                /* 其他情況（例如重複相同邏輯電平）不需處理 */
            }
        }
    }
}
