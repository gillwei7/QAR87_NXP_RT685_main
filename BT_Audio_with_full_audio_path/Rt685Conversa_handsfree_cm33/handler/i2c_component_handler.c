/*
 * i2_component_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#include "i2c_component_handler.h"
#include "spi_handler.h"
#include "button_handler.h"
#include "buttons_handler.h"
#include "system_status.h"
#include "hal_pmic.h"
#include "hal_power.h"
#include "hal_touch.h"
#include "hal_led.h"
#include "hal_amp.h"
#include "WorkStateManager.h"
#include "app_handsfree.h"
#include "hal_sar.h"

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
volatile hal_led_event_t g_led_event = HAL_LED_EVENT_NONE;

extern uint8_t led_status;
extern volatile uint8_t System_Status ;
extern uint8_t Novatek_boot_completed;

uint8_t is_first_read_battery_level = 1;
static power_on_reason_t power_on_reason = POWER_ON_UNEXPECTED;

static uint8_t has_sar_event = 0;
static uint8_t has_led_event = 0;
static uint8_t has_gauge_event = 0;


extern uint32_t s_bq256xx_iindpm_target_ua;

extern RingtoneState general_RingtoneState;

static void BatteryReadTimerCb(TimerHandle_t xTimer)
{
    if (is_first_read_battery_level) {
		Novatek_boot_completed = 1;
        xTimerChangePeriod(s_battery_timer, pdMS_TO_TICKS(BATTERY_READ_PERIOD_MS), 0);
    }
#if 0
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, GAUGE_EVENT_BIT);
    }
#endif
    has_gauge_event = 1;
}

static power_on_reason_t get_power_on_reason(void)
{
	/* 讀取當下按鍵狀態 */
	uint8_t pin_state = (uint8_t)GPIO_PinRead(GPIO, NXP_BQ_MR_N_PORT, NXP_BQ_MR_N_PIN);

		if (pin_state == 0U)
		{
			/* 按鍵為低（按下）→ 需連續 2 秒才允許離開 ship mode */
			if (power_key_low_for_ms(BOOT_LONG_PRESS_MS))
			{
				PRINTF("[PCA9422] PCA9422 leave ship mode (press >= %u ms)\r\n", BOOT_LONG_PRESS_MS);

				if (hal_power_is_power_off_charging_mode()) {
					return POWER_ON_BUTTON_AND_CHARGER;
				}
				return POWER_ON_BUTTON;
			}
			else
			{
				/* 沒達到 2 秒長按 → 進入 ship mode */
				PRINTF("[PCA9422] Power key (press < %u ms)\r\n", BOOT_LONG_PRESS_MS);
				//hal_pmic_pca9422_enter_ship_mode();
				//hal_pmic_pca9422_power_down();
//				bq256xx_enter_ship_mode();
				if (hal_power_is_power_off_charging_mode()) {
					return POWER_ON_CHARGER;
				}
				return POWER_ON_UNEXPECTED;
			}
		}
		else
		{
			/* 沒有按住按鍵（高電位）→ 直接進入 ship mode */
			//hal_pmic_pca9422_enter_ship_mode();
			//hal_pmic_pca9422_power_down();
//			bq256xx_enter_ship_mode();
			if (hal_power_is_power_off_charging_mode()) {
				return POWER_ON_CHARGER;
			}
			return POWER_ON_UNEXPECTED;

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
#if TOUCH_AW93305_ENABLE
	hal_touch_aw93305_init(); //Touch Init
#endif

#if AMP_AW88166_ENABLE
	hal_amp_aw88166_power_on();
	hal_amp_aw88166_init(); // Init AMP
#endif

	hal_scan_i2c_devices(BOARD_PMIC_I3C_BASEADDR);

	power_on_reason = get_power_on_reason();

	if (power_on_reason == POWER_ON_CHARGER) {
		hal_power_go_to_power_off_charging();
	} else if (power_on_reason == POWER_ON_UNEXPECTED) {
		bq256xx_enter_ship_mode();
	}
	PRINTF("[System] Version= %s \n", HAL_MCU_APP_VERSION);

#if PMIC_GLF70583_ENABLE
	hal_pmic_glf70583_actual_board_init();
#endif

//	hal_soc_enable();

#if LED_KTD2027_ENABLE
	hal_led_ktd2027_power_on_indicator(); //White light turns on first
#endif

#if FG_GLF70302_ENABLE
	hal_power_gauge_glf70302_get_battery_level(); //Read the battery level after powering on
#if 0
	//battery_info.soc = hal_power_get_battery_percentage(battery_info.voltage);
#endif
	ss_set_battery(hal_power_get_battery_percentage(battery_info.soc));
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


#if TOUCH_EWD608_ENABLE
	hal_touch_ewd608_init();
#endif

#if SAR_SX9204_ENABLE
	hal_sar_init();
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



void led_post_event(hal_led_event_t e)
{
    g_led_event = e;
#if 0
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, LED_EVENT_BIT);
    }
#endif
    has_led_event = 1;
}


void sar_post_event(void *param)
{
	has_sar_event = 1;
}



void I2C_Task(void *pvParameters)
{
    while (1)
    {
    	i2c_device_handler();
    }
}



