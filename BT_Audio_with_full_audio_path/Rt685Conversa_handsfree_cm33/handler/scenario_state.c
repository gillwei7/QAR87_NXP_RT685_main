/*
 * scenario_state.c
 *
 *  Created on: Apr 4, 2026
 *      Author: Lydia
 */

#include "scenario_state.h"
#include "system_status.h"
#include "hal_amp.h"
#include "WorkStateManager.h"
#include "app_avrcp.h"
#include "spi_command_set.h"
#include "hal_led.h"
#include "i2c_component_handler.h"
#include "ringtone_handler.h"


static volatile scenario_state_t current_scenario_state = SCENARIO_STATE_HOME;
static volatile uint8_t is_media_playing = MUSIC_PAUSE;

static uint8_t menu_handler_start_state = 0;
static uint8_t menu_handler_stop_state = 0;

static uint8_t settings_handler_start_state = 0;
static uint8_t settings_handler_stop_state = 0;

static uint8_t about_handler_start_state = 0;
static uint8_t about_handler_stop_state = 0;

static uint8_t media_player_handler_start_state = 0;
static uint8_t media_player_handler_stop_state = 0;

static uint8_t music_player_handler_start_state = 0;
static uint8_t music_player_handler_stop_state = 0;

static uint8_t audio_call_handler_start_state = 0;
static uint8_t audio_call_handler_stop_state = 0;

static uint16_t video_recording_handler_start_state = 0;
static uint16_t video_recording_handler_stop_state = 0;

static uint8_t video_call_handler_start_state = 0;
static uint8_t video_call_handler_stop_state = 0;

static uint8_t video_ai_handler_start_state = 0;
static uint8_t video_ai_handler_stop_state = 0;

static uint8_t translation_handler_start_state = 0;
static uint8_t translation_handler_stop_state = 0;



uint8_t get_scenario_state(void)
{
	return current_scenario_state;
}

void set_scenario_state(uint8_t state)
{
	PRINTF("[System] Usage State change: %d to %d \r\n", current_scenario_state,state);
	if (current_scenario_state == state) {
		return;
	}

	if (state == SCENARIO_STATE_HOME) {
		if (current_scenario_state == SCENARIO_STATE_MEDIA_PLAYER) {
			current_scenario_state = state;
			media_player_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_RECORDING) {
			current_scenario_state = state;
			video_recording_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_CALL) {
			current_scenario_state = state;
			video_call_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_AI) {
			current_scenario_state = state;
			video_ai_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_TRANSLATION) {
			current_scenario_state = state;
			translation_handler_stop_state = 1;

		}
#if ABOUT_STATE_ENABLE
		else if (current_scenario_state == SCENARIO_STATE_SETTINGS) {
			current_scenario_state = state;
			settings_handler_stop_state = 1;

		}
		else if (current_scenario_state == SCENARIO_STATE_ABOUT) {
			current_scenario_state = state;
			about_handler_stop_state = 1;

		}
#endif
#if MENU_STATE_ENABLE
		else if (current_scenario_state == SCENARIO_STATE_MENU) {
			current_scenario_state = state;
			menu_handler_stop_state = 1;

		}
#endif
	}
#if MENU_STATE_ENABLE
	else if (state == SCENARIO_STATE_MENU && current_scenario_state == SCENARIO_STATE_HOME) {
		current_scenario_state = state;
		menu_handler_start_state = 1;
	}
#endif
#if ABOUT_STATE_ENABLE
	else if (state == SCENARIO_STATE_SETTINGS && (current_scenario_state == SCENARIO_STATE_HOME || current_scenario_state == SCENARIO_STATE_MENU)) {
		current_scenario_state = state;
		settings_handler_start_state = 1;
	}
	else if (state == SCENARIO_STATE_ABOUT && (current_scenario_state == SCENARIO_STATE_HOME || current_scenario_state == SCENARIO_STATE_MENU)) {
		current_scenario_state = state;
		about_handler_start_state = 1;
	}

#endif
	else if (state == SCENARIO_STATE_MEDIA_PLAYER && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		current_scenario_state = state;
		media_player_handler_start_state = 1;

	} else if (state == SCENARIO_STATE_VIDEO_RECORDING && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		current_scenario_state = state;
		video_recording_handler_start_state = 1;


	}  else if (state == SCENARIO_STATE_VIDEO_CALL && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		current_scenario_state = state;
		video_call_handler_start_state = 1;


	} else if (state == SCENARIO_STATE_VIDEO_AI && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		current_scenario_state = state;
		video_ai_handler_start_state = 1;

	} else if (state == SCENARIO_STATE_TRANSLATION && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		current_scenario_state = state;
		translation_handler_start_state = 1;
	}
}

