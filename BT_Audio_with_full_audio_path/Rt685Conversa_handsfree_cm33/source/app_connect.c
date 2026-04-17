/*
 * Copyright 2020, 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <porting.h>
#include <string.h>
#include <errno/errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/slist.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/a2dp_codec_sbc.h>
#include <bluetooth/hfp_ag.h>
#include <bluetooth/hfp_hf.h>
#include <bluetooth/map_mce.h>
#include <bluetooth/sdp.h>
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "app_connect.h"
#include "app_avrcp.h"
#include "app_a2dp_sink.h"
#include "app_handsfree.h"
#include "display_interface.h"
#if !((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
#include "lfs.h"
#include "littlefs_pl.h"
#endif
#include "app_handsfree.h"
#if UsingQAR87Board == 1
#include "ringtone_handler.h"
#endif

static int app_auto_connect_del_addr(bt_addr_t const *addr);
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);

#define SDP_CLIENT_USER_BUF_LEN        512U
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_USER_BUF_LEN, CONFIG_NET_BUF_USER_DATA_SIZE, NULL);

struct bt_conn *phone_le_conn=NULL;;

bt_addr_t g_riderHsAddr, g_riderPhoneAddr, g_passengerHsAddr;

//static uint8_t g_defaultConnectInitialized;
uint8_t g_connectInitRiderHs = 0, g_connectInitRiderPhone = 0 , g_connectInitPassengerHs = 0;
uint8_t g_profileConnectedPhone = 0, g_profileConnectedRiderHs = 0, g_profileConnectedPassengerHs = 0;
uint8_t g_isRiderHeadset=1;
uint8_t g_auto_connect_paired_devices = 0;
uint8_t g_auto_connect_device_index = 1;
struct bt_conn *default_conn;

static struct bt_conn_cb conn_callbacks = {
    .connected        = connected,
    .disconnected     = disconnected,
    .security_changed = security_changed,
};

paired_device_t paired_devices[MAX_PAIRED_DEVICES] = {0};
int g_pairedDeviceCount = 0;

static uint8_t g_defaultConnectInitialized;
static uint8_t g_connectableSet;
static bt_addr_t g_autoConnectDevice;
#if !((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
static lfs_t * lfs;
static lfs_file_t lfs_file;
#define FILE_NAME "peer_addr"
#endif

#define LFS_PAIRED_DEVICES_FILE  "paired_devices"

void app_hf_set_connectable(void)
{
    int err;
    
    if (g_connectableSet)
    {
        PRINTF("app_hf_set_connectable error.g_connectableSet = 1\r\n");
        return;
    }

    g_connectableSet = 1U;

    err = bt_br_set_connectable(true);
    if (err)
    {
        PRINTF("BR/EDR set/rest connectable failed (err %d)\n", err);
        return;
    }
    err = bt_br_set_discoverable(true);
    if (err)
    {
        PRINTF("BR/EDR set discoverable failed (err %d)\n", err);
        return;
    }
    PRINTF("BR/EDR set connectable and discoverable done\n");
    PRINTF("Wait for connection\r\n");
}

void sdp_discover_for_hfp_hf(struct bt_conn_info *info)
{
	int res;
	/*
	 * Do an SDP Query on Successful ACL connection complete with the
	 * required device
	 */
	if (0 == memcmp(info->br.dst, &g_riderPhoneAddr, 6U))
	{
		int res;
		res = bt_hfp_hf_discover(conn_rider_phone, &app_hfp_hf_discover);
		if (res)
		{
#ifdef APP_DEBUG_EN
			PRINTF("DUT as HFP-Dev SDP discovery failed (err %d)\n",res);
#endif
		}
		else
		{
#ifdef APP_DEBUG_EN
			PRINTF("DUT as HFP-Dev SDP discovery started\n");
#endif
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info info;
	if (err)
	{
		PRINTF("ACL Connection Failed (err %d)\n",err);

		if (g_connectInitRiderPhone)
		{
			g_connectInitRiderPhone = 0U;
#ifdef APP_DEBUG_EN
			PRINTF("DUT as HFP-HF to remote as HFP-AG Connection failed (err 0x%02x)\n", err);
#endif
			//app_hf_set_connectable();
			if (conn_rider_phone != NULL)
			{
				conn_rider_phone = NULL;
			}
		}
		return;
	}

	bt_conn_get_info(conn, &info);

	 if (info.type == BT_CONN_TYPE_BR && g_connectInitRiderPhone)
	{
		PRINTF("BTC Connected.Role:%d\r\n", info.role);
		g_connectInitRiderPhone = 0U;
		conn_rider_phone = conn;

#if AVRCP_BROWSING_ENABLE
		if(!bt_avrcp_browsing_connect(conn_rider_phone))
		{
			PRINTF("AVRCP Browsing Connect SUCCESS\r\n");
		}
#endif

		/*Profile level connection HFP HF*/
		sdp_discover_for_hfp_hf(&info);
	} else if (info.type == BT_CONN_TYPE_LE)
	{
		PRINTF("LE Connected\r\n");
		phone_le_conn= conn;

#if CONFIG_BT_SMP
       if (bt_conn_set_security(conn, BT_SECURITY_L0))
        {
            PRINTF("Failed to set security\n");
        }
#endif

	}else{
		PRINTF("ACL Connected.Connection Type:%d\r\n",info.type);
	}
    set_ringtone_state(Ringtone_BT_Connected);
	PRINTF("general_RingtoneState = Ringtone_BT_Connected\r\n");
}


static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    PRINTF("ACL Disconnection (reason %d)\n", reason);
    set_ringtone_state(Ringtone_BT_Disconnected);

   if(conn_rider_phone == conn)
    {

    	conn_rider_phone = NULL;
		g_connectInitRiderPhone = 0U;
		//app_hf_set_connectable();
    }
    else if (phone_le_conn == conn)
	{

    	PRINTF("LE ACL Disconnection (reason %d)\n", reason);
      
        phone_le_conn = NULL;
		return;
	}
	else
	{
#ifdef APP_DEBUG_EN
    	PRINTF("Disconnection event from unknown device !!\n");
#endif
    	return;
	}
}

