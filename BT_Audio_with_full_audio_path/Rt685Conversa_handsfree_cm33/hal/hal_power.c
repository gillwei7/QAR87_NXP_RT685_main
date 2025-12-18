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
#include "hal_led.h"
#include "hal_pmic.h"

#define EN_HIZ_BIT      7
#define EN_HIZ_MASK     (1u << EN_HIZ_BIT)

#define BATTERY_FULL_VOLTAGE   4450//(mV)
#define BATTERY_EMPTY_VOLTAGE  3300//(mV)


volatile bq256xx_status_t charger_status;
volatile BatteryInfo battery_info;

uint32_t s_bq256xx_iindpm_target_ua = 0;

static uint8_t is_booting = 1;
static uint8_t is_power_off_charging_mode = 0;

void hal_power_charger_bq25618_init(void)
{
#if CHG_BQ25618_ENABLE
	/* ============== Charger Init Start==============*/
		bq256xx_cfg_t charger_cfg = {
				.vindpm_uv = 4500000, //4.5V
				.iindpm_ua = 2000000, //2.0A
				.ichg_ua = 600000,    //600mA
				.vbatreg_uv = 4450000,//4.45V
				.iprechg_ua = 20000,  //20mA
				.iterm_ua = 20000,    //20mA
				.wdt_ms = 0
		};
		status_t bq_ret = bq256xx_init(&charger_cfg);
		if ( bq_ret!= kStatus_Success) {
			PRINTF("[Charger] bq256xx init failed!,ret:%d \n",bq_ret);

		}
		else{
			PRINTF("[Charger] bq256xx initialized.OK \n");
		}

		uint8_t reg_val = 0;
		bq256xx_read_reg(0x00, &reg_val, 1);
		if ((reg_val & EN_HIZ_MASK) != 0)
		{
			uint8_t new_val = (uint8_t)(reg_val & ~EN_HIZ_MASK);
			bq256xx_write_reg(0x00, new_val);
		}

		bq256xx_write_reg(0x03, 0x11); // IPRECHG = 20mA, ITERM = 20mA
		bq256xx_write_reg(0x04, 0xC0); // VBATREG = 4.45V
		s_bq256xx_iindpm_target_ua = charger_cfg.iindpm_ua;
		/*
	    uint8_t val;
	    for (uint8_t reg = 0x00; reg <= 0x0A; reg++)
	    {
	    	(void)bq256xx_read_reg(reg, &val, 1);
	    	 PRINTF("[Debug][bq256xx] REG %u = 0x%02X\r\n", reg, val);
	    }
		*/
	/* ============== Charger Init End==============*/
#endif
}

void hal_power_charger_bq25618_get_charging_status(void)
{
#if CHG_BQ25618_ENABLE
	bq256xx_poll_status(&charger_status);
#endif
}

void hal_power_gauge_glf70302_init(void)
{
#if FG_GLF70302_ENABLE
	glf70302_init();//Gauge Init
#endif
}

void hal_power_gauge_glf70302_get_battery_level(void)
{
#if FG_GLF70302_ENABLE
	glf70302_polling(&battery_info); //Read the battery level after powering on
#endif
}

uint8_t hal_power_get_battery_percentage (uint32_t mv)
{
    // 邊界保護
    if (mv <= BATTERY_EMPTY_VOLTAGE) return 0;
    if (mv >= BATTERY_FULL_VOLTAGE)  return 100;

    // 線性比例： (mv - empty) * 100 / (full - empty)
    uint32_t range = (uint32_t)(BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE);
    uint32_t num   = (uint32_t)(mv - BATTERY_EMPTY_VOLTAGE) * 100u;

    // 四捨五入：+range/2
    uint8_t soc = (uint8_t)((num + (range / 2u)) / range);

    return soc;   // 0~100
}

uint8_t hal_power_is_power_off_charging_mode(void) {
	if (is_booting) {
		is_booting = 0;
		SDK_DelayAtLeastUs(100 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk)); //Delay 100ms
		hal_power_charger_bq25618_get_charging_status();
		if(charger_status.vbus_good)
		{
			is_power_off_charging_mode = 1;
		}
	}
	return is_power_off_charging_mode;
}

void power_off_charging(void)
{
	uint8_t LED_state=0; //1->charging, 2->full-charge

	if(hal_power_is_power_off_charging_mode())
	{
		PRINTF("[System] power off charging \r\n");
		while(1)
		{

			hal_power_charger_bq25618_get_charging_status();
			hal_power_gauge_glf70302_get_battery_level();
			battery_info.soc = hal_power_get_battery_percentage(battery_info.voltage);
			if(charger_status.vbus_good)
			{
				switch (LED_state) {
					case 0:
							hal_led_ktd2027_off();
							hal_led_ktd2027_charging_indicator();
							LED_state = 1;
							break;
					case 1:
							if(battery_info.soc>=99)
							{
								hal_led_ktd2027_off();
								hal_led_ktd2027_full_charged_indicator();
								LED_state = 2;
							}
							break;
                    default:
                        	break;
				}
			}
			else
			{
				hal_led_ktd2027_off();
				PRINTF("[System] power off charging -> power down \r\n");
				hal_pmic_pca9422_power_down();
			}

			SDK_DelayAtLeastUs(1000 * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk)); //Delay 1s
		}
	}
}

#endif
