/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"
#include "fsl_pint.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"
#include "pmic_support.h"
#include "pmic_pca9422.h"
#include "glf70583.h"
#include "aw933xx.h"
#include "aw93305.h"
#include "bq256xx_charger.h"
#include "glf70302.h"
#include "ktd202x_leds.h"
#include "aw88166.h"

#include "spi_handler.h"
#include "led_status.h"

#include <string.h>
#include <stdlib.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USE_EVENT 1
#define USE_SEMAPHORE 0

#define SCAN_I2C_ADDRESS_ENABLE 1


/* event bits */
#define BTN_NOTIFY_EDGE   (1UL << 0)  // 在sButtonTaskHandle，來自 FunKey PINT ISR 的雙邊緣事件，
#define BTN_NOTIFY_DBL    (1UL << 1)  // 在sButtonTaskHandle，雙擊計時器到時，
#define PWR_NOTIFY_EDGE   (1UL << 0)  // 在sPowerKeyTaskHandle，來自 PowerKey PINT ISR 的邊緣事件

/* === 按鍵行為參數 === */
#define BTN_ACTIVE_LEVEL          0   // active-low: 按下=0，放開=1；若相反改為 1
#define BTN_DEBOUNCE_MS          30
#define BTN_LONG_MS            1000
#define BTN_DBLCLICK_GAP_MS     300
#define PWR_ACTIVE_LEVEL          0      // active-low: 按下=0；若為 active-high 改為 1
#define PWR_DEBOUNCE_MS          30      // 軟體去抖
#define PWR_LONG_MS            3000      // 長按 3 秒（放開才判斷）
#define PWR_MIN_SHORT_MS         50      // 太短的抖動/誤觸（<50ms）不算短按
#define POWER_KEY_PINT_CH  		  0   // PINT 通道
#define FUN_KEY1_PINT_CH  		  1   // PINT 通道
/* === 按鈕任務的 handle 與軟體定時器 === */
static TaskHandle_t   sButtonTaskHandle = NULL;
static TimerHandle_t  sBtnDblTimer  = NULL;
static TaskHandle_t sPowerKeyTaskHandle = NULL;

/*****************************************************************************************************/
/* ===== I2C EventGroup bits (for unified I2C_Task) ===== */
#define TOUCH_EVENT_BIT      (1UL << 0)
#define CHARGER_EVENT_BIT    (1UL << 1)
#define GAUGE_EVENT_BIT      (1UL << 2)
#define LED_EVENT_BIT        (1UL << 3)   /* LED task wake-up flag */

BatteryInfo battery;
/* ===== I2C synchronization objects ===== */
static EventGroupHandle_t i2c_event_group = NULL;
static SemaphoreHandle_t  i2c_mutex       = NULL;
static TaskHandle_t       sI2CTaskHandle  = NULL;

/* ===== I2C task prototype & external device handlers ===== */
static void I2C_Task(void *pvParameters);

extern volatile struct aw933xx_dev aw933xx;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void button_task(void *pvParameters);

/* ===== external SPI handlers ===== */
extern QueueHandle_t spi_request_queue ;
extern EventGroupHandle_t spi_event_group ;
extern SemaphoreHandle_t spi_semaphore ;

/*===== LED handlers =====*/
TaskHandle_t sLEDTaskHandle;
volatile led_event_t g_led_event = LED_EVT_NONE;

uint8_t reg_led =0;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void led_post_event(led_event_t e)
{
    g_led_event = e;
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, LED_EVENT_BIT);

    }
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
static void button_task(void *pvParameters)
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