void i2c_device_handler (void)
{

    /* --- SAR event --- */
    if (has_sar_event)
    {
    	has_sar_event = 0;

#if SAR_SX9204_ENABLE
    	hal_sar_handler();
#endif
        GPIO_PinClearInterruptFlag(GPIO, PROX1_INT_N_PORT, PROX1_INT_N_PIN, kGPIO_InterruptA);
        GPIO_PinEnableInterrupt(GPIO, PROX1_INT_N_PORT, PROX1_INT_N_PIN, kGPIO_InterruptA);
    }


	/* --- TOUCH event --- */
	if (hal_touch_has_new_event())
	{

#if TOUCH_EWD608_ENABLE
		hal_touch_ewd608_set_state();
		hal_touch_ewd608_handler();

#endif

#if TOUCH_AW93305_ENABLE
		hal_touch_aw93305_handler();
#endif
		/* 任務側重新啟用觸控中斷（先清旗標再開） */
		GPIO_PinClearInterruptFlag(GPIO, NXP_TOUCH_INT_PORT, NXP_TOUCH_INT_PIN, kGPIO_InterruptA);
		GPIO_PinEnableInterrupt(GPIO, NXP_TOUCH_INT_PORT, NXP_TOUCH_INT_PIN, kGPIO_InterruptA);
	}

	/* --- AMP event --- */

	if (hal_amp_has_new_event()) {

#if AMP_AW88166_ENABLE
		hal_amp_aw88166_handler();
#endif
	}

	/* --- CHARGER event --- */
	if (hal_power_charger_has_new_event())
	{
#if CHG_BQ25618_ENABLE
		hal_power_charger_bq25618_handler();
#endif
		GPIO_PinClearInterruptFlag(GPIO, CHG_INT_N_R_PORT, CHG_INT_N_R_PIN, kGPIO_InterruptA);
		GPIO_PinEnableInterrupt(GPIO, CHG_INT_N_R_PORT, CHG_INT_N_R_PIN, kGPIO_InterruptA);
	}

	/* --- GAUGE event --- */
	if (has_gauge_event)
	{
		has_gauge_event = 0;
#if FG_GLF70302_ENABLE
		glf70302_polling(&battery_info);
#if 0
		//battery_info.soc = hal_power_get_battery_percentage(battery_info.voltage);
#endif
		uint8_t battery_level = hal_power_get_battery_percentage(battery_info.soc);
		PRINTF("[Battery] SOC: %d%%, battery_level: %d%%\r\n",battery_info.soc, battery_level);
		if (ss_is_charging()) {
			if (battery_level >= FULLY_CHARGE_PERCENTAGE) {
				hal_power_set_battery_state(BATTERY_STATE_FULL);
				hal_led_set_situation(HAL_LED_STATUS_CHARGING, SITUATION_DISABLE);
				hal_led_set_situation(HAL_LED_STATUS_FULL_CHARGED, SITUATION_ENABLE);
				hal_led_set_situation(HAL_LED_STATUS_LOW_BATTERY, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH);
			}
			else if (battery_level > LOW_POWER_PERCENTAGE)
			{
				hal_power_set_battery_state(BATTERY_STATE_NORMAL);
				hal_led_set_situation(HAL_LED_STATUS_LOW_BATTERY, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH);
			}
		} else {
			if (battery_level <= LOW_POWER_PERCENTAGE)
			{
				hal_power_set_battery_state(BATTERY_STATE_LOW);
				hal_led_set_situation(HAL_LED_STATUS_LOW_BATTERY, SITUATION_ENABLE);
				hal_led_set_situation(HAL_LED_STATUS_CHARGING, SITUATION_DISABLE);
				hal_led_set_situation(HAL_LED_STATUS_FULL_CHARGED, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH);
			}
			else
			{
				hal_power_set_battery_state(BATTERY_STATE_NORMAL);
				hal_led_set_situation(HAL_LED_STATUS_LOW_BATTERY, SITUATION_DISABLE);
				hal_led_set_situation(HAL_LED_STATUS_CHARGING, SITUATION_DISABLE);
				hal_led_set_situation(HAL_LED_STATUS_FULL_CHARGED, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH);
			}
		}
		ss_set_battery(battery_level);
		if(battery_info.voltage<=3500 && ss_is_charging() == false)//Automatic shutdown when battery voltage drops below 3.5V
		{
			PRINTF("[Gauge] Low battery, so it shuts down. \r\n");
#if SOC_SPI_ENABLE
				if (Novatek_boot_completed) {
					//send_spi_request(POWER_LONG_PRESS_HEX_VALUE);
				} else {
					general_RingtoneState = Ringtone_PowerOFF;
					vTaskDelay(pdMS_TO_TICKS(200));
					led_post_event(HAL_LED_EVENT_POWER_OFF_PROGRESS);
				}
#endif
//                        led_post_event(HAL_LED_EVENT_POWER_OFF_PROGRESS);
//                        general_RingtoneState = Ringtone_PowerOFF;
		}
#endif
		if (is_first_read_battery_level) {
			is_first_read_battery_level = 0;
		}
		GPIO_PinClearInterruptFlag(GPIO, FG_INT_GLF70302_PORT, FG_INT_GLF70302_PIN, kGPIO_InterruptA);
		GPIO_PinEnableInterrupt(GPIO, FG_INT_GLF70302_PORT, FG_INT_GLF70302_PIN, kGPIO_InterruptA);
	}

	/*--- LED event --- */
	if (has_led_event) {
		has_led_event = 0;
#if LED_KTD2027_ENABLE
		vTaskDelay(1); /* 確保 g_led_event 已更新 */
		hal_led_event_handler(g_led_event);
#endif
	}

	if(System_Status && Novatek_boot_completed)
	{
		System_Status=0;
#if SOC_SPI_ENABLE
		//send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
	}
}

