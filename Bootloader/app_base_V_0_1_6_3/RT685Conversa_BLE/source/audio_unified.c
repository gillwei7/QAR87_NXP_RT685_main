/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include "GlobalDef.h"
#if EnableUsbComAndAudio==1

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_audio.h"

#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"

#include "UsbApp.h"
#include "fsl_debug_console.h"

#include "fsl_device_registers.h"
#include "clock_config.h"
#include "board.h"
#include <stdio.h>
#include <stdlib.h>

#include "SubFunc.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
extern void SCTIMER_CaptureInit(void);
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/
//USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
//uint8_t audioPlayDataBuff[AUDIO_SPEAKER_DATA_WHOLE_BUFFER_LENGTH * FS_ISO_OUT_ENDP_PACKET_SIZE];
U16 UsbUpStreamingStopMonitorCnt;
U16 UsbDnStreamingStopMonitorCnt;
U16 UsbUpStreamingIsStarted;
U16 UsbDnStreamingIsStarted;

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
	uint8_t audioPlayPacket[FS_ISO_OUT_ENDP_PACKET_SIZE];
#else
	USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
	uint8_t audioPlayPacket[(FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE)];
	USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
	uint8_t audioFeedBackBuffer[4];
#endif

extern usb_device_composite_struct_t *PtrUsbDevComposite;
extern usb_device_composite_struct_t g_composite;
//extern uint8_t s_wavBuff[];
/*******************************************************************************
 * Code
 ******************************************************************************/

int Evt_UsbPlug;
int Evt_UsbUnPlug;

/*!
 * @brief Audio class specific request function.
 *
 * This function handles the Audio class specific requests.
 *
 * @param handle           The USB device handle.
 * @param event            The USB device event type.
 * @param param            The parameter of the device specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
__attribute__((section("CodeQuickAccess")))
usb_status_t USB_DeviceAudioRequest(class_handle_t handle, uint32_t event, void *param)
{
    usb_device_control_request_struct_t *request = (usb_device_control_request_struct_t *)param;
    usb_status_t error                           = kStatus_USB_Success;
    uint16_t volume;
#if USB_DEVICE_CONFIG_AUDIO_CLASS_2_0
    uint8_t entityId;
#endif

    switch (event)
    {
        case USB_DEVICE_AUDIO_GET_CUR_MUTE_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.curMute;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curMute);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_VOLUME_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.curVolume;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curVolume);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_BASS_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.curBass;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curBass);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_MID_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.curMid;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curMid);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_TREBLE_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.curTreble;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curTreble);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_AUTOMATIC_GAIN_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.curAutomaticGain;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curAutomaticGain);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_DELAY_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.curDelay;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curDelay);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_SAMPLING_FREQ_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.curSamplingFrequency;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_GET_MIN_VOLUME_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.minVolume;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.minVolume);
            break;
        case USB_DEVICE_AUDIO_GET_MIN_BASS_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.minBass;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.minBass);
            break;
        case USB_DEVICE_AUDIO_GET_MIN_MID_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.minMid;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.minMid);
            break;
        case USB_DEVICE_AUDIO_GET_MIN_TREBLE_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.minTreble;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.minTreble);
            break;
        case USB_DEVICE_AUDIO_GET_MIN_DELAY_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.minDelay;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.minDelay);
            break;
        case USB_DEVICE_AUDIO_GET_MIN_SAMPLING_FREQ_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.minSamplingFrequency;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.minSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_GET_MAX_VOLUME_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.maxVolume;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.maxVolume);
            break;
        case USB_DEVICE_AUDIO_GET_MAX_BASS_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.maxBass;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.maxBass);
            break;
        case USB_DEVICE_AUDIO_GET_MAX_MID_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.maxMid;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.maxMid);
            break;
        case USB_DEVICE_AUDIO_GET_MAX_TREBLE_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.maxTreble;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.maxTreble);
            break;
        case USB_DEVICE_AUDIO_GET_MAX_DELAY_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.maxDelay;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.maxDelay);
            break;
        case USB_DEVICE_AUDIO_GET_MAX_SAMPLING_FREQ_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.maxSamplingFrequency;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.maxSamplingFrequency);
            break;
        case USB_DEVICE_AUDIO_GET_RES_VOLUME_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.resVolume;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.resVolume);
            break;
        case USB_DEVICE_AUDIO_GET_RES_BASS_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.resBass;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.resBass);
            break;
        case USB_DEVICE_AUDIO_GET_RES_MID_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.resMid;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.resMid);
            break;
        case USB_DEVICE_AUDIO_GET_RES_TREBLE_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.resTreble;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.resTreble);
            break;
        case USB_DEVICE_AUDIO_GET_RES_DELAY_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.resDelay;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.resDelay);
            break;
        case USB_DEVICE_AUDIO_GET_RES_SAMPLING_FREQ_CONTROL:
            request->buffer = PtrUsbDevComposite->audioUnified.resSamplingFrequency;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.resSamplingFrequency);
            break;
#if USB_DEVICE_CONFIG_AUDIO_CLASS_2_0
        case USB_DEVICE_AUDIO_GET_CUR_SAM_FREQ_CONTROL:
            entityId = (uint8_t)(request->setup->wIndex >> 0x08);
            if (entityId == USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID)
            {
                request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.curRecorderSampleFrequency;
                ;
                request->length = sizeof(PtrUsbDevComposite->audioUnified.curRecorderSampleFrequency);
            }
            else if (entityId == USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID)
            {
                request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.curSpeakerSampleFrequency;
                ;
                request->length = sizeof(PtrUsbDevComposite->audioUnified.curSpeakerSampleFrequency);
            }
            else
            {
                /* no action */
            }
            break;
        case USB_DEVICE_AUDIO_GET_RANGE_SAM_FREQ_CONTROL:
            entityId = (uint8_t)(request->setup->wIndex >> 0x08);
            if (entityId == USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID)
            {
                request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.freqRecorderControlRange;
                request->length = sizeof(PtrUsbDevComposite->audioUnified.freqRecorderControlRange);
            }
            else if (entityId == USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID)
            {
                request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.freqSpeakerControlRange;
                request->length = sizeof(PtrUsbDevComposite->audioUnified.freqSpeakerControlRange);
            }
            else
            {
            }
            break;
        case USB_DEVICE_AUDIO_GET_CUR_CLOCK_VALID_CONTROL:
            request->buffer = &PtrUsbDevComposite->audioUnified.curClockValid;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curClockValid);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_MUTE_CONTROL_AUDIO20:
            request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.curMute20;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curMute20);
            break;
        case USB_DEVICE_AUDIO_GET_CUR_VOLUME_CONTROL_AUDIO20:
            request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.curVolume20;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.curVolume20);
            break;
        case USB_DEVICE_AUDIO_GET_RANGE_VOLUME_CONTROL_AUDIO20:
            request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.volumeControlRange;
            request->length = sizeof(PtrUsbDevComposite->audioUnified.volumeControlRange);
            break;
