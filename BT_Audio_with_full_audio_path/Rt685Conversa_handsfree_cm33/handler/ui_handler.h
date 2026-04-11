/*
 * ui_handler.h
 *
 *  Created on: Apr 11, 2026
 *      Author: Lydia
 */

#ifndef UI_HANDLER_H_
#define UI_HANDLER_H_

#include "hal_common.h"

typedef enum {
	UI_VIEW_HOME = 0,
	UI_VIEW_MENU_MEDIA,
	UI_VIEW_MENU_SETTINGS,
	UI_VIEW_MENU_ABOUT,
	UI_VIEW_ABOUT,
	UI_VIEW_SETTINGS,
	UI_VIEW_MUSIC_PLAYER,
	UI_VIEW_AUDIO_CALL,
	UI_VIEW_MEDIA_PLAYER,
	UI_VIEW_VIDEO_RECORDING,
	UI_VIEW_TAKE_PHOTO,
	UI_VIEW_VIDEO_AI,
	UI_VIEW_TRANSLATION,
	UI_VIEW_VIDEO_CALL,
	UI_VIEW_VIDEO_STREAMING,
	UI_VIEW_NOTIFICATION,
} ui_view_t;

ui_view_t get_ui_view (void);
void set_ui_view (ui_view_t ui_view);

#endif /* UI_HANDLER_H_ */
