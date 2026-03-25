/*
 * Copyright (c) 2020 SixOctets Systems
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <errno/errno.h>
#include <toolchain.h>
#include <porting.h>
#include <fsl_debug_console.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/gls.h>

#include "peripheral_gls.h"

#define BT_DIS_MANUF     "NXP"
#define BT_DIS_NAME      "GLASSES Demo"
#define BT_DIS_SN        "BLESN01"

#define BT_DIS_STR_MAX   (20U)
#define BT_CHAR_VALUE_MAX_LEN  30

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint8_t manuf[BT_DIS_STR_MAX] = BT_DIS_MANUF;
static uint8_t name[BT_DIS_STR_MAX] = BT_DIS_NAME;
static uint8_t sn[BT_DIS_STR_MAX] = BT_DIS_SN;

static uint16_t g_char1DataLength = 0;
static uint16_t g_char2DataLength = 0;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void glasses_char1_cfg_changed(const struct bt_gatt_attr *attr,
         uint16_t value);

static void glasses_char2_cfg_changed(const struct bt_gatt_attr *attr,
         uint16_t value);

static ssize_t read_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
          void *buf, uint16_t len, uint16_t offset);

static ssize_t write_char1(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t write_char2(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static int8_t char1_data[BT_CHAR_VALUE_MAX_LEN];
static int8_t char2_data[BT_CHAR_VALUE_MAX_LEN];

static struct status_update status_update;
static struct status_update status_update_min;

static char statusChar1[BT_CHAR_VALUE_MAX_LEN] = "State_1";
static char statusChar2[BT_CHAR_VALUE_MAX_LEN] = "State_2";

bool notif_enabled_char1 = false;
bool notif_enabled_char2 = false;

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Device Information Service Declaration */
static BT_GATT_SERVICE_DEFINE(dev_info,
        BT_GATT_PRIMARY_SERVICE(BT_UUID_DIS),
        BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MANUFACTURER_NAME, BT_GATT_CHRC_READ,
             BT_GATT_PERM_READ, read_char, NULL, manuf),
        BT_GATT_CHARACTERISTIC(BT_UUID_DIS_MODEL_NUMBER, BT_GATT_CHRC_READ,
             BT_GATT_PERM_READ, read_char, NULL, name),
        BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SERIAL_NUMBER, BT_GATT_CHRC_READ,
             BT_GATT_PERM_READ, read_char, NULL, sn));

/*
 * Glasses Service Attribute Structure:
 *
 * attrs[0] - Primary Service Declaration (0x2800)
 * attrs[1] - Characteristic 1 Declaration (0x2803)
 * attrs[2] - Characteristic 1 VALUE (0x9b34) ← Use for Char1 notify
 * attrs[3] - Characteristic 1 CCCD (0x2902)
 * attrs[4] - Characteristic 2 Declaration (0x2803)
 * attrs[5] - Characteristic 2 VALUE (0x9b35) ← Use for Char2 notify
 * attrs[6] - Characteristic 2 CCCD (0x2902)
 */

/* Glasses Service Declaration */
static BT_GATT_SERVICE_DEFINE(glasses_svc,
		BT_GATT_PRIMARY_SERVICE(BT_UUID_GLASSES),
		BT_GATT_CHARACTERISTIC(BT_UUID_GLASSES_CHARACTERISTIC1, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_char, write_char1, NULL),
		BT_GATT_CCC(glasses_char1_cfg_changed,
				BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
		BT_GATT_CHARACTERISTIC(BT_UUID_GLASSES_CHARACTERISTIC2, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, read_char, write_char2, NULL),
		BT_GATT_CCC(glasses_char2_cfg_changed,
			  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);

/*******************************************************************************
 * Code
 ******************************************************************************/

static void glasses_char1_cfg_changed(const struct bt_gatt_attr *attr,
         uint16_t value)
{
   	ARG_UNUSED(attr);

   	notif_enabled_char1 = (value == BT_GATT_CCC_NOTIFY);

	PRINTF(" CH1 Notifications %s", notif_enabled_char1 ? "enabled\n" : "disabled\n");

    (void)notif_enabled_char1;

}

static void glasses_char2_cfg_changed(const struct bt_gatt_attr *attr,
         uint16_t value)
{
   	ARG_UNUSED(attr);

   	notif_enabled_char2 = (value == BT_GATT_CCC_NOTIFY);

	PRINTF(" CH2 Notifications %s", notif_enabled_char2 ? "enabled\n" : "disabled\n");

    (void)notif_enabled_char2;

}


void bt_status_notify()
{
	int rc;
	if(notif_enabled_char1)
	{

		PRINTF("Status is %s\n", statusChar1);

		// Prepare status_update structure
		status_update.flags = 0x00;
		memcpy(status_update.status, statusChar1, 15);

		// Notify the status update
		rc = bt_gatt_notify(NULL, &glasses_svc.attrs[2],
							(uint8_t *)&status_update, sizeof(status_update));

		if (rc < 0 && rc != -ENOTCONN)
		{
		    PRINTF("Char1 notify failed: %d\n", rc);
		}

	}

	if(notif_enabled_char2)
	{

			PRINTF("Status is %s\n", statusChar2);

			// Prepare status_update structure
			status_update_min.flags = 0x00;
			memcpy(status_update_min.status, statusChar2, 15);

			// Notify the status update
			rc = bt_gatt_notify(NULL, &glasses_svc.attrs[5],
								(uint8_t *)&status_update_min, sizeof(status_update_min));

			if (rc < 0 && rc != -ENOTCONN)
			{
			    PRINTF("Char2 notify failed: %d\n", rc);
			}
	}

}

static ssize_t read_char(struct bt_conn *conn, const struct bt_gatt_attr *attr,
          void *buf, uint16_t len, uint16_t offset)
{

	   if (bt_uuid_cmp(attr->uuid, BT_UUID_GLASSES_CHARACTERISTIC1) == 0) {

		   return bt_gatt_attr_read(conn, attr, buf, len, offset, (void *)char1_data,
				   g_char1DataLength);

	    }else  if (bt_uuid_cmp(attr->uuid, BT_UUID_GLASSES_CHARACTERISTIC2) == 0) {

			   return bt_gatt_attr_read(conn, attr, buf, len, offset, (void *)char2_data,
					   g_char2DataLength);
	    } else {

	    	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 strlen((char *)attr->user_data));
	    }
}

ssize_t write_char1(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len >= BT_CHAR_VALUE_MAX_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    memcpy(char1_data, buf, len);
    char1_data[len] = '\0';
    g_char1DataLength = len;

    PRINTF("Char1 write: %s, Length: %d\n", char1_data, len);

    return len;

}


ssize_t write_char2(struct bt_conn *conn, const struct bt_gatt_attr *attr,
             const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (len >= BT_CHAR_VALUE_MAX_LEN) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    memcpy(char2_data, buf, len);
    char2_data[len] = '\0';
    g_char2DataLength = len;

    PRINTF("BLE Char2 write: %s, Length: %d\n", char2_data, len);

    peripheral_gls_handle_ble_command(&glasses_svc.attrs[5], (const char *)char2_data);
//    // Prepare status_update structure
//    char test_char[] = "ACK:HOTSPOT_ON";
//
//    // Notify the status update
//    int rc = bt_gatt_notify(NULL,
//                        &glasses_svc.attrs[5],
//                        test_char,
//                        strlen(test_char));
//
//    PRINTF("notify result:%d\r\n", rc);
//
//    if (rc < 0 && rc != -ENOTCONN)
//    {
//        PRINTF("Char2 notify failed: %d\r\n", rc);
//    }

    return len;

}
