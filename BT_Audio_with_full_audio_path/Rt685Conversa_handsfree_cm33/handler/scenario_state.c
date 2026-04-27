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
#include "ui_handler.h"
#include "hal_led.h"
#include "hal_pmic.h"
#include "i2c_component_handler.h"
#include "ringtone_handler.h"
#include <ble_event_handler.h>
#include <ble_packet_handler.h>

extern uint8_t Novatek_boot_completed;

static volatile scenario_state_t current_scenario_state = SCENARIO_STATE_HOME;
static volatile uint8_t is_media_playing = MUSIC_PAUSE;


static uint8_t power_off_handler_state = 0;


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

static uint8_t start_wifi_ap_request = 0;
static uint8_t stop_wifi_ap_request = 0;
static uint8_t soc_wifi_ap_opened_status = 0;
static uint8_t start_wifi_ip_request = 0;
static uint8_t phone_wifi_connected_status = 0;
static uint8_t soc_wifi_connected_status = 0;
static uint8_t start_video_call_request = 0;
static uint8_t stop_video_call_request = 0;

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
			media_player_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_RECORDING) {
			video_recording_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_CALL) {
			video_call_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_AI) {
			video_ai_handler_stop_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_TRANSLATION) {
			translation_handler_stop_state = 1;

		}
#if ABOUT_STATE_ENABLE
		else if (current_scenario_state == SCENARIO_STATE_SETTINGS) {
			settings_handler_stop_state = 1;

		}
		else if (current_scenario_state == SCENARIO_STATE_ABOUT) {
			about_handler_stop_state = 1;

		}
#endif
#if MENU_STATE_ENABLE
		else if (current_scenario_state == SCENARIO_STATE_MENU) {
			menu_handler_stop_state = 1;

		}
#endif
	}
#if MENU_STATE_ENABLE
	else if (state == SCENARIO_STATE_MENU && current_scenario_state == SCENARIO_STATE_HOME) {
		menu_handler_start_state = 1;
	}
#endif
#if ABOUT_STATE_ENABLE
	else if (state == SCENARIO_STATE_SETTINGS && (current_scenario_state == SCENARIO_STATE_HOME || current_scenario_state == SCENARIO_STATE_MENU)) {
		settings_handler_start_state = 1;
	}
	else if (state == SCENARIO_STATE_ABOUT && (current_scenario_state == SCENARIO_STATE_HOME || current_scenario_state == SCENARIO_STATE_MENU)) {
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
		media_player_handler_start_state = 1;

	} else if (state == SCENARIO_STATE_VIDEO_RECORDING && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		video_recording_handler_start_state = 1;


	}  else if (state == SCENARIO_STATE_VIDEO_CALL && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		video_call_handler_start_state = 1;


	} else if (state == SCENARIO_STATE_VIDEO_AI && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		video_ai_handler_start_state = 1;

	} else if (state == SCENARIO_STATE_TRANSLATION && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		translation_handler_start_state = 1;
	} else if (state == SCENARIO_STATE_MUSIC && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		translation_handler_start_state = 1;
	} else if (state == SCENARIO_STATE_AUDIO_CALL && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		translation_handler_start_state = 1;
	}
}

