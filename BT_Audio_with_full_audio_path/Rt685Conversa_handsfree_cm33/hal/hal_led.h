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

/**@brief Enumeration type of the led situation status . */
typedef enum
{
	HAL_LED_STATUS_OFF                = 0x00,
	HAL_LED_STATUS_CHARGING           = 0x01,
	HAL_LED_STATUS_FULL_CHARGED       = 0x02,
	HAL_LED_STATUS_LOW_BATTERY        = 0x04,
	HAL_LED_STATUS_RECORDING          = 0x08,
	HAL_LED_STATUS_PAIRING            = 0x10,
	HAL_LED_STATUS_OTA                = 0x20,
	HAL_LED_STATUS_OTA_SUCCESS        = 0x40,
	HAL_LED_STATUS_OTA_FAILED         = 0x80,
} hal_led_status_t;


/*===== LED ===== */
/* LED events (single, centralized event model) */
typedef enum {
	HAL_LED_EVENT_NONE = 0,
	HAL_LED_EVENT_POWER_ON_PROGRESS,       /* 白燈常亮直到ready */
	HAL_LED_EVENT_POWER_OFF_PROGRESS,      /* 紅燈亮1秒 */
	HAL_LED_EVENT_PHOTO_CAPTURE,           /* 白燈閃一次 */
	HAL_LED_EVENT_REFRESH,                 /* Refresh the led indicator */
	HAL_LED_EVENT_ALL_OFF                  /* 全部關閉 */
} hal_led_event_t;


void hal_led_ktd2027_init(void);
hal_led_indicator_status_t hal_led_get_indicator_status (void);
void hal_led_set_indicator_status (hal_led_indicator_status_t status);
void hal_led_set_situation (uint8_t situation, uint8_t enable);

void hal_led_ktd2027_power_on_indicator(void);
void hal_led_ktd2027_full_charged_indicator(void);
void hal_led_ktd2027_charging_indicator(void);
void hal_led_ktd2027_off(void);
void hal_led_event_handler (hal_led_event_t event);

#endif /* HAL_LED_H_ */
#endif
