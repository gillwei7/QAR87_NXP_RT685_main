/*
 * hal_amp_driver.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */

#ifndef HAL_AMP_DRIVER_H_
#define HAL_AMP_DRIVER_H_

/* Quanta hal functions for amp */

#include "hal_common.h"
#include "aw88166.h"

void hal_amp_aw88166_power_on(void);
void hal_amp_aw88166_init(void);
void hal_amp_aw88166_left_start(char *prof_name);
void hal_amp_aw88166_right_start(char *prof_name);
void hal_amp_aw88166_left_stop(void);
void hal_amp_aw88166_right_stop(void);



#endif /* HAL_AMP_DRIVER_H_ */
