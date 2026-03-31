/*
 * buttons_handler.h
 *
 *  Created on: Mar 11, 2026
 *      Author: Lydia
 */

#ifndef BUTTONS_HANDLER_H_
#define BUTTONS_HANDLER_H_


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
#include "hal_common.h"
#include "hal_led.h"

#define BOOT_LONG_PRESS_MS            (2000U)  // Press and hold the button for 3 seconds to power on (1 s to exit ship mode + 2 s hold)

#define POWER_BUTTON_PINT_CH          0   // PINT channel
#define FUNCTION_BUTTON_PINT_CH       1   // PINT channel

void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status);

bool power_key_low_for_ms(uint32_t ms) ; //for ship mode


#endif /* BUTTONS_HANDLER_H_ */
