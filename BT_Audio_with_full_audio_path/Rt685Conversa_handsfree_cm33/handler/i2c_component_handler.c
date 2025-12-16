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

#define BATTERY_READ_PERIOD_MS  (60000U)
static TimerHandle_t s_battery_timer = NULL;

/* ===== I2C synchronization objects ===== */
EventGroupHandle_t i2c_event_group = NULL;
SemaphoreHandle_t  i2c_mutex       = NULL;
TaskHandle_t       sI2CTaskHandle  = NULL;

extern QueueHandle_t spi_request_queue ;

extern volatile bq256xx_status_t charger_status;
extern volatile BatteryInfo battery_info;
volatile led_event_t g_led_event = LED_EVT_NONE;
volatile amp_event_t g_amp_event = AMP_EVT_NONE;
extern volatile struct aw933xx_dev aw933xx;

extern uint8_t led_status;
extern volatile uint8_t System_Status ;
extern uint8_t Novatek_boot_completed;

battery_state_t battery_state = BATTERY_STATE_NORMAL;

extern uint32_t s_bq256xx_iindpm_target_ua;

static inline uint8_t battery_soc_percent_mv(uint32_t mv)
{
    // 邊界保護
    if (mv <= BATTERY_EMPTY_VOLTAGE) return 0;
    if (mv >= BATTERY_FULL_VOLTAGE)  return 100;

    // 線性比例： (mv - empty) * 100 / (full - empty)
    uint32_t range = (uint32_t)(BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE);
    uint32_t num   = (uint32_t)(mv - BATTERY_EMPTY_VOLTAGE) * 100u;

    // 四捨五入：+range/2
    uint8_t soc = (uint8_t)((num + (range / 2u)) / range);

    return soc;   // 0~100
}


static void BatteryReadTimerCb(TimerHandle_t xTimer)
{

    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, GAUGE_EVENT_BIT);
    }

}


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
				hal_pmic_pca9422_power_down();
			}
		}
		else
		{
			/* 沒有按住按鍵（高電位）→ 直接進入 ship mode */
			//hal_pmic_pca9422_enter_ship_mode();
			hal_pmic_pca9422_power_down();
		}
}

void Init_I2C_Component(void)
{
#if PMIC_PCA9422_ENABLE
	hal_pmic_pca9422_init();
#endif
	power_off_charging();
	Determine_pca9422_enter_ship_mode();
#if PMIC_GLF70583_ENABLE
	hal_pmic_glf70583_actual_board_init();
#endif
#if CHG_BQ25618_ENABLE
	hal_power_charger_bq25618_init();
#endif

	hal_soc_enable();

#if LED_KTD2027_ENABLE
	hal_led_ktd2027_init();
	hal_led_ktd2027_power_on_indicator(); //White light turns on first
#endif

#if TOUCH_AW93305_ENABLE
	hal_touch_aw93305_init(); //Touch Init
#endif
#if FG_GLF70302_ENABLE
	glf70302_init();//Gauge Init
	hal_power_gauge_glf70302_get_battery_level(); //Read the battery level after powering on
	battery_info.soc = battery_soc_percent_mv(battery_info.voltage);
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
	                               pdMS_TO_TICKS(BATTERY_READ_PERIOD_MS),
	                               pdTRUE,     // auto-reload
	                               NULL,
	                               BatteryReadTimerCb);
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

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
#if AMP_AW88166_ENABLE
                switch (evt) {
                    case AMP_EVT_MUSIC_START:
//                        StartSoundPlayback();
                        Start_AMP(AMP_MODE_MUSIC);
                        break;
                    case AMP_EVT_RECEIVER_START:
//                        StartSoundPlayback();
                        Start_AMP(AMP_MODE_RECEIVER);
                        break;
                    case AMP_EVT_STOP:
                    	Stop_AMP();
//                        StopSoundPlayback();
                        break;
                    default:
                        break;
                }
