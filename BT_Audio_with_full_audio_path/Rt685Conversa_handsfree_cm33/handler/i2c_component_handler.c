/*
 * i2_component_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#include "i2c_component_handler.h"
#include "spi_handler.h"
#include "button_handler.h"
#include "system_status.h"
#include "hal_pmic.h"
#include "hal_power.h"
#include "hal_touch.h"
#include "hal_led.h"
#include "hal_amp.h"
#include "WorkStateManager.h"
#include "app_handsfree.h"

#define BATTERY_READ_PERIOD_MS  (60000U)
#define FIRST_BATTERY_READ_PERIOD_MS  (6000U)
static TimerHandle_t s_battery_timer = NULL;

/* ===== I2C synchronization objects ===== */
EventGroupHandle_t i2c_event_group = NULL;
SemaphoreHandle_t  i2c_mutex       = NULL;
TaskHandle_t       sI2CTaskHandle  = NULL;

extern QueueHandle_t spi_request_queue ;

extern SemaphoreHandle_t sys_bus_mutex;

extern volatile bq256xx_status_t charger_status;
extern volatile BatteryInfo battery_info;
volatile led_event_t g_led_event = LED_EVT_NONE;
volatile amp_event_t g_amp_event = AMP_EVT_NONE;
extern volatile struct aw933xx_dev aw933xx;

extern uint8_t led_status;
extern volatile uint8_t System_Status ;
extern uint8_t Novatek_boot_completed;

battery_state_t battery_state = BATTERY_STATE_NORMAL;
uint8_t is_first_read_battery_level = 1;

extern uint32_t s_bq256xx_iindpm_target_ua;

extern RingtoneState general_RingtoneState;

static void BatteryReadTimerCb(TimerHandle_t xTimer)
{
    if (is_first_read_battery_level) {
        xTimerChangePeriod(s_battery_timer, pdMS_TO_TICKS(BATTERY_READ_PERIOD_MS), 0);
        is_first_read_battery_level = 0;
    }
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, GAUGE_EVENT_BIT);
    }
}

static void Determine_pca9422_enter_ship_mode(void)
{
	/* 讀取當下按鍵狀態 */
	uint8_t pin_state = (uint8_t)GPIO_PinRead(GPIO, NXP_BQ_MR_N_PORT, NXP_BQ_MR_N_PIN);

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
				//hal_pmic_pca9422_enter_ship_mode();
				//hal_pmic_pca9422_power_down();
				bq256xx_enter_ship_mode();
			}
		}
		else
		{
			/* 沒有按住按鍵（高電位）→ 直接進入 ship mode */
			//hal_pmic_pca9422_enter_ship_mode();
			//hal_pmic_pca9422_power_down();
			bq256xx_enter_ship_mode();
		}
}

void Init_I2C_Component(void)
{
#if PMIC_PCA9422_ENABLE
	hal_pmic_pca9422_init();
#endif
#if FG_GLF70302_ENABLE
	hal_power_gauge_glf70302_init();//Gauge Init
#endif
#if LED_KTD2027_ENABLE
	hal_led_ktd2027_init();
#endif
#if CHG_BQ25618_ENABLE
	hal_power_charger_bq25618_init();
#endif

	if (hal_power_is_power_off_charging_mode()) {
		hal_power_go_to_power_off_charging();
	}
	Determine_pca9422_enter_ship_mode();

#if PMIC_GLF70583_ENABLE
	hal_pmic_glf70583_actual_board_init();
#endif

	hal_soc_enable();

#if LED_KTD2027_ENABLE
	hal_led_ktd2027_power_on_indicator(); //White light turns on first
#endif

#if TOUCH_AW93305_ENABLE
	hal_touch_aw93305_init(); //Touch Init
#endif
#if FG_GLF70302_ENABLE
	hal_power_gauge_glf70302_get_battery_level(); //Read the battery level after powering on
	//battery_info.soc = hal_power_get_battery_percentage(battery_info.voltage);
	ss_set_battery(battery_info.soc);
#endif

#if AMP_AW88166_ENABLE
	hal_amp_aw88166_power_on();
	hal_amp_aw88166_init(); // Init AMP
#endif

#if CHG_BQ25618_ENABLE

	hal_power_charger_bq25618_get_charging_status();
	if(charger_status.vbus_good)
	{
		ss_set_charging(true);
	}
#endif

#if FG_GLF70302_ENABLE
	s_battery_timer = xTimerCreate("BattTimer",
	                               pdMS_TO_TICKS(FIRST_BATTERY_READ_PERIOD_MS),
	                               pdTRUE,     // auto-reload
	                               NULL,
	                               BatteryReadTimerCb);
#endif

}