static void reverse_order(uint8_t dest[6], const uint8_t src[6])
{
    for (int i = 0; i < 6; i++)
    {
        dest[i] = src[5 - i];  // Reverse order
    }
}


static void security_changed(struct bt_conn *conn, bt_security_t level,  enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_to_str(bt_conn_get_dst_br(conn), addr, sizeof(addr));

    if (!err)
    {
        PRINTF("Security changed: %s level %u\n", addr, level);

        if(g_isRiderHeadset){
            save_new_paired_device(conn,g_isRiderHeadset);
        }else {
            save_new_paired_device(conn,0);
        }

    }
    else
    {
        PRINTF("Security failed: %s level %u err %d\n", addr, level, err);
        if (err == BT_SECURITY_ERR_PIN_OR_KEY_MISSING || err == BT_SECURITY_ERR_AUTH_FAIL)
        {
            bt_addr_le_t addr;
            struct bt_conn_info info;
            int ret;

            bt_conn_get_info(conn, &info);
            if (info.type == BT_CONN_TYPE_LE)
            {
                return;
            }

            PRINTF("The peer device seems to have lost the bonding information.\n");
            PRINTF("Delete the bonding information of the peer, please try again.\n");
            addr.type = BT_ADDR_LE_PUBLIC;
            addr.a = *info.br.dst;
            ret = bt_unpair(BT_ID_DEFAULT, &addr);
            if (ret)
            {
                PRINTF("fail to delete.\n");
            }
            else
            {
            	uint8_t device_addr[6] = {0};
            	reverse_order(device_addr, addr.a.val);

            	remove_paired_device(device_addr);
                PRINTF("success to delete.\n");
            }
        }
//        PRINTF("Security failed: %s level %u err %d\n", addr, level, err);
    }
}

