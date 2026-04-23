/* hts.h - Health Thermometer Service sample */

/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 * Copyright 2022 NXP
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/
void peripheral_gls_task(void *pvParameters);
void peripheral_gls_le_adv_start(void);
void peripheral_gls_le_adv_stop(void);
uint8_t peripheral_ble_get_cmd_id(const char *cmd, uint8_t * parameters, size_t param_max_len);
void peripheral_ble_cmd_parser(uint8_t cmd_id, const struct bt_gatt_attr *attr, uint8_t * parameters);


void ble_cmd_exec_ap_id_ssid(const struct bt_gatt_attr *attr);
void ble_cmd_exec_ack_hotspot_on(const struct bt_gatt_attr *attr);
//void peripheral_gls_handle_ble_command(const struct bt_gatt_attr *attr, const char *cmd, char * resp_buf, size_t resp_buf_size, bool* notify_ack);

#define BLE_CMD_ID_UNKNOWN 						0xFF
#define BLE_CMD_ID_START_AP 					0x00
#define BLE_CMD_ID_START_AP_IP 					0x01
#define BLE_CMD_ID_WIFI_CONNECTED 				0x02
#define BLE_CMD_ID_RTSP_AV 						0x03
#define BLE_CMD_ID_START_VIDEO_CALL_URL 		0x04
#define BLE_CMD_ID_STOP_VIDEO_CALL	 			0x05
#define BLE_CMD_ID_ENTER_VIDEO_CALL	 			0x06
#define BLE_CMD_ID_LEAVE_VIDEO_CALL	 			0x07
#ifdef __cplusplus
}
#endif