static void scenario_menu_handler (void)
{
	if (menu_handler_start_state == 0 && menu_handler_stop_state == 0) {
			return;
	}
	//Start: 1. UI
	if (menu_handler_start_state == 1) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_LAUNCHER);

		menu_handler_start_state = 0;

	} else if (menu_handler_start_state > 0) {
		menu_handler_start_state++;
	}

	//Stop: 1. UI
	if (menu_handler_stop_state == 1) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME);

		menu_handler_stop_state = 0;

	} else if (menu_handler_stop_state > 0) {
		menu_handler_stop_state++;
	}
}

static void scenario_settings_handler (void)
{
	if (settings_handler_start_state == 0 && settings_handler_stop_state == 0) {
			return;
	}
	//Start: 1. UI
	if (settings_handler_start_state == 1) {
		//TODO Need to add UI for settings
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_ABOUT);

		settings_handler_start_state = 0;

	} else if (settings_handler_start_state > 0) {
		settings_handler_start_state++;
	}

	//Stop: 1. UI
	if (settings_handler_stop_state == 1) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME);

		settings_handler_stop_state = 0;

	} else if (settings_handler_stop_state > 0) {
		settings_handler_stop_state++;
	}
}

static void scenario_about_handler (void)
{
	if (about_handler_start_state == 0 && about_handler_stop_state == 0) {
			return;
	}
	//Start: 1. UI
	if (about_handler_start_state == 1) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_ABOUT);

		about_handler_start_state = 0;

	} else if (about_handler_start_state > 0) {
		about_handler_start_state++;
	}

	//Stop: 1. UI
	if (about_handler_stop_state == 1) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME);

		about_handler_stop_state = 0;

	} else if (about_handler_stop_state > 0) {
		about_handler_stop_state++;
	}
}

static void scenario_media_player_handler (void)
{
	if (media_player_handler_start_state == 0 && media_player_handler_stop_state == 0) {
			return;
	}
	//Start
	if (media_player_handler_start_state == 1) {
		if (get_music_status() == STATUS_ON) {
			avrcp_pause_button(0);
			PRINTF("[Music] pause the music before starting media player\r\n");
		}
		PRINTF("[MediaPlayer] Start media player...\r\n");
		media_player_handler_start_state++;

	} else if (media_player_handler_start_state == 5) {

		RequestToGetIntoMediaPlayer = 1;
		is_media_playing = 1;

		media_player_handler_start_state++;

	} else if (media_player_handler_start_state == 6) {
		amp_post_event(AMP_EVT_MUSIC);
		media_player_handler_start_state++;

	} else if (media_player_handler_start_state == 7) {
		spi_command_atomic_exec_media_start; // Start media player

		media_player_handler_start_state = 0;

	} else if (media_player_handler_start_state > 0) {
		media_player_handler_start_state++;
	}

	//Stop
	if (media_player_handler_stop_state == 1) {
		PRINTF("[MediaPlayer] Stop media player...\r\n");
		amp_post_event(AMP_EVT_STOP);
		media_player_handler_stop_state++;

	} else if (media_player_handler_stop_state == 2) {
		RequestToGetOutofMediaPlayer = 1;
		is_media_playing = MUSIC_PAUSE;

		media_player_handler_stop_state++;

	} else if (media_player_handler_stop_state == 3) {
		spi_command_atomic_exec_media_stop(); // media player: leave

		media_player_handler_stop_state++;

	} else if (media_player_handler_stop_state == 4) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME);
		media_player_handler_stop_state = 0;

	} else if (media_player_handler_stop_state > 0) {
		media_player_handler_stop_state++;
	}
}

