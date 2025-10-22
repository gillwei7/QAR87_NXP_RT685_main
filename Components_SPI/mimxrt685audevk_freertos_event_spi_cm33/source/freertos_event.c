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

#include "fsl_dma.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "music.h"

#include "spi_handler.h"
#include "led_status.h"
#include "button_handler.h"

#include <string.h>
#include <stdlib.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USE_EVENT 1
#define USE_SEMAPHORE 0

#define SCAN_I2C_ADDRESS_ENABLE 1

/*****************************************************************************************************/
/* ===== I2C EventGroup bits (for unified I2C_Task) ===== */
#define TOUCH_EVENT_BIT      (1UL << 0)
#define CHARGER_EVENT_BIT    (1UL << 1)
#define GAUGE_EVENT_BIT      (1UL << 2)
#define LED_EVENT_BIT        (1UL << 3)   /* LED task wake-up flag */
#define AMP_EVENT_BIT     	 (1UL << 4)

BatteryInfo battery;
/* ===== I2C synchronization objects ===== */
static EventGroupHandle_t i2c_event_group = NULL;
static SemaphoreHandle_t  i2c_mutex       = NULL;
static TaskHandle_t       sI2CTaskHandle  = NULL;

/* ===== I2C task prototype & external device handlers ===== */
static void I2C_Task(void *pvParameters);

extern volatile struct aw933xx_dev aw933xx;

/*====== I2S prototype======*/
static dma_handle_t s_DmaTxHandle;
static i2s_config_t s_TxConfig;
static i2s_dma_handle_t s_TxHandle;
static i2s_transfer_t s_TxTransfer;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/


/* ===== external SPI handlers ===== */
extern QueueHandle_t spi_request_queue ;
extern EventGroupHandle_t spi_event_group ;
extern SemaphoreHandle_t spi_semaphore ;

/*===== LED handlers =====*/
TaskHandle_t sLEDTaskHandle;
volatile led_event_t g_led_event = LED_EVT_NONE;

uint8_t reg_led =0;

/*===== AMP handlers ===== */
typedef enum {
    AMP_EVT_NONE = 0,
    AMP_EVT_MUSIC_START,
	AMP_EVT_RECEIVER_START,
    AMP_EVT_STOP,
} amp_event_t;

typedef enum {
    AMP_MODE_MUSIC = 0,
    AMP_MODE_RECEIVER = 1,
} amp_mode_t;


volatile amp_event_t g_amp_event = AMP_EVT_NONE;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void pca9422_ship_mode(void)
{
	PRINTF("[PCA9422] PCA9422 enter ship mode \r\n");
	/* pca9422 ship mode process */
	uint8_t value;
    value = 0x10;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}
static void pca9422_power_down(void)
{
    PRINTF("[PCA9422] PCA9422 power down \r\n");
	/* pca9422 power down  process */
    uint8_t value;
    value = 0x08;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}

static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
    /* Enqueue the same original buffer all over again */
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_TxTransferSendDMA(base, handle, *transfer);
}
static void StopSoundPlayback(void)
{
    PRINTF("[I2S1]Stopping sound playback\r\n");

    close_aw88166_pa(AW_DEV_0);
    close_aw88166_pa(AW_DEV_1);

    I2S_TransferAbortDMA(DEMO_I2S_TX_toAmp, &s_TxHandle);
}

static void StartSoundPlayback(amp_mode_t mode)
{
    const char *profile = (mode == AMP_MODE_RECEIVER) ? "Receiver" : "Music";

    PRINTF("[I2S1]Setup looping playback of sine wave (%s mode)\r\n", profile);

    s_TxTransfer.data     = &g_Music[0];
    s_TxTransfer.dataSize = sizeof(g_Music);

    I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX_toAmp,  &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
    I2S_TxTransferSendDMA(DEMO_I2S_TX_toAmp,  &s_TxHandle, s_TxTransfer);

    start_aw88166_pa(AW_DEV_0, profile);
    start_aw88166_pa(AW_DEV_1, profile);
}

static void amp_post_event(amp_event_t e)
{
    g_amp_event = e;
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, AMP_EVENT_BIT);
    }
}


void led_post_event(led_event_t e)
{
    g_led_event = e;
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, LED_EVENT_BIT);

    }
}


