/*
 * buttons_handler.c
 *
 *  Created on: Mar 11, 2026
 *      Author: Lydia
 */

#include "buttons_handler.h"
#include "i2c_component_handler.h"
#include "spi_handler.h"
#include "WorkStateManager.h"
#include "system_status.h"
#include "app_connect.h"
#include "app_handsfree.h"

#define SAMPLE_MS                     (10U) // Polling interval 10ms, with simple anti-shake function
#define LONG_PRESS_MS                 (1000U)
#define POWER_OFF_MS                  (5000U)
#define CAMERA_RECORDING_MS           (1000U)
#define PRESS_COUNTER_TIMEOUT_MS      (250U)
#define BUTTON_DEBOUNCE_MS            (40U)

extern volatile SystemStatus ss;
extern uint8_t Novatek_boot_completed;

#if UsingQAR87BoardHwVersion == 1 // Actual Board
static uint8_t current_usb_output = 0;
#endif

static TickType_t function_button_down_tick = 0;
static TickType_t power_button_down_tick = 0;
static TickType_t function_button_tick = 0;
static TickType_t power_button_tick = 0;

// Short press counter
static uint8_t power_button_short_press_cnt = 0;
static uint8_t function_button_short_press_cnt = 0;
// Flags for press and release event
static uint8_t power_button_press_set = 0;
static uint8_t function_button_press_set = 0;
static uint8_t power_button_release_set = 0;
static uint8_t function_button_release_set = 0;
// Flags for Press and Hold 1s/5s
static uint8_t power_button_press_hold_1s_set = 0;
static uint8_t power_button_press_hold_shutdown_set = 0;
static uint8_t function_button_press_hold_recording_set = 0;
static uint8_t function_button_press_hold_5s_set = 0;
// Ticks for short press counter timeout
static TickType_t power_button_press_tick = 0;
static TickType_t function_button_press_tick = 0;
// Timers for button debounce
static TimerHandle_t s_power_button_timer = NULL;
static TimerHandle_t s_function_button_timer = NULL;

// gill
extern RingtoneState general_RingtoneState;

/* 簡單阻塞式 delay：使用 NXP SDK，依核心時脈做最少延遲 */
static inline void delay_ms(uint32_t ms)
{
//    SDK_DelayAtLeastUs(ms * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk));
	vTaskDelay(pdMS_TO_TICKS(ms));

}

/* 檢查按鍵是否連續維持低電位（active-low）達指定毫秒數：for pca9422 ship mode */
bool power_key_low_for_ms(uint32_t ms)
{
    uint32_t elapsed = 0U;
    while (elapsed < ms)
    {
        /* 低電位＝按下（active-low 假設） */
        if (GPIO_PinRead(GPIO, NXP_BQ_MR_N_PORT, NXP_BQ_MR_N_PIN) != 0U)
        {
            return false; /* 中途釋放，長按失敗 */
        }
        delay_ms(SAMPLE_MS);
        elapsed += SAMPLE_MS;
    }
    return true; /* 全程保持低電位達指定時間 */
}

/* 讀腳位：PowerKey */
static inline uint8_t pwr_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, NXP_BQ_MR_N_PORT, NXP_BQ_MR_N_PIN);
}

/* 讀腳位：FunKey */
static inline uint8_t btn_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, FUN_KEY_PORT, FUN_KEY_PIN);
}

static void power_button_timer_callback(TimerHandle_t xTimer)
{
	if (pwr_raw_read()) {
		power_button_tick = xTaskGetTickCount();
//		PRINTF("[Button] Release Button(%ld)\r\n", power_button_tick);

		// Filter long press
		if( (power_button_tick - power_button_down_tick) <= LONG_PRESS_MS) {
			power_button_release_set = 1;
			power_button_short_press_cnt++;
		}
		power_button_press_set = 0;

	} else {
		power_button_down_tick = xTaskGetTickCount();
		if (power_button_short_press_cnt == 0) {
			power_button_press_tick = power_button_down_tick;
		}
		power_button_press_set = 1;

//		PRINTF("[Button] Press Button(%ld)\r\n", power_button_down_tick);
	}
}

static void power_button_timer_init (void)
{
	s_power_button_timer = xTimerCreate("PowerButtonTimer",
	                               pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS),
								   pdFALSE,     // auto-reload
	                               NULL,
								   power_button_timer_callback);
}

