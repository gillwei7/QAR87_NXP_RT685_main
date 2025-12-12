/*
 * hal_led.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_led.h"
#include "ktd202x_leds.h"



void hal_led_ktd2027_init(void)
{
    ktd202x_probe();
}

void hal_led_ktd2027_power_on_indicator(void)
{
    ktd202x_ch2_led_on(LED_CURRENT_CH2);
    hal_loop_delay_ms(500);
    ktd202x_led_off();

    ktd202x_ch3_led_on(LED_CURRENT_CH3);
    hal_loop_delay_ms(500);
    ktd202x_led_off();

    ktd202x_ch1_led_on(LED_CURRENT_CH1);
    hal_loop_delay_ms(500);
    ktd202x_led_off();

    ktd202x_ch4_led_on(LED_CURRENT_CH4);
}

void hal_led_ktd2027_power_off_indicator(void)
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

void hal_led_ktd2027_low_battery_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch2_led_blink(500, 4500, TIM_1);

}

void hal_led_ktd2027_full_charged_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch3_led_on(LED_CURRENT_CH3);

}

void hal_led_ktd2027_take_photo_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch4_led_on(LED_CURRENT_CH4);
    vTaskDelay(50);
    ktd202x_ch4_led_off();

}

void hal_led_ktd2027_recording_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch4_led_blink(500, 500, TIM_2);

}

void hal_led_ktd2027_pairing_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch1_led_blink(100, 100, TIM_2);

}

void hal_led_ktd2027_ota_indicator(void)
{
    ktd202x_led_off();
    ktd202x_ch4_led_blink(300, 300, TIM_1);

}

void hal_led_ktd2027_ota_success_indicator(void)
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

void hal_led_ktd2027_ota_fail_indicator(void)
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

#endif

