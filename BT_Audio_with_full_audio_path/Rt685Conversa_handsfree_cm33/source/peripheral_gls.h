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
void send_ble_data (uint8_t * ble_data, uint16_t ble_data_len);
void peripheral_gls_task(void *pvParameters);
void peripheral_gls_le_adv_start(void);
void peripheral_gls_le_adv_stop(void);
void peripheral_ble_cmd_parser(uint8_t * ble_data, uint16_t data_len);

void ble_cmd_exec_ap_id_ssid(void);
void ble_cmd_exec_ack_hotspot_on(void);
//void peripheral_gls_handle_ble_command(const struct bt_gatt_attr *attr, const char *cmd, char * resp_buf, size_t resp_buf_size, bool* notify_ack);




#ifdef __cplusplus
}
#endif