static TickType_t power_off_tick = 0;
static void scenario_power_off_handler (void)
{
	if (power_off_handler_state == 0) {
			return;
	}

	// 1. SPI + UI, 2. Ringtone, 3. LED, 4. wait 5 s, 5. disable PMIC, 6. LED off, 7. shipmode
	if (power_off_handler_state == 1) {
		if (Novatek_boot_completed) {
			if (spi_protocol_get_status() == S_IDLE) {
#if SOC_SPI_ENABLE
				spi_command_atomic_exec_soft_power_off(); // Power off
#endif
				power_off_handler_state++;
			}
		} else {
			set_ringtone_state(Ringtone_PowerOFF);
			power_off_handler_state += 2; //Jump to 3
		}

	} else if (power_off_handler_state == 2) {
		set_ringtone_state(Ringtone_PowerOFF);
		power_off_handler_state++;

	} else if (power_off_handler_state == 3) {
		led_post_event(HAL_LED_EVENT_POWER_OFF_PROGRESS);
        PRINTF("xTaskGetTickCount(): %d \r\n", xTaskGetTickCount());

		power_off_tick = xTaskGetTickCount();
		power_off_handler_state++;

	} else if (power_off_handler_state == 4) {
		if (xTaskGetTickCount() - power_off_tick > 5000) {
			power_off_handler_state++;
		}

	} else if (power_off_handler_state == 5) {
		hal_pmic_glf70583_cutoff_all(); //Disable power to the SoC and Wi-Fi/BT module
		power_off_handler_state++;

	} else if (power_off_handler_state == 6) {
		led_post_event(HAL_LED_EVENT_ALL_OFF);
		power_off_handler_state++;

	} else if (power_off_handler_state == 10) {
		NVIC_SystemReset(); //Reboot (Defer the power-on and power-off decision to post-boot evaluation)
		power_off_handler_state = 0;

	} else if (power_off_handler_state > 0) {
		power_off_handler_state++;
	}
}

