/*
 * soc_handler.h
 *
 *  Created on: Apr 20, 2026
 *      Author: Lydia
 */

#ifndef SOC_HANDLER_H_
#define SOC_HANDLER_H_

#include "hal_common.h"

typedef enum {
    SOC_POWER_ON_ACTION_OE,
    SOC_POWER_ON_ACTION_CAPTURE,
    SOC_POWER_ON_ACTION_RECORDING,

    SOC_POWER_ON_ACTION_TOTAL,
} soc_power_on_action_t;


uint8_t soc_get_power_status (void);
void soc_power_down (void);
void soc_power_on (soc_power_on_action_t action);
void soc_power_toggle (void);

#endif /* SOC_HANDLER_H_ */
