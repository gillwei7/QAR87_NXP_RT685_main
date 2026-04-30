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

#define CONFIG_BT_SERVICE_NAME "Glasses Service"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
 //static void connected(struct bt_conn *conn, uint8_t err);
 //static void disconnected(struct bt_conn *conn, uint8_t reason);

/*******************************************************************************
 * Variables
 ******************************************************************************/
//struct bt_conn *default_conn;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
    		BT_UUID_128_ENCODE(0x0000fff0, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)),
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
void le_adv_start()
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
