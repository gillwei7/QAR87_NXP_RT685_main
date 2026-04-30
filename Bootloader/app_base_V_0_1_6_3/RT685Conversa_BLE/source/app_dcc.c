/*
 *  Copyright 2025 NXP
 *
 *  SPDX-License-Identifier: BSD-3-Clause
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
#include <bluetooth/hfp_ag.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/a2dp_codec_sbc.h>
#include <bluetooth/sdp.h>
#include <app_dcc.h>

#include <peripheral_gls.h>
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "app_connect.h"
//#include "a2dp_pl_media_48KHz.h"
#include "app_shell.h"

#include "BT_config.h"
#include "app_handsfree.h"
#include "app_a2dp_sink.h"


#define A2DP_CLASS_OF_DEVICE (0x200404U)

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    PRINTF("Pairing cancelled: %s\n", addr);
}

static void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    PRINTF("Passkey %06u\n", passkey);
}

#if 0
static void passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    PRINTF("Confirm passkey for %s: %06u", addr, passkey);
    s_passkeyConfirm = 1;
}
#endif

static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel, .passkey_display = passkey_display, /* Passkey display callback */
                                                               //  .passkey_confirm = passkey_confirm,
};

extern void connect_paired_device(uint8_t device_index);

void bt_ready(int err)
{
    struct net_buf *buf = NULL;
    struct bt_hci_cp_write_class_of_device *cp;
    PRINTF("[BTDBG] bt_ready enter, err=%d\n", err);

    if (err) {
        PRINTF("[BTDBG] bt_ready fail, err=%d\n", err);
        return;
    }

#if (defined(CONFIG_BT_SETTINGS) && (CONFIG_BT_SETTINGS > 0))
#if APP_SKIP_BT_SETTINGS_FLOW
    PRINTF("[BTDBG] skip settings_load()\n");
#else
    PRINTF("[BTDBG] settings_load begin\n");
    settings_load();
    PRINTF("[BTDBG] settings_load done\n");
#endif
#endif /* CONFIG_BT_SETTINGS */

    PRINTF("[BTDBG] Bluetooth initialized\n");

    PRINTF("[BTDBG] bt_hci_cmd_create begin\n");
    buf = bt_hci_cmd_create(BT_HCI_OP_WRITE_CLASS_OF_DEVICE, sizeof(*cp));
    if (buf != NULL)
    {
        cp = net_buf_add(buf, sizeof(*cp));
        sys_put_le24(A2DP_CLASS_OF_DEVICE, &cp->class_of_device[0]);
        PRINTF("[BTDBG] bt_hci_cmd_send_sync begin\n");
        err = bt_hci_cmd_send_sync(BT_HCI_OP_WRITE_CLASS_OF_DEVICE, buf, NULL);
        PRINTF("[BTDBG] bt_hci_cmd_send_sync done, err=%d\n", err);
    }
    else
    {
        err = -ENOBUFS;
        PRINTF("[BTDBG] bt_hci_cmd_create failed, err=%d\n", err);
    }

    if (err)
    {
#ifdef APP_DEBUG_EN
        PRINTF("setting class of device failed with err%d\n", err);
#endif
    }

   // PRINTF("Read FW version");
    //err = bt_hci_cmd_send_sync(BT_HCI_READ_FW_VERSION, NULL, NULL);

    if (err)
    {
    	PRINTF("Failed to read FW version");
    }

    PRINTF("[BTDBG] a2dp_sink_ready begin\n");
    	a2dp_sink_ready();
    PRINTF("[BTDBG] a2dp_sink_ready done\n");
    PRINTF("[BTDBG] hfp_hf_init begin\n");
    hfp_hf_init();
    PRINTF("[BTDBG] hfp_hf_init done\n");
    PRINTF("[BTDBG] app_connect_init begin\n");
    app_connect_init();
    PRINTF("[BTDBG] app_connect_init done\n");
    bt_conn_auth_cb_register(&auth_cb_display);

	#if (Using_UART5ToPrint)||(Using_UART2ToPrint)
		app_shell_init();
	#endif

    a2dp_sink_register_service();
    hfp_hf_register_service();

#if APP_SKIP_BT_SETTINGS_FLOW
    PRINTF("[BTDBG] skip app_lfs_init()\n");
#else
    PRINTF("[BTDBG] app_lfs_init begin\n");
    app_lfs_init();
    PRINTF("[BTDBG] app_lfs_init done\n");
#endif

#if defined(APP_LE_PERIPHERAL_ENABLE) && (APP_LE_PERIPHERAL_ENABLE == 1)
    le_adv_start();
    PRINTF("[BTDBG] le_adv_start done\n");
#endif
    app_a2dp_hf_auto_connect();
#if 0
   // if(g_pairedDeviceCount)
   // {
   // 	PRINTF("Trying connect paired device at startup\n");
    	//try to connect paired device at startup
   //  	connect_paired_device(paired_devices[0].addr[0]);
   // }else
   // {
    	//configure BT discoverable and connectable at startup
    	bt_br_set_connectable(false);
    	if (bt_br_set_connectable(true))
    	{
    	   PRINTF("BR/EDR set/reset connectable failed\n");
    	   return;
    	}
    	if (bt_br_set_discoverable(true))
    	{
    	  PRINTF("BR/EDR set discoverable failed\n");
    	   return;
    	}
    	PRINTF("BR/EDR set connectable and discoverable done\n");

        if(g_pairedDeviceCount)
        {
        	PRINTF("Trying connect paired device at startup\n");
        	//try to connect paired device at startup
         	connect_paired_device(paired_devices[0].addr[0]);
        }
    //}
#endif
}

#if 0
void hfp_hf_a2dp_task(void *pvParameters)
{
    int err = 0;
    (void)err;

    PRINTF("HFP+A2DP SINK Demo started...\n");

    /* Initializate BT Host stack */
    err = bt_enable(bt_ready);
    if (err) {
        PRINTF("Bluetooth init failed (err %d)\n", err);
        return;
    }

    vTaskDelete(NULL);
}
#endif
