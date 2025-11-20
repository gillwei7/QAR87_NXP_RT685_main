/*
 * hal_power.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_power.h"
#include "bq256xx_charger.h"
#include "glf70302_driver.h"

volatile bq256xx_status_t charger_status;
volatile BatteryInfo battery_info;

void hal_power_charger_bq25618_init(void)
{
#if CHG_BQ25618_ENABLE
	/* ============== Charger Init Start==============*/
		bq256xx_cfg_t charger_cfg = {
				.vindpm_uv = 4450000,
				.iindpm_ua = 2000000,
				.ichg_ua = 530000,
				.vbatreg_uv = 4005000,
				.iprechg_ua = 60000,
				.iterm_ua = 20000,
				.wdt_ms = 0
		};
		status_t bq_ret = bq256xx_init(&charger_cfg);
		if ( bq_ret!= kStatus_Success) {
			PRINTF("[Charger] bq256xx init failed!,ret:%d \n",bq_ret);

		}
		else{
			PRINTF("[Charger] bq256xx initialized.OK \n");
		}
		bq256xx_write_reg(0x03, 0x31); // IPRECHG = 60mA, ITERM = 20mA
	/* ============== Charger Init End==============*/
#endif
}

void hal_power_charger_bq25618_get_charging_status(void)
{
#if CHG_BQ25618_ENABLE
	bq256xx_poll_status(&charger_status);
#endif
}


void hal_power_gauge_glf70302_get_battery_level(void)
{
#if FG_GLF70302_ENABLE
	glf70302_polling(&battery_info); //Read the battery level after powering on
#endif
}

#endif