static void power_key_task(void *pvParameters)
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

                    } else if (press_dur >= minShortTicks) {
                        /* 短按 */
                        PRINTF("[PWR] Short Press detected.\r\n");
                        reg_led++;
                        led_post_event(reg_led);
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


static void I2C_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(
            i2c_event_group,
            TOUCH_EVENT_BIT | CHARGER_EVENT_BIT | GAUGE_EVENT_BIT | LED_EVENT_BIT,
            pdTRUE,     /* clear on exit */
            pdFALSE,    /* wait for any bit */
            portMAX_DELAY);

        /* --- TOUCH event --- */
        if ((bits & TOUCH_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
            	AW93305_EXTI_Callback();
                xSemaphoreGive(i2c_mutex);

                if(aw933xx.event.click >0)
                {
                	unsigned int btn_event = aw933xx.event.click;
                	PRINTF("[Touch] click= %d \n",btn_event);
                	if(btn_event==1)
                	{
                		uint8_t v = ONE_TOUCH_HEX_VALUE;
                		(void)xQueueSend(spi_request_queue, &v, 0);
                	}
                	else if(btn_event==2)
                	{
                		uint8_t v = DOUBLE_TOUCH_HEX_VALUE;
                		(void)xQueueSend(spi_request_queue, &v, 0);
                	}

                }
                else if(aw933xx.event.press)
                {
                	PRINTF("[Touch] press \n");
                }
                else if(aw933xx.event.long_press)
                {
                	PRINTF("[Touch] long_press \n");
                }
                else if(aw933xx.event.super_long_press)
                {
                	PRINTF("[Touch] super_long_press \n");
                }
                else if(aw933xx.event.right_wareds)
                {
                	PRINTF("[Touch] slide_right \n");
                }
                else if(aw933xx.event.left_wareds)
                {
                	PRINTF("[Touch] slide_left \n");
                }

            }

            /* 任務側重新啟用觸控中斷（先清旗標再開） */
            GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
        }

        /* --- CHARGER event --- */
        if ((bits & CHARGER_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
    			bq256xx_status_t status;
    			if (bq256xx_poll_status(&status) == kStatus_Success) {
    				PRINTF("[Charger] Power Good: %s\n", status.power_good ? "Yes" : "No");
    				PRINTF("[Charger] VBUS Status: 0x%02X\n", status.vbus_stat);
    				PRINTF("[Charger] Charge Status: 0x%02X\n", status.chg_stat);
    				PRINTF("[Charger] Fault Status: 0x%02X\n", status.fault_stat);
    				PRINTF("[Charger] VBUS Good: %s\n", status.vbus_good ? "Yes" : "No");
    				PRINTF("\n");
    			} else {
    				PRINTF("[Charger] Failed to read charger status.\n");
    			}
                xSemaphoreGive(i2c_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);

        }

        /* --- GAUGE event --- */
        if ((bits & GAUGE_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
            	glf70302_read_battery(&battery);
                xSemaphoreGive(i2c_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

        }

        /*--- LED event --- */
        if ((bits & LED_EVENT_BIT) != 0) {
                vTaskDelay(1); /* 確保 g_led_event 已更新 */
                led_event_t evt = g_led_event;

                if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                    /* 根據事件控制 LED */
                    switch (evt) {
                    case LED_EVT_POWER_ON_PROGRESS:
                    	ktd202x_led_off();
                        ktd202x_ch4_led_on(LED_ON);
                        break;
                    case LED_EVT_POWER_OFF_PROGRESS:
                    	ktd202x_led_off();
                    	ktd202x_ch2_led_on(LED_ON);
                    	vTaskDelay(1000);
                    	ktd202x_ch2_led_off();
                        break;
                    case LED_EVT_CHARGING:
                    	ktd202x_led_off();
                        ktd202x_ch2_led_blink(500, 500, TIM_1);
                        break;
                    case LED_EVT_LOW_BATTERY:
                    	ktd202x_led_off();
                        ktd202x_ch2_led_blink(500, 4500, TIM_1);
                        break;
                    case LED_EVT_FULL_CHARGERED:
                    	ktd202x_led_off();
                    	ktd202x_ch3_led_on(LED_ON);
                        break;
                    case LED_EVT_PHOTO_CAPTURE:
                    	ktd202x_led_off();
                    	ktd202x_ch4_led_on(LED_ON);
                    	vTaskDelay(50);
                    	ktd202x_ch4_led_off();
                        break;
                    case LED_EVT_VIDEO_CAPTURE:
                    	ktd202x_led_off();
                    	ktd202x_ch4_led_blink(500, 500, TIM_2);
                        break;
                    case LED_EVT_PAIRING_MODE:
                    	ktd202x_led_off();
                    	ktd202x_ch1_led_blink(100, 100, TIM_2);
                        break;
                    case LED_EVT_OTA_PROGRESS:
                    	ktd202x_led_off();
                    	ktd202x_ch4_led_blink(300, 300, TIM_1);
                        break;
                    case LED_EVT_ALL_OFF:
                    	ktd202x_led_off();
                        break;
                    default:
                        break;
                    }
                    xSemaphoreGive(i2c_mutex);
                }
            }
    }
}


/**
 * @brief 掃描 I2C 總線上的設備。
 * @details 此函式遍歷所有有效的 7 位 I2C 地址 (從 0x08 到 0x77)，
 * 並嘗試與每個地址進行通信。如果通信成功 (收到 ACK)，
 * 則會在控制台打印出找到的設備地址。
 * @param base 指向 I3C/I2C 控制器的基地址。
 */
static void Scan_I2C_Devices(I3C_Type *base)
{
    uint8_t dummyData = 0x00;
    status_t result;

    PRINTF("[I2C]Scanning I2C addresses...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) // I2C valid 7-bit address range
    {
        result = BOARD_I3C_Send(base, addr, 0x00, 0, &dummyData, 0);

        if (result == kStatus_Success)
        {
        	PRINTF("[I2C]Device found at 0x%02X\n", addr);
        }
    }

    PRINTF("[I2C]Scan complete.\n");
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

void GPIO_INTA_DriverIRQHandler(void)
{

	BaseType_t xHPW = pdFALSE;

	uint32_t status_1 = GPIO_PortGetInterruptStatus(GPIO, GPIO1_PORT, kGPIO_InterruptA);
	uint32_t status_2 = GPIO_PortGetInterruptStatus(GPIO, GPIO2_PORT, kGPIO_InterruptA);

    if (status_1 & (1 << TOUCH_INT_PIN)) { //Touch
        GPIO_PinDisableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, TOUCH_EVENT_BIT, &xHPW);
        }

        //PRINTF("[Debug] TOUCH_GPIO_INTA_IRQHandler \r\n");
    }
    if (status_1 & (1 << CHARG_INT_PIN)) { //Charger
    	GPIO_PinDisableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);

        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, CHARGER_EVENT_BIT, &xHPW);
        }

        //PRINTF("[Debug] CHARG_INT_INTA_IRQHandler \r\n");
    }
    if (status_2 & (1 << GAUGE_INT_PIN)) { //Gauge

    	GPIO_PinDisableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, GAUGE_EVENT_BIT, &xHPW);
        }

        //PRINTF("[Debug] GAUGE_INT_PIN_INTA_IRQHandler \r\n");
    }

    portYIELD_FROM_ISR(xHPW);
    SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief 主程式進入點。
 * @details 負責初始化硬體、時鐘、GPIO、I2C 和 SPI 等周邊設備。
 * 同時，它會建立 FreeRTOS 所需的事件組、佇列和各個任務 (如按鈕處理、SPI 處理等)，
 * 最後啟動 FreeRTOS 排程器，將控制權交給作業系統。
 */
