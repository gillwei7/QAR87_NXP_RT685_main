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
#include "system_status.h"

#include "FreeRTOS.h"
#include "task.h"


#define EN_HIZ_BIT      7
#define EN_HIZ_MASK     (1u << EN_HIZ_BIT)

#define BATTERY_FULL_VOLTAGE   4450//(mV)
#define BATTERY_EMPTY_VOLTAGE  3300//(mV)


volatile bq256xx_status_t charger_status;
volatile BatteryInfo battery_info;

uint32_t s_bq256xx_iindpm_target_ua = 0;

static uint8_t is_booting = 1;
static uint8_t is_power_off_charging_mode = 0;


#define PWR_OFF_CHG_TASK_STACK     (configMINIMAL_STACK_SIZE + 256)
#define PWR_OFF_CHG_TASK_PRIORITY  (tskIDLE_PRIORITY + 2)

static TaskHandle_t s_powerOffChargingTaskHandle = NULL;


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
		uint8_t is_charging = 0;

		is_booting = 0;
//		SDK_DelayAtLeastUs(100 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk)); //Delay 100ms
		vTaskDelay(pdMS_TO_TICKS(100));

		is_charging = (uint8_t)GPIO_PinRead(GPIO, 0U, 19U); // VBUS detection
		if(is_charging)
//		hal_power_charger_bq25618_get_charging_status();
//		if(charger_status.vbus_good)
		{
			is_power_off_charging_mode = 1;
		}
	}
	return is_power_off_charging_mode;
}



static void PowerOffChargingTask(void *pvParameters)
{
    (void)pvParameters;
    uint8_t LED_state = 0; //1->charging, 2->full-charge

	gpio_pin_config_t input_pin_config    = {kGPIO_DigitalInput, 0};
	GPIO_PinInit(GPIO, 0U, 19U, &input_pin_config);

    PRINTF("[System] power off charging (RTOS Task) \r\n");

    for (;;)
    {
        //hal_power_charger_bq25618_get_charging_status();
        //if (charger_status.vbus_good)
        uint8_t charger_status = (uint8_t)GPIO_PinRead(GPIO, 0U, 19U);
        if(charger_status==1)
        {
        	vTaskDelay(pdMS_TO_TICKS(100));
            hal_power_gauge_glf70302_get_battery_level();
            switch (LED_state) {
                case 0:
                    hal_led_ktd2027_off();
                    hal_led_ktd2027_charging_indicator();
                    LED_state = 1;
                    break;

                case 1:
                    if (battery_info.soc >= FULLY_CHARGE_PERCENTAGE)
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
            vTaskDelay(pdMS_TO_TICKS(100));
            //hal_pmic_pca9422_power_down();
            bq256xx_enter_ship_mode();
            vTaskSuspend(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


static void StartPowerOffChargingTask(void)
{
    if (s_powerOffChargingTaskHandle == NULL)
    {
        BaseType_t ret = xTaskCreate(
            PowerOffChargingTask,
            "PwrOffChg",
            PWR_OFF_CHG_TASK_STACK,
            NULL,
            PWR_OFF_CHG_TASK_PRIORITY,
            &s_powerOffChargingTaskHandle
        );
        configASSERT(ret == pdPASS);
    }
}

void hal_power_go_to_power_off_charging(void)
{

//	StartPowerOffChargingTask();
//
//    vTaskStartScheduler();
//    for (;;)
//        ;

    uint8_t LED_state = 0; //1->charging, 2->full-charge

    uint16_t battery_level_delay = 0;

	gpio_pin_config_t input_pin_config    = {kGPIO_DigitalInput, 0};
	GPIO_PinInit(GPIO, 0U, 19U, &input_pin_config);

    PRINTF("[System] power off charging (RTOS Task) \r\n");

    for (;;)
    {
        //hal_power_charger_bq25618_get_charging_status();
        //if (charger_status.vbus_good)
        uint8_t charger_status = (uint8_t)GPIO_PinRead(GPIO, 0U, 19U);
        if(charger_status==1)
        {
//        	vTaskDelay(pdMS_TO_TICKS(100));
            if (battery_level_delay > 300 || battery_level_delay == 0) {
        		battery_level_delay = 0;
                hal_power_gauge_glf70302_get_battery_level();
            }
            switch (LED_state) {
                case 0:
                    hal_led_ktd2027_off();
                    hal_led_ktd2027_charging_indicator();
                    LED_state = 1;
                    break;

                case 1:
                    if (battery_info.soc >= FULLY_CHARGE_PERCENTAGE)
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
            vTaskDelay(pdMS_TO_TICKS(100));

            //hal_pmic_pca9422_power_down();
            bq256xx_enter_ship_mode();
            vTaskSuspend(NULL);

        }
        vTaskDelay(pdMS_TO_TICKS(100));
        battery_level_delay++;
    }


}



#endif
