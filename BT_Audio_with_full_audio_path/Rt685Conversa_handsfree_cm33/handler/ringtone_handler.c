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
#include "system_status.h"
#include "WorkStateManager.h"
#include "DefForBothMcuAndDsp.h"
#include "WorkStateManager.h"
#include "MainAudioFlow.h"


static RingtoneState general_RingtoneState = Ringtone_No;

static uint16_t ringtone_audio_handler_state = 0;
static uint8_t ringtone_is_playing = 0;



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

uint8_t is_playing_ringtone (void)
{
	return ringtone_is_playing;
}

void set_ringtone_state (RingtoneState state)
{
	general_RingtoneState = state;

	if (get_amp_status() == AMP_STATUS_OFF && general_RingtoneState != Ringtone_No) {
		ringtone_audio_set_state();
		ringtone_is_playing = 1;

	} else if (general_RingtoneState != Ringtone_No) {
		startOpusPlayIndex(general_RingtoneState - 1);
		general_RingtoneState = Ringtone_No;
		ringtone_is_playing = 1;

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
		if (VarBlockSharedByDspAndMcu.NeedToStartPlayOpus == 2 || ringtone_audio_handler_state > 2000) {
			// TODO: Check AMP disable conditions based on the active scenario
			amp_post_event(AMP_EVT_STOP);
			general_RingtoneState = Ringtone_No;
			ringtone_audio_handler_state = 0;
			PRINTF("[Ringtone] Stop ringtone\r\n");
		} else {
			ringtone_audio_handler_state++;
		}
	}
	if (ringtone_is_playing && VarBlockSharedByDspAndMcu.NeedToStartPlayOpus == 2) {
		ringtone_is_playing = 0;
	}
}

void ringtone_handler (void)
{
	ringtone_audio_handler();
}

static void app_task(void *pvParameters)
{
    while(1){
    	connect_handler();
    	ringtone_handler();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif
