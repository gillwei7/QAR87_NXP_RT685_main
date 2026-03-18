/*
 * system_status.c
 *
 *  Created on: 2025年10月23日
 *      Author: 11301026
 */


#include "system_status.h"
#include "spi_handler.h"
#include "WorkStateManager.h"
#include "app_handsfree.h"
#include "board.h"
#include "hal_amp.h"

extern RingtoneState general_RingtoneState;

volatile uint8_t System_Status = 0; //Send system status to Novatek
volatile SystemStatus ss = {0};

static volatile usage_state_t current_usage_state = USAGE_STATE_HOME;

static volatile uint8_t current_state_value = 0;
static volatile uint8_t capture_status = 0;
static volatile uint8_t recording_status = 0;
static volatile uint8_t music_status = 0; // 0: off, 1: on
static volatile uint8_t is_turning_on_camera = 0;
static volatile uint8_t need_send_state = 0;
static volatile uint8_t need_send_music_status = 0;
static volatile uint8_t is_media_playing = 0;

static uint8_t bt_addr_0 = 0;
static uint8_t bt_addr_1 = 0;
static uint8_t bt_addr_2 = 0;
static uint8_t bt_addr_3 = 0;
static uint8_t bt_addr_4 = 0;
static uint8_t bt_addr_5 = 0;

extern QueueHandle_t      spi_request_queue;

uint8_t ss_get_state(void)
{
	return current_usage_state;
}

void ss_set_state(uint8_t state)
{
	PRINTF("[System] Usage State change: %d to %d \r\n",current_usage_state,state);
	if (current_usage_state == state) {
		return;
	}

	if (state == USAGE_STATE_HOME) {
		if (current_usage_state == USAGE_STATE_MUSIC_PLAYER) {
//			RequestToGetOutofA2dpPlay = 1;
//			current_usage_state = state;
//			need_send_music_status = 1;
//			music_status = 0;

		} else
		if (current_usage_state == USAGE_STATE_MEDIA_PLAYER) {
			hal_amp_aw88166_left_stop(); //workaround for noise
			hal_amp_aw88166_right_stop(); //workaround for noise
			RequestToGetOutofMediaPlayer = 1;
			current_usage_state = state;
			need_send_state = 1;

		} else if (current_usage_state == USAGE_STATE_VIDEO_RECORDING) {
			RequestToGetOutofVideoRecording = 1;
			current_usage_state = state;
			need_send_state = 1;

		} else if (current_usage_state == USAGE_STATE_VIDEO_AI) {
			RequestToGetOutofVideoAI = 1;
			current_usage_state = state;
			need_send_state = 1;

		} else if (current_usage_state == USAGE_STATE_TRANSLATION) {
			RequestToGetOutofTranslation = 1;
			current_usage_state = state;
			need_send_state = 1;

		} else if (current_usage_state == USAGE_STATE_TAKE_PHOTO) {
			current_usage_state = state;
			need_send_state = 1;

			if (music_status == COMPONENT_ON) {
				send_state_to_soc();
				return;
			}

			RequestToGetOutofTakePhoto = 1;

		}
#if ABOUT_STATE_ENABLE
		else if (current_usage_state == USAGE_STATE_ABOUT) {
			current_usage_state = state;
			need_send_state = 1;

			if (music_status == COMPONENT_ON) {
				send_state_to_soc();
				return;
			}

			RequestToGetOutofAbout = 1;

		}
#endif
#if MENU_STATE_ENABLE
		else if (current_usage_state == USAGE_STATE_MENU) {
			current_usage_state = state;
			need_send_state = 1;

			if (music_status == COMPONENT_ON) {
				send_state_to_soc();
				return;
			}

			RequestToGetOutofMenu = 1;

		}
#endif
	}
#if MENU_STATE_ENABLE
	else if (state == USAGE_STATE_MENU && current_usage_state == USAGE_STATE_HOME) {
		current_usage_state = state;
		need_send_state = 1;

		if (music_status == COMPONENT_ON) {
			send_state_to_soc();
			return;
		}

		RequestToGetIntoMenu = 1;

	}
#endif
#if ABOUT_STATE_ENABLE
	else if (state == USAGE_STATE_ABOUT && (current_usage_state == USAGE_STATE_HOME || current_usage_state == USAGE_STATE_MENU)) {
		current_usage_state = state;
		need_send_state = 1;

		if (music_status == COMPONENT_ON) {
			send_state_to_soc();
			return;
		}

		RequestToGetIntoAbout = 1;

	}
#endif
	else if (state == USAGE_STATE_MUSIC_PLAYER && (current_usage_state == USAGE_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_ABOUT
#endif
			)) {
//		RequestToGetIntoA2dpPlay = 1;
//		current_usage_state = state;
//		need_send_music_status = 1;
//		music_status = 1;


	} else if (state == USAGE_STATE_MEDIA_PLAYER && (current_usage_state == USAGE_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_ABOUT
#endif
			)) {
		RequestToGetIntoMediaPlayer = 1;
		current_usage_state = state;
		need_send_state = 1;
		is_media_playing = 1;

	} else if (state == USAGE_STATE_VIDEO_RECORDING && (current_usage_state == USAGE_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_ABOUT
#endif
			)) {
		RequestToGetIntoVideoRecording = 1;
		current_usage_state = state;
		need_send_state = 1;

	} else if (state == USAGE_STATE_TAKE_PHOTO && (current_usage_state == USAGE_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_ABOUT
#endif
			)) {
		current_usage_state = state;
		need_send_state = 1;

		if (music_status == COMPONENT_ON) {
			send_state_to_soc();
			return;
		}

		RequestToGetIntoTakePhoto = 1;

	} else if (state == USAGE_STATE_VIDEO_AI && (current_usage_state == USAGE_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_ABOUT
#endif
			)) {
		RequestToGetIntoVideoAI = 1;
		current_usage_state = state;
		need_send_state = 1;

	} else if (state == USAGE_STATE_TRANSLATION && (current_usage_state == USAGE_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_usage_state == USAGE_STATE_ABOUT
#endif
			)) {
		RequestToGetIntoTranslation = 1;
		current_usage_state = state;
		need_send_state = 1;

	}

}

