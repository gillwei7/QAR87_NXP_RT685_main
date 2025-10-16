/*
 * led_status.h
 *
 *  Created on: 2025年10月16日
 *      Author: 11301026
 */

#ifndef LED_STATUS_H_
#define LED_STATUS_H_

#define LED_ON 10

/* LED events (single, centralized event model) */
typedef enum {
    LED_EVT_NONE = 0,
    LED_EVT_POWER_ON_PROGRESS,       /* 白燈常亮直到ready */
	LED_EVT_POWER_OFF_PROGRESS,		 /* 紅燈亮1秒 */
    LED_EVT_CHARGING,                /* 紅燈閃爍(週期1秒) */
    LED_EVT_LOW_BATTERY,             /* 紅燈閃爍(週期5秒) */
	LED_EVT_FULL_CHARGERED,			 /* 綠燈常亮 */
	LED_EVT_PHOTO_CAPTURE,			 /* 白燈閃一次 */
	LED_EVT_VIDEO_CAPTURE,			 /* 白燈閃爍(週期1秒) */
	LED_EVT_PAIRING_MODE,			 /* 藍燈閃爍 */
	LED_EVT_OTA_PROGRESS,			 /* 白燈閃爍 */
	LED_EVT_OTA_SUCCESS,			 /* 綠燈呼吸 */
	LED_EVT_OTA_FAIL,			     /* 紅燈呼吸 */
    LED_EVT_ALL_OFF                  /* 全部關閉 */
} led_event_t;

#endif /* LED_STATUS_H_ */
