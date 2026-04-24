/*
 * ble_command_set.h
 *
 *  Created on: 2026年4月17日
 *      Author: 11301026
 */

#ifndef BLE_COMMAND_SET_H_
#define BLE_COMMAND_SET_H_


#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno/errno.h>
#include <toolchain.h>
#include <porting.h>
#include <stdbool.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/gls.h>
#include <fsl_debug_console.h>
#include <host_msd_fatfs.h>

void ble_send_event_ip_ssid(void);

void ble_send_event_hotspot_on(void);
void ble_send_event_hotspot_off(void);
void ble_send_event_hotspot_off_done(void);

void ble_send_event_rtsp_recv(void);
void ble_send_event_rtsp_recv_stop(void);

void ble_send_event_camera_use(void);
void ble_send_event_camera_state_taking_photo(void);
void ble_send_event_camera_state_recording(void);
void ble_send_event_camera_state_idle(void);
void ble_send_event_camera_state_error(void);

void ble_send_event_preview_show(void);
void ble_send_event_preview_hide(void);

void ble_send_event_action_glass_black_preview(void);
void ble_send_event_action_glass_clear_black_preview(void);

#endif /* BLE_COMMAND_SET_H_ */
