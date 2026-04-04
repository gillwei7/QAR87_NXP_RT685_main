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


static volatile scenario_state_t current_scenario_state = SCENARIO_STATE_HOME;
static volatile uint8_t is_media_playing = MUSIC_PAUSE;


uint8_t get_scenario_state(void)
{
	return current_scenario_state;
}

void set_scenario_state(uint8_t state)
{
	PRINTF("[System] Usage State change: %d to %d \r\n",current_scenario_state,state);
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
			hal_amp_aw88166_left_stop(); //workaround for noise
			hal_amp_aw88166_right_stop(); //workaround for noise
			RequestToGetOutofMediaPlayer = 1;
			current_scenario_state = state;
//			need_send_state = 1;
			is_media_playing = MUSIC_PAUSE;

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
		RequestToGetIntoMediaPlayer = 1;
		current_scenario_state = state;
//		need_send_state = 1;
		is_media_playing = 1;

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
