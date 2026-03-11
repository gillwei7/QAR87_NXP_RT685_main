/*
 * button_handler.h
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#ifndef BUTTON_HANDLER_H_
#define BUTTON_HANDLER_H_
#if 0
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_pint.h"

/* pca9422 ship mode  */
#define LONG_PRESS_MS    (2000U)  // Must last 2 seconds
#define SAMPLE_MS        (10U)    // Polling interval 10ms, with simple anti-shake function

/* === Button behavior parameters === */
#define BTN_5TIMES_IN_LIMIT   6000U   //Click five times within a time limit
#define BTN_HOLD_MS 		  1000U
#define BTN_ACTIVE_LEVEL          0   // active-low: pressed = 0, released = 1; if the opposite is true, it will be 1
#define BTN_DEBOUNCE_MS          30
#define BTN_LONG_MS            1000
#define BTN_DBLCLICK_GAP_MS     300
#define PWR_ACTIVE_LEVEL          0      // active-low: pressed = 0; if active-high, change to 1
#define PWR_DEBOUNCE_MS          30
#define PWR_LONG_MS            5000      // Press and hold for 3 seconds (judgment only after releasing)
#define PWR_MIN_SHORT_MS         50      // Too short jitter/false touch (<50ms) is not considered a short press
#define POWER_KEY_PINT_CH  		  0   // PINT channel
#define FUN_KEY1_PINT_CH  		  1   // PINT channel

void button_task(void *pvParameters);

void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status);

bool power_key_low_for_ms(uint32_t ms) ; //for pca9422 ship mode
#endif
#endif /* BUTTON_HANDLER_H_ */
