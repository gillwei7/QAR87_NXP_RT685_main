/*
 * spi_command_set.c
 *
 *  Created on: 2026年4月8日
 *      Author: 11301026
 */


#include "spi_command_set.h"
#include "scenario_state.h"

static spi_command_ui_page_t s_ui_page_id = SPI_COMMAND_UI_PAGE_HOME;
static spi_command_media_play_pause_t s_media_play_pause = SPI_COMMAND_MEDIA_PLAY_TOGGLE;


void spi_command_atomic_exec_open_oe(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_OPEN_OE);
}

void spi_command_atomic_exec_close_oe(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_CLOSE_OE);
}

void spi_command_atomic_exec_soft_power_off(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_SOFT_POWER_OFF);
}

void spi_command_atomic_exec_unload_wifi_driver(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_UNLOAD_WIFI_DRIVER);
}

void spi_command_atomic_exec_enter(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_ENTER);
}

void spi_command_atomic_exec_exit(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_EXIT);
}

void spi_command_atomic_exec_menu_left(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MENU_LEFT);
}

void spi_command_atomic_exec_menu_right(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MENU_RIGHT);
}

void spi_command_atomic_exec_media_start(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_START);
}

void spi_command_atomic_exec_media_stop(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_STOP);
}

void spi_command_atomic_exec_next_media(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_NEXT_MEDIA);
}

void spi_command_atomic_exec_previous_media(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_PREVIOUS_MEDIA);
}

void spi_command_atomic_exec_media_play_pause(uint8_t media_state)
{
	s_media_play_pause = media_state;
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_PLAY_PAUSE);
}

void spi_command_atomic_exec_take_picture(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_TAKE_PICTURE);
}

void spi_command_atomic_exec_stop_recording(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_STOP_RECORDING);
}

void spi_command_atomic_exec_start_recording(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_START_RECORDING);
}


void spi_command_atomic_exec_switch_ui_page(spi_command_ui_page_t state)
{
    s_ui_page_id = state;
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_SWITCH_UI_PAGE);
}

void spi_command_atomic_status_version_info(void)
{
    send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_VERSION_INFO_SYNC);
}

void spi_command_atomic_status_sys_status(void)
{
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_SYS_STATUS);
}

void spi_command_atomic_status_time_sync(   uint16_t year,
											uint8_t  month,
											uint8_t  day,
											uint8_t  hour,
											uint8_t  minute,
											uint8_t  second )
{
	set_system_time(year, month, day, hour, minute, second);
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_TIME_SYNC);

}

void spi_command_atomic_status_message_notification(app_msg_type_t app_type, const char *title, const char *body)
{
	message_processing(app_type, title, body);
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_MSG_NOTIFICATION);
}

spi_command_ui_page_t get_ui_page_id(void)
{
	return s_ui_page_id;
}

spi_command_media_play_pause_t get_media_play_pause_cmd(void)
{
	return s_media_play_pause;
}