uint8_t get_media_status(void)
{
	return is_media_playing;
}

void set_media_status(uint8_t status)
{
	is_media_playing = status;

	if (is_media_playing == MUSIC_PLAYING) {
		hal_amp_aw88166_left_start("Music");
		hal_amp_aw88166_right_start("Music");
	} else {
		hal_amp_aw88166_left_stop();
		hal_amp_aw88166_right_stop();
	}
}

uint8_t get_music_status(void)
{
	return music_status;
}

void set_music_status(uint8_t status)
{
	music_status = status;
}

void send_state_to_soc(void) // send state to soc if both audio path and state are ready
{
	PRINTF("[System] send_state_to_soc (%d) \r\n", current_usage_state);

	if (need_send_state) {
		PRINTF("[System] need_send_state send_state_to_soc (%d) \r\n", current_usage_state);
		current_state_value = USAGE_STATE_HEX_VALUE + current_usage_state;
		(void)xQueueSend(spi_request_queue, &current_state_value, 0);
		need_send_state = 0;

	}
//	else if (need_send_music_status) {
//		PRINTF("[System] send_music_status_to_soc (%d) \r\n", music_status);
//		send_music_status_to_soc();
//		need_send_music_status = 0;
//	}
}

void send_music_status_to_soc(void)
{
	// Todo
	// To make novatek open music ui or open home ui
#if SOC_SPI_ENABLE
	if (music_status) {
		send_spi_request(MUSIC_START_HEX_VALUE); //music start
	} else {
		send_spi_request(MUSIC_STOP_HEX_VALUE); //music stop
	}
#endif
	//send_spi_request(MUSIC_START_HEX_VALUE); //music start
	//send_spi_request(MUSIC_STOP_HEX_VALUE); //music stop
}

