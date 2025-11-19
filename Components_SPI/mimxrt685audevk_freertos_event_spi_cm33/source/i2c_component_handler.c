/*
 * i2_component_handler.c
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#include "i2c_component_handler.h"
#include "spi_handler.h"
#include "i2s_handler.h"
#include "button_handler.h"
#include "system_status.h"

#define BATTERY_READ_PERIOD_MS  (60000U)
static TimerHandle_t s_battery_timer = NULL;

/* ===== I2C synchronization objects ===== */
EventGroupHandle_t i2c_event_group = NULL;
SemaphoreHandle_t  i2c_mutex       = NULL;
TaskHandle_t       sI2CTaskHandle  = NULL;

extern QueueHandle_t spi_request_queue ;

volatile bq256xx_status_t charger_status;
volatile BatteryInfo battery ;
volatile led_event_t g_led_event = LED_EVT_NONE;
volatile amp_event_t g_amp_event = AMP_EVT_NONE;
extern volatile struct aw933xx_dev aw933xx;

extern uint8_t led_status;
volatile SystemStatus ss = {0};
extern volatile uint8_t System_Status ;
extern uint8_t Novatek_boot_completed;

battery_state_t battery_state = BATTERY_STATE_NORMAL;


static inline uint8_t battery_soc_percent_mv(uint32_t mv)
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

static void BatteryReadTimerCb(TimerHandle_t xTimer)
{

    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, GAUGE_EVENT_BIT);
    }

}

static void Stop_AMP(void)
{
    close_aw88166_pa(AW_DEV_0);
    close_aw88166_pa(AW_DEV_1);
}
static void Start_AMP(amp_mode_t mode)
{
	const char *profile = (mode == AMP_MODE_RECEIVER) ? "Receiver" : "Music";

    start_aw88166_pa(AW_DEV_0, profile);
    start_aw88166_pa(AW_DEV_1, profile);
}
static void Init_bq25618_charger(void)
{
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
}

static void Init_glf70583_pmic(void)
{
    /* ======================PMIC glf70583================== */
	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
	PRINTF("[GLF70583] top_stat:%X \n",top_stat);

	//Solution: The manufacturer did not set it to LOAD SWITCH
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0xF5, 0xC6);
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB8);
	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB9);

	// BUCK1 Delay 4ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x66, 0x0C);
	// BUCK2 Delay 2ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x67, 0x08);
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x67, 0x08);
	// BUCK3 Delay 0ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x68, 0x00);
	// LDO1 Delay 5ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x6A, 0x12);

#if 1 //for QAR87Board
	// 0x25->BUCK4 off、Others on
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x26, 0xEC);
    // 0x26->BUCK1、2、4 ON、Others off
    glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xD0);
    //glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);//BUCK2 ON、Others off
#else
    //TODO: C4 (0x26:SW3、LDO1、LDO2) power sequence
	// 0x25->All on
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x26, 0xFC);
    // 0x26->All on
    glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xFC);
#endif

	PRINTF("[GLF70583] Enable GLF70583 \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	PRINTF("[System] Enable Novatek \n");
	GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1);
}

static void Init_pca9422_pmic(void)
{
    /* Init PCA9422 PMIC. */
 	BOARD_InitPmic();
 	PRINTF("[PCA9422] BOARD_InitPmic OK \r\n");
 	/* Apply PMIC mode and voltage settings */
 	BOARD_Init_PMICConfigure();
 	PRINTF("[PCA9422] BOARD_Init_PMICConfigure OK \r\n");

}
static void Determine_pca9422_enter_ship_mode(void)
{
	/* 讀取當下按鍵狀態 */
	uint8_t pin_state = (uint8_t)GPIO_PinRead(GPIO, POWER_KEY_PORT, POWER_KEY_PIN);

		if (pin_state == 0U)
		{
			/* 按鍵為低（按下）→ 需連續 2 秒才允許離開 ship mode */
			if (power_key_low_for_ms(LONG_PRESS_MS))
			{
				PRINTF("[PCA9422] PCA9422 leave ship mode (press >= %u ms)\r\n", LONG_PRESS_MS);
			}
			else
			{
				/* 沒達到 2 秒長按 → 進入 ship mode */
				PRINTF("[PCA9422] Power key (press < %u ms)\r\n", LONG_PRESS_MS);
				pca9422_ship_mode();
			}
		}
		else
		{
			/* 沒有按住按鍵（高電位）→ 直接進入 ship mode */
			pca9422_ship_mode();
		}
}

