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
	SCENARIO_STATE_HOME = 0,          // 使用 Home
	SCENARIO_STATE_MENU,              // 使用 Menu
	SCENARIO_STATE_ABOUT,             // 使用 About
	SCENARIO_STATE_MUSIC_PLAYER,      // 使用 Music Player
	SCENARIO_STATE_MEDIA_PLAYER,      // 使用 Media Player
	SCENARIO_STATE_VIDEO_RECORDING,   // 使用 Video Recording
	SCENARIO_STATE_TAKE_PHOTO,        // 使用拍照
	SCENARIO_STATE_VIDEO_AI,          // 使用 Video AI
	SCENARIO_STATE_TRANSLATION        // 使用翻譯
} scenario_state_t;

uint8_t get_scenario_state(void);
void set_scenario_state(uint8_t state);
uint8_t get_media_status(void);
void set_media_status(uint8_t status);
#endif /* SCENARIO_STATE_H_ */