static void scenario_music_player_handler (void)
{
	if (music_player_handler_start_state == 0 && music_player_handler_stop_state == 0) {
			return;
	}
	//Start
	if (music_player_handler_start_state == 1) {
		if (get_scenario_state() == SCENARIO_STATE_MUSIC) {
			set_scenario_state(SCENARIO_STATE_HOME);
			PRINTF("[MediaPlayer] pause the Media Player before starting music player\r\n");
			music_player_handler_start_state++;

		} else {
			RequestToGetIntoA2dpPlay = 1;

			music_player_handler_start_state = 6; // go to next state (Enable AMP)
		}
		PRINTF("[Music] Start music player...\r\n");

	} else if (music_player_handler_start_state == 5) {
		RequestToGetIntoA2dpPlay = 1;

		music_player_handler_start_state++;

	} else if (music_player_handler_start_state == 6) {
		amp_post_event(AMP_EVT_MUSIC);

		music_player_handler_start_state = 0;

	} else if (music_player_handler_start_state > 0) {
		music_player_handler_start_state++;
	}

	//Stop
	if (music_player_handler_stop_state == 1) {
		PRINTF("[MediaPlayer] Stop music player...\r\n");

		amp_post_event(AMP_EVT_STOP);

		music_player_handler_stop_state++;

	} else if (music_player_handler_stop_state == 2) {
		RequestToGetOutofA2dpPlay = 1;

		music_player_handler_stop_state = 0;

	} else if (music_player_handler_stop_state > 0) {
		music_player_handler_stop_state++;
	}
}

static void scenario_audio_call_handler (void)
{
	if (audio_call_handler_start_state == 0 && audio_call_handler_stop_state == 0) {
			return;
	}
	//Start
	if (audio_call_handler_start_state == 1) {
		PRINTF("[MediaPlayer] Start audio call...\r\n");
	    RequestToGetIntoHfp=1;
		audio_call_handler_start_state++;

	} else if (audio_call_handler_start_state == 2) {
		amp_post_event(AMP_EVT_RECEIVER);
		audio_call_handler_start_state++;

	} else if (audio_call_handler_start_state == 3) {
		//TODO SPI: change UI for audio call
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home
		set_audio_call_status(STATUS_ON);
		audio_call_handler_start_state = 0;

	} else if (audio_call_handler_start_state > 0) {
		audio_call_handler_start_state++;
	}

	//Stop
	if (audio_call_handler_stop_state == 1) {
		PRINTF("[MediaPlayer] Stop audio call...\r\n");
		amp_post_event(AMP_EVT_STOP);
		set_audio_call_status(STATUS_OFF);
		audio_call_handler_stop_state++;

	} else if (audio_call_handler_stop_state == 2) {
		RequestToGetOutofHfp = 1;
		is_media_playing = MUSIC_PAUSE;

		audio_call_handler_stop_state++;

	} else if (audio_call_handler_stop_state == 3) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home

		audio_call_handler_stop_state = 0;

	} else if (audio_call_handler_stop_state > 0) {
		audio_call_handler_stop_state++;
	}
}

static void scenario_video_recording_handler (void)
{
	if (video_recording_handler_start_state == 0 && video_recording_handler_stop_state == 0) {
			return;
	}
	//Start: 1. Audio path 2. SPI 3. LED
	if (video_recording_handler_start_state == 1) {
		if (!is_playing_ringtone()) { // Wait for the ringtone to finish
			PRINTF("[VideoRecording] Start video recording...\r\n");
			RequestToGetIntoVideoRecording = 1;
			video_recording_handler_start_state++;

		}

	} else if (video_recording_handler_start_state == 2) {
		spi_command_atomic_exec_start_recording(); // Start recording
		video_recording_handler_start_state++;

	} else if (video_recording_handler_start_state > 0) {
		if (ss_get_recording_status() == STATUS_ON) {
			hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
			led_post_event(HAL_LED_EVENT_REFRESH);
			video_recording_handler_start_state = 0;

		} else if (video_recording_handler_start_state > 200) {
			//TODO Retry 5 time
			PRINTF("[VideoRecording] Start video recording Failed...\r\n");
			video_recording_handler_start_state = 0;

		} else {
			video_recording_handler_start_state++;
		}
	}

	//Stop: 1. SPI 2. Audio path 3. LED, Ringtone and UI
	if (video_recording_handler_stop_state == 1) {
		PRINTF("[VideoRecording] Stop video recording...\r\n");
		spi_command_atomic_exec_stop_recording(); // Stop Recording

		video_recording_handler_stop_state++;

	} else if (video_recording_handler_stop_state == 2) {
		RequestToGetOutofVideoRecording = 1;

		video_recording_handler_stop_state++;
	} else if (video_recording_handler_stop_state == 4) {

		video_recording_handler_stop_state++;

	} else if (video_recording_handler_stop_state > 0) { // Wait for Novatek ready
		if (ss_get_recording_status() == STATUS_OFF) {
			hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
			led_post_event(HAL_LED_EVENT_REFRESH);
			set_ringtone_state(Ringtone_StopRecording);
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home

			video_recording_handler_stop_state = 0;

		} else if (video_recording_handler_stop_state > 200) {
			//TODO Retry 5 time
			PRINTF("[VideoRecording] Stop video recording Failed...\r\n");
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home

			video_recording_handler_stop_state = 0;

		} else {
			video_recording_handler_stop_state++;
		}
	}
}

static void scenario_video_call_handler (void)
{
	if (video_call_handler_start_state == 0 && video_call_handler_stop_state == 0) {
			return;
	}
	//Start: 1. Audio path 2. SPI 3. BLE 4. LED and AMP
	if (video_call_handler_start_state == 1) {
		PRINTF("[VideoCall] Start video call...\r\n");
		RequestToGetIntoVideoAI = 1; //Audio path
		video_call_handler_start_state++;

	} else if (video_call_handler_start_state == 2) {
		// TODO send SPI command (Start video call) to Novatek
		video_call_handler_start_state++;

	} else if (video_call_handler_start_state == 3) {
		// TODO Send a BLE event (Wi-Fi IP) to the phone when the Novatek Wi-Fi IP is ready
		video_call_handler_start_state++;

	} else if (video_call_handler_start_state == 4) {
		// TODO Set LED and AMP when the Novatek RTSP is ready
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		amp_post_event(AMP_EVT_MUSIC);

		video_call_handler_start_state = 0;

	} else if (video_call_handler_start_state > 0) {
		video_call_handler_start_state++;
	}

	//Stop: 1. AMP and SPI 2. Audio path 3. UI 4. LED
	if (video_call_handler_stop_state == 1) {
		PRINTF("[VideoCall] Stop video call...\r\n");
		amp_post_event(AMP_EVT_STOP);
		// TODO send SPI command (Stop video call) to Novatek

		video_call_handler_stop_state++;

	} else if (video_call_handler_stop_state == 2) {
		RequestToGetOutofVideoAI = 1; //Audio path

		video_call_handler_stop_state++;

	} else if (video_call_handler_stop_state == 3) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home

		video_call_handler_stop_state++;

	} else if (video_call_handler_stop_state == 4) {
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);

		video_call_handler_stop_state++;

	} else if (video_call_handler_stop_state > 0) {
		video_call_handler_stop_state++;

	}
}

static void scenario_video_ai_handler (void)
{
	if (video_ai_handler_start_state == 0 && video_ai_handler_stop_state == 0) {
			return;
	}
	//Start: 1. Ringtone 2. Audio path 3. SPI 4. BLE 5. LED and AMP
	if (video_ai_handler_start_state == 1) {
		PRINTF("[VideoAI] Start video ai...\r\n");
		set_ringtone_state(Ringtone_StartVideoAI);
		video_ai_handler_start_state++;

	} else if (video_ai_handler_start_state == 2) {
		if (!is_playing_ringtone()) { // Wait for the ringtone to finish
			RequestToGetIntoVideoAI = 1; //Audio path
			video_ai_handler_start_state++;
		}

	} else if (video_ai_handler_start_state == 3) {
		// TODO send SPI command (Start video ai) to Novatek
		video_ai_handler_start_state++;

	} else if (video_ai_handler_start_state == 4) {
		// TODO Send a BLE event (Wi-Fi IP) to the phone when the Novatek Wi-Fi IP is ready
		video_ai_handler_start_state++;

	} else if (video_ai_handler_start_state == 5) {
		// TODO Set LED and AMP when the Novatek RTSP is ready
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		amp_post_event(AMP_EVT_MUSIC);

		video_ai_handler_start_state = 0;

	} else if (video_ai_handler_start_state > 0) {
		video_ai_handler_start_state++;
	}

	//Stop: 1. AMP and SPI 2. Audio path 3. UI 4. LED and Ringtone
	if (video_ai_handler_stop_state == 1) {
		PRINTF("[VideoAI] Stop video ai...\r\n");
		amp_post_event(AMP_EVT_STOP);
		// TODO send SPI command (Stop video ai) to Novatek
		video_ai_handler_stop_state++;

	} else if (video_ai_handler_stop_state == 2) {
		RequestToGetOutofVideoAI = 1; //Audio path

		video_ai_handler_stop_state++;

	} else if (video_ai_handler_stop_state == 3) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home

		video_ai_handler_stop_state++;

	} else if (video_ai_handler_stop_state == 4) {
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		set_ringtone_state(Ringtone_StopVideoAI);

		video_ai_handler_stop_state++;

	} else if (video_ai_handler_stop_state > 0) {
		video_ai_handler_stop_state++;

	}
}