#endif
        case USB_DEVICE_AUDIO_SET_CUR_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.curVolume;
            }
            else
            {
                volume = (uint16_t)((uint16_t)PtrUsbDevComposite->audioUnified.curVolume[1] << 8U);
                volume |= (uint8_t)(PtrUsbDevComposite->audioUnified.curVolume[0]);
                PtrUsbDevComposite->audioUnified.codecTask |= VOLUME_CHANGE_TASK;
            }

            int16_t  *VolPtr;
            VolPtr=(int16_t *)PtrUsbDevComposite->audioUnified.curVolume;

            UsbDnStreamGainL=(*VolPtr/17152.0f);	//0X4300 is the max volume value
            UsbDnStreamGainR=(*VolPtr/17152.0f);	//0X4300 is the max volume value
            break;
        case USB_DEVICE_AUDIO_SET_CUR_MUTE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curMute;
            }
            else
            {
                if (PtrUsbDevComposite->audioUnified.curMute)
                {
                    PtrUsbDevComposite->audioUnified.codecTask |= MUTE_CODEC_TASK;
                }
                else
                {
                    PtrUsbDevComposite->audioUnified.codecTask |= UNMUTE_CODEC_TASK;
                }
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curBass;
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curMid;
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curTreble;
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_AUTOMATIC_GAIN_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curAutomaticGain;
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.curDelay;
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.curSamplingFrequency;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MIN_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.minVolume;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MIN_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.minBass;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MIN_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.minMid;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MIN_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.minTreble;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MIN_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.minDelay;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MIN_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.minSamplingFrequency;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MAX_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.maxVolume;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MAX_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.maxBass;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MAX_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.maxMid;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MAX_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.maxTreble;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MAX_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.maxDelay;
            }
            break;
        case USB_DEVICE_AUDIO_SET_MAX_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.maxSamplingFrequency;
            }
            break;
        case USB_DEVICE_AUDIO_SET_RES_VOLUME_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.resVolume;
            }
            break;
        case USB_DEVICE_AUDIO_SET_RES_BASS_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.resBass;
            }
            break;
        case USB_DEVICE_AUDIO_SET_RES_MID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.resMid;
            }
            break;
        case USB_DEVICE_AUDIO_SET_RES_TREBLE_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.resTreble;
            }
            break;
        case USB_DEVICE_AUDIO_SET_RES_DELAY_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.resDelay;
            }
            break;
        case USB_DEVICE_AUDIO_SET_RES_SAMPLING_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.resSamplingFrequency;
            }
            break;
#if USB_DEVICE_CONFIG_AUDIO_CLASS_2_0
        case USB_DEVICE_AUDIO_SET_CUR_SAM_FREQ_CONTROL:
            if (request->isSetup == 1U)
            {
                entityId = (uint8_t)(request->setup->wIndex >> 0x08);
                if (entityId == USB_AUDIO_RECORDER_CONTROL_CLOCK_SOURCE_ID)
                {
                    request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.curRecorderSampleFrequency;
                }
                else if (entityId == USB_AUDIO_SPEAKER_CONTROL_CLOCK_SOURCE_ID)
                {
                    request->buffer = (uint8_t *)&PtrUsbDevComposite->audioUnified.curSpeakerSampleFrequency;
                }
                else
                {
                    /* no action */
                }
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_CLOCK_VALID_CONTROL:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curClockValid;
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_MUTE_CONTROL_AUDIO20:
            if (request->isSetup == 1U)
            {
                request->buffer = &PtrUsbDevComposite->audioUnified.curMute20;
            }
            else
            {
                if (PtrUsbDevComposite->audioUnified.curMute20)
                {
                    PtrUsbDevComposite->audioUnified.codecTask |= MUTE_CODEC_TASK;
                }
                else
                {
                    PtrUsbDevComposite->audioUnified.codecTask |= UNMUTE_CODEC_TASK;
                }
            }
            break;
        case USB_DEVICE_AUDIO_SET_CUR_VOLUME_CONTROL_AUDIO20:
            if (request->isSetup == 1U)
            {
                request->buffer = PtrUsbDevComposite->audioUnified.curVolume20;
            }
            else
            {
                PtrUsbDevComposite->audioUnified.codecTask |= VOLUME_CHANGE_TASK;
            }
            break;
#endif
        default:
            error = kStatus_USB_InvalidRequest;
            break;
    }
    return error;
}

int USB_AudioSpeakerBufferSpaceUsed(void)
{
	//to be complete later
	return 0;
}

