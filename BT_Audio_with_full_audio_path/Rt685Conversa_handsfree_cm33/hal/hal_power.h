/*
 * hal_power.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */

#ifndef HAL_POWER_H_
#define HAL_POWER_H_

#include "hal_common.h"


void hal_power_charger_bq25618_init(void);
void hal_power_charger_bq25618_get_charging_status(void);

void hal_power_gauge_glf70302_init(void);
void hal_power_gauge_glf70302_get_battery_level(void);


#endif /* HAL_POWER_H_ */