void power_button_timer_start(void)
{
	if (s_power_button_timer != NULL) {
	    xTimerStart(s_power_button_timer, 0);
	}
}

static void function_button_timer_callback(TimerHandle_t xTimer)
{
	if (btn_raw_read()) {
		function_button_tick = xTaskGetTickCount();
//		PRINTF("[Button] Release Button(%ld)\r\n", function_button_tick);

		// Filter long press
		if( (function_button_tick - function_button_down_tick) <= LONG_PRESS_MS) {
			function_button_release_set = 1;
			function_button_short_press_cnt++;
		}
		function_button_press_set = 0;

	} else {
		function_button_down_tick = xTaskGetTickCount();
		if (function_button_short_press_cnt == 0) {
			function_button_press_tick = function_button_down_tick;
		}
		function_button_press_set = 1;
//		PRINTF("[Button] Press Button(%ld)\r\n", function_button_down_tick);
	}
}

static void function_button_timer_init (void)
{
	s_function_button_timer = xTimerCreate("FunctionButtonTimer",
	                               pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS),
								   pdFALSE,     // auto-reload
	                               NULL,
								   function_button_timer_callback);
}

void function_button_timer_start(void)
{
	if (s_function_button_timer != NULL) {
	    xTimerStart(s_function_button_timer, 0);
	}
}

void button_init (void)
{
	power_button_timer_init();
	function_button_timer_init();
}

void button_press_handler (void)
{
	if(power_button_release_set)
	{
		if(power_button_short_press_cnt >= 1)
		{
#if SOC_SPI_ENABLE
#if 1
			if (Novatek_boot_completed) {
				send_spi_request(POWER_SHORT_PRESS_HEX_VALUE);
			}
#else // use button to switch state
//			RequestToGetIntoMediaPlayer = 1;
//			RequestToGetIntoTranslation = 1;
//			RequestToGetIntoVideoAI = 1;
			RequestToGetIntoVideoRecording = 1;
#endif
#endif

			PRINTF("[Button] press power button\r\n");
			// Clear cnt
			power_button_short_press_cnt = 0;
			// Clear flag
			power_button_release_set = 0;
		}
	}
	if(function_button_release_set)
	{
		if (!ss_is_charging()) {
			// 250ms Time Out
			if(xTaskGetTickCount() - function_button_press_tick > PRESS_COUNTER_TIMEOUT_MS)
			{
				if(function_button_short_press_cnt == 1)
				{
					PRINTF("[Button] Single press function button\r\n");
#if SOC_SPI_ENABLE
#if !CES_DEMO || CES_DEMO_FOR_NOVATEK
					if (Novatek_boot_completed && (ss_get_state() == USAGE_STATE_HOME || ss_get_state() == USAGE_STATE_MENU ||
							ss_get_state() == USAGE_STATE_VIDEO_RECORDING || ss_get_state() == USAGE_STATE_ABOUT) && !ss_get_capture_status()) {
						send_spi_request(SHORT_PRESS_HEX_VALUE); // Stop Recording and Take Photo
					}
#endif
#endif
				}
				else if(function_button_short_press_cnt >= 2)
				{
                    ss_print_bt_addr();
#if UsingQAR87BoardHwVersion == 1 // Actual Board
                    if (current_usb_output == 0) {
                        PRINTF("[USB] Novatek USB.\r\n");
                        current_usb_output++;
                        GPIO_PinWrite(GPIO, NXP_532_USB_SWITCH_PORT, NXP_532_USB_SWITCH_PIN, 1U);
                        GPIO_PinWrite(GPIO, USB_SWDIO_SWITCH_PORT, USB_SWDIO_SWITCH_PIN, 1U);

                    } else if (current_usb_output == 1) {
                        PRINTF("[USB] NXP USB.\r\n");
                        current_usb_output++;
                        GPIO_PinWrite(GPIO, NXP_532_USB_SWITCH_PORT, NXP_532_USB_SWITCH_PIN, 0U);
                        GPIO_PinWrite(GPIO, USB_SWDIO_SWITCH_PORT, USB_SWDIO_SWITCH_PIN, 1U);

                    } else if (current_usb_output == 2) {
                        PRINTF("[USB] NXP SWD.\r\n");
                        current_usb_output = 0;
                        GPIO_PinWrite(GPIO, NXP_532_USB_SWITCH_PORT, NXP_532_USB_SWITCH_PIN, 0U);
                        GPIO_PinWrite(GPIO, USB_SWDIO_SWITCH_PORT, USB_SWDIO_SWITCH_PIN, 0U);
                    }
#endif

					PRINTF("[Button] Double press function button\r\n");
				}

				// Clear cnt
				function_button_short_press_cnt = 0;
				// Clear flag
				function_button_release_set = 0;
			}
		} else {
			if(function_button_short_press_cnt > 5)
			{
				PRINTF("[Button] Press the button five times into discoverable mode\r\n");
                app_clear_device_enter_discoverable();

				// Clear cnt
				function_button_short_press_cnt = 0;
				// Clear flag
				function_button_release_set = 0;
			}
			if (xTaskGetTickCount() - function_button_press_tick > 6000) {
				// Clear cnt
				function_button_short_press_cnt = 0;
				// Clear flag
				function_button_release_set = 0;
			}

		}
	}
}