#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else

//uint32_t USBAudio_FeedBackEp_feedbackValue=(AUDIO_OUT_SAMPLING_RATE_KHZ*1024);
uint32_t USBAudio_FeedBackEp_feedbackValue=AUDIO_SAMPLING_RATE_1_TO_16_16;

int UacDnAOD_ForFbAdjust;

/* The USB_AudioFeedbackDataUpdate() function calculates the feedback data */
__attribute__((section("CodeQuickAccess")))
void USB_AudioFeedbackDataUpdate(void)
{
    //static int32_t audioSpeakerUsedDiff = 0x0, audioSpeakerDiffThres = 0x0;
    //static uint32_t feedbackValue = 0x0, originFeedbackValue = 0x0, UacDnAOD_ForFbAdjust = 0x0, audioSpeakerLastUsedSpace = 0x0;


    /* feedback interval is AUDIO_CALCULATE_Ff_INTERVAL */
    if (USB_SPEED_HIGH == PtrUsbDevComposite->speed)
    {
		#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0) /* high speed, feedback interval is AUDIO_CALCULATE_Ff_INTERVAL ms */
			if (PtrUsbDevComposite->audioUnified.speakerIntervalCount !=
				AUDIO_CALCULATE_Ff_INTERVAL * (AUDIO_OUT_TRANSFER_LENGTH_ONE_FRAME / HS_ISO_OUT_ENDP_PACKET_SIZE))
		#else
			if (PtrUsbDevComposite->audioUnified.speakerIntervalCount != AUDIO_CALCULATE_Ff_INTERVAL)
		#endif
        {
				PtrUsbDevComposite->audioUnified.speakerIntervalCount++;
            return;
        }
    }
    else /* full speed, feedback interval is AUDIO_CALCULATE_Ff_INTERVAL ms */
    {
        if (PtrUsbDevComposite->audioUnified.speakerIntervalCount != AUDIO_CALCULATE_Ff_INTERVAL)
        {
        	PtrUsbDevComposite->audioUnified.speakerIntervalCount++;
            return;
        }
    }

    PtrUsbDevComposite->audioUnified.speakerIntervalCount = 1;
	PtrUsbDevComposite->audioUnified.timesFeedbackCalculate++;
	#if 0
		//SDK example original
		if (PtrUsbDevComposite->audioUnified.timesFeedbackCalculate == 2)
		{
			originFeedbackValue = ((PtrUsbDevComposite->audioUnified.audioSendCount -
									PtrUsbDevComposite->audioUnified.lastAudioSendCount)
								   << 4) /
								  (AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE);
			feedbackValue = originFeedbackValue;
			AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, originFeedbackValue);
			UacDnAOD_ForFbAdjust     = USB_AudioSpeakerBufferSpaceUsed();
			audioSpeakerLastUsedSpace = UacDnAOD_ForFbAdjust;
		}
		else if (PtrUsbDevComposite->audioUnified.timesFeedbackCalculate > 2)
		{
			UacDnAOD_ForFbAdjust = USB_AudioSpeakerBufferSpaceUsed();
			audioSpeakerUsedDiff += (UacDnAOD_ForFbAdjust - audioSpeakerLastUsedSpace);
			audioSpeakerLastUsedSpace = UacDnAOD_ForFbAdjust;

			if ((audioSpeakerUsedDiff > -AUDIO_OUT_SAMPLING_RATE_KHZ) && (audioSpeakerUsedDiff < AUDIO_OUT_SAMPLING_RATE_KHZ))
			{
				audioSpeakerDiffThres = 4 * AUDIO_OUT_SAMPLING_RATE_KHZ;
			}
			if (audioSpeakerUsedDiff <= -audioSpeakerDiffThres)
			{
				audioSpeakerDiffThres += 4 * AUDIO_OUT_SAMPLING_RATE_KHZ;
	//            feedbackValue += (AUDIO_OUT_SAMPLING_RATE_KHZ / AUDIO_SAMPLING_RATE_16KHZ) * (AUDIO_ADJUST_MIN_STEP);
			}
			if (audioSpeakerUsedDiff >= audioSpeakerDiffThres)
			{
				audioSpeakerDiffThres += 4 * AUDIO_OUT_SAMPLING_RATE_KHZ;
	//            feedbackValue -= (AUDIO_OUT_SAMPLING_RATE_KHZ / AUDIO_SAMPLING_RATE_16KHZ) * (AUDIO_ADJUST_MIN_STEP);
			}
			AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, feedbackValue);
		}
		else
		{
		}
	#else
		if (PtrUsbDevComposite->audioUnified.timesFeedbackCalculate < 20)
		{
			#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
				if (USB_SPEED_HIGH == g_composite.speed)
				{
					AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_1_TO_16_16);
				}
				if (USB_SPEED_FULL == g_composite.speed)
				{
					AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_1_TO_10_14);
				}
			#else
				AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_1_TO_10_14);
			#endif

		}else
		{
			int Delta;
			//UacDnAOD_ForFbAdjust = CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh);		//reading AOD is moved to the main audio flow frame proc --- AOD taken from there is better
			Delta=UacDnAOD_ForFbAdjust-AUDIO_SPEAKER_UsbDnBufCenterLevelInSamples;

			#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
				if (USB_SPEED_HIGH == g_composite.speed)
				{
					USBAudio_FeedBackEp_feedbackValue = AUDIO_SAMPLING_RATE_1_TO_16_16-2*Delta;
					AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, USBAudio_FeedBackEp_feedbackValue);
				}
				if (USB_SPEED_FULL == g_composite.speed)
				{
					USBAudio_FeedBackEp_feedbackValue = AUDIO_SAMPLING_RATE_1_TO_10_14-2*Delta;
					AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, USBAudio_FeedBackEp_feedbackValue);
				}
			#else
				USBAudio_FeedBackEp_feedbackValue = AUDIO_SAMPLING_RATE_1_TO_10_14-Delta;
				AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, USBAudio_FeedBackEp_feedbackValue);
			#endif

			VarBlockSharedByDspAndMcu.MonitorInfoArray1[11]=Delta;
		}
    #endif
    PtrUsbDevComposite->audioUnified.lastAudioSendCount = PtrUsbDevComposite->audioUnified.audioSendCount;
}
#endif

