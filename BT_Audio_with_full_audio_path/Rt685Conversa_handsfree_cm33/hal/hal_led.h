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

typedef enum {
	HAL_LED_OFF = 0,
	HAL_LED_POWER_ON,
	HAL_LED_POWER_OFF,
	HAL_LED_TAKE_PHOTO,
	HAL_LED_REFRESH,
} hal_led_indicator_status_t;

/**@brief Enumeration type of the led situation event . */
typedef enum
{
	HAL_LED_EVENT_OFF                = 0x00,
	HAL_LED_EVENT_CHARGING           = 0x01,
	HAL_LED_EVENT_FULL_CHARGED       = 0x02,
	HAL_LED_EVENT_LOW_BATTERY        = 0x04,
	HAL_LED_EVENT_RECORDING          = 0x08,
	HAL_LED_EVENT_PAIRING            = 0x10,
	HAL_LED_EVENT_OTA                = 0x20,
	HAL_LED_EVENT_OTA_SUCCESS        = 0x40,
	HAL_LED_EVENT_OTA_FAILED         = 0x80,
} HAL_LED_EVENT_T;


void hal_led_ktd2027_init(void);
hal_led_indicator_status_t hal_led_get_indicator_status (void);
void hal_led_set_indicator_status (hal_led_indicator_status_t status);
void hal_led_set_situation (uint8_t situation, uint8_t enable);
void hal_led_refresh (void);
void hal_led_status_handler (void);

void hal_led_ktd2027_power_on_indicator(void);
void hal_led_ktd2027_full_charged_indicator(void);
void hal_led_ktd2027_charging_indicator(void);
void hal_led_ktd2027_off(void);

#endif /* HAL_LED_H_ */
#endif