void app_connect(uint8_t device_type,uint8_t *addr)
{
    uint8_t corrected_addr[6] = {0};

	if(device_type == RIDER_PHONE)
	{
		if(conn_rider_phone == NULL )
		{
			g_connectInitRiderPhone = 1U;
			memcpy(&g_riderPhoneAddr, addr, 6U);


			conn_rider_phone = bt_conn_create_br(&g_riderPhoneAddr, BT_BR_CONN_PARAM_DEFAULT);
			if (!conn_rider_phone)
			{
				g_connectInitRiderPhone = 0U;
				PRINTF("ACL connection failed\r\n");

			}
			else
			{
				/* unref connection obj in advance as app user */
				bt_conn_unref(conn_rider_phone);

				if(conn_rider_phone == NULL )
				{
					PRINTF("Debug NULL Connecting Rider Phone\r\n");
				}
#if BT_CONNECTION_LOG
				PRINTF("Connecting Rider Phone\r\n");
#endif
			}
		} else
		{

			PRINTF(" Rider Phone connection already exists !! \r\n");
		}

	}

}

void app_disconnect(uint8_t device_type)
{
	int err=0;

	if(device_type == RIDER_PHONE)
	{
	    if (err = bt_conn_disconnect(conn_rider_phone, BT_HCI_ERR_REMOTE_USER_TERM_CONN))
	    {
	        PRINTF("Disconnection failed (err %d)\r\n",err);
	    }
	}
}

void app_auto_connect_paired_devices()
{
   if(g_auto_connect_device_index > 0 && g_pairedDeviceCount >= g_auto_connect_device_index)
   {
	   g_auto_connect_paired_devices =1;
	   connect_paired_device(g_auto_connect_device_index);
   }

}

void app_connect_init(void)
{
    bt_conn_cb_register(&conn_callbacks);
}

#if !((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))

int app_read_paired_devices()
{
    if (!lfs)
    {
        PRINTF("LittleFS not initialized!\n");
        return -1;
    }

    lfs_file_t file;
    int err = lfs_file_open(lfs, &file, LFS_PAIRED_DEVICES_FILE, LFS_O_RDONLY);
    if (err < 0)
    {
//        PRINTF("Failed to open paired devices file: error code = %d\n", err);
        return err;
    }

    // First, read the stored g_pairedDeviceCount
    err = lfs_file_read(lfs, &file, &g_pairedDeviceCount, sizeof(g_pairedDeviceCount));
    if (err < 0 || g_pairedDeviceCount > MAX_PAIRED_DEVICES)
    {
        PRINTF("Failed to read paired device count or count is invalid.\n");
        g_pairedDeviceCount = 0;  // Reset in case of error
        lfs_file_close(lfs, &file);
        return -1;
    }

    //Then, read the paired devices list
    err = lfs_file_read(lfs, &file, paired_devices, g_pairedDeviceCount * sizeof(paired_device_t));
    if (err < 0)
    {
        PRINTF("Failed to read paired devices. Error code = %d\n", err);
        g_pairedDeviceCount = 0;  // Reset in case of error
        lfs_file_close(lfs, &file);
        return -1;
    }

    lfs_file_close(lfs, &file);

    if(g_pairedDeviceCount){
    	PRINTF("Paired Devices List:\n");
    }

    for (int i = 0; i < g_pairedDeviceCount; i++)
    {
        PRINTF("[%d] Address: %02X:%02X:%02X:%02X:%02X:%02X, Name: %s, Type: %d\n",
               i + 1,
               paired_devices[i].addr[0], paired_devices[i].addr[1], paired_devices[i].addr[2],
               paired_devices[i].addr[3], paired_devices[i].addr[4], paired_devices[i].addr[5],
               paired_devices[i].name, paired_devices[i].device_type);
    }

    return 0;
}

void app_lfs_init(void)
{

    lfs = lfs_pl_init();
    if (!lfs)
    {
    	PRINTF("lfs_pl_init failed!\n");
    }

    app_read_paired_devices();

}