U16 MicUpStreamDataRateAdjustCnt=0;
int UsbAudioUpstreamLengthAdjusted;
/*
Note: when not in a call, local I2S Fs is at the default PLL freq. After a call, local I2S follows BT. So, the UAC upstream buf AOD changes at different rate
when in a call and not in a call.

It is important that the adjusted packet length should only be used in the USB callback once. Other wise, the adjusting will be too much and over tuned.
Also, the frequency of really calling this function(no skipping) should not be too slow. In this demo, each time call this function is 8ms, the skip count should control that the really
tuning is about 30ms each time. If the skip count makes 300ms each real call, this is too slow.
*/
__attribute__((section("CodeQuickAccess")))
void USB_MicUpStreamDataRateControl_AdjustPacketLength(int AodInCirBuf)//amount of free space
{
    U32 PacketSize;
    U16 HiLimitInSample, LoLimitInSample;

	#if HS_ISO_IN_ENDP_INTERVAL==1
		PacketSize = AUDIO_IN_SAMPLING_RATE_KHZ/8;	    //no increase no decrease
		if(MicUpStreamDataRateAdjustCnt++%8)
		{
		    UsbAudioUpstreamLengthAdjusted=PacketSize;
			return;
		}
	#endif
	#if HS_ISO_IN_ENDP_INTERVAL==2
		PacketSize = AUDIO_IN_SAMPLING_RATE_KHZ/4;	    //no increase no decrease
		if(MicUpStreamDataRateAdjustCnt++%4)
		{
		   UsbAudioUpstreamLengthAdjusted=PacketSize;
			return;
		}
	#endif
	#if HS_ISO_IN_ENDP_INTERVAL==3
		PacketSize = AUDIO_IN_SAMPLING_RATE_KHZ/2;	    //no increase no decrease
		if(MicUpStreamDataRateAdjustCnt++%2)
		{
		    UsbAudioUpstreamLengthAdjusted=PacketSize;
			return;
		}
	#endif
	#if HS_ISO_IN_ENDP_INTERVAL==4
		PacketSize = AUDIO_IN_SAMPLING_RATE_KHZ;	    //no increase no decrease
		//if(MicUpStreamDataRateAdjustCnt++%10)
		//{
		    UsbAudioUpstreamLengthAdjusted=PacketSize;
		//	return;
		//}
	#endif


	HiLimitInSample=(UacUpBuf_LengthInSamples-AudioFrameSizeInSamplePerCh_16KHz)*3/4+AudioFrameSizeInSamplePerCh_16KHz;
	LoLimitInSample=(UacUpBuf_LengthInSamples-AudioFrameSizeInSamplePerCh_16KHz)*1/4+AudioFrameSizeInSamplePerCh_16KHz;

	//if don't adjust packet length, the buffer does over/underflow --- this is proved !!!
	//check samples in the buffer --- and do adjust packet size, one more or one less ---- should be used when USB upstream is in async mode
	if (AodInCirBuf > HiLimitInSample)
	{	//mic audio data is going to be too much
		PacketSize++;		//cir buffer data gets more and more, should let UAC take away 1 more sample
		#if EnableUacCirBufUnderflowOverFlowPrint==1
			PRINTF("+\r\n");
		#endif
	}
	else if (AodInCirBuf >= LoLimitInSample)
	{	//mic audio data is not too many nor too little
		#if EnableUacCirBufUnderflowOverFlowPrint==1
			//PRINTF("x\r\n");
		#endif
	}else if(AodInCirBuf >= AudioFrameSizeInSamplePerCh_16KHz)
	{	//mic audio data is going to be not enough
		PacketSize--;		//cir buffer data gets less and less, should let UAC take away 1 less sample
		#if EnableUacCirBufUnderflowOverFlowPrint==1
			PRINTF("-\r\n");
		#endif
	}else
	{
		PacketSize=0;
		#if EnableUacCirBufUnderflowOverFlowPrint==1
			PRINTF("0\r\n");
		#endif
	}
    UsbAudioUpstreamLengthAdjusted=PacketSize;
}
/*!
 * @brief device Audio callback function.
 *
 * This function handle the Audio class specified event.
 * @param handle          The USB class  handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the class specific event.
 * @return kStatus_USB_Success or error.
 */
