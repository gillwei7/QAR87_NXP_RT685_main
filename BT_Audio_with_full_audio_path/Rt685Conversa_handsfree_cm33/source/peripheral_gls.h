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
void peripheral_gls_handle_ble_command(const struct bt_gatt_attr *attr, const char *cmd);
#ifdef __cplusplus
}
#endif