void battery_timer_start(void)
{
#if FG_GLF70302_ENABLE
	if (s_battery_timer != NULL) {
	    xTimerStart(s_battery_timer, 0);
	}
#endif
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

            if (sys_bus_mutex != NULL)
            	{
            		xSemaphoreTake(sys_bus_mutex, portMAX_DELAY);
            	}

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
#if AMP_AW88166_ENABLE
                switch (evt) {
                    case AMP_EVT_MUSIC_START:
                        hal_amp_aw88166_left_start("Music");
                        hal_amp_aw88166_right_start("Music");
                        AmpState=AmpState_ConfiguredAndActive;
                        PRINTF("AMP_EVT_MUSIC_START done\r\n");
                        break;
                    case AMP_EVT_RECEIVER_START:
                        hal_amp_aw88166_left_start("Receiver");
                        hal_amp_aw88166_right_start("Receiver");
                        AmpState=AmpState_ConfiguredAndActive;
                        PRINTF("AMP_EVT_RECEIVER_START done\r\n");
                        break;
                    case AMP_EVT_STOP:
                    	hal_amp_aw88166_left_stop();
                    	hal_amp_aw88166_right_stop();
                    	AmpState=AmpState_UnConfigured;
                    	PRINTF("AMP_EVT_STOP done\r\n");
                        break;
                    default:
                        break;
                }
#endif
                xSemaphoreGive(i2c_mutex);
            }

            if (sys_bus_mutex != NULL) {
            	xSemaphoreGive(sys_bus_mutex);
            }
        }


        /* --- TOUCH event --- */
        if ((bits & TOUCH_EVENT_BIT) != 0)
        {
        	if (sys_bus_mutex != NULL)
        		{
					xSemaphoreTake(sys_bus_mutex, portMAX_DELAY);
        		}

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
#if TOUCH_AW93305_ENABLE
            	AW93305_EXTI_Callback();
#endif
                xSemaphoreGive(i2c_mutex);

#if TOUCH_AW93305_ENABLE
                if(aw933xx.event.click >0)
                {
                	unsigned int btn_event = aw933xx.event.click;
                	PRINTF("[Touch] click= %d \n",btn_event);
                	if(btn_event==1)
                	{
#if SOC_SPI_ENABLE
                        if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MEDIA_PLAYER ||
                        		ss_get_state() == USAGE_STATE_MENU || ss_get_state() == USAGE_STATE_HOME)) {
                	        send_spi_request(ONE_TOUCH_HEX_VALUE);
                        }
#endif
                	}
                	else if(btn_event==2)
                	{
#if SOC_SPI_ENABLE
                        if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MEDIA_PLAYER)) {
                            send_spi_request(DOUBLE_TOUCH_HEX_VALUE);
                        }
#endif
                	}

                }
                else if(aw933xx.event.press)
                {
                	PRINTF("[Touch] press \n");
#if SOC_SPI_ENABLE
                    if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MEDIA_PLAYER ||
                    		ss_get_state() == USAGE_STATE_HOME || ss_get_state() == USAGE_STATE_MENU ||
                    		ss_get_state() == USAGE_STATE_ABOUT)) {
                    	// Media Player: Go Home (if the OE is on), Wake Up (if the OE is off)
                    	// Home: Wake Up (if the OE is off)
                        send_spi_request(PRESS_TOUCH_HEX_VALUE);
                    }
#endif
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
#if SOC_SPI_ENABLE
                    if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MENU)) {
                        send_spi_request(FORWARD_SLIDE_HEX_VALUE);
                    }
#endif
                    if (ss_get_state() == USAGE_STATE_MEDIA_PLAYER) {
                        // Volume down
                    	ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
                        PRINTF("[Touch] Volume down\r\n");

                    }
                }
                else if(aw933xx.event.left_wareds)
                {
                	PRINTF("[Touch] slide_left \n");
#if SOC_SPI_ENABLE
                    if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_MENU)) {
                        send_spi_request(BACK_SLIDE_HEX_VALUE);
                    }
