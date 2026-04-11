/*
 * ringtone_handler.c
 *
 *  Created on: Apr 4, 2026
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "ringtone_handler.h"
#include "spi_handler.h"
#include "button_handler.h"
#include "i2c_component_handler.h"
#include "hal_common.h"
#include "hal_amp.h"
#include "app_connect.h"
#include "system_status.h"
#include "WorkStateManager.h"
#include "DefForBothMcuAndDsp.h"
#include "GlobalDef.h"
#include "WorkStateManager.h"
#include "MainAudioFlow.h"

#define CONNECTION_TIMER_TASK_DELAY              10
#define RINGTONE_TIME_DELAY                      5000
#define CONNECTION_TIMER_TIMEOUT_MILLISECOND     20000

static RingtoneState general_RingtoneState = Ringtone_No;
static uint8_t            ringtone_is_played = 0;
static uint32_t           ringtone_time_count = 0;
static uint16_t connection_timer_count = 0;

static uint8_t ringtone_audio_handler_state = 0;



static void startOpusPlayIndex(int opus_index){
    int play_opus_index  = 0;
    if(opus_index < 0){
        PRINTF("startOpusPlay: opus index < 0\r\n");
        return;
    } else if (opus_index > (OPUS_INDEX_MAXIMUM-1)){
        play_opus_index = OPUS_INDEX_MAXIMUM;
        PRINTF("startOpusPlay: opus index > %d\r\n", OPUS_INDEX_MAXIMUM-1);
    } else {
        play_opus_index = opus_index;
        PRINTF("startOpusPlay: opus index %d\r\n", play_opus_index);
    }
    VarBlockSharedByDspAndMcu.NeedToStartPlayOpus=1;
    VarBlockSharedByDspAndMcu.PlayOpusFileIdx=play_opus_index;
}

void ringtone_test(int opus_index){
	startOpusPlayIndex(opus_index);
}

static void ringtone_audio_set_state (void)
{
	if (ringtone_audio_handler_state == 0) {
		ringtone_audio_handler_state = 1;
	}
}

void set_ringtone_state (RingtoneState state)
{
	general_RingtoneState = state;

	if (get_amp_status() == AMP_STATUS_OFF && general_RingtoneState != Ringtone_No) {
		ringtone_audio_set_state();

	} else if (general_RingtoneState != Ringtone_No) {
		startOpusPlayIndex(general_RingtoneState - 1);
		general_RingtoneState = Ringtone_No;
	}
}

static void ringtone_audio_handler (void)
{
	if (ringtone_audio_handler_state == 0) {
		return;

	} else if (ringtone_audio_handler_state == 1) {
		amp_post_event(AMP_EVT_RECEIVER);
		ringtone_audio_handler_state++;

	} else if (ringtone_audio_handler_state == 2) {
		startOpusPlayIndex(general_RingtoneState - 1);
		ringtone_audio_handler_state++;

	} else if (ringtone_audio_handler_state > 0) {
		if (VarBlockSharedByDspAndMcu.NeedToStartPlayOpus == 2) {
			amp_post_event(AMP_EVT_STOP);
			general_RingtoneState = Ringtone_No;
			ringtone_audio_handler_state = 0;
			PRINTF("[Ringtone] Stop ringtone\r\n");
		}
	}
}

void ringtone_handler (void)
{
	ringtone_audio_handler();
}

void connect_handler (void)
{
#if !CES_DEMO
#if AUTO_CONNECT_ENABLE
        if(connection_timer_count < (CONNECTION_TIMER_TIMEOUT_MILLISECOND/CONNECTION_TIMER_TASK_DELAY) && (conn_rider_phone == NULL)){
            connection_timer_count++;
        }else{
            connection_timer_count = 0;
            if((g_pairedDeviceCount > 0) && (conn_rider_phone == NULL)){
#if BT_CONNECTION_LOG
                PRINTF("Connection timeout. Connect previous paired device\r\n");
#endif
                app_auto_connect_paired_devices();
            }
        }
#endif
#endif
}


static void app_task(void *pvParameters)
{
    while(1){
    	connect_handler();
    	ringtone_handler();

        vTaskDelay(pdMS_TO_TICKS(CONNECTION_TIMER_TASK_DELAY));
    }
}
#endif
