/*
 * bluetooth_handler.c
 *
 *  Created on: Apr 18, 2026
 *      Author: Lydia
 */

#include "bluetooth_handler.h"
#include "app_connect.h"
#include "GlobalDef.h"

#define CONNECTION_TIMER_TASK_DELAY              10
#define CONNECTION_TIMER_TIMEOUT_MILLISECOND     20000

static uint16_t connection_timer_count = 0;
static TimerHandle_t s_bluetooth_reconnect_timer = NULL;


static void bluetooth_reconnect_timer_callback(TimerHandle_t xTimer)
{
	bluetooth_reconnect();
	PRINTF("[BT] bluetooth_reconnect\r\n");
}

void bluetooth_reconnect_timer_init (void)
{
	s_bluetooth_reconnect_timer = xTimerCreate("BluetoothReconnectTimer",
	                               pdMS_TO_TICKS(5000),
								   pdFALSE,     // auto-reload
	                               NULL,
								   bluetooth_reconnect_timer_callback);
}

void bluetooth_reconnect_timer_start(void)
{
	if (s_bluetooth_reconnect_timer != NULL) {
	    xTimerStart(s_bluetooth_reconnect_timer, 0);
	}
}

void bluetooth_reconnect (void)
{
    if((g_pairedDeviceCount > 0) && (conn_rider_phone == NULL)){
#if BT_CONNECTION_LOG
        PRINTF("Connection timeout. Connect previous paired device\r\n");
#endif
        app_auto_connect_paired_devices();
    }

}

void bluetooth_reconnect_handler (void)
{
#if !CES_DEMO
#if AUTO_CONNECT_ENABLE
        if(connection_timer_count < (CONNECTION_TIMER_TIMEOUT_MILLISECOND/CONNECTION_TIMER_TASK_DELAY) && (conn_rider_phone == NULL)){
            connection_timer_count++;
        }else{
            connection_timer_count = 0;
            bluetooth_reconnect();
        }
#endif
#endif
}
