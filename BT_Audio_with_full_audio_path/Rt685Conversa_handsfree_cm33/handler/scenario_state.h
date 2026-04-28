/*
 * scenario_state.h
 *
 *  Created on: Apr 4, 2026
 *      Author: Lydia
 */

#ifndef SCENARIO_STATE_H_
#define SCENARIO_STATE_H_

#include "hal_common.h"

#define MUSIC_PLAYING          1
#define MUSIC_PAUSE            0


typedef enum {
	SCENARIO_STATE_HOME = 0,
	SCENARIO_STATE_MENU,
	SCENARIO_STATE_SETTINGS,
	SCENARIO_STATE_ABOUT,
	SCENARIO_STATE_MEDIA_PLAYER,
	SCENARIO_STATE_VIDEO_AI,
	SCENARIO_STATE_TRANSLATION,
	SCENARIO_STATE_VIDEO_CALL,
	SCENARIO_STATE_MUSIC,
	SCENARIO_STATE_AUDIO_CALL,
	SCENARIO_STATE_VIDEO_RECORDING,
} scenario_state_t;

typedef enum {
	PHONE_PAGE_STATUS_HOME = 0,
	PHONE_PAGE_STATUS_VIDEO_AI,
	PHONE_PAGE_STATUS_TRANSLATION,
	PHONE_PAGE_STATUS_VIDEO_CALL,
} phone_page_status_t;

void wifi_ap_off_timer_init (void);
uint8_t get_scenario_state(void);
void set_scenario_state(uint8_t state);
phone_page_status_t get_phone_page_status (void);

void set_power_off_handler_state (void);
void set_music_player_handler_start_state (void);
void set_music_player_handler_stop_state (void);

void set_audio_call_handler_start_state (void);
void set_audio_call_handler_stop_state (void);

uint8_t get_media_status(void);
void set_media_status(uint8_t status);

// Request command from Phone (BLE)
uint8_t get_start_wifi_ap_request (void);
void set_start_wifi_ap_request (uint8_t on);

// Request command from Phone (BLE) or from System Controller
uint8_t get_stop_wifi_ap_request (void);
void set_stop_wifi_ap_request (uint8_t on);

// Status event from Novatek (SPI)
uint8_t get_soc_wifi_ap_opened_status (void);
void set_soc_wifi_ap_opened_status (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_start_wifi_ip_request (void);
void set_start_wifi_ip_request (uint8_t on);

// Status event from Phone (BLE)
uint8_t get_phone_wifi_connected_status (void);
void set_phone_wifi_connected_status (uint8_t on);

// Status event from Novatek (SPI)
uint8_t get_soc_wifi_connected_status (void);
void set_soc_wifi_connected_status (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_start_video_call_request (void);
void set_start_video_call_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_start_video_ai_request (void);
void set_start_video_ai_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_start_translation_request (void);
void set_start_translation_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_stop_video_call_request (void);
void set_stop_video_call_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_stop_video_ai_request (void);
void set_stop_video_ai_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_stop_translation_request (void);
void set_stop_translation_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_enter_video_call_request (void);
void set_enter_video_call_request (uint8_t on);
// Request command from Phone (BLE)
uint8_t get_leave_video_call_request (void);
void set_leave_video_call_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_enter_video_ai_request (void);
void set_enter_video_ai_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_leave_video_ai_request (void);
void set_leave_video_ai_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_enter_translation_request (void);
void set_enter_translation_request (uint8_t on);

// Request command from Phone (BLE)
uint8_t get_leave_translation_request (void);
void set_leave_translation_request (uint8_t on);

#endif /* SCENARIO_STATE_H_ */