static void I2C_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(
            i2c_event_group,
            TOUCH_EVENT_BIT | CHARGER_EVENT_BIT | GAUGE_EVENT_BIT | LED_EVENT_BIT | AMP_EVENT_BIT,
            pdTRUE,     /* clear on exit */
            pdFALSE,    /* wait for any bit */
            portMAX_DELAY);


        /* --- AMP event --- */
        if ((bits & AMP_EVENT_BIT) != 0) {
            vTaskDelay(1); /* 確保 g_amp_event 已更新 */
            amp_event_t evt = g_amp_event;

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                switch (evt) {
                    case AMP_EVT_MUSIC_START:
                        StartSoundPlayback(AMP_MODE_MUSIC);
                        break;
                    case AMP_EVT_RECEIVER_START:
                        StartSoundPlayback(AMP_MODE_RECEIVER);
                        break;
                    case AMP_EVT_STOP:
                        StopSoundPlayback();
                        break;
                    default:
                        break;
                }
                xSemaphoreGive(i2c_mutex);
            }
        }


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
                	uint8_t v = PRESS_TOUCH_HEX_VALUE;
                	(void)xQueueSend(spi_request_queue, &v, 0);
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
                	uint8_t v = FORWARD_SLIDE_HEX_VALUE;
                	(void)xQueueSend(spi_request_queue, &v, 0);
                }
                else if(aw933xx.event.left_wareds)
                {
                	PRINTF("[Touch] slide_left \n");
                	uint8_t v = BACK_SLIDE_HEX_VALUE;
                	(void)xQueueSend(spi_request_queue, &v, 0);
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
    				if(status.vbus_good)
    				{
    					led_post_event(LED_EVT_CHARGING);
    				}
    				else
    				{
    					led_post_event(LED_EVT_ALL_OFF);
    				}
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
                    	vTaskDelay(pdMS_TO_TICKS(1000));
                    	ktd202x_ch2_led_off();
                    	pca9422_power_down();
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
                    case LED_EVT_OTA_SUCCESS:
                    	ktd202x_led_off();
                    	ktd202x_ch3_led_breathe(PERIOD_CODE_1P5S,
                    	                        RISE_CODE_600MS,
                    	                        FALL_CODE_600MS,
                    	                        ON_PERCENT_60,
                    	                        RAMP_SCALE_2X_SLOW,
												true,
                    	                        LED_CURRENT_CH3 );
                        break;
                    case LED_EVT_OTA_FAIL:
                    	ktd202x_led_off();
                    	ktd202x_ch2_led_breathe(PERIOD_CODE_1P5S,
                    							RISE_CODE_600MS,
                    							FALL_CODE_600MS,
                    	                        ON_PERCENT_60,
                    							RAMP_SCALE_2X_SLOW,
                    	                        true,
                    							LED_CURRENT_CH2);
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
 	PRINTF("[PCA9422] BOARD_InitPmic OK \r\n");
 	/* Apply PMIC mode and voltage settings */
 	BOARD_Init_PMICConfigure();
 	PRINTF("[PCA9422] BOARD_Init_PMICConfigure OK \r\n");
 	/* ====== PCA9422 ship mode start ======*/
	/* 讀取當下按鍵狀態 */
	uint8_t pin_state = (uint8_t)GPIO_PinRead(GPIO, POWER_KEY_PORT, POWER_KEY_PIN);

	    if (pin_state == 0U)
	    {
	        /* 按鍵為低（按下）→ 需連續 2 秒才允許離開 ship mode */
	        if (power_key_low_for_ms(LONG_PRESS_MS))
	        {
	            PRINTF("[PCA9422] PCA9422 leave ship mode (press >= %u ms)\r\n", LONG_PRESS_MS);
	        }
	        else
	        {
	            /* 沒達到 2 秒長按 → 進入 ship mode */
	            PRINTF("[PCA9422] Power key (press < %u ms)\r\n", LONG_PRESS_MS);
	            pca9422_ship_mode();
	        }
	    }
	    else
	    {
	        /* 沒有按住按鍵（高電位）→ 直接進入 ship mode */
	    	pca9422_ship_mode();
	    }
	/* ====== PCA9422 ship mode end ======*/

	/* init SPI peripheral */
    spi_slave_config_t slave_config = {0};
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)EXAMPLE_SPI_SPOL;
    SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slave_config);

    NVIC_SetPriority(EXAMPLE_SPI_SLAVE_IRQ, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(EXAMPLE_SPI_SLAVE_IRQ);

    /* ======================PMIC glf70583================== */
	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
	PRINTF("[GLF70583] top_stat:%X \n",top_stat);

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

	PRINTF("[GLF70583] Enable GLF70583 \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	PRINTF("[System] Enable Novatek \n");
	GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1);

	/* Init I2C Component */
	ktd202x_ch4_led_on(LED_ON); //White light turns on first

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

	init_aw88166(); // Init AMP

	/*===============I2S Init ======================*/
	PRINTF("[I2S]Configure I2S \r\n");

    I2S_TxGetDefaultConfig(&s_TxConfig);

    // I2S 32bits
    s_TxConfig.dataLength  = 32;
    s_TxConfig.frameLength = 64;
    s_TxConfig.divider     = 8;//(24576000U / 16000U / 32U / 2);//DEMO_I2S_CLOCK_DIVIDER;
    s_TxConfig.masterSlave = DEMO_I2S_TX_MODE;

    I2S_TxInit(DEMO_I2S_TX_toAmp, &s_TxConfig);

    DMA_Init(DEMO_DMA);

    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp, kDMA_ChannelPriority3);
    DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp);

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