__attribute__((section("CodeQuickAccess")))
usb_status_t USB_DeviceAudioCompositeCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    usb_device_endpoint_callback_message_struct_t *ep_cb_param;
    ep_cb_param = (usb_device_endpoint_callback_message_struct_t *)param;

    //PRINTF("0\n");
    switch (event)
    {
        case kUSB_DeviceAudioEventStreamSendResponse:
            //PRINTF("r\n");
            if ((PtrUsbDevComposite->audioUnified.attach) && (ep_cb_param->length != (USB_UNINITIALIZED_VAL_32)))
            {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                if (0)
                {
                }
#else
                if (ep_cb_param->length == ((USB_SPEED_HIGH == g_composite.speed) ? HS_ISO_FEEDBACK_ENDP_PACKET_SIZE :
                                                                                    FS_ISO_FEEDBACK_ENDP_PACKET_SIZE))
                {
                    error =
                        USB_DeviceAudioSend(PtrUsbDevComposite->audioUnified.audioSpeakerHandle,
                                            USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT, audioFeedBackBuffer,
                                            (USB_SPEED_HIGH == g_composite.speed) ? HS_ISO_FEEDBACK_ENDP_PACKET_SIZE :
                                                                                    FS_ISO_FEEDBACK_ENDP_PACKET_SIZE);
                    VarBlockSharedByDspAndMcu.MonitorInfoArray1[11]++;
                }
#endif
                else
                {
                	//LedPin4Up();
                	//take out samples from UacUpAudioBuf_MCh, and put to Usb audio tx packet to send to USB host
                	USB_AUDIO_ENTER_CRITICAL();
					//adjust mic upstream data rate once every 15 packets --- adjust packet size, one sample more ore one sample less
					if(UsbAudioUpstreamLengthAdjusted)
					{
						//upstream LR mixed, 2 mic ch
						T_MCh32BitUacUpAudioSample *TmpMultiChPtr=CirUacUpAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacUpAudioBuf_MCh,UsbAudioUpstreamLengthAdjusted);
						USB_DeviceAudioSend(PtrUsbDevComposite->audioUnified.audioRecorderHandle,
											USB_AUDIO_RECORDER_STREAM_ENDPOINT,
											//s_wavBuff,
											(U8 *)TmpMultiChPtr,
											UsbAudioUpstreamLengthAdjusted*AUDIO_IN_FORMAT_CHANNELS*AUDIO_IN_FORMAT_SIZE);

						//following part: let non standard packet length only be used once after it is changed
						#if HS_ISO_IN_ENDP_INTERVAL==1
							int StandardPacketSize = AUDIO_IN_SAMPLING_RATE_KHZ/8;
						#endif
						#if HS_ISO_IN_ENDP_INTERVAL==2
							int StandardPacketSize = AUDIO_IN_SAMPLING_RATE_KHZ/4;
						#endif
						#if HS_ISO_IN_ENDP_INTERVAL==3
							int StandardPacketSize = AUDIO_IN_SAMPLING_RATE_KHZ/2;
						#endif
						#if HS_ISO_IN_ENDP_INTERVAL==4
							int StandardPacketSize = AUDIO_IN_SAMPLING_RATE_KHZ;
						#endif
						if(StandardPacketSize!=UsbAudioUpstreamLengthAdjusted)
							UsbAudioUpstreamLengthAdjusted=StandardPacketSize;
					}else
					{
						//not enough data available, upstream buffer is empty or almost empty --- should never happen
						#if EnableUacCirBufUnderflowOverFlowPrint==1
							PRINTF("UacUp E\r\n");
						#endif
						error =
							USB_DeviceAudioSend(PtrUsbDevComposite->audioUnified.audioRecorderHandle,
												USB_AUDIO_RECORDER_STREAM_ENDPOINT,
												//s_wavBuff,
												(U8 *)AllZeroBuf_48PointsSingleCh_16Bit,
												g_composite.speed==USB_SPEED_FULL? FS_ISO_IN_ENDP_PACKET_SIZE:HS_ISO_IN_ENDP_PACKET_SIZE
												);		//HS_ISO_IN_ENDP_PACKET_SIZE is the same value as FS_ISO_IN_ENDP_PACKET_SIZE
					}
					UsbUpStreamingIsStarted=1;
					UsbUpStreamingStopMonitorCnt=0;
					USB_AUDIO_EXIT_CRITICAL();
                	//LedPin4Dn();
                }
            }
            break;
        case kUSB_DeviceAudioEventStreamRecvResponse:
            //PRINTF("t\n");
            if ((PtrUsbDevComposite->audioUnified.attach) && (ep_cb_param->length != (USB_UNINITIALIZED_VAL_32)))
            {
            	//LedPin1Up();
            	USB_AUDIO_ENTER_CRITICAL();

                if (PtrUsbDevComposite->audioUnified.startPlay == 0)
                {
                    PtrUsbDevComposite->audioUnified.startPlay = 1;
                }

				//half full means: after taking away 1 frame audio from the DN buffer, memaining audio data is >= half of the size of (total buffer size - 1 frame amount)
				if ((CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= UacDnBuf_CenterLevelInSamples) && (PtrUsbDevComposite->audioUnified.startPlayHalfFull == 0))
				//if ((CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= (UacDnBuf_LengthInSamples-AudioFrameSizeInSamplePerCh_16KHz)/2+AUDIO_OUT_SAMPLING_RATE_KHZ) && (PtrUsbDevComposite->audioUnified.startPlayHalfFull == 0))
				{
					PtrUsbDevComposite->audioUnified.startPlayHalfFull = 1;
				}


                PtrUsbDevComposite->audioUnified.usbRecvCount += ep_cb_param->length;
                PtrUsbDevComposite->audioUnified.usbRecvTimes++;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
                error = USB_DeviceAudioRecv(handle, USB_AUDIO_SPEAKER_STREAM_ENDPOINT, &audioPlayPacket[0],
                		(g_composite.speed==USB_SPEED_FULL? FS_ISO_OUT_ENDP_PACKET_SIZE:HS_ISO_OUT_ENDP_PACKET_SIZE)
                		                     );
#else
                //USB_AUDIO_ENTER_CRITICAL();
                USB_AudioFeedbackDataUpdate();
                //USB_AUDIO_EXIT_CRITICAL();
                //error = USB_DeviceAudioRecv(handle, USB_AUDIO_SPEAKER_STREAM_ENDPOINT, &audioPlayPacket[0], (FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE));
    			if (USB_SPEED_HIGH == PtrUsbDevComposite->speed)
    			{
    				error = USB_DeviceAudioRecv(handle, USB_AUDIO_SPEAKER_STREAM_ENDPOINT, &audioPlayPacket[0], (HS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE));
    			}
    			if (USB_SPEED_FULL == PtrUsbDevComposite->speed)
    			{
    				error = USB_DeviceAudioRecv(handle, USB_AUDIO_SPEAKER_STREAM_ENDPOINT, &audioPlayPacket[0], (FS_ISO_OUT_ENDP_PACKET_SIZE + AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE));
    			}
#endif

				if(CirUacDnAudioBuf_SpaceAvailableInSamples_MultiCh(&UacDnAudioBuf_MCh) > ep_cb_param->length/(AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE))
				{
					if(PtrUsbDevComposite->audioUnified.startPlayHalfFull == 1)
					{
						//half full play started --- copy real downstream audio in the data packet to the downstreaming (feedback calculation) buffer
						CirUacDnAudioBuf_WriteSamples_MultiCh(&UacDnAudioBuf_MCh, ep_cb_param->length/(AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE), (T_MCh32BitUacDnAudioSample*)audioPlayPacket);
					}else
					{
						//half full play not started --- put 0 to buffer, this is to prevent dirty sampels from the host, seems there are dirty samples from the end of last time host playing
						CirUacDnAudioBuf_WriteSamples_MultiCh(&UacDnAudioBuf_MCh, ep_cb_param->length/(AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE),
								(T_MCh32BitUacDnAudioSample*)AllZeroBuf_48PointsSingleCh_16Bit
								);	//this is to prevent play/pause click
					}
				}else
				{
					#if EnableUacCirBufUnderflowOverFlowPrint==1
						//PRINTF("UacDn F\r\n");
					#endif
				}
				UsbDnStreamingIsStarted=1;
				UsbDnStreamingStopMonitorCnt=0;
        		USB_AUDIO_EXIT_CRITICAL();
            	//LedPin1Dn();
            }
            break;

        default:
            if (param && (event > 0xFF))
            {
                error = USB_DeviceAudioRequest(handle, event, param);
            }
            break;
    }

    return error;
}