void ss_set_camera_status(uint8_t status)
{
	if (status == COMPONENT_ON)
	{
		is_turning_on_camera = 1;
	}
}

void ss_set_capture_status(uint8_t status)
{
	if (status == COMPONENT_START)
	{
		capture_status = 1;
		general_RingtoneState = Ringtone_PhotoCapture;
	}
	else if (status == COMPONENT_END)
	{
		capture_status = 0;

	}
}

uint8_t ss_get_capture_status(void)
{
	return capture_status;
}

void ss_set_recording_status(uint8_t status)
{
	if (status == COMPONENT_START)
	{
		recording_status = 1;
	}
	else if (status == COMPONENT_END)
	{
		recording_status = 0;
	}
}

uint8_t ss_get_recording_status(void)
{
	return recording_status;
}

void ss_set_bt_addr_0 (uint8_t addr)
{
	bt_addr_0 = addr;
}

void ss_set_bt_addr_1 (uint8_t addr)
{
	bt_addr_1 = addr;
}

void ss_set_bt_addr_2 (uint8_t addr)
{
	bt_addr_2 = addr;
}

void ss_set_bt_addr_3 (uint8_t addr)
{
	bt_addr_3 = addr;
}

void ss_set_bt_addr_4 (uint8_t addr)
{
	bt_addr_4 = addr;
}

void ss_set_bt_addr_5 (uint8_t addr)
{
	bt_addr_5 = addr;
}

void ss_print_bt_addr (void)
{
	PRINTF("BT Address: %02X:%02X:%02X:%02X:%02X:%02X\r\n", bt_addr_5, bt_addr_4, bt_addr_3, bt_addr_2, bt_addr_1, bt_addr_0);

}

/* ====== BLE/HA/BT/MIC：開關與讀取 ====== */
void ss_ble_on() {
	ss.flags |=  SS_BLE_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}
void ss_ble_off() {
	ss.flags &= ~SS_BLE_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_ble_is_on() {
	return (ss.flags & SS_BLE_BIT) != 0;
}

void ss_ha_on() {
	ss.flags |=  SS_HA_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

void ss_ha_off() {
	ss.flags &= ~SS_HA_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_ha_is_on() {
	return (ss.flags & SS_HA_BIT) != 0;
}

void ss_bt_on() {
	ss.flags |=  SS_BT_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

void ss_bt_off() {
	ss.flags &= ~SS_BT_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_bt_is_on() {
	return (ss.flags & SS_BT_BIT) != 0;
}

void ss_mic_on() {
	ss.flags |=  SS_MIC_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

void ss_mic_off() {
	ss.flags &= ~SS_MIC_BIT;
	System_Status=1;
#if SOC_SPI_ENABLE
	send_spi_request(SYSTEM_STATUS_HEX_VALUE);
#endif
}

bool ss_mic_is_on() {
	return (ss.flags & SS_MIC_BIT) != 0;
}


/* ====== Layer：設定與讀取 ====== */
void ss_set_layer(uint8_t layer) {
	ss.layer = layer; System_Status=1;
}

uint8_t ss_get_layer() {
	return ss.layer;
}

/* ====== 充電與電量：設定與讀取 ====== */
  void ss_set_charging(bool on) {
    if (on) {
    	ss.batt |= SS_CHARGER_BIT;
    }
    else {
    	ss.batt &= ~SS_CHARGER_BIT;
    }
    System_Status=1;
}
  bool ss_is_charging() {
    return (ss.batt & SS_CHARGER_BIT) != 0;
}

  void ss_set_battery(uint8_t percent) {
    if (percent > 100) {
    	percent = 100; // clamp 到 0..100
    }
    ss.batt = (uint8_t)((ss.batt & SS_CHARGER_BIT) | (percent << SS_LEVEL_SHIFT));
    System_Status=1;
}
  uint8_t ss_get_battery() {
    return (uint8_t)((ss.batt & SS_LEVEL_MASK) >> SS_LEVEL_SHIFT);
}


