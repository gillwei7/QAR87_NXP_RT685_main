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

#include "system_status.h"
#include "ble_command_set.h"
#include "spi_command_set.h"

#if defined(APP_LOWPOWER_ENABLED) && (APP_LOWPOWER_ENABLED > 0)
#include "PWR_Interface.h"
#include "fwk_platform_lowpower.h"
#endif /* APP_LOWPOWER_ENABLED */

#if defined(APP_MEM_POWER_OPT) && (APP_MEM_POWER_OPT > 0)
#include "fsl_mmc.h"
#include "sdmmc_config.h"
#endif /* APP_MEM_POWER_OPT */


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
 //static void connected(struct bt_conn *conn, uint8_t err);
 //static void disconnected(struct bt_conn *conn, uint8_t reason);
// TODO Need to get Wi-Fi AP IP and SSID
const char* get_device_ip() { return "192.168.1.1"; }
const char* get_device_ssid() { return "Quanta_Demo_2026_5G_907df2"; }
extern uint8_t bt_id_read_public_addr(bt_addr_le_t *addr);

/*******************************************************************************
 * Variables
 ******************************************************************************/
//struct bt_conn *default_conn;
static char ble_service_name[20]; // Buffer to hold "QAR88a_XXXX_BLE"
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
//    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
//    		BT_UUID_128_ENCODE(0x0000fff0, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			BT_UUID_16_ENCODE(0xFFF0)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			BT_UUID_16_ENCODE(BT_UUID_DIS_VAL)),
};

static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, ble_service_name, 0),
};

#if defined(APP_MEM_POWER_OPT) && (APP_MEM_POWER_OPT > 0)
extern mmc_card_t g_mmc;
#endif /* APP_MEM_POWER_OPT */

/*******************************************************************************
 * Code
 ******************************************************************************/
void peripheral_gls_init_ble_name(void)
{
    bt_addr_le_t addrs[1];

    // Get the current identity address
    bt_id_read_public_addr(addrs);

    // Format the name: QAR88a_ + 2 bytes of address + _BLE
    // Using val[1] and val[0] as per your bt_pal_br.c logic
    snprintf(ble_service_name, sizeof(ble_service_name), "QAR88a_%02X%02X_BLE",
             addrs[0].a.val[1], addrs[0].a.val[0]);

    // Update the length in the advertising data structure
    sd[0].data_len = strlen(ble_service_name);

    PRINTF("[BLE]BLE Service Device Name set to: %s\n", ble_service_name);
}

void peripheral_gls_le_adv_stop(void)
{
    int err = bt_le_adv_stop();
    if (err)
    {
        PRINTF("Advertising failed to stop (err %d)\n", err);
        return;
    }else
    PRINTF("Advertising Stop\r\n");
}


void peripheral_gls_le_adv_start()
{
    peripheral_gls_init_ble_name();
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

    	//spi_command_atomic_exec_start_wifi_ap(get_device_ip(), ss_get_wifi_ssid());

    	bt_notify_hotspot_on(response, &should_notify);
    }
    else if (strcmp(cmd, "Start_AP_IP") == 0) {
    	// TODO: Need IP and SSID from Novatek
    	bt_notify_ip_ssid(response, sizeof(response), &should_notify,get_device_ip(), get_device_ssid());
    }
    else if (strcmp(cmd, "WIFI_CONNECTED") == 0) {
    	//bt_notify_camera_use(response, &should_notify);
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

    	bt_notify_camera_state_idle(response, &should_notify);
    	//bt_notify_camera_state_taking_photo(response, &should_notify);
    	//bt_notify_camera_state_error(response, &should_notify);

    }
    else if (strcmp(cmd, "START_RECORDING") == 0) {
    	PRINTF("");
    	// TODO: Need to check Camera state?

    	bt_notify_camera_state_recording(response, &should_notify);
    	//bt_notify_camera_state_error(response, &should_notify);
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

    	bt_notify_camera_state_idle(response, &should_notify);
    	//bt_notify_camera_state_error(response, &should_notify);
    }
    else if (strcmp(cmd, "RTSP_AUDIO_ONLY_ON") == 0) {
        // TODO Switch RTSP to audio. No return to APP
    }
    else if (strcmp(cmd, "RTSP_AV") == 0) {
        // TODO Switch RTSP to A/V. No return to APP
    }
	else if (strncmp(cmd, "Start_Video_Call_URL:", 21) == 0)
	{
		//spi_command_atomic_exec_start_video_call("rtsp://192.168.1.22:8554/mystream");
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
