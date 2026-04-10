/*
 * hal_amp_driver.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_AMP_DRIVER_H_
#define HAL_AMP_DRIVER_H_

/* Quanta hal functions for amp */

#include "hal_common.h"
#include "aw88166.h"


/*===== AMP ===== */
typedef enum {
	AMP_EVT_STOP = 0,
    AMP_EVT_MUSIC,
	AMP_EVT_RECEIVER,
} amp_event_t;

typedef enum {
    AMP_MODE_MUSIC = 0,
    AMP_MODE_RECEIVER = 1,
} amp_mode_t;



void hal_amp_aw88166_power_on(void);
void hal_amp_aw88166_power_off(void);
void hal_amp_aw88166_init(void);


void hal_amp_aw88166_handler (void);
void amp_post_event(amp_event_t e);
uint8_t hal_amp_has_new_event(void);
#endif


#endif /* HAL_AMP_DRIVER_H_ */
