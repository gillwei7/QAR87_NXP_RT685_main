/*
 * ble_command_set.c
 *
 *  Created on: 2026年4月17日
 *      Author: 11301026
 */

#include "ble_command_set.h"
#include "system_status.h"
#include "peripheral_gls.h"


static char ble_cmd_exec_buff[128] = {0};




void ble_send_event_ip_ssid(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:IP:%s:SSID:%s", ss_get_wifi_ip(), ss_get_wifi_ssid());
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_hotspot_on(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:HOTSPOT_ON");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_hotspot_off(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:HOTSPOT_OFF");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_hotspot_off_done(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:HOTSPOT_OFF_DONE");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_rtsp_recv(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:RTSP_RECV");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_rtsp_recv_stop(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:RTSP_RECV_STOP");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_camera_use(void)
{
    // When Camera is using, return "CAMERA_USE"
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "CAMERA_USE");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_camera_state_taking_photo(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "CAMERA_STATE:TAKING_PHOTO");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_camera_state_recording(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "CAMERA_STATE:RECORDING");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_camera_state_idle(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "CAMERA_STATE:IDLE");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_camera_state_error(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "CAMERA_STATE:ERROR");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_preview_show(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:PHONE_PREVIEW_SHOW");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_preview_hide(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:PHONE_PREVIEW_HIDE");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_action_glass_black_preview(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:ACTION_GLASS_BLACK_PREVIEW");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}

void ble_send_event_action_glass_clear_black_preview(void)
{
	snprintf(ble_cmd_exec_buff, sizeof(ble_cmd_exec_buff), "ACK:ACTION_GLASS_CLEAR_BLACK_PREVIEW");
	send_ble_data(ble_cmd_exec_buff, strlen(ble_cmd_exec_buff));
}
