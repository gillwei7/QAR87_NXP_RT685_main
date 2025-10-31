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

void hal_amp_aw88166_power_on(void);
void hal_amp_aw88166_power_off(void);
void hal_amp_aw88166_init(void);
/* prof_name: 1. "Music", 2. "Receiver"(Be used in phone call) */
void hal_amp_aw88166_left_start(char *prof_name);
void hal_amp_aw88166_right_start(char *prof_name);
void hal_amp_aw88166_left_stop(void);
void hal_amp_aw88166_right_stop(void);

#endif


#endif /* HAL_AMP_DRIVER_H_ */
