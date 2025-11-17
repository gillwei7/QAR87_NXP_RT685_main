/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __USB_AUDIO_H__
#define __USB_AUDIO_H__ 1

#if EnableUsbComAndAudio==1
/*******************************************************************************
* Definitions
******************************************************************************/


#define USB_AUDIO_ENTER_CRITICAL()	\
									\
    OSA_SR_ALLOC();            		\
    OSA_ENTER_CRITICAL()

#define USB_AUDIO_EXIT_CRITICAL() OSA_EXIT_CRITICAL()



#define UacUpBuf_LengthInMs 					(6 * 2)									//12ms
#define UacUpBuf_LengthInSamples                (UacUpBuf_LengthInMs * AUDIO_IN_SAMPLING_RATE_KHZ)
#define UacUpBuf_MaxReadLengthInSamples			(1*AUDIO_IN_SAMPLING_RATE_KHZ)									//USB audio event will read this buffer, and the read size is 1 ms time audio at most (could be 0.5ms, 0.25ms, 0.125ms)

//NOTE: when I2S freq is 16KHz, framesize is 128, do need UacDnBuf_LengthInMs to be 12*2, otherwise feedback adj makes noise, because adjust is not smooth enough ???
//NOTE: when I2S freq is 48KHz, framesize is 48, UacDnBuf_LengthInMs can be be 6*2, and feedback adj makes NO noise, because adjust is smooth enough ???
#define UacDnBuf_LengthInMs 					(12 * 2)								//24ms
#define UacDnBuf_LengthInSamples 				(UacDnBuf_LengthInMs * AUDIO_OUT_SAMPLING_RATE_KHZ)									//12ms
//#define UacDnBuf_MaxReadLengthInSamples			(AudioFrameSizeInSamplePerCh_16KHz)			//I2S DMA intr(callback)  will read this buffer, and the read size is 1 frame
#define UacDnBuf_MaxReadLengthInSamples			(AudioFrameSizeInSamplePerCh_16KHz*3)		//when UAC is 48KHz, I2S is 16KHz, max read len is 3*Frmsize

#define AUDIO_SPEAKER_UsbDnBufCenterLevelInSamples  (UacDnBuf_LengthInMs*AUDIO_OUT_SAMPLING_RATE_KHZ/2)


#define UacDnBuf_CenterLevelInSamples  			(UacDnBuf_LengthInSamples/2)


//#define AUDIO_BUFFER_UPPER_LIMIT(x) (((x)*5) / 8)
//#define AUDIO_BUFFER_LOWER_LIMIT(x) (((x)*3) / 8)

#if defined(USB_DEVICE_CONFIG_AUDIO_CLASS_2_0) && (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0 > 0U)
//#define AUDIO_CALCULATE_Ff_INTERVAL (256) /* suggest: 1024U, 512U, 256U */
#define AUDIO_CALCULATE_Ff_INTERVAL (32) /* suggest: 1024U, 512U, 256U */
#else
#define AUDIO_CALCULATE_Ff_INTERVAL (1024)
#endif
#define TSAMFREQ2BYTES(f)                       (f & 0xFFU), ((f >> 8U) & 0xFFU), ((f >> 16U) & 0xFFU)
#define TSAMFREQ2BYTESHS(f)                     (f & 0xFFU), ((f >> 8U) & 0xFFU), ((f >> 16U) & 0xFFU), ((f >> 24U) & 0xFFU)
#define AUDIO_ADJUST_MIN_STEP                   (0x10)
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
/**********************************************************************
Audio PLL contants
      AUDIO_PLL_USB1_SOF_INTERVAL_COUNT
      The Audio PLL clock is 24.576Mhz, and the USB1_SOF_TOGGLE frequency is 4kHz when the device is attached,
      so AUDIO_PLL_USB1_SOF_INTERVAL_COUNT = (24576000 * 100 (stands for counter interval)) /4000 = 614400
      AUDIO_PLL_FRACTIONAL_CHANGE_STEP
      The Audio input clock is 24Mhz, and denominator is 4500, divider is 15 and PFD is 26.
      so AUDIO_PLL_FRACTIONAL_CHANGE_STEP = (24000000 * 100 (stands for counter interval) * 18) / (27000 * 26 * 15 * 4000) + 1
**********************************************************************/

#define AUDIO_PLL_ADJUST_INTERVAL (100U)