/* The USB_DeviceAudioSpeakerStatusReset() function resets the audio speaker status to the initialized status */
__attribute__((section("CodeQuickAccess")))
void USB_DeviceAudioSpeakerStatusReset(void)
{
    PtrUsbDevComposite->audioUnified.startPlay              = 0;
    PtrUsbDevComposite->audioUnified.startPlayHalfFull      = 0;
    PtrUsbDevComposite->audioUnified.tdReadNumberPlay       = 0;
    PtrUsbDevComposite->audioUnified.tdWriteNumberPlay      = 0;
    PtrUsbDevComposite->audioUnified.audioSendCount         = 0;
    PtrUsbDevComposite->audioUnified.usbRecvCount           = 0;
    PtrUsbDevComposite->audioUnified.lastAudioSendCount     = 0;
    PtrUsbDevComposite->audioUnified.audioSendTimes         = 0;
    PtrUsbDevComposite->audioUnified.usbRecvTimes           = 0;
    PtrUsbDevComposite->audioUnified.speakerIntervalCount   = 0;
    PtrUsbDevComposite->audioUnified.speakerReservedSpace   = 0;
    PtrUsbDevComposite->audioUnified.timesFeedbackCalculate = 0;
    PtrUsbDevComposite->audioUnified.speakerDetachOrNoInput = 0;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	PtrUsbDevComposite->audioUnified.audioPllTicksPrev   = 0;
	PtrUsbDevComposite->audioUnified.audioPllTicksDiff   = 0;
	PtrUsbDevComposite->audioUnified.audioPllTicksEma    = AUDIO_PLL_USB1_SOF_INTERVAL_COUNT;
	PtrUsbDevComposite->audioUnified.audioPllTickEmaFrac = 0;
	PtrUsbDevComposite->audioUnified.audioPllStep        = AUDIO_PLL_FRACTIONAL_CHANGE_STEP;
#endif
    UsbDnStreamGainPrevValueL=0.0f;
    UsbDnStreamGainPrevValueR=0.0f;
}