int main(void)
{
	BOARD_InitHardware();

	BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);

#if USE_EVENT
    spi_event_group = xEventGroupCreate();
    if (spi_event_group == NULL)
    {
        PRINTF("Failed to create event group\r\n");
        while (1);
    }
#endif

#if USE_SEMAPHORE
    spi_semaphore = xSemaphoreCreateBinary();
    if (spi_semaphore == NULL)
    {
        PRINTF("Failed to create semaphore\r\n");
        while (1);
    }
#endif

    /* <<< NEW: 建立訊息佇列 >>> */
    // 佇列長度為 10，每個訊息的大小是一個 uint8_t
    spi_request_queue = xQueueCreate(10, sizeof(uint8_t));
    if (spi_request_queue == NULL)
    {
        PRINTF("Failed to create spi_request_queue\r\n");
        while (1);
    }

#if SCAN_I2C_ADDRESS_ENABLE
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);
#endif

	/* Init GPIO */
    GPIO_PortInit(GPIO, GPIO0_PORT);
    GPIO_PortInit(GPIO, GPIO2_PORT);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, 2, 15, &output_int_config);
    GPIO_PinWrite(GPIO, 2, 15, 0);  // 預設為低位

	GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);
    GPIO_PinInit(GPIO, RESET553_N_PORT, RESET553_N_PIN, &output_int_config);

    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
    gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
    NVIC_SetPriority(GPIO_INTA_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(GPIO_INTA_IRQn);

    /* AMP GPIO */
    gpio_pin_config_t amp_config = { kGPIO_DigitalOutput, 1, };
	GPIO_PinInit(GPIO, AMP_RESET_PORT, AMP_RESET_PIN, &amp_config);
    /* Touch INT GPIO*/
    GPIO_PinInit(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
    /* Charger INT GPIO */
    GPIO_PinInit(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
    /* Gauge INT GPIO */
    GPIO_PinInit(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

    /* Initialize PINT */ /* Init FUN_KEY1 & Power_Key*/
	PINT_Init(EXAMPLE_PINT_BASE);
	NVIC_SetPriority(PIN_INT0_IRQn + FUN_KEY1_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_SetPriority(PIN_INT0_IRQn + POWER_KEY_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

	PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt0, kPINT_PinIntEnableBothEdges, pint_intr_callback);
	PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt0);
	PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt1, kPINT_PinIntEnableBothEdges, pint_intr_callback);
	PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt1);

    /* Init PCA9422 PMIC. */
 	BOARD_InitPmic();
 	PRINTF("-------------- PCA9422 BOARD_InitPmic OK--------------\r\n");
 	/* Apply PMIC mode and voltage settings */
 	BOARD_Init_PMICConfigure();
 	PRINTF("-------------- PCA9422 BOARD_Init_PMICConfigure OK--------------\r\n");

	/* init SPI peripheral */
    spi_slave_config_t slave_config = {0};
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)EXAMPLE_SPI_SPOL;
    SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slave_config);

    NVIC_SetPriority(EXAMPLE_SPI_SLAVE_IRQ, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(EXAMPLE_SPI_SLAVE_IRQ);

    /* ======================PMIC PCA9422================== */
	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
	PRINTF("[GLF70583]top_stat:%X \n",top_stat);

	//Solution: The manufacturer did not set it to LOAD SWITCH
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0xF5, 0xC6);
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB8);
	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB9);

	// BUCK1 Delay 4ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x66, 0x0C);
	// BUCK2 Delay 2ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x67, 0x08);
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x67, 0x08);
	// BUCK3 Delay 0ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x68, 0x00);
	// LDO1 Delay 5ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x6A, 0x12);
	// 0x25->BUCK4、LDO2 off
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x26, 0xE8);
	// 0x26->BUCK1、2、4 ON、Others off
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xD0);
	//glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);//BUCK2 ON、Others off

	PRINTF("GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	PRINTF("GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1); \n");
	GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1);

	/* Init I2C Component */
	awinic_single_enter(); //Touch Init

	/* ============== Charger Init Start==============*/
		bq256xx_cfg_t charger_cfg = {
				.vindpm_uv = 4450000,
				.iindpm_ua = 2000000,
				.ichg_ua = 530000,
				.vbatreg_uv = 4005000,
				.iprechg_ua = 60000,
				.iterm_ua = 20000,
				.wdt_ms = 0
		};
		status_t bq_ret = bq256xx_init(&charger_cfg);
		if ( bq_ret!= kStatus_Success) {
			PRINTF("[Charger] bq256xx init failed!,ret:%d \n",bq_ret);
			return -1;
		}
		else{
			PRINTF("[Charger] bq256xx initialized.OK \n");
		}
		bq256xx_write_reg(0x03, 0x31); // IPRECHG = 60mA, ITERM = 20mA
	/* ============== Charger Init End==============*/
	glf70302_read_battery(&battery); //Read the battery level after powering on


	/* ===== A. 建立 I2C EventGroup 與 Mutex ===== */
	i2c_event_group = xEventGroupCreate();
	configASSERT(i2c_event_group);

	i2c_mutex = xSemaphoreCreateMutex();
	configASSERT(i2c_mutex);


	/* 建立 tasks */
    /* <<< MODIFIED: 建立 spi_handler_task 來取代舊的 console_task >>> */
    if (xTaskCreate(spi_handler_task, "SPI_HANDLER", configMINIMAL_STACK_SIZE + 500, NULL,
                    tskIDLE_PRIORITY + 2, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

    if (xTaskCreate(spi_task, "SPI_TASK", configMINIMAL_STACK_SIZE + 100, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

    if (xTaskCreate(passive_handler_task, "PASSIVE", configMINIMAL_STACK_SIZE + 200, NULL,
                    tskIDLE_PRIORITY + 3, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

	if (xTaskCreate(button_task, "BUTTON", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL)!= pdPASS)
    {
        PRINTF(" BUTTON Task creation failed!.\r\n");
        while (1);
    }
	if (xTaskCreate(power_key_task, "POWER_KEY", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL)!= pdPASS)
    {
        PRINTF(" POWER_KEY Task creation failed!.\r\n");
        while (1);
    }
	/* ===== C. 建立 I2C_Task（建議比 passive_handler_task 略高，避免事件延遲） ===== */
	if (xTaskCreate(I2C_Task, "I2C_TASK",
	                configMINIMAL_STACK_SIZE + 256,
	                NULL,
	                tskIDLE_PRIORITY + 3,
	                &sI2CTaskHandle) != pdPASS)
	{
	    PRINTF("I2C_TASK creation failed!\r\n");
	    while (1) { ; }
	}


	vTaskStartScheduler();



    for (;;);
}