static void scenario_menu_handler (void)
{
	if (menu_handler_start_state == 0 && menu_handler_stop_state == 0) {
			return;
	}
	//Start: 1. UI
	if (menu_handler_start_state == 1) {
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_MENU_MEDIA);
			current_scenario_state = SCENARIO_STATE_MENU;
			menu_handler_start_state = 0;
		}
	} else if (menu_handler_start_state > 0) {
		menu_handler_start_state++;
	}

	//Stop: 1. UI
	if (menu_handler_stop_state == 1) {
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_HOME);
			current_scenario_state = SCENARIO_STATE_HOME;

			menu_handler_stop_state = 0;
		}
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
		if (spi_protocol_get_status() == S_IDLE) {

			set_ui_view(UI_VIEW_ABOUT);
			current_scenario_state = SCENARIO_STATE_SETTINGS;

			settings_handler_start_state = 0;
		}
	} else if (settings_handler_start_state > 0) {
		settings_handler_start_state++;
	}

	//Stop: 1. UI
	if (settings_handler_stop_state == 1) {
		if (spi_protocol_get_status() == S_IDLE) {

			set_ui_view(UI_VIEW_HOME);
			current_scenario_state = SCENARIO_STATE_HOME;

			settings_handler_stop_state = 0;
		}
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
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_ABOUT);
			current_scenario_state = SCENARIO_STATE_ABOUT;

			about_handler_start_state = 0;
		}
	} else if (about_handler_start_state > 0) {
		about_handler_start_state++;
	}

	//Stop: 1. UI
	if (about_handler_stop_state == 1) {
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_HOME);
			current_scenario_state = SCENARIO_STATE_HOME;

			about_handler_stop_state = 0;
		}
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
		if (spi_protocol_get_status() == S_IDLE) {
			spi_command_atomic_exec_media_start(); // Start media player
			current_scenario_state = SCENARIO_STATE_MEDIA_PLAYER;
			media_player_handler_start_state = 0;
		}
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
		if (spi_protocol_get_status() == S_IDLE) {
			spi_command_atomic_exec_media_stop(); // media player: leave
			media_player_handler_stop_state++;
		}

	} else if (media_player_handler_stop_state == 4) {
		if (spi_protocol_get_status() == S_IDLE) {

			set_ui_view(UI_VIEW_HOME);
			current_scenario_state = SCENARIO_STATE_HOME;

			media_player_handler_stop_state = 0;
		}
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
		if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
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
		music_player_handler_start_state++;

	} else if (music_player_handler_start_state == 7) {
		if (spi_protocol_get_status() == S_IDLE) {
			current_scenario_state = SCENARIO_STATE_MUSIC;
			set_ui_view(UI_VIEW_MUSIC_PLAYER); // UI: music

			music_player_handler_start_state = 0;
		}
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
		current_scenario_state = SCENARIO_STATE_HOME;

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
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_AUDIO_CALL); // UI: call
			set_audio_call_status(STATUS_ON);
			current_scenario_state = SCENARIO_STATE_AUDIO_CALL;

			audio_call_handler_start_state = 0;
		}
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
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_HOME); // UI: home
			current_scenario_state = SCENARIO_STATE_HOME;

			audio_call_handler_stop_state = 0;
		}
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
		if (spi_protocol_get_status() == S_IDLE) {
			spi_command_atomic_exec_start_recording(); // Start recording
			video_recording_handler_start_state++;
		}
	} else if (video_recording_handler_start_state > 0) {
		if (ss_get_recording_status() == STATUS_ON) {
			if (spi_protocol_get_status() == S_IDLE) {
				set_ui_view(UI_VIEW_VIDEO_RECORDING); // UI: recording
				hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
				led_post_event(HAL_LED_EVENT_REFRESH);
				current_scenario_state = SCENARIO_STATE_VIDEO_RECORDING;

				video_recording_handler_start_state = 0;
			}
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
		if (spi_protocol_get_status() == S_IDLE) {
			PRINTF("[VideoRecording] Stop video recording...\r\n");
			spi_command_atomic_exec_stop_recording(); // Stop Recording

			video_recording_handler_stop_state++;
		}
	} else if (video_recording_handler_stop_state == 2) {
		RequestToGetOutofVideoRecording = 1;

		video_recording_handler_stop_state++;
	} else if (video_recording_handler_stop_state == 4) {

		video_recording_handler_stop_state++;

	} else if (video_recording_handler_stop_state > 0) { // Wait for Novatek ready
		if (ss_get_recording_status() == STATUS_OFF) {
			if (spi_protocol_get_status() == S_IDLE) {
				hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH);
				set_ringtone_state(Ringtone_StopRecording);

				set_ui_view(UI_VIEW_HOME); // UI: home
				current_scenario_state = SCENARIO_STATE_HOME;

				video_recording_handler_stop_state = 0;
			}
		} else if (video_recording_handler_stop_state > 200) {
			//TODO Retry 5 time
			if (spi_protocol_get_status() == S_IDLE) {

				PRINTF("[VideoRecording] Stop video recording Failed...\r\n");
				set_ui_view(UI_VIEW_HOME); // UI: home
				current_scenario_state = SCENARIO_STATE_HOME;

				video_recording_handler_stop_state = 0;
			}
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
	//Start: 1. Audio path 2. LED and AMP
	if (video_call_handler_start_state == 1) {
		PRINTF("[VideoCall] Start video call...\r\n");
		RequestToGetIntoVideoAI = 1; //Audio path
		video_call_handler_start_state++;

	} else if (video_call_handler_start_state == 2) {
		if (start_video_call_request) {
			if (spi_protocol_get_status() == S_IDLE) {
				if (phone_wifi_connected_status && soc_wifi_connected_status) {
					PRINTF("[VideoCall] Both phone and Novatek are connected\r\n");
					phone_wifi_connected_status = 0;
					soc_wifi_connected_status = 0;

				} else if (phone_wifi_connected_status) {
					PRINTF("[VideoCall] Only phone connected\r\n");
					phone_wifi_connected_status = 0;

				} else if (soc_wifi_connected_status) {
					PRINTF("[VideoCall] Only Novatek connected\r\n");
					soc_wifi_connected_status = 0;

				} else {
					PRINTF("[VideoCall] Both phone and Novatek are disconnected\r\n");
				}
				vTaskDelay(pdMS_TO_TICKS(5));
				PRINTF("[VideoCall] video_call_url: %s\r\n", get_video_call_url());
				spi_command_atomic_exec_start_video_call(get_video_call_url());
				start_video_call_request = 0;
				video_call_handler_start_state++;
			}
		}

	} else if (video_call_handler_start_state == 3) {
		// TODO Set LED and AMP when the Novatek RTSP is ready
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		amp_post_event(AMP_EVT_MUSIC);
		current_scenario_state = SCENARIO_STATE_VIDEO_CALL;

		video_call_handler_start_state = 0;

	} else if (video_call_handler_start_state > 0) {
		video_call_handler_start_state++;
	}

	//Stop: 1. AMP 2. Audio path 3. UI 4. LED
	if (video_call_handler_stop_state == 1) {
		PRINTF("[VideoCall] Stop video call...\r\n");
		amp_post_event(AMP_EVT_STOP);
		video_call_handler_stop_state++;

	} else if (video_call_handler_stop_state == 2) {
		RequestToGetOutofVideoAI = 1; //Audio path
		video_call_handler_stop_state++;

	} else if (video_call_handler_stop_state == 3) {
		if (stop_video_call_request) {
			if (spi_protocol_get_status() == S_IDLE) {
				spi_command_atomic_exec_stop_video_call();
				stop_video_call_request = 0;
				video_call_handler_stop_state++;
			}
		}
	} else if (video_call_handler_stop_state == 4) {
		if (spi_protocol_get_status() == S_IDLE) {
			set_ui_view(UI_VIEW_HOME); // UI: home
			video_call_handler_stop_state++;
		}
	} else if (video_call_handler_stop_state == 5) {
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		current_scenario_state = SCENARIO_STATE_HOME;
		//TODO Add a 60-second timer to close the Wi-Fi AP.
		video_call_handler_stop_state = 0;

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
		current_scenario_state = SCENARIO_STATE_VIDEO_AI;

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
		if (spi_protocol_get_status() == S_IDLE) {

			set_ui_view(UI_VIEW_HOME); // UI: home

			video_ai_handler_stop_state++;
		}
	} else if (video_ai_handler_stop_state == 4) {
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		set_ringtone_state(Ringtone_StopVideoAI);
		current_scenario_state = SCENARIO_STATE_HOME;

		video_ai_handler_stop_state = 0;

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
		current_scenario_state = SCENARIO_STATE_TRANSLATION;

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
		if (spi_protocol_get_status() == S_IDLE) {

			set_ui_view(UI_VIEW_HOME); // UI: home

			translation_handler_stop_state++;
		}
	} else if (translation_handler_stop_state == 4) {
		hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
		led_post_event(HAL_LED_EVENT_REFRESH);
		set_ringtone_state(Ringtone_StopTranslation);
		current_scenario_state = SCENARIO_STATE_HOME;

		translation_handler_stop_state = 0;

	} else if (translation_handler_stop_state > 0) {
		translation_handler_stop_state++;

	}
}

static void wifi_request_handler (void)
{
	// Send SPI command (Start Wi-Fi AP) to Novatek
	if (start_wifi_ap_request) {
		if (spi_protocol_get_status() == S_IDLE) {
			spi_command_atomic_exec_start_wifi_ap();
			start_wifi_ap_request = 0;
		}
	}

	// Send a BLE ACK (HOTSPOT_ON) to the phone when the Novatek Wi-Fi AP is opened
	if (soc_wifi_ap_opened_status) {
		ble_send_event_hotspot_on();
		soc_wifi_ap_opened_status = 0;
	}

	// Send a BLE ACK (IP and SSID) to the phone
	if (start_wifi_ip_request) {
		ble_send_event_ip_ssid();
		start_wifi_ip_request = 0;
	}

	if (stop_wifi_ap_request) {
		if (spi_protocol_get_status() == S_IDLE) {
			spi_command_atomic_exec_stop_wifi_ap();
			stop_wifi_ap_request = 0;
		}
	}
}

// For Backup
#if 0
static void video_call_request_handler (void)
{
	// Check if both phone and Novatek are connected
	if (start_video_call_request) {
		if (spi_protocol_get_status() == S_IDLE) {

			if (phone_wifi_connected_status && soc_wifi_connected_status) {
				PRINTF("[VideoCall] Both phone and Novatek are connected\r\n");
				phone_wifi_connected_status = 0;
				soc_wifi_connected_status = 0;

			} else if (phone_wifi_connected_status) {
				PRINTF("[VideoCall] Only phone connected\r\n");
				phone_wifi_connected_status = 0;

			} else if (soc_wifi_connected_status) {
				PRINTF("[VideoCall] Only Novatek connected\r\n");
				soc_wifi_connected_status = 0;

			} else {
				PRINTF("[VideoCall] Both phone and Novatek are disconnected\r\n");
			}
			vTaskDelay(pdMS_TO_TICKS(5));
			hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_ENABLE);
			led_post_event(HAL_LED_EVENT_REFRESH);
			amp_post_event(AMP_EVT_MUSIC);
			PRINTF("[VideoCall] video_call_url: %s\r\n", get_video_call_url());

			spi_command_atomic_exec_start_video_call(get_video_call_url());
			start_video_call_request = 0;
		}
	}

	if (stop_video_call_request) {
		if (spi_protocol_get_status() == S_IDLE) {
			amp_post_event(AMP_EVT_STOP);
			hal_led_set_situation(HAL_LED_STATUS_RECORDING, SITUATION_DISABLE);
			led_post_event(HAL_LED_EVENT_REFRESH);

			spi_command_atomic_exec_stop_video_call();
			stop_video_call_request = 0;
		}
	}
}
#endif

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
	scenario_power_off_handler();
	wifi_request_handler();
