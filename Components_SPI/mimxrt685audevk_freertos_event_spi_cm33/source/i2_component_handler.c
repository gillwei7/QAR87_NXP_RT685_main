/*
 * i2_component_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#include "i2_component_handler.h"
#include "spi_handler.h"
#include "i2s_handler.h"

/* ===== I2C synchronization objects ===== */
EventGroupHandle_t i2c_event_group = NULL;
SemaphoreHandle_t  i2c_mutex       = NULL;
TaskHandle_t       sI2CTaskHandle  = NULL;

extern QueueHandle_t spi_request_queue ;

volatile BatteryInfo battery ;
volatile led_event_t g_led_event = LED_EVT_NONE;
volatile amp_event_t g_amp_event = AMP_EVT_NONE;
extern volatile struct aw933xx_dev aw933xx;

static void Stop_AMP(void)
{
    close_aw88166_pa(AW_DEV_0);
    close_aw88166_pa(AW_DEV_1);
}
static void Start_AMP(amp_mode_t mode)
{
	const char *profile = (mode == AMP_MODE_RECEIVER) ? "Receiver" : "Music";

    start_aw88166_pa(AW_DEV_0, profile);
    start_aw88166_pa(AW_DEV_1, profile);
}

void pca9422_ship_mode(void)
{
	PRINTF("[PCA9422] PCA9422 enter ship mode \r\n");
	/* pca9422 ship mode process */
	uint8_t value;
    value = 0x10;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}
void pca9422_power_down(void)
{
    PRINTF("[PCA9422] PCA9422 power down \r\n");
	/* pca9422 power down  process */
    uint8_t value;
    value = 0x08;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}


void amp_post_event(amp_event_t e)
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

void Scan_I2C_Devices(I3C_Type *base)
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


void I2C_Task(void *pvParameters)
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
                        StartSoundPlayback();
                        Start_AMP(AMP_MODE_MUSIC);
                        break;
                    case AMP_EVT_RECEIVER_START:
                        StartSoundPlayback();
                        Start_AMP(AMP_MODE_RECEIVER);
                        break;
                    case AMP_EVT_STOP:
                    	Stop_AMP();
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