/*!
 * @brief Audio set configuration function.
 *
 * @param handle The Audio class handle.
 * @param configure The Audio class configure index.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
__attribute__((section("CodeQuickAccess")))
usb_status_t USB_DeviceAudioCompositeSetConfigure(class_handle_t handle, uint8_t configure)
{
    if (USB_COMPOSITE_CONFIGURE_INDEX == configure)
    {
        PtrUsbDevComposite->audioUnified.attach = 1U;
        Evt_UsbPlug=1;
    }
    return kStatus_USB_Success;
}

__attribute__((section("CodeQuickAccess")))
usb_status_t USB_DeviceAudioRecorderSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    usb_status_t error = kStatus_USB_Success;
    USB_AUDIO_ENTER_CRITICAL();

	//LedPin3Up();LedPin1Up();

    if (alternateSetting == 1U)
    {

    	//UsbUpStreamingIsStarted=1;			//this is moved to usb packet receive event processing, only when up stream packet is received, set this to 1
		CirUacUpAudioBuf_ClearAllSamples_MultiCh(&UacUpAudioBuf_MCh);
        error = USB_DeviceAudioSend(PtrUsbDevComposite->audioUnified.audioRecorderHandle,
                                    USB_AUDIO_RECORDER_STREAM_ENDPOINT,
									//s_wavBuff,
									(U8 *)AllZeroBuf_48PointsSingleCh_16Bit,
									g_composite.speed==USB_SPEED_FULL? FS_ISO_IN_ENDP_PACKET_SIZE:HS_ISO_IN_ENDP_PACKET_SIZE
									);
        //PRINTF("n\n");
    }else
    {
    	//UsbUpStreamingIsStarted=0;		//this is moved to audio main processing, when usb up streaming packet is lost for 40 frames, clear this
		CirUacUpAudioBuf_ClearAllSamples_MultiCh(&UacUpAudioBuf_MCh);
		#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=0;
		#endif
        //PRINTF("f\n");
    }

	//UsbUpStreamingStopMonitorCnt=0;
    UsbAudioUpstreamLengthAdjusted=0;
	//LedPin3Dn();LedPin1Dn();
    USB_AUDIO_EXIT_CRITICAL();
    return error;
}

__attribute__((section("CodeQuickAccess")))
usb_status_t USB_DeviceAudioSpeakerSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    usb_status_t error = kStatus_USB_Success;
    USB_AUDIO_ENTER_CRITICAL();

	//LedPin1Up();
    if (alternateSetting == 1U)
    {
        USB_DeviceAudioSpeakerStatusReset();
    	//UsbDnStreamingIsStarted=1;			//this is moved to usb packet receive event processing, only when dn stream packet is received, set this to 1
        error =
            USB_DeviceAudioRecv(PtrUsbDevComposite->audioUnified.audioSpeakerHandle,
                                USB_AUDIO_SPEAKER_STREAM_ENDPOINT, 
								//&audioPlayDataBuff[0], 
								(uint8_t *)UacDnAudioBuf_MCh.PtrWr,
								(g_composite.speed==USB_SPEED_FULL? FS_ISO_OUT_ENDP_PACKET_SIZE:HS_ISO_OUT_ENDP_PACKET_SIZE)
								);
        if (error != kStatus_USB_Success)
        {
            return error;
        }
        else
        {
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#else
            error = USB_DeviceAudioSend(PtrUsbDevComposite->audioUnified.audioSpeakerHandle,
                                        USB_AUDIO_SPEAKER_FEEDBACK_ENDPOINT, audioFeedBackBuffer,
                                        (USB_SPEED_HIGH == g_composite.speed) ? HS_ISO_FEEDBACK_ENDP_PACKET_SIZE :
                                                                                FS_ISO_FEEDBACK_ENDP_PACKET_SIZE);
#endif
        }
    }else
    {
    	//UsbDnStreamingIsStarted=0;		//this is moved to audio main processing, when usb dn streaming packet is lost for 40 frames, clear this
        USB_DeviceAudioSpeakerStatusReset();

        CirUacDnAudioBuf_ClearAllSamples_MultiCh(&UacDnAudioBuf_MCh);
		#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[9]=0;
		#endif
    }

    PtrUsbDevComposite->audioUnified.timesFeedbackCalculate = 0;

	//LedPin1Dn();
    USB_AUDIO_EXIT_CRITICAL();
    return error;
}
/*!
 * @brief Audio init function.
 *
 * This function initializes the device with the composite device class information.
 *
 * @param device_composite          The pointer to the composite device structure.
 * @return kStatus_USB_Success .
 */
usb_status_t USB_DeviceAudioCompositeInit(usb_device_composite_struct_t *device_composite)
{
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	SCTIMER_CaptureInit();
#else
    AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_1_TO_16_16);
#endif
    PtrUsbDevComposite                                       = device_composite;
    PtrUsbDevComposite->audioUnified.copyProtect             = 0x01U;
    PtrUsbDevComposite->audioUnified.curMute                 = 0x00U;
    PtrUsbDevComposite->audioUnified.curVolume[0]            = 0x00U;
    PtrUsbDevComposite->audioUnified.curVolume[1]            = 0x1fU;
    PtrUsbDevComposite->audioUnified.minVolume[0]            = 0x00U;
    PtrUsbDevComposite->audioUnified.minVolume[1]            = 0x00U;
    PtrUsbDevComposite->audioUnified.maxVolume[0]            = 0x00U;
    PtrUsbDevComposite->audioUnified.maxVolume[1]            = 0X43U;
    PtrUsbDevComposite->audioUnified.resVolume[0]            = 0x01U;
    PtrUsbDevComposite->audioUnified.resVolume[1]            = 0x00U;
    PtrUsbDevComposite->audioUnified.curBass                 = 0x00U;
    PtrUsbDevComposite->audioUnified.curBass                 = 0x00U;
    PtrUsbDevComposite->audioUnified.minBass                 = 0x80U;
    PtrUsbDevComposite->audioUnified.maxBass                 = 0x7FU;
    PtrUsbDevComposite->audioUnified.resBass                 = 0x01U;
    PtrUsbDevComposite->audioUnified.curMid                  = 0x00U;
    PtrUsbDevComposite->audioUnified.minMid                  = 0x80U;
    PtrUsbDevComposite->audioUnified.maxMid                  = 0x7FU;
    PtrUsbDevComposite->audioUnified.resMid                  = 0x01U;
    PtrUsbDevComposite->audioUnified.curTreble               = 0x01U;
    PtrUsbDevComposite->audioUnified.minTreble               = 0x80U;
    PtrUsbDevComposite->audioUnified.maxTreble               = 0x7FU;
    PtrUsbDevComposite->audioUnified.resTreble               = 0x01U;
    PtrUsbDevComposite->audioUnified.curAutomaticGain        = 0x01U;
    PtrUsbDevComposite->audioUnified.curDelay[0]             = 0x00U;
    PtrUsbDevComposite->audioUnified.curDelay[1]             = 0x40U;
    PtrUsbDevComposite->audioUnified.minDelay[0]             = 0x00U;
    PtrUsbDevComposite->audioUnified.minDelay[1]             = 0x00U;
    PtrUsbDevComposite->audioUnified.maxDelay[0]             = 0xFFU;
    PtrUsbDevComposite->audioUnified.maxDelay[1]             = 0xFFU;
    PtrUsbDevComposite->audioUnified.resDelay[0]             = 0x00U;
    PtrUsbDevComposite->audioUnified.resDelay[1]             = 0x01U;
    PtrUsbDevComposite->audioUnified.curLoudness             = 0x01U;
    PtrUsbDevComposite->audioUnified.curSamplingFrequency[0] = 0x00U;
    PtrUsbDevComposite->audioUnified.curSamplingFrequency[1] = 0x00U;
    PtrUsbDevComposite->audioUnified.curSamplingFrequency[2] = 0x01U;
    PtrUsbDevComposite->audioUnified.minSamplingFrequency[0] = 0x00U;
    PtrUsbDevComposite->audioUnified.minSamplingFrequency[1] = 0x00U;
    PtrUsbDevComposite->audioUnified.minSamplingFrequency[2] = 0x01U;
    PtrUsbDevComposite->audioUnified.maxSamplingFrequency[0] = 0x00U;
    PtrUsbDevComposite->audioUnified.maxSamplingFrequency[1] = 0x00U;
    PtrUsbDevComposite->audioUnified.maxSamplingFrequency[2] = 0x01U;
    PtrUsbDevComposite->audioUnified.resSamplingFrequency[0] = 0x00U;
    PtrUsbDevComposite->audioUnified.resSamplingFrequency[1] = 0x00U;
    PtrUsbDevComposite->audioUnified.resSamplingFrequency[2] = 0x01U;
