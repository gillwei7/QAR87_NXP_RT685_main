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

void bt_notify_ip_ssid(char *resp_buf, bool *notify_flag,const char* ip,const char* ssid);

void bt_notify_hotspot_on(char *resp_buf, bool *notify_flag);
void bt_notify_hotspot_off(char *resp_buf, bool *notify_flag);
void bt_notify_hotspot_off_done(char *resp_buf, bool *notify_flag);

void bt_notify_rtsp_recv(char *resp_buf, bool *notify_flag);
void bt_notify_rtsp_recv_stop(char *resp_buf, bool *notify_flag);

void bt_notify_camera_use(char *resp_buf, bool *notify_flag);
void bt_notify_camera_state_taking_photo(char *resp_buf, bool *notify_flag);
void bt_notify_camera_state_recording(char *resp_buf, bool *notify_flag);
void bt_notify_camera_state_idle(char *resp_buf, bool *notify_flag);
void bt_notify_camera_state_error(char *resp_buf, bool *notify_flag);

void bt_notify_phone_preview_show(char *resp_buf, bool *notify_flag);
void bt_notify_phone_preview_hide(char *resp_buf, bool *notify_flag);

void bt_notify_phone_action_glass_black_preview(char *resp_buf, bool *notify_flag);
void bt_notify_phone_action_glass_clear_black_preview(char *resp_buf, bool *notify_flag);

#endif /* BLE_COMMAND_SET_H_ */
