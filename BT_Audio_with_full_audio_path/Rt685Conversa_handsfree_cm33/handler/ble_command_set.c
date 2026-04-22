/*
 * ble_command_set.c
 *
 *  Created on: 2026年4月17日
 *      Author: 11301026
 */

#include "ble_command_set.h"

void bt_notify_ip_ssid(char *resp_buf, size_t resp_buf_size, bool *notify_flag,const char* ip,const char* ssid)
{
    if (resp_buf == NULL || notify_flag == NULL) return;

    snprintf(resp_buf, sizeof(char) * resp_buf_size, "ACK:IP:%s:SSID:%s", ip, ssid);
    *notify_flag = true;
}

void bt_notify_hotspot_on(char *resp_buf, bool *notify_flag)
{
    if (resp_buf == NULL || notify_flag == NULL) return;

    strcpy(resp_buf, "ACK:HOTSPOT_ON");
    *notify_flag = true;
}
void bt_notify_hotspot_off(char *resp_buf, bool *notify_flag)
{
    if (resp_buf == NULL || notify_flag == NULL) return;

    strcpy(resp_buf, "ACK:HOTSPOT_OFF");
    *notify_flag = true;
}
void bt_notify_hotspot_off_done(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

    strcpy(resp_buf, "ACK:HOTSPOT_OFF_DONE");
    *notify_flag = true;
}

void bt_notify_rtsp_recv(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

    strcpy(resp_buf, "ACK:RTSP_RECV");
    *notify_flag = true;
}
void bt_notify_rtsp_recv_stop(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

    strcpy(resp_buf, "ACK:RTSP_RECV_STOP");
    *notify_flag = true;
}

void bt_notify_camera_use(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

    // When Camera is using, return "CAMERA_USE"
	strcpy(resp_buf, "CAMERA_USE");
	*notify_flag = true;
}
void bt_notify_camera_state_taking_photo(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "CAMERA_STATE:TAKING_PHOTO");
	*notify_flag = true;
}
void bt_notify_camera_state_recording(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "CAMERA_STATE:RECORDING");
	*notify_flag = true;
}
void bt_notify_camera_state_idle(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "CAMERA_STATE:IDLE");
	*notify_flag = true;
}
void bt_notify_camera_state_error(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "CAMERA_STATE: ERROR");
	*notify_flag = true;
}
void bt_notify_phone_preview_show(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "ACK:PHONE_PREVIEW_SHOW");
	*notify_flag = true;
}
void bt_notify_phone_preview_hide(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "ACK:PHONE_PREVIEW_HIDE");
	*notify_flag = true;
}
void bt_notify_phone_action_glass_black_preview(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "ACK:ACTION_GLASS_BLACK_PREVIEW");
	*notify_flag = true;
}
void bt_notify_phone_action_glass_clear_black_preview(char *resp_buf, bool *notify_flag)
{
	if (resp_buf == NULL || notify_flag == NULL) return;

	strcpy(resp_buf, "ACK:ACTION_GLASS_CLEAR_BLACK_PREVIEW");
	*notify_flag = true;
}
