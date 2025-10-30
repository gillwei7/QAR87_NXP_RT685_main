/*
 * hal_touch.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_touch.h"
#include "aw93305.h"

extern volatile struct aw933xx_dev aw933xx;

void hal_touch_aw93305_init(void)
{
	awinic_single_enter();
}

void hal_touch_aw93305_gesture_callback(void)
{
	AW93305_EXTI_Callback();
}

#endif
