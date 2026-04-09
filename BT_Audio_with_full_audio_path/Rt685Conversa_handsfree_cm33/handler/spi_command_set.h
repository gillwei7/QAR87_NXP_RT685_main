/*
 * spi_command_set.h
 *
 *  Created on: 2026年4月8日
 *      Author: 11301026
 */

#ifndef SPI_COMMAND_SET_H_
#define SPI_COMMAND_SET_H_

#include "spi_handler.h"

void spi_command_atomic_exec_open_oe(void);
void spi_command_atomic_exec_close_oe(void);
void spi_command_atomic_exec_soft_power_off(void);
void spi_command_atomic_exec_unload_wifi_driver(void);

void spi_command_atomic_exec_enter(void);
void spi_command_atomic_exec_exit(void);
void spi_command_atomic_exec_menu_left(void);
void spi_command_atomic_exec_menu_right(void);

void spi_command_atomic_exec_media_start(void);
void spi_command_atomic_exec_media_stop(void);
void spi_command_atomic_exec_next_media(void);
void spi_command_atomic_exec_previous_media(void);
void spi_command_atomic_exec_media_play_pause(uint8_t media_state);

void spi_command_atomic_exec_take_picture(void);
void spi_command_atomic_exec_stop_recording(void);
void spi_command_atomic_exec_start_recording(void);

void spi_command_atomic_exec_switch_ui_page(uint8_t state);


void spi_command_atomic_status_version_info(void);
void spi_command_atomic_status_sys_status(void);
void spi_command_atomic_status_message_notification(app_msg_type_t app_type, const char *title, const char *body);
void spi_command_atomic_status_time_sync(   uint16_t year,
											uint8_t  month,
											uint8_t  day,
											uint8_t  hour,
											uint8_t  minute,
											uint8_t  second );



#endif /* SPI_COMMAND_SET_H_ */
