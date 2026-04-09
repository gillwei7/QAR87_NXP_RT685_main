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
#include "spi_handler.h"


static volatile scenario_state_t current_scenario_state = SCENARIO_STATE_HOME;
static volatile uint8_t is_media_playing = MUSIC_PAUSE;

static uint8_t media_player_handler_start_state = 0;
static uint8_t media_player_handler_stop_state = 0;

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
		if (current_scenario_state == SCENARIO_STATE_MUSIC_PLAYER) {
//			RequestToGetOutofA2dpPlay = 1;
//			current_scenario_state = state;
//			need_send_music_status = 1;
//			music_status = 0;

		} else
		if (current_scenario_state == SCENARIO_STATE_MEDIA_PLAYER) {
			current_scenario_state = state;
			media_player_handler_stop_state = 1;


		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_RECORDING) {
			RequestToGetOutofVideoRecording = 1;
			current_scenario_state = state;
//			need_send_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_VIDEO_AI) {
			RequestToGetOutofVideoAI = 1;
			current_scenario_state = state;
//			need_send_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_TRANSLATION) {
			RequestToGetOutofTranslation = 1;
			current_scenario_state = state;
//			need_send_state = 1;

		} else if (current_scenario_state == SCENARIO_STATE_TAKE_PHOTO) {
			current_scenario_state = state;
//			need_send_state = 1;

//			if (music_status == COMPONENT_ON) {
//				send_state_to_soc();
//				return;
//			}

			RequestToGetOutofTakePhoto = 1;

		}
#if ABOUT_STATE_ENABLE
		else if (current_scenario_state == SCENARIO_STATE_ABOUT) {
			current_scenario_state = state;
			need_send_state = 1;

			if (music_status == COMPONENT_ON) {
				send_state_to_soc();
				return;
			}

			RequestToGetOutofAbout = 1;

		}
#endif
#if MENU_STATE_ENABLE
		else if (current_scenario_state == SCENARIO_STATE_MENU) {
			current_scenario_state = state;
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
	else if (state == SCENARIO_STATE_MENU && current_scenario_state == SCENARIO_STATE_HOME) {
		current_scenario_state = state;
		need_send_state = 1;

		if (music_status == COMPONENT_ON) {
			send_state_to_soc();
			return;
		}

		RequestToGetIntoMenu = 1;

	}
#endif
#if ABOUT_STATE_ENABLE
	else if (state == SCENARIO_STATE_ABOUT && (current_scenario_state == SCENARIO_STATE_HOME || current_scenario_state == SCENARIO_STATE_MENU)) {
		current_scenario_state = state;
		need_send_state = 1;

		if (music_status == COMPONENT_ON) {
			send_state_to_soc();
			return;
		}

		RequestToGetIntoAbout = 1;

	}
#endif
	else if (state == SCENARIO_STATE_MUSIC_PLAYER && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
//		RequestToGetIntoA2dpPlay = 1;
//		current_scenario_state = state;
//		need_send_music_status = 1;
//		music_status = 1;


	} else if (state == SCENARIO_STATE_MEDIA_PLAYER && (current_scenario_state == SCENARIO_STATE_HOME
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
		RequestToGetIntoVideoRecording = 1;
		current_scenario_state = state;
//		need_send_state = 1;

	} else if (state == SCENARIO_STATE_TAKE_PHOTO && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		current_scenario_state = state;
//		need_send_state = 1;
#if 0
		if (music_status == COMPONENT_ON) {
			send_state_to_soc();
			return;
		}
#endif
		RequestToGetIntoTakePhoto = 1;

	} else if (state == SCENARIO_STATE_VIDEO_AI && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		RequestToGetIntoVideoAI = 1;
		current_scenario_state = state;
//		need_send_state = 1;

	} else if (state == SCENARIO_STATE_TRANSLATION && (current_scenario_state == SCENARIO_STATE_HOME
#if MENU_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_MENU
#endif
#if ABOUT_STATE_ENABLE
			 || current_scenario_state == SCENARIO_STATE_ABOUT
#endif
			)) {
		RequestToGetIntoTranslation = 1;
		current_scenario_state = state;
//		need_send_state = 1;

	}
}


void scenario_media_player_handler (void)
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

	} else if (media_player_handler_start_state == 3) {

		RequestToGetIntoMediaPlayer = 1;
		is_media_playing = 1;

		media_player_handler_start_state++;

	} else if (media_player_handler_start_state == 4) {
		send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_START); // Start media player

		media_player_handler_start_state = 0;

	} else if (media_player_handler_start_state > 0) {
		media_player_handler_start_state++;
	}

	//Stop
	if (media_player_handler_stop_state == 1) {
		PRINTF("[MediaPlayer] Stop media player...\r\n");

		hal_amp_aw88166_left_stop(); //workaround for noise
		hal_amp_aw88166_right_stop(); //workaround for noise

		media_player_handler_stop_state++;

	} else if (media_player_handler_stop_state == 2) {
		RequestToGetOutofMediaPlayer = 1;
//			need_send_state = 1;
		is_media_playing = MUSIC_PAUSE;

		media_player_handler_stop_state++;

	} else if (media_player_handler_stop_state == 3) {
		send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_STOP); // media player: leave

		media_player_handler_stop_state++;

	} else if (media_player_handler_stop_state == 4) {
		send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_SWITCH_UI_PAGE); // UI: home

		media_player_handler_stop_state = 0;

	} else if (media_player_handler_stop_state > 0) {
		media_player_handler_stop_state++;
	}
}

void scenario_state_handler (void)
{
	scenario_media_player_handler();
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
