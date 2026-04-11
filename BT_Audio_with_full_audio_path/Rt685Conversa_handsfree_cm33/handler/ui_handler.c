/*
 * ui_handler.c
 *
 *  Created on: Apr 11, 2026
 *      Author: Lydia
 */


#include "ui_handler.h"
#include "spi_command_set.h"

static volatile ui_view_t current_ui_view = UI_VIEW_HOME;


void update_ui_view (void) {
	switch (current_ui_view) {
		case UI_VIEW_HOME:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_HOME);
			break;
		case UI_VIEW_MENU_MEDIA:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_LAUNCHER);
			break;

		case UI_VIEW_MENU_SETTINGS:
			break;

		case UI_VIEW_MENU_ABOUT:
			break;

		case UI_VIEW_ABOUT:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_ABOUT);
			break;

		case UI_VIEW_SETTINGS:
			break;

		case UI_VIEW_MUSIC_PLAYER:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_MUSIC_PLAYER);
			break;

		case UI_VIEW_AUDIO_CALL:
			break;

		case UI_VIEW_MEDIA_PLAYER:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_MEDIA);
			break;

		case UI_VIEW_VIDEO_RECORDING:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_RECORDING);
			break;

		case UI_VIEW_TAKE_PHOTO:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_SNAPSHOT);
			break;

		case UI_VIEW_VIDEO_AI:
			break;

		case UI_VIEW_TRANSLATION:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_TRANSLATION);
			break;

		case UI_VIEW_VIDEO_CALL:
			break;

		case UI_VIEW_VIDEO_STREAMING:
			break;

		case UI_VIEW_NOTIFICATION:
			spi_command_atomic_exec_switch_ui_page(SPI_COMMAND_UI_PAGE_NOTIFICATION);
			break;

		default:
			break;
	}

}

ui_view_t get_ui_view (void) {
	return current_ui_view;
}

void set_ui_view (ui_view_t ui_view) {
	if (current_ui_view != ui_view) {
		current_ui_view = ui_view;
		update_ui_view();
	}
}

