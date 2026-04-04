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


void hal_touch_aw93305_init(void);
void hal_touch_aw93305_handler(void);

void hal_touch_ewd608_init (void);
void hal_touch_ewd608_handler (void);
void hal_touch_ewd608_set_state (void);
uint8_t hal_touch_has_new_event(void);

#endif /* HAL_TOUCH_H_ */
#endif