#if USB_DEVICE_CONFIG_AUDIO_CLASS_2_0
    PtrUsbDevComposite->audioUnified.curMute20                  = 0U;
    PtrUsbDevComposite->audioUnified.curClockValid              = 1U;
    PtrUsbDevComposite->audioUnified.curVolume20[0]             = 0x00U;
    PtrUsbDevComposite->audioUnified.curVolume20[1]             = 0x1FU;

    PtrUsbDevComposite->audioUnified.curSpeakerSampleFrequency             = (AUDIO_OUT_SAMPLING_RATE_KHZ*1000);
    PtrUsbDevComposite->audioUnified.freqSpeakerControlRange.wNumSubRanges = 1U;
    PtrUsbDevComposite->audioUnified.freqSpeakerControlRange.wMIN          = (AUDIO_OUT_SAMPLING_RATE_KHZ*1000);
    PtrUsbDevComposite->audioUnified.freqSpeakerControlRange.wMAX          = (AUDIO_OUT_SAMPLING_RATE_KHZ*1000);
    PtrUsbDevComposite->audioUnified.freqSpeakerControlRange.wRES          = 0U;

    PtrUsbDevComposite->audioUnified.curRecorderSampleFrequency             = (AUDIO_IN_SAMPLING_RATE_KHZ*1000);
    PtrUsbDevComposite->audioUnified.freqRecorderControlRange.wNumSubRanges = 1U;
    PtrUsbDevComposite->audioUnified.freqRecorderControlRange.wMIN          = (AUDIO_IN_SAMPLING_RATE_KHZ*1000);
    PtrUsbDevComposite->audioUnified.freqRecorderControlRange.wMAX          = (AUDIO_IN_SAMPLING_RATE_KHZ*1000);
    PtrUsbDevComposite->audioUnified.freqRecorderControlRange.wRES          = 0U;

    PtrUsbDevComposite->audioUnified.volumeControlRange.wNumSubRanges = 1U;
    PtrUsbDevComposite->audioUnified.volumeControlRange.wMIN          = 0x8001U;
    PtrUsbDevComposite->audioUnified.volumeControlRange.wMAX          = 0x7FFFU;
    PtrUsbDevComposite->audioUnified.volumeControlRange.wRES          = 1U;

#endif
    PtrUsbDevComposite->audioUnified.tdReadNumberPlay       = 0;
    PtrUsbDevComposite->audioUnified.tdWriteNumberPlay      = 0;
    PtrUsbDevComposite->audioUnified.audioSendCount         = 0;
    PtrUsbDevComposite->audioUnified.lastAudioSendCount     = 0;
    PtrUsbDevComposite->audioUnified.usbRecvCount           = 0;
    PtrUsbDevComposite->audioUnified.audioSendTimes         = 0;
    PtrUsbDevComposite->audioUnified.usbRecvTimes           = 0;
    PtrUsbDevComposite->audioUnified.startPlay              = 0;
    PtrUsbDevComposite->audioUnified.startPlayHalfFull      = 0;
    PtrUsbDevComposite->audioUnified.speakerIntervalCount   = 0;
    PtrUsbDevComposite->audioUnified.speakerReservedSpace   = 0;
    PtrUsbDevComposite->audioUnified.timesFeedbackCalculate = 0;
    PtrUsbDevComposite->audioUnified.speakerDetachOrNoInput = 0;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	PtrUsbDevComposite->audioUnified.audioPllTicksPrev   = 0;
	PtrUsbDevComposite->audioUnified.audioPllTicksDiff   = 0;
	PtrUsbDevComposite->audioUnified.audioPllTicksEma    = AUDIO_PLL_USB1_SOF_INTERVAL_COUNT;
	PtrUsbDevComposite->audioUnified.audioPllTickEmaFrac = 0;
	PtrUsbDevComposite->audioUnified.audioPllStep        = AUDIO_PLL_FRACTIONAL_CHANGE_STEP;
#endif

	UsbUpStreamingStopMonitorCnt=0;
	UsbDnStreamingStopMonitorCnt=0;

    return kStatus_USB_Success;
}

void USB_AudioSpeakerResetTask(void)
{
    if (PtrUsbDevComposite->audioUnified.speakerDetachOrNoInput)
    {
        USB_DeviceAudioSpeakerStatusReset();
    }
}

#endif

