/*
 * soc_handler.c
 *
 *  Created on: Apr 20, 2026
 *      Author: Lydia
 */


#include "soc_handler.h"
#include "hal_pmic.h"

static uint8_t s_soc_power_status = 0;

uint8_t soc_get_power_status (void) {
	return s_soc_power_status;
}

static void soc_enable_power (uint8_t enable) {
	if (enable) {
		hal_pmic_glf70583_cutoff_nt98532();
		hal_pmic_glf70583_turn_on_nt98532();

	} else {
		hal_pmic_glf70583_cutoff_nt98532();
	}
}

void soc_power_down (void) {
	soc_enable_power(0);
	s_soc_power_status = 0;
}

void soc_power_on (soc_power_on_action_t action) {
	if (action == SOC_POWER_ON_ACTION_OE) {
		//TODO Set GPIO for action
		soc_enable_power(1);
		hal_soc_enable();
		s_soc_power_status = 1;

	} else if (action == SOC_POWER_ON_ACTION_CAPTURE) {
		//TODO Set GPIO for action
		soc_enable_power(1);
		hal_soc_enable();
		s_soc_power_status = 1;
	} else if (action == SOC_POWER_ON_ACTION_RECORDING) {
		//TODO Set GPIO for action
		soc_enable_power(1);
		hal_soc_enable();
		s_soc_power_status = 1;

	}
}

static uint8_t s_soc_power_action_count = SOC_POWER_ON_ACTION_OE;
void soc_power_toggle (void) { // for test only
	if (s_soc_power_status == 1) {
		soc_power_down();
	} else {
		soc_power_on(s_soc_power_action_count);
		s_soc_power_action_count++;
		if (s_soc_power_action_count >= SOC_POWER_ON_ACTION_TOTAL) {
			s_soc_power_action_count == SOC_POWER_ON_ACTION_OE;
		}
	}
}