static void scenario_translation_handler (void)
{
	if (translation_handler_start_state == 0 && translation_handler_stop_state == 0) {
			return;
	}
	//Start: 1. Ringtone 2. Audio path 3. SPI 4. BLE 5. LED and AMP
	if (translation_handler_start_state == 1) {
		PRINTF("[Translation] Start Translation...\r\n");
		set_ringtone_state(Ringtone_StartTranslation);
		translation_handler_start_state++;

	} else if (translation_handler_start_state == 2) {
		if (!is_playing_ringtone()) { // Wait for the ringtone to finish
			RequestToGetIntoVideoAI = 1; //Audio path
			translation_handler_start_state++;
		}

	} else if (translation_handler_start_state == 3) {
		// TODO send SPI command (Start Translation) to Novatek
		translation_handler_start_state++;

	} else if (translation_handler_start_state == 4) {
		// TODO Send a BLE event (Wi-Fi IP) to the phone when the Novatek Wi-Fi IP is ready
		translation_handler_start_state++;

	} else if (translation_handler_start_state == 5) {
		// TODO Set LED and AMP when the Novatek RTSP is ready
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		amp_post_event(AMP_EVT_MUSIC);

		translation_handler_start_state = 0;

	} else if (translation_handler_start_state > 0) {
		translation_handler_start_state++;
	}

	//Stop: 1. AMP and SPI 2. Audio path 3. UI 4. LED and Ringtone
	if (translation_handler_stop_state == 1) {
		PRINTF("[Translation] Stop Translation...\r\n");
		amp_post_event(AMP_EVT_STOP);
		// TODO send SPI command (Stop Translation) to Novatek
		translation_handler_stop_state++;

	} else if (translation_handler_stop_state == 2) {
		RequestToGetOutofVideoAI = 1; //Audio path

		translation_handler_stop_state++;

	} else if (translation_handler_stop_state == 3) {
		spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME); // UI: home

		translation_handler_stop_state++;

	} else if (translation_handler_stop_state == 4) {
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		set_ringtone_state(Ringtone_StopTranslation);

		translation_handler_stop_state++;

	} else if (translation_handler_stop_state > 0) {
		translation_handler_stop_state++;

	}
}

void scenario_state_handler (void)
{
	scenario_menu_handler();
	scenario_settings_handler();
	scenario_about_handler();
	scenario_media_player_handler();
	scenario_music_player_handler();
	scenario_audio_call_handler();
	scenario_video_recording_handler();
	scenario_video_call_handler();
	scenario_video_ai_handler();
	scenario_translation_handler();
}

void set_music_player_handler_start_state (void)
{
	if (!music_player_handler_start_state) {
		music_player_handler_stop_state = 0;
		music_player_handler_start_state = 1;
	}
}

void set_music_player_handler_stop_state (void)
{
	if (!music_player_handler_stop_state) {
		music_player_handler_start_state = 0;
		music_player_handler_stop_state = 1;
	}
}

void set_audio_call_handler_start_state (void)
{
	if (!audio_call_handler_start_state) {
		audio_call_handler_stop_state = 0;
		audio_call_handler_start_state = 1;
	}
}

void set_audio_call_handler_stop_state (void)
{
	if (!audio_call_handler_stop_state) {
		audio_call_handler_start_state = 0;
		audio_call_handler_stop_state = 1;
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
		amp_post_event(AMP_EVT_MUSIC);
	} else {
		amp_post_event(AMP_EVT_STOP);
	}
}
