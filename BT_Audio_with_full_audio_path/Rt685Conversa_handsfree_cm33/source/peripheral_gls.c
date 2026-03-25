/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno/errno.h>
#include <toolchain.h>
#include <porting.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/gls.h>
#include <fsl_debug_console.h>
#include <host_msd_fatfs.h>

#if defined(APP_LOWPOWER_ENABLED) && (APP_LOWPOWER_ENABLED > 0)
#include "PWR_Interface.h"
#include "fwk_platform_lowpower.h"
#endif /* APP_LOWPOWER_ENABLED */

#if defined(APP_MEM_POWER_OPT) && (APP_MEM_POWER_OPT > 0)
#include "fsl_mmc.h"
#include "sdmmc_config.h"
#endif /* APP_MEM_POWER_OPT */

#define CONFIG_BT_SERVICE_NAME "QAR88n_07F3_BLE"//"Glasses Service"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
 //static void connected(struct bt_conn *conn, uint8_t err);
 //static void disconnected(struct bt_conn *conn, uint8_t reason);
// TODO Need to get Wi-Fi AP IP and SSID
const char* get_device_ip() { return "192.168.1.1"; }
const char* get_device_ssid() { return "Quanta_Demo_2026_5G_907df2"; }

/*******************************************************************************
 * Variables
 ******************************************************************************/
//struct bt_conn *default_conn;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
//    		BT_UUID_128_ENCODE(0x0000fff0, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			BT_UUID_16_ENCODE(0xFFF0)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_SERVICE_NAME, sizeof(CONFIG_BT_SERVICE_NAME) - 1),
};


#if defined(APP_MEM_POWER_OPT) && (APP_MEM_POWER_OPT > 0)
extern mmc_card_t g_mmc;
#endif /* APP_MEM_POWER_OPT */

/*******************************************************************************
 * Code
 ******************************************************************************/
void peripheral_gls_le_adv_start()
{
    //int err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        PRINTF("Advertising failed to start (err %d)\n", err);
        return;
    }

    PRINTF("Advertising successfully started\n");
}

void peripheral_gls_task(void *pvParameters) {

	while(1)
    {
        vTaskDelay(2000);
        bt_status_notify();
    }
}

void peripheral_gls_handle_ble_command(const struct bt_gatt_attr *attr, const char *cmd)
{
    char response[128] = {0};
    bool should_notify = false;

    if (strcmp(cmd, "Start_AP") == 0) {
        strcpy(response, "ACK:HOTSPOT_ON");
        should_notify = true;
    }
    else if (strcmp(cmd, "Start_AP_IP") == 0) {
    	// TODO: Need IP and SSID from Novatek
        snprintf(response, sizeof(response), "ACK:IP:%s:SSID:%s", get_device_ip(), get_device_ssid());
        should_notify = true;
    }
    else if (strcmp(cmd, "WIFI_CONNECTED") == 0) {
        // When Camera is using, return "CAMERA_USE"
    	//strcpy(response, "CAMERA_USE");
    }
    else if (strcmp(cmd, "STOP_VIDEOCHAT") == 0) {
        // TODO Stop videochat, No Return to APP
    }
    else if (strcmp(cmd, "stopTranslation") == 0) {
        // TODO Action: Stop translation, No Return to APP
    }
    else if (strcmp(cmd, "TAKE_PHOTO") == 0) {
        // TODO Need to Check Camera state?
        // Valid returns: CAMERA_STATE:TAKING_PHOTO, CAMERA_STATE:IDLE, or CAMERA_STATE:ERROR
        strcpy(response, "CAMERA_STATE:IDLE");
        should_notify = true;
    }
    else if (strcmp(cmd, "START_RECORDING") == 0) {
    	PRINTF("");
    	// TODO: Need to check Camera state?
        strcpy(response, "CAMERA_STATE:RECORDING");
        should_notify = true;
    }
    else if (strcmp(cmd, "startFileSync") == 0) {
        // TODO Send Sync media to Novatek, No Return to APP
        PRINTF("[BLE]Starting File Sync...\n");
    }

    else if (strcmp(cmd, "NEW_MEDIA") == 0) {
        // TODO Open WiFi Socket
        PRINTF("Opening Wi-Fi Socket...\n");
    }
    else if (strcmp(cmd, "STOP_RECORDING") == 0) {
        strcpy(response, "CAMERA_STATE:IDLE");
        should_notify = true;
    }
    else if (strcmp(cmd, "RTSP_AUDIO_ONLY_ON") == 0) {
        // TODO Switch RTSP to audio. No return to APP
    }
    else if (strcmp(cmd, "RTSP_AV") == 0) {
        // TODO Switch RTSP to A/V. No return to APP
    }
    else {
        PRINTF("Unknown command: %s\n", cmd);
    }

    // Send the notification if the table requires a return string
    if (should_notify && strlen(response) > 0) {
        int rc = bt_gatt_notify(NULL, attr, response, strlen(response));

        if (rc == 0) {
            PRINTF("[BLE]Notification Sent: %s\n", response);
        } else if (rc == -ENOTCONN) {
            PRINTF("[BLE]Notification skipped: App not connected.\n");
        } else {
            PRINTF("[BLE]Notification failed (err %d)\n", rc);
        }
    }
}