#if 0
	video_call_request_handler();
#endif
}

void set_power_off_handler_state (void)
{
	power_off_handler_state = 1;
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

// Request command from Phone (BLE)
uint8_t get_start_wifi_ap_request (void)
{
	return start_wifi_ap_request;
}

void set_start_wifi_ap_request (uint8_t on)
{
	if (on) {
		start_wifi_ap_request = 1;
	} else {
		start_wifi_ap_request = 0;
	}
}

// Request command from Phone (BLE) or from System Controller
uint8_t get_stop_wifi_ap_request (void)
{
	return stop_wifi_ap_request;
}

void set_stop_wifi_ap_request (uint8_t on)
{
	if (on) {
		stop_wifi_ap_request = 1;
	} else {
		stop_wifi_ap_request = 0;
	}
}

// Status event from Novatek (SPI)
uint8_t get_soc_wifi_ap_opened_status (void)
{
	return soc_wifi_ap_opened_status;

}

void set_soc_wifi_ap_opened_status (uint8_t on)
{
	if (on) {
		soc_wifi_ap_opened_status = 1;
	} else {
		soc_wifi_ap_opened_status = 0;
	}

}

// Request command from Phone (BLE)
uint8_t get_start_wifi_ip_request (void)
{
	return start_wifi_ip_request;

}

void set_start_wifi_ip_request (uint8_t on)
{
	if (on) {
		start_wifi_ip_request = 1;
	} else {
		start_wifi_ip_request = 0;
	}

}

// Status event from Phone (BLE)
uint8_t get_phone_wifi_connected_status (void)
{
	return phone_wifi_connected_status;

}

void set_phone_wifi_connected_status (uint8_t on)
{
	if (on) {
		phone_wifi_connected_status = 1;
	} else {
		phone_wifi_connected_status = 0;
	}

}

// Status event from Novatek (SPI)
uint8_t get_soc_wifi_connected_status (void)
{
	return soc_wifi_connected_status;

}

void set_soc_wifi_connected_status (uint8_t on)
{
	if (on) {
		soc_wifi_connected_status = 1;
	} else {
		soc_wifi_connected_status = 0;
	}

}

// Request command from Phone (BLE)
uint8_t get_start_video_call_request (void)
{
	return start_video_call_request;

}

void set_start_video_call_request (uint8_t on)
{
	if (on) {
		start_video_call_request = 1;
	} else {
		start_video_call_request = 0;
	}

}

// Request command from Phone (BLE)
uint8_t get_stop_video_call_request (void)
{
	return stop_video_call_request;

}

void set_stop_video_call_request (uint8_t on)
{
	if (on) {
		stop_video_call_request = 1;
	} else {
		stop_video_call_request = 0;
	}

}
