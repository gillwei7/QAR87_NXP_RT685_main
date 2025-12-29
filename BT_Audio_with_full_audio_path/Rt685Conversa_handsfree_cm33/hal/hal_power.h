/*
 * hal_power.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_POWER_H_
#define HAL_POWER_H_

#include "hal_common.h"


void hal_power_charger_bq25618_init(void);
void hal_power_charger_bq25618_get_charging_status(void);

void hal_power_gauge_glf70302_init(void);
void hal_power_gauge_glf70302_get_battery_level(void);

#if 0
uint8_t hal_power_get_battery_percentage (uint32_t mv);
#endif
uint8_t hal_power_get_battery_percentage(uint8_t soc);
uint8_t hal_power_is_power_off_charging_mode(void);
void hal_power_go_to_power_off_charging(void);

#endif /* HAL_POWER_H_ */
#endif