int app_save_paired_device(const uint8_t addr[6], const char *name, uint8_t device_type)
{

	if (!lfs)  //Ensure LFS is initialized
    {
        PRINTF("LittleFS not initialized!\n");
        return -1;
    }

	for (int i = 0; i < g_pairedDeviceCount; i++)
	{
		if (memcmp(paired_devices[i].addr, addr, 6) == 0)
		{
			PRINTF("Device already exists in paired list.\n");
			return 0;
		}
	}

    if (g_pairedDeviceCount >= MAX_PAIRED_DEVICES)
    {
    	PRINTF("Paired devices reached max number, \n Please remove an existing device to save new paired device.\n");
    	return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
    // Add new device
    memcpy(paired_devices[g_pairedDeviceCount].addr, addr, 6);
    strncpy(paired_devices[g_pairedDeviceCount].name, name, sizeof(paired_devices[g_pairedDeviceCount].name) - 1);
    paired_devices[g_pairedDeviceCount].device_type = device_type;
    g_pairedDeviceCount++;

    lfs_remove(lfs, LFS_PAIRED_DEVICES_FILE);
    vTaskDelay(pdMS_TO_TICKS(50));
    // Save to LittleFS
    lfs_file_t file;
    int err = lfs_file_open(lfs, &file, LFS_PAIRED_DEVICES_FILE, LFS_O_WRONLY | LFS_O_CREAT );
    if (err < 0)  //Debug: Check open file error
    {
        PRINTF("Failed to open file: error code = %d\n", err);
        return err;
    }

    err = lfs_file_write(lfs, &file, &g_pairedDeviceCount, sizeof(g_pairedDeviceCount));
    if (err < 0)
    {
        PRINTF("Failed to write paired device count. Error code = %d\n", err);
        lfs_file_close(lfs, &file);
        return err;
    }

    err = lfs_file_write(lfs, &file, paired_devices, g_pairedDeviceCount * sizeof(paired_device_t));
    if (err < 0)
    {
        PRINTF("Failed to write paired device. Error code = %d\n", err);
        lfs_file_close(lfs, &file);
        return err;
    }

    lfs_file_close(lfs, &file);
    return 0;
}


int app_save_paired_devices()
{
    if (!lfs)
    {
        PRINTF("LittleFS not initialized!\n");
        return -1;
    }

    lfs_remove(lfs, LFS_PAIRED_DEVICES_FILE);
    vTaskDelay(pdMS_TO_TICKS(50));

    lfs_file_t file;
    int err = lfs_file_open(lfs, &file, LFS_PAIRED_DEVICES_FILE, LFS_O_WRONLY | LFS_O_CREAT );
    if (err < 0)
    {
        PRINTF("Failed to open paired devices file: error code = %d\n", err);
        return err;
    }

    // First, write `g_pairedDeviceCount`
    err = lfs_file_write(lfs, &file, &g_pairedDeviceCount, sizeof(g_pairedDeviceCount));
    if (err < 0)
    {
        PRINTF("Failed to write paired device count. Error code = %d\n", err);
        lfs_file_close(lfs, &file);
        return err;
    }

    // Then, write the paired devices list
    err = lfs_file_write(lfs, &file, paired_devices, g_pairedDeviceCount * sizeof(paired_device_t));
    if (err < 0)
    {
        PRINTF("Failed to write paired devices. Error code = %d\n", err);
        lfs_file_close(lfs, &file);
        return err;
    }

    lfs_file_close(lfs, &file);
   // PRINTF("Paired devices saved to LFS.\n");
    return 0;
}

int app_clear_paired_devices()
{
    if (!lfs)
    {
        PRINTF("LittleFS not initialized!\n");
        return -1;
    }

    PRINTF("Clearing all paired devices from LittleFS...\n");

    //step 1: Reset the paired devices array in RAM
    memset(paired_devices, 0, sizeof(paired_devices));
    g_pairedDeviceCount = 0;

    //Step 2: Remove the paired devices file from LittleFS
    int err = lfs_remove(lfs, LFS_PAIRED_DEVICES_FILE);
    if (err < 0)
    {
        PRINTF("Failed to remove paired devices file: error code = %d\n", err);
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    PRINTF("All paired devices successfully removed from LFS.\n");
    return 0;
}
#endif


#if !((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
static int app_auto_connect_get_addr()
{
    int err;
    int len;

    err = lfs_file_open(lfs, &lfs_file, FILE_NAME, LFS_O_RDONLY);

    if (err)
    {
        return err;
    }

    len = lfs_file_read(lfs, &lfs_file, (char *)&g_autoConnectDevice.val[0], 6U);
    if (len != 6U)
    {
        err = -EIO;
    }
    else
    {
        err = 0;
    }
    lfs_file_close(lfs, &lfs_file);

    return err;
}

static int app_auto_connect_del_addr(bt_addr_t const *addr)
{
    int err = lfs_remove(lfs, FILE_NAME);

    return err;
}

//int app_auto_connect_save_addr(bt_addr_t const *addr)
//{
//    int err;
//    int len;
//
//    lfs_remove(lfs, FILE_NAME);
//    err = lfs_file_open(lfs, &lfs_file, FILE_NAME, LFS_O_WRONLY | LFS_O_CREAT);
//
//    if (err)
//    {
//        PRINTF("fail to save device addr\r\n");
//        return err;
//    }
//
//    len = lfs_file_write(lfs, &lfs_file, addr, 6U);
//    if (len != 6U)
//    {
//        PRINTF("fail to save device addr\r\n");
//        err = -EIO;
//    }
//    else
//    {
//        err = 0;
//    }
//    lfs_file_close(lfs, &lfs_file);
//
//    return err;
//}
#endif

#if ((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
static void bond_info(const struct bt_bond_info *info, void *user_data)
{
	uint8_t *valid = (uint8_t *)user_data;

    if (!(*valid))
    {
        *valid = 1u;
        g_autoConnectDevice = info->addr.a;
    }
}
#endif

void app_a2dp_hf_auto_connect(void)
{
#if ((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
    uint8_t valid = 0;

    memset(&g_autoConnectDevice, 0, sizeof(g_autoConnectDevice));
    bt_foreach_bond(BT_ID_DEFAULT, bond_info, &valid);
    
    if (valid)
    {
        struct bt_conn *conn;

        g_defaultConnectInitialized = 1U;
        conn = bt_conn_create_br(&g_autoConnectDevice, BT_BR_CONN_PARAM_DEFAULT);
        if (!conn)
        {
            PRINTF("Connection failed\r\n");
        }
        else
        {
            bt_conn_unref(conn);
            PRINTF("Connection pending\r\n");
        }
    }
    else
    {
        app_hf_set_connectable();
    }
#else
    lfs = lfs_pl_init();
    if ((lfs != NULL) && !app_auto_connect_get_addr())
    {
        struct bt_conn *conn;

        g_defaultConnectInitialized = 1U;
        conn = bt_conn_create_br(&g_autoConnectDevice, BT_BR_CONN_PARAM_DEFAULT);
        if (!conn)
        {
            PRINTF("Connection failed\r\n");
        }
        else
        {
            bt_conn_unref(conn);
            PRINTF("Connection pending\r\n");
        }
    }
    else
    {
        app_hf_set_connectable();
    }
#endif
}

void app_clear_device_enter_discoverable(void){

    // Delete all paired device and set Connectable and Discoverable

    if (conn_rider_phone != NULL)
    {
        app_disconnect(RIDER_PHONE);
        while(conn_rider_phone != NULL);
    }

#if !((defined AUTO_CONNECT_USE_BOND_INFO) && (AUTO_CONNECT_USE_BOND_INFO))
    /*First need to read the paired device*/
    if (!app_read_paired_devices())
    {
        uint8_t addr[6];
        PRINTF("Number of paired device count is %d\n", g_pairedDeviceCount);
        for(int i = 0;i < g_pairedDeviceCount; i++)
        {
            PRINTF("[%d] Address: %02X:%02X:%02X:%02X:%02X:%02X, Name: %s, Type: %d\n",
                    i + 1,
                    paired_devices[i].addr[0], paired_devices[i].addr[1], paired_devices[i].addr[2],
                    paired_devices[i].addr[3], paired_devices[i].addr[4], paired_devices[i].addr[5],
                    paired_devices[i].name, paired_devices[i].device_type);

            if (memcmp(paired_devices[i].addr, addr, 6) == 0)
            {
                bt_unpair(BT_ID_DEFAULT,(bt_addr_le_t *)addr);
            }
        }
        PRINTF("clear_paired_devices_from_lfs.\n\n");
        vTaskDelay(pdMS_TO_TICKS(50));
        app_clear_paired_devices();
    }
#endif

    app_hf_set_connectable();
}
