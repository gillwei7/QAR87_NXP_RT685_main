/*
 * ringtone_handler.h
 *
 *  Created on: Apr 4, 2026
 *      Author: Lydia
 */

#ifndef RINGTONE_HANDLER_H_
#define RINGTONE_HANDLER_H_

#include "hal_common.h"


typedef enum
{
    Ringtone_No  = 0,
    Ringtone_PowerON,
    Ringtone_PowerOFF,
    Ringtone_LowBattery,
    Ringtone_WiFi_Disconnected,
    Ringtone_BT_Disconnected,
    Ringtone_StartRecording,
    Ringtone_StopRecording,
    Ringtone_PhotoCapture,
    Ringtone_StartVideoAI,
    Ringtone_StopVideoAI,
    Ringtone_StartTranslation,
    Ringtone_StopTranslation,
    Ringtone_WakeWord,
	Ringtone_BT_Connected
} RingtoneState;

void startOpusPlayIndex(int opus_index);


#endif /* RINGTONE_HANDLER_H_ */