void Init_I2C_Component(void)
{
	Init_pca9422_pmic();
	Determine_pca9422_enter_ship_mode();
	Init_glf70583_pmic();
	Init_bq25618_charger();
	ktd202x_ch4_led_on(LED_ON); //White light turns on first
	awinic_single_enter(); //Touch Init
	glf70302_read_battery(&battery); //Read the battery level after powering on
	battery.soc = battery_soc_percent_mv(battery.voltage);
	ss_set_battery(&ss, battery.soc);
	init_aw88166(); // Init AMP


	bq256xx_poll_status(&charger_status);
	if(charger_status.vbus_good)
	{
		ss_set_charging(&ss, true);
	}


	s_battery_timer = xTimerCreate("BattTimer",
	                               pdMS_TO_TICKS(BATTERY_READ_PERIOD_MS),
	                               pdTRUE,     // auto-reload
	                               NULL,
	                               BatteryReadTimerCb);
	if (s_battery_timer != NULL) {
	    xTimerStart(s_battery_timer, 0);
	}



}

void pca9422_ship_mode(void)
{
	PRINTF("[PCA9422] PCA9422 enter ship mode \r\n");
	/* pca9422 ship mode process */
	uint8_t value;
    value = 0x10;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}
void pca9422_power_down(void)
{
    PRINTF("[PCA9422] PCA9422 power down \r\n");
	/* pca9422 power down  process */
    uint8_t value;
    value = 0x08;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}


void amp_post_event(amp_event_t e)
{
    g_amp_event = e;
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, AMP_EVENT_BIT);
    }
}


void led_post_event(led_event_t e)
{
    g_led_event = e;
    if (i2c_event_group) {
        xEventGroupSetBits(i2c_event_group, LED_EVENT_BIT);

    }
}