#endif
                    if (ss_get_state() == USAGE_STATE_MEDIA_PLAYER) {
                        // Volume up
                    	ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
                        PRINTF("[Touch] Volume up\r\n");
                    }
                }
#endif

            }

            if (sys_bus_mutex != NULL) {
            	xSemaphoreGive(sys_bus_mutex);
            }

            /* 任務側重新啟用觸控中斷（先清旗標再開） */
            GPIO_PinClearInterruptFlag(GPIO, NXP_TOUCH_INT_PORT, NXP_TOUCH_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, NXP_TOUCH_INT_PORT, NXP_TOUCH_INT_PIN, kGPIO_InterruptA);
        }


        /* --- CHARGER event --- */
        if ((bits & CHARGER_EVENT_BIT) != 0)
        {
        	if (sys_bus_mutex != NULL) {
        		xSemaphoreTake(sys_bus_mutex, portMAX_DELAY);
        	}

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
#if CHG_BQ25618_ENABLE
                vTaskDelay(10); // Wait for the charger to become ready
            	(void)bq256xx_set_iindpm(s_bq256xx_iindpm_target_ua);
    			if (bq256xx_poll_status(&charger_status) == kStatus_Success) {
    				PRINTF("[Charger] Power Good: %s\n", charger_status.power_good ? "Yes" : "No");
    				PRINTF("[Charger] VBUS Status: 0x%02X\n", charger_status.vbus_stat);
    				PRINTF("[Charger] Charge Status: 0x%02X\n", charger_status.chg_stat);
    				PRINTF("[Charger] Fault Status: 0x%02X\n", charger_status.fault_stat);
    				PRINTF("[Charger] VBUS Good: %s\n", charger_status.vbus_good ? "Yes" : "No");
    				PRINTF("\n");
    				if(charger_status.vbus_good)
    				{
    					hal_led_set_situation(HAL_LED_EVENT_CHARGING, SITUATION_ENABLE);
    					led_post_event(LED_EVT_REFRESH);
    					ss_set_charging(true);
    				}
    				else
    				{
    					hal_led_set_situation(HAL_LED_EVENT_CHARGING, SITUATION_DISABLE);
    					hal_led_set_situation(HAL_LED_EVENT_FULL_CHARGED, SITUATION_DISABLE);
    					led_post_event(LED_EVT_REFRESH);
    					ss_set_charging(false);
    				}
    				if(charger_status.chg_stat==0x03)//Charging status: 00 – Not Charging、01 – Pre-charge、10 – Fast Charging、11 – Charge Termination
    				{
    					battery_state = BATTERY_STATE_FULL;
    					hal_led_set_situation(HAL_LED_EVENT_CHARGING, SITUATION_DISABLE);
    					hal_led_set_situation(HAL_LED_EVENT_FULL_CHARGED, SITUATION_ENABLE);
    					led_post_event(LED_EVT_REFRESH);
    				}
    				/*
    			    uint8_t val;
    			    for (uint8_t reg = 0x00; reg <= 0x0A; reg++)
    			    {
    			    	(void)bq256xx_read_reg(reg, &val, 1);
    			    	 PRINTF("[Debug][bq256xx] REG %u = 0x%02X\r\n", reg, val);
    			    }
    				 */
    			} else {
    				PRINTF("[Charger] Failed to read charger status.\n");
    			}
#endif
                xSemaphoreGive(i2c_mutex);
            }

            if (sys_bus_mutex != NULL) {
            	xSemaphoreGive(sys_bus_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, CHG_INT_N_R_PORT, CHG_INT_N_R_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, CHG_INT_N_R_PORT, CHG_INT_N_R_PIN, kGPIO_InterruptA);

        }

        /* --- GAUGE event --- */
        if ((bits & GAUGE_EVENT_BIT) != 0)
        {

        	if (sys_bus_mutex != NULL) {
        		xSemaphoreTake(sys_bus_mutex, portMAX_DELAY);
        	}

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
#if FG_GLF70302_ENABLE
            	glf70302_polling(&battery_info);
            	//battery_info.soc = hal_power_get_battery_percentage(battery_info.voltage);
                PRINTF("[Battery] SOC: %d%%\r\n",battery_info.soc);
                if (ss_is_charging()) {
                	if (battery_info.soc >= FULLY_CHARGE_PERCENTAGE) {
                    	battery_state = BATTERY_STATE_FULL;
                    	hal_led_set_situation(HAL_LED_EVENT_CHARGING, SITUATION_DISABLE);
                    	hal_led_set_situation(HAL_LED_EVENT_FULL_CHARGED, SITUATION_ENABLE);
                    	led_post_event(LED_EVT_REFRESH);
                	}
                } else {
                    if (battery_info.soc <= LOW_POWER_PERCENTAGE)
                    {
                    	battery_state = BATTERY_STATE_LOW;
                    	hal_led_set_situation(HAL_LED_EVENT_LOW_BATTERY, SITUATION_ENABLE);
                    	hal_led_set_situation(HAL_LED_EVENT_CHARGING, SITUATION_DISABLE);
                    	hal_led_set_situation(HAL_LED_EVENT_FULL_CHARGED, SITUATION_DISABLE);
                    	led_post_event(LED_EVT_REFRESH);
                    }
                    else
                    {
                    	battery_state = BATTERY_STATE_NORMAL;
                    	hal_led_set_situation(HAL_LED_EVENT_LOW_BATTERY, SITUATION_DISABLE);
                    	hal_led_set_situation(HAL_LED_EVENT_CHARGING, SITUATION_DISABLE);
                    	hal_led_set_situation(HAL_LED_EVENT_FULL_CHARGED, SITUATION_DISABLE);
                    	led_post_event(LED_EVT_REFRESH);
                    }
                }
            	ss_set_battery(battery_info.soc);
            	if(battery_info.voltage<=3500 && ss_is_charging() == false)//Automatic shutdown when battery voltage drops below 3.5V
            	{
            		PRINTF("[Gauge] Low battery, so it shuts down. \r\n");
#if SOC_SPI_ENABLE
                        if (Novatek_boot_completed) {
                            send_spi_request(POWER_LONG_PRESS_HEX_VALUE);
                        }
#endif
//                        led_post_event(LED_EVT_POWER_OFF_PROGRESS);
//                        general_RingtoneState = Ringtone_PowerOFF;
            	}
#endif
                xSemaphoreGive(i2c_mutex);
            }

            if (sys_bus_mutex != NULL) {
            	xSemaphoreGive(sys_bus_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, FG_INT_GLF70302_PORT, FG_INT_GLF70302_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, FG_INT_GLF70302_PORT, FG_INT_GLF70302_PIN, kGPIO_InterruptA);

        }


        /*--- LED event --- */
        if ((bits & LED_EVENT_BIT) != 0) {
            if (sys_bus_mutex != NULL) {
                xSemaphoreTake(sys_bus_mutex, portMAX_DELAY);
            }

            vTaskDelay(1); /* 確保 g_led_event 已更新 */
            led_event_t evt = g_led_event;

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                /* 根據事件控制 LED */
#if LED_KTD2027_ENABLE
                switch (evt) {
                case LED_EVT_POWER_ON_PROGRESS:
                    hal_led_set_indicator_status(HAL_LED_POWER_ON);
                    break;
                case LED_EVT_POWER_OFF_PROGRESS:
                    hal_led_set_indicator_status(HAL_LED_POWER_OFF);
                    //hal_pmic_pca9422_power_down();
                    bq256xx_enter_ship_mode();
                    break;
                case LED_EVT_PHOTO_CAPTURE:
                    hal_led_set_indicator_status(HAL_LED_TAKE_PHOTO);
                    break;
                case LED_EVT_REFRESH:
//                  hal_led_set_indicator_status(HAL_LED_REFRESH);
                    hal_led_status_handler();
                    break;
                case LED_EVT_ALL_OFF:
                    hal_led_set_indicator_status(HAL_LED_OFF);
                    break;
                default:
                    break;
                }
#endif
                xSemaphoreGive(i2c_mutex);
            }

            if (sys_bus_mutex != NULL) {
                xSemaphoreGive(sys_bus_mutex);
            }
        }


        if(System_Status && Novatek_boot_completed)
        {
            System_Status=0;
#if SOC_SPI_ENABLE
            send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
        }

    }
}