#endif
                xSemaphoreGive(i2c_mutex);
            }
        }


        /* --- TOUCH event --- */
        if ((bits & TOUCH_EVENT_BIT) != 0)
        {
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
                        if (ss_get_state() != USAGE_STATE_MUSIC_PLAYER && ss_get_state() != USAGE_STATE_MEDIA_PLAYER) {
                	        send_spi_request(ONE_TOUCH_HEX_VALUE);
                        }
#endif
                	}
                	else if(btn_event==2)
                	{
#if SOC_SPI_ENABLE
                        if (ss_get_state() != USAGE_STATE_MUSIC_PLAYER) {
                            send_spi_request(DOUBLE_TOUCH_HEX_VALUE);
                        }
#endif
                	}

                }
                else if(aw933xx.event.press)
                {
                	PRINTF("[Touch] press \n");
#if SOC_SPI_ENABLE
                    if (ss_get_state() != USAGE_STATE_MUSIC_PLAYER) {
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
                    if (ss_get_state() != USAGE_STATE_MUSIC_PLAYER && ss_get_state() != USAGE_STATE_MEDIA_PLAYER) {
                        send_spi_request(FORWARD_SLIDE_HEX_VALUE);
                    }
#endif
                    if (ss_get_state() == USAGE_STATE_MEDIA_PLAYER) {
                        // Volume up
                    	ChangeMasterVolumeLevel15_UpDown(1); // pass positive value to increase volume
                        PRINTF("[Touch] Volume up\r\n");
                    }
                }
                else if(aw933xx.event.left_wareds)
                {
                	PRINTF("[Touch] slide_left \n");
#if SOC_SPI_ENABLE
                    if (ss_get_state() != USAGE_STATE_MUSIC_PLAYER && ss_get_state() != USAGE_STATE_MEDIA_PLAYER) {
                        send_spi_request(BACK_SLIDE_HEX_VALUE);
                    }
#endif
                    if (ss_get_state() == USAGE_STATE_MEDIA_PLAYER) {
                        // Volume down
                    	ChangeMasterVolumeLevel15_UpDown(0); // pass zero or negative value to decrease volume
                        PRINTF("[Touch] Volume down\r\n");
                    }
                }
#endif

            }

            /* 任務側重新啟用觸控中斷（先清旗標再開） */
            GPIO_PinClearInterruptFlag(GPIO, NXP_TOUCH_INT_PORT, NXP_TOUCH_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, NXP_TOUCH_INT_PORT, NXP_TOUCH_INT_PIN, kGPIO_InterruptA);
        }

        /* --- CHARGER event --- */
        if ((bits & CHARGER_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
#if CHG_BQ25618_ENABLE
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
    					led_post_event(LED_EVT_CHARGING);
    					ss_set_charging(true);
    				}
    				else
    				{
    					led_post_event(LED_EVT_ALL_OFF);
    					ss_set_charging(false);
    				}
    				if(charger_status.chg_stat==0x11)//Charging status: 00 – Not Charging、01 – Pre-charge、10 – Fast Charging、11 – Charge Termination
    				{
    					battery_state = BATTERY_STATE_FULL;
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

            GPIO_PinClearInterruptFlag(GPIO, CHG_INT_N_R_PORT, CHG_INT_N_R_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, CHG_INT_N_R_PORT, CHG_INT_N_R_PIN, kGPIO_InterruptA);

        }

        /* --- GAUGE event --- */
        if ((bits & GAUGE_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
#if FG_GLF70302_ENABLE
            	glf70302_polling(&battery_info);
            	battery_info.soc = battery_soc_percent_mv(battery_info.voltage);
                PRINTF("[Battery] SOC: %d%%\r\n",battery_info.soc);
                if(battery_info.soc>=99 && ss_is_charging())
                {
                	battery_state = BATTERY_STATE_FULL;
                	led_post_event(LED_EVT_FULL_CHARGERED);
                }
                else if (battery_info.soc<=20)
                {
                	battery_state = BATTERY_STATE_LOW;
                	led_post_event(LED_EVT_LOW_BATTERY);
                }
                else
                {
                	battery_state = BATTERY_STATE_NORMAL;
                }
            	ss_set_battery(battery_info.soc);
#endif
                xSemaphoreGive(i2c_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, FG_INT_GLF70302_PORT, FG_INT_GLF70302_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, FG_INT_GLF70302_PORT, FG_INT_GLF70302_PIN, kGPIO_InterruptA);

        }

        /*--- LED event --- */
        if ((bits & LED_EVENT_BIT) != 0) {
                vTaskDelay(1); /* 確保 g_led_event 已更新 */
                led_event_t evt = g_led_event;

                if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                    /* 根據事件控制 LED */
#if LED_KTD2027_ENABLE
                    switch (evt) {
                    case LED_EVT_POWER_ON_PROGRESS:
                    	ktd202x_led_off();
                        ktd202x_ch4_led_on(LED_ON);
                        break;
                    case LED_EVT_POWER_OFF_PROGRESS:
                    	ktd202x_led_off();
                    	ktd202x_ch2_led_on(LED_ON);
                    	vTaskDelay(pdMS_TO_TICKS(10000));
                    	ktd202x_ch2_led_off();
                    	hal_pmic_pca9422_power_down();
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
#endif
                    xSemaphoreGive(i2c_mutex);
                }
            }

        if(System_Status && Novatek_boot_completed)
        {
        	System_Status=0;
#if SOC_SPI_ENABLE
        	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
        }
#if LED_KTD2027_ENABLE && CHG_BQ25618_ENABLE
        //Confirm the final status
        if(led_status==0) // When other events are executed, causing the LED to turn off
        {
        	if(ss_is_charging())
        	{
            	if(battery_state==BATTERY_STATE_FULL)
            		led_post_event(LED_EVT_FULL_CHARGERED);
            	else
                	led_post_event(LED_EVT_CHARGING);
        	}
        	else
        	{
        		if(battery_state==BATTERY_STATE_LOW)
        			led_post_event(LED_EVT_LOW_BATTERY);
        	}

        }
#endif

    }
}
void power_off_charging(void)
{
	uint8_t LED_state=0; //1->charging； 2->full-charge


	glf70302_init();//Gauge Init
	ktd202x_probe();
	hal_power_charger_bq25618_init();
	SDK_DelayAtLeastUs(100 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk)); //Delay 100ms

	hal_power_charger_bq25618_get_charging_status();
	if(charger_status.vbus_good)
	{
		PRINTF("[System] power off charging \r\n");
		while(1)
		{

			hal_power_charger_bq25618_get_charging_status();
			hal_power_gauge_glf70302_get_battery_level();
			battery_info.soc = battery_soc_percent_mv(battery_info.voltage);
			if(charger_status.vbus_good)
			{
				switch (LED_state) {
					case 0:
							ktd202x_led_off();
							ktd202x_ch2_led_blink(500, 500, TIM_1);
							LED_state++;
							break;
					case 1:
							if(battery_info.soc>=99)
							{
								ktd202x_led_off();
								ktd202x_ch3_led_on(LED_ON);
								LED_state++;
							}
							break;
                    default:
                        	break;
									}
			}
			else
			{
				ktd202x_led_off();
				PRINTF("[System] power off charging -> power down \r\n");
				hal_pmic_pca9422_power_down();
			}

			SDK_DelayAtLeastUs(10 * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk)); //Delay 10ms
		}
	}
}