void Scan_I2C_Devices(I3C_Type *base)
{
    uint8_t dummyData = 0x00;
    status_t result;

    PRINTF("[I2C]Scanning I2C addresses...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) // I2C valid 7-bit address range
    {
        result = BOARD_I3C_Send(base, addr, 0x00, 0, &dummyData, 0);

        if (result == kStatus_Success)
        {
        	PRINTF("[I2C]Device found at 0x%02X\n", addr);
        }
    }

    PRINTF("[I2C]Scan complete.\n");
}


void I2C_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(
            i2c_event_group,
            TOUCH_EVENT_BIT | CHARGER_EVENT_BIT | GAUGE_EVENT_BIT | LED_EVENT_BIT | AMP_EVENT_BIT,
            pdTRUE,     /* clear on exit */
            pdFALSE,    /* wait for any bit */
            portMAX_DELAY);


        /* --- AMP event --- */
        if ((bits & AMP_EVENT_BIT) != 0) {
            vTaskDelay(1); /* 確保 g_amp_event 已更新 */
            amp_event_t evt = g_amp_event;

            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                switch (evt) {
                    case AMP_EVT_MUSIC_START:
                        StartSoundPlayback();
                        Start_AMP(AMP_MODE_MUSIC);
                        break;
                    case AMP_EVT_RECEIVER_START:
                        StartSoundPlayback();
                        Start_AMP(AMP_MODE_RECEIVER);
                        break;
                    case AMP_EVT_STOP:
                    	Stop_AMP();
                        StopSoundPlayback();
                        break;
                    default:
                        break;
                }
                xSemaphoreGive(i2c_mutex);
            }
        }


        /* --- TOUCH event --- */
        if ((bits & TOUCH_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
            	AW93305_EXTI_Callback();
                xSemaphoreGive(i2c_mutex);

                if(aw933xx.event.click >0)
                {
                	unsigned int btn_event = aw933xx.event.click;
                	PRINTF("[Touch] click= %d \n",btn_event);
                	if(btn_event==1)
                	{
                		uint8_t v = ONE_TOUCH_HEX_VALUE;
                		(void)xQueueSend(spi_request_queue, &v, 0);
                	}
                	else if(btn_event==2)
                	{
                		uint8_t v = DOUBLE_TOUCH_HEX_VALUE;
                		(void)xQueueSend(spi_request_queue, &v, 0);
                	}

                }
                else if(aw933xx.event.press)
                {
                	PRINTF("[Touch] press \n");
                	uint8_t v = PRESS_TOUCH_HEX_VALUE;
                	(void)xQueueSend(spi_request_queue, &v, 0);
                }
                else if(aw933xx.event.long_press)
                {
                	PRINTF("[Touch] long_press \n");
                }
                else if(aw933xx.event.super_long_press)
                {
                	PRINTF("[Touch] super_long_press \n");
                }
                else if(aw933xx.event.right_wareds)
                {
                	PRINTF("[Touch] slide_right \n");
                	uint8_t v = FORWARD_SLIDE_HEX_VALUE;
                	(void)xQueueSend(spi_request_queue, &v, 0);
                }
                else if(aw933xx.event.left_wareds)
                {
                	PRINTF("[Touch] slide_left \n");
                	uint8_t v = BACK_SLIDE_HEX_VALUE;
                	(void)xQueueSend(spi_request_queue, &v, 0);
                }

            }

            /* 任務側重新啟用觸控中斷（先清旗標再開） */
            GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
        }

        /* --- CHARGER event --- */
        if ((bits & CHARGER_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
    			if (bq256xx_poll_status(&charger_status) == kStatus_Success) {
    				PRINTF("[Charger] Power Good: %s\n", charger_status.power_good ? "Yes" : "No");
    				PRINTF("[Charger] VBUS Status: 0x%02X\n", charger_status.vbus_stat);
    				PRINTF("[Charger] Charge Status: 0x%02X\n", charger_status.chg_stat);
    				PRINTF("[Charger] Fault Status: 0x%02X\n", charger_status.fault_stat);
    				PRINTF("[Charger] VBUS Good: %s\n", charger_status.vbus_good ? "Yes" : "No");
    				PRINTF("\n");
    				if(charger_status.vbus_good)
    				{
    					led_post_event(LED_EVT_CHARGING);
    					ss_set_charging(&ss, true);
    				}
    				else
    				{
    					led_post_event(LED_EVT_ALL_OFF);
    					ss_set_charging(&ss, false);
    				}

    				if(charger_status.chg_stat==0x11)//Charging status: 00 – Not Charging、01 – Pre-charge、10 – Fast Charging、11 – Charge Termination
    				{
    					battery_state = BATTERY_STATE_FULL;
    					led_post_event(LED_EVT_FULL_CHARGERED);
    				}

    			} else {
    				PRINTF("[Charger] Failed to read charger status.\n");
    			}
                xSemaphoreGive(i2c_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);

        }

        /* --- GAUGE event --- */
        if ((bits & GAUGE_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
            	glf70302_read_battery(&battery);
            	battery.soc = battery_soc_percent_mv(battery.voltage);
                PRINTF("[Battery] SOC: %d%%\r\n",battery.soc);
                if(battery.soc>=99 && ss_is_charging(&ss))
                {
                	battery_state = BATTERY_STATE_FULL;
                	led_post_event(LED_EVT_FULL_CHARGERED);
                }
                else if (battery.soc<=20)
                {
                	battery_state = BATTERY_STATE_LOW;
                	led_post_event(LED_EVT_LOW_BATTERY);
                }
                else
                {
                	battery_state = BATTERY_STATE_NORMAL;
                }
            	ss_set_battery(&ss, battery.soc);
                xSemaphoreGive(i2c_mutex);
            }

            GPIO_PinClearInterruptFlag(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

        }

        /*--- LED event --- */
        if ((bits & LED_EVENT_BIT) != 0) {
                vTaskDelay(1); /* 確保 g_led_event 已更新 */
                led_event_t evt = g_led_event;

                if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE) {
                    /* 根據事件控制 LED */
                    switch (evt) {
                    case LED_EVT_POWER_ON_PROGRESS:
                    	ktd202x_led_off();
                        ktd202x_ch4_led_on(LED_ON);
                        break;
                    case LED_EVT_POWER_OFF_PROGRESS:
                    	ktd202x_led_off();
                    	ktd202x_ch2_led_on(LED_ON);
                    	vTaskDelay(pdMS_TO_TICKS(10000));
                    	ktd202x_ch2_led_off();
                    	pca9422_power_down();
                        break;
                    case LED_EVT_CHARGING:
                    	ktd202x_led_off();
                        ktd202x_ch2_led_blink(500, 500, TIM_1);
                        break;
                    case LED_EVT_LOW_BATTERY:
                    	ktd202x_led_off();
                        ktd202x_ch2_led_blink(500, 4500, TIM_1);
                        break;
                    case LED_EVT_FULL_CHARGERED:
                    	ktd202x_led_off();
                    	ktd202x_ch3_led_on(LED_ON);
                        break;
                    case LED_EVT_PHOTO_CAPTURE:
                    	ktd202x_led_off();
                    	ktd202x_ch4_led_on(LED_ON);
                    	vTaskDelay(50);
                    	ktd202x_ch4_led_off();
                        break;
                    case LED_EVT_VIDEO_CAPTURE:
                    	ktd202x_led_off();
                    	ktd202x_ch4_led_blink(500, 500, TIM_2);
                        break;
                    case LED_EVT_PAIRING_MODE:
                    	ktd202x_led_off();
                    	ktd202x_ch1_led_blink(100, 100, TIM_2);
                        break;
                    case LED_EVT_OTA_PROGRESS:
                    	ktd202x_led_off();
                    	ktd202x_ch4_led_blink(300, 300, TIM_1);
                        break;
                    case LED_EVT_OTA_SUCCESS:
                    	ktd202x_led_off();
                    	ktd202x_ch3_led_breathe(PERIOD_CODE_1P5S,
                    	                        RISE_CODE_600MS,
                    	                        FALL_CODE_600MS,
                    	                        ON_PERCENT_60,
                    	                        RAMP_SCALE_2X_SLOW,
												true,
                    	                        LED_CURRENT_CH3 );
                        break;
                    case LED_EVT_OTA_FAIL:
                    	ktd202x_led_off();
                    	ktd202x_ch2_led_breathe(PERIOD_CODE_1P5S,
                    							RISE_CODE_600MS,
                    							FALL_CODE_600MS,
                    	                        ON_PERCENT_60,
                    							RAMP_SCALE_2X_SLOW,
                    	                        true,
                    							LED_CURRENT_CH2);
                        break;

                    case LED_EVT_ALL_OFF:
                    	ktd202x_led_off();
                        break;
                    default:
                        break;
                    }
                    xSemaphoreGive(i2c_mutex);
                }
            }

        if(System_Status && Novatek_boot_completed)
        {
        	System_Status=0;
    		uint8_t v = SYSTEM_STATUS_HEX_VALUE;
    	    (void)xQueueSend(spi_request_queue, &v, 0);

        }

        //Confirm the final status
        if(led_status==0) // When other events are executed, causing the LED to turn off
        {
        	if(ss_is_charging(&ss))
        	{
            	if(battery_state==BATTERY_STATE_FULL)
            		led_post_event(LED_EVT_FULL_CHARGERED);
            	else
                	led_post_event(LED_EVT_CHARGING);
        	}
        	else
        	{
        		if(battery_state==BATTERY_STATE_LOW)
        			led_post_event(LED_EVT_LOW_BATTERY);
        	}

        }

    }
}
