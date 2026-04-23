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


uint8_t get_scenario_state(void);
void set_scenario_state(uint8_t state);
void scenario_state_handler (void);

void set_power_off_handler_state (void);
void set_music_player_handler_start_state (void);
void set_music_player_handler_stop_state (void);

void set_audio_call_handler_start_state (void);
void set_audio_call_handler_stop_state (void);

uint8_t get_media_status(void);
void set_media_status(uint8_t status);
#endif /* SCENARIO_STATE_H_ */