//#define AUDIO_PLL_USB1_SOF_INTERVAL_COUNT  (614400) /* The USB1_SOF_TOGGLE's frequency is 4kHz. */
#define AUDIO_PLL_USB1_SOF_INTERVAL_COUNT  (24576000 / 4000 * AUDIO_PLL_ADJUST_INTERVAL )			//this is 61440 when interval is 100

#define AUDIO_PLL_USB1_SOF_INTERVAL_COUNT1 (491520) /* The USB1_SOF_TOGGLE's frequency is 4kHz. */
#define AUDIO_PLL_FRACTIONAL_CHANGE_STEP   (1)
#endif

#define MUTE_CODEC_TASK    (1UL << 0U)
#define UNMUTE_CODEC_TASK  (1UL << 1U)
#define VOLUME_CHANGE_TASK (1UL << 2U)

typedef struct _usb_audio_composite_struct
{
    usb_device_handle deviceHandle;    /* USB device handle.                   */
    class_handle_t audioSpeakerHandle; /* USB AUDIO GENERATOR class handle.    */
    class_handle_t audioRecorderHandle;
    uint8_t copyProtect;
	
	//if optimization is enabled, must have these variables 4 bytes aligned --- or it crashes due to U32 *p pointing to it.

    __attribute__((aligned(4)))
    uint8_t curMute;
    __attribute__((aligned(4)))
    uint8_t curVolume[2];
    __attribute__((aligned(4)))
    uint8_t minVolume[2];
    __attribute__((aligned(4)))
    uint8_t maxVolume[2];
    __attribute__((aligned(4)))
    uint8_t resVolume[2];
    uint8_t curBass;
    uint8_t minBass;
    uint8_t maxBass;
    uint8_t resBass;
    uint8_t curMid;
    uint8_t minMid;
    uint8_t maxMid;
    uint8_t resMid;
    uint8_t curTreble;
    uint8_t minTreble;
    uint8_t maxTreble;
    uint8_t resTreble;
    uint8_t curAutomaticGain;
    uint8_t curDelay[2];
    uint8_t minDelay[2];
    uint8_t maxDelay[2];
    uint8_t resDelay[2];
    uint8_t curLoudness;
    uint8_t curSamplingFrequency[3];
    uint8_t minSamplingFrequency[3];
    uint8_t maxSamplingFrequency[3];
    uint8_t resSamplingFrequency[3];
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
    uint8_t curMute20;
    uint8_t curClockValid;
    uint8_t curVolume20[2];
    uint32_t curSpeakerSampleFrequency;
    uint32_t curRecorderSampleFrequency;
    usb_device_control_range_layout3_struct_t freqSpeakerControlRange;
    usb_device_control_range_layout3_struct_t freqRecorderControlRange;
    usb_device_control_range_layout2_struct_t volumeControlRange;
#endif
    uint8_t currentConfiguration;
    uint8_t currentInterfaceAlternateSetting[USB_DEVICE_INTERFACE_COUNT];
    uint8_t speed;
    uint8_t attach;
    volatile uint8_t startPlay;
    volatile uint8_t startPlayHalfFull;
    volatile uint32_t tdReadNumberPlay;
    volatile uint32_t tdWriteNumberPlay;
    volatile uint32_t audioSendCount;
    volatile uint32_t lastAudioSendCount;
    volatile uint32_t usbRecvCount;
    volatile uint32_t audioSendTimes;
    volatile uint32_t usbRecvTimes;
    volatile uint32_t speakerIntervalCount;
    volatile uint32_t speakerReservedSpace;
    volatile uint32_t timesFeedbackCalculate;
    volatile uint32_t speakerDetachOrNoInput;
    volatile uint32_t codecTask;
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	volatile uint32_t curAudioPllFrac;
	volatile uint32_t curAudioPllFrac_starting;
	volatile uint32_t audioPllTicksPrev;
	volatile int32_t audioPllTicksDiff;
	volatile int32_t audioPllTicksEma;
	volatile int32_t audioPllTickEmaFrac;
	volatile int32_t audioPllStep;
#endif
} usb_audio_composite_struct_t;

extern uint8_t audioFeedBackBuffer[4];
extern int Evt_UsbPlug;
extern int Evt_UsbUnPlug;
extern int UsbAudioUpstreamLengthAdjusted;

extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;

#endif

#endif /* __USB_AUDIO_GENERATOR_H__ */