void button_press_hold_handler (void)
{
	if(power_button_press_set)
	{
		// Check Press & Hold
		if(!power_button_press_hold_shutdown_set)
		{
			if(xTaskGetTickCount() - power_button_down_tick > POWER_OFF_MS)
			{
#if SOC_SPI_ENABLE
				if (Novatek_boot_completed) {
					send_spi_request(POWER_LONG_PRESS_HEX_VALUE);
				} else {
					general_RingtoneState = Ringtone_PowerOFF;
					vTaskDelay(pdMS_TO_TICKS(200));
					led_post_event(LED_EVT_POWER_OFF_PROGRESS);
				}
#endif

				power_button_press_hold_shutdown_set = 1;
				PRINTF("[Button] power button Press & Hold (%d ms)\r\n", POWER_OFF_MS);
			}
		}
		if(!power_button_press_hold_1s_set)
		{
			if(xTaskGetTickCount() - power_button_down_tick > 1000)
			{
				power_button_press_hold_1s_set = 1;
				PRINTF("[Button] power button Press & Hold (1s)\r\n");
			}
		}
	}
	else
	{
		if(power_button_press_hold_1s_set || power_button_press_hold_shutdown_set)
		{
			// Clear flag
			power_button_press_hold_1s_set = 0;
			power_button_press_hold_shutdown_set = 0;
			PRINTF("[Button] power button Press & Hold Release\r\n");
		}
	}

	if(function_button_press_set)
	{
		// Check Press & Hold
		if(!function_button_press_hold_5s_set)
		{
			if(xTaskGetTickCount() - function_button_down_tick > 5000)
			{
				function_button_press_hold_5s_set = 1;
				PRINTF("[Button] function button Press & Hold (5s)\r\n");
			}
		}
		if(!function_button_press_hold_recording_set)
		{
			if(xTaskGetTickCount() - function_button_down_tick > CAMERA_RECORDING_MS)
			{
#if SOC_SPI_ENABLE
#if !CES_DEMO || CES_DEMO_FOR_NOVATEK
				if (Novatek_boot_completed && !get_music_status() && (ss_get_state() == USAGE_STATE_HOME || ss_get_state() == USAGE_STATE_MENU || ss_get_state() == USAGE_STATE_ABOUT)) {
					send_spi_request(LONG_PRESS_HEX_VALUE); // Start Recording
				}
#endif
#endif

				function_button_press_hold_recording_set = 1;
				PRINTF("[Button] function button Press & Hold (%d ms)\r\n", CAMERA_RECORDING_MS);
			}
		}
	}
	else
	{
		if(function_button_press_hold_recording_set || function_button_press_hold_5s_set)
		{
			// Clear flag
			function_button_press_hold_recording_set = 0;
			function_button_press_hold_5s_set = 0;
			PRINTF("[Button] function button Press & Hold Release\r\n");
		}
	}
}

void button_handler (void)
{
	button_press_handler();
	button_press_hold_handler();
}

/* PINT ISR 回呼：將不同來源的中斷以不同 bit 通知同一個任務 */
void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status)
{
    if (pintr == POWER_BUTTON_PINT_CH) {
    	power_button_timer_start();
    }
    if (pintr == FUNCTION_BUTTON_PINT_CH) {
    	function_button_timer_start();
    }
}
