/*
 * hal_led.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_LED_H_
#define HAL_LED_H_

#include "hal_common.h"


void hal_led_ktd2027_init(void);
void hal_led_ktd2027_red_on(void);
void hal_led_ktd2027_red_blink(void);
void hal_led_ktd2027_green_on(void);
void hal_led_ktd2027_green_blink(void);
void hal_led_ktd2027_blue_on(void);
void hal_led_ktd2027_blue_blink(void);



#endif /* HAL_LED_H_ */
#endif
