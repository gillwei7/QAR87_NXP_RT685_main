/*
 * hal_led.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_LED_H_
#define HAL_LED_H_

#include "hal_common.h"


void hal_led_ktd2027_init(void);
void hal_led_ktd2027_power_on_indicator(void);
void hal_led_ktd2027_power_off_indicator(void);
void hal_led_ktd2027_charging_indicator(void);
void hal_led_ktd2027_low_battery_indicator(void);
void hal_led_ktd2027_full_charged_indicator(void);
void hal_led_ktd2027_take_photo_indicator(void);
void hal_led_ktd2027_recording_indicator(void);
void hal_led_ktd2027_pairing_indicator(void);
void hal_led_ktd2027_ota_indicator(void);
void hal_led_ktd2027_ota_success_indicator(void);
void hal_led_ktd2027_ota_fail_indicator(void);
void hal_led_ktd2027_off(void);



#endif /* HAL_LED_H_ */
#endif
