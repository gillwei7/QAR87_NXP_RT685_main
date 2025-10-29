/*
 * hal_touch.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_TOUCH_H_
#define HAL_TOUCH_H_

#include "hal_common.h"


void hal_power_charger_aw93305_init(void);
void hal_power_charger_aw93305_get_gesture(void);


#endif /* HAL_TOUCH_H_ */
#endif
