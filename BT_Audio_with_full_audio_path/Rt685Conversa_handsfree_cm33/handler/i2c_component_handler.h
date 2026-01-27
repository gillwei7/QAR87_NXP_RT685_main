/*
 * i2_component_handler.h
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#ifndef I2C_COMPONENT_HANDLER_H_
#define I2C_COMPONENT_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "fsl_debug_console.h"
#include "board.h"

#include "pmic_support.h"
#include "pmic_pca9422.h"
#include "glf70583.h"
#include "aw933xx.h"
#include "aw93305.h"
#include "bq256xx_charger.h"
#include "glf70302_driver.h"
#include "aw88166.h"
#include "sx9324.h"


#define SCAN_I2C_ADDRESS_ENABLE 1
#define LED_ON 					10

/* ===== I2C EventGroup bits (for unified I2C_Task) ===== */
#define TOUCH_EVENT_BIT      (1UL << 0)
#define CHARGER_EVENT_BIT    (1UL << 1)
#define GAUGE_EVENT_BIT      (1UL << 2)
#define LED_EVENT_BIT        (1UL << 3)   /* LED task wake-up flag */
#define AMP_EVENT_BIT     	 (1UL << 4)
#define GAUGE_2_EVENT_BIT    (1UL << 5)
#define SAR_EVENT_BIT 		 (1UL << 6)

/*===== AMP ===== */
typedef enum {
    AMP_EVT_NONE = 0,
    AMP_EVT_MUSIC_START,
	AMP_EVT_RECEIVER_START,
    AMP_EVT_STOP,
} amp_event_t;

typedef enum {
    AMP_MODE_MUSIC = 0,
    AMP_MODE_RECEIVER = 1,
} amp_mode_t;

/*===== LED ===== */
/* LED events (single, centralized event model) */
typedef enum {
	LED_EVT_NONE = 0,
	LED_EVT_POWER_ON_PROGRESS,       /* 白燈常亮直到ready */
	LED_EVT_POWER_OFF_PROGRESS,      /* 紅燈亮1秒 */
	LED_EVT_PHOTO_CAPTURE,           /* 白燈閃一次 */
	LED_EVT_REFRESH,                 /* Refresh the led indicator */
	LED_EVT_ALL_OFF                  /* 全部關閉 */
} led_event_t;


typedef enum {
    POWER_ON_BUTTON = 0,
    POWER_ON_CHARGER,
    POWER_ON_BUTTON_AND_CHARGER,
    POWER_ON_UNEXPECTED,
} power_on_reason_t;



//void pca9422_ship_mode(void);
//void pca9422_power_down(void);

void Init_I2C_Component(void);

void battery_timer_start(void);
void led_post_event(led_event_t e);
void amp_post_event(amp_event_t e);

//void Scan_I2C_Devices(I3C_Type *base);
void I2C_Task(void *pvParameters);


#endif /* I2C_COMPONENT_HANDLER_H_ */
