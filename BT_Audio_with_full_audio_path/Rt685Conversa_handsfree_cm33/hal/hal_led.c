/*
 * hal_led.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_led.h"
#include "ktd202x_leds.h"


static hal_led_indicator_status_t led_indicator_status_t = HAL_LED_OFF;
static uint8_t led_situation = HAL_LED_EVENT_OFF;
static uint8_t led_has_new_situation = 1;


void hal_led_ktd2027_power_on_indicator(void)
{
    ktd202x_ch2_led_on(LED_CURRENT_CH2);
    hal_delay_ms(500);
    ktd202x_led_off();

    ktd202x_ch3_led_on(LED_CURRENT_CH3);
    hal_delay_ms(500);
    ktd202x_led_off();

    ktd202x_ch1_led_on(LED_CURRENT_CH1);
    hal_delay_ms(500);
    ktd202x_led_off();

    ktd202x_ch4_led_on(LED_CURRENT_CH4);
}

static void hal_led_ktd2027_power_off_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch2_led_on(LED_CURRENT_CH2);
    vTaskDelay(pdMS_TO_TICKS(10000));
    ktd202x_ch2_led_off();
}

void hal_led_ktd2027_charging_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch2_led_blink(500, 500, TIM_1);

}

static void hal_led_ktd2027_low_battery_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch2_led_blink(500, 4500, TIM_1);

}

void hal_led_ktd2027_full_charged_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch3_led_on(LED_CURRENT_CH3);

}

static void hal_led_ktd2027_take_photo_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch4_led_on(LED_CURRENT_CH4);
    vTaskDelay(1250);
    ktd202x_ch4_led_off();

}

static void hal_led_ktd2027_recording_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch4_led_blink(500, 500, TIM_2);

}

static void hal_led_ktd2027_pairing_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch1_led_blink(100, 100, TIM_2);

}

static void hal_led_ktd2027_ota_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch4_led_blink(300, 300, TIM_1);

}

static void hal_led_ktd2027_ota_success_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch3_led_breathe(PERIOD_CODE_1P5S,
                            RISE_CODE_600MS,
                            FALL_CODE_600MS,
                            ON_PERCENT_60,
                            RAMP_SCALE_2X_SLOW,
                            true,
                            LED_CURRENT_CH3 );

}

static void hal_led_ktd2027_ota_fail_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch2_led_breathe(PERIOD_CODE_1P5S,
                            RISE_CODE_600MS,
                            FALL_CODE_600MS,
                            ON_PERCENT_60,
                            RAMP_SCALE_2X_SLOW,
                            true,
                            LED_CURRENT_CH2);

}

void hal_led_ktd2027_off(void)
{
    ktd202x_led_off();

}

void hal_led_ktd2027_init(void)
{
    ktd202x_probe();
}

hal_led_indicator_status_t hal_led_get_indicator_status (void)
{
	return led_indicator_status_t;
}

void hal_led_set_indicator_status (hal_led_indicator_status_t status)
{

	switch (status) {
	case HAL_LED_OFF:
		hal_led_ktd2027_off();
		break;
	case HAL_LED_POWER_ON:
		hal_led_ktd2027_power_on_indicator();
		break;
	case HAL_LED_POWER_OFF:
		hal_led_ktd2027_power_off_indicator();
		break;
	case HAL_LED_TAKE_PHOTO:
		hal_led_ktd2027_take_photo_indicator();
		led_has_new_situation = 1;
		break;
	case HAL_LED_REFRESH:
		led_has_new_situation = 1;
		break;
	default:
		break;
	}
}

void hal_led_refresh (void)
{
	led_has_new_situation = 1;
}

void hal_led_status_handler (void) {

	if (!led_has_new_situation) {
		return;
	}

	led_has_new_situation = 0;


	if (led_situation & HAL_LED_EVENT_PAIRING) {
		hal_led_ktd2027_pairing_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_RECORDING) {
		hal_led_ktd2027_recording_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_OTA) {
		hal_led_ktd2027_ota_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_OTA_SUCCESS) {
		hal_led_ktd2027_ota_success_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_OTA_FAILED) {
		hal_led_ktd2027_ota_fail_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_FULL_CHARGED) {
		hal_led_ktd2027_full_charged_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_CHARGING) {
		hal_led_ktd2027_charging_indicator();
		return;

	} else if (led_situation & HAL_LED_EVENT_LOW_BATTERY) {
		hal_led_ktd2027_low_battery_indicator();
		return;

	} else {
		hal_led_ktd2027_off();
		return;
	}
}

void hal_led_set_situation (uint8_t situation, uint8_t enable) {
	if (enable == SITUATION_ENABLE) {
		if (led_situation != (led_situation | situation)) {
			led_situation = led_situation | situation;
			led_has_new_situation = 1;
			PRINTF("[HAL_LED] led_situation: 0x%02x\r\n", led_situation);
		}
	} else if (enable == SITUATION_DISABLE) {
		if (led_situation != (led_situation & ~situation)) {
			led_situation = led_situation & ~situation;
			led_has_new_situation = 1;
			PRINTF("[HAL_LED] led_situation: 0x%02x\r\n", led_situation);
		}
	}
}


#endif

