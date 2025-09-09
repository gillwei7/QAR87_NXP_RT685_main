/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __DefForBothMcuAndDsp_h__
#define __DefForBothMcuAndDsp_h__


#if EnableUsbComAndAudio==1

//-----------------------------USB audio config that user can change---------------------------------
#if 1	//---folding

//interval value to select, only 0x01, 0x02 or 0x03 is allowed --- recommend not to change this
#define HS_ISO_OUT_ENDP_INTERVAL (0x02)			//the reason to have interval value <4 is to to decrease packet size. when having 8 ch, 32bit, 1ms packet is > 1023 byte --- must reduce packet length to have packet size < 1023
#define HS_ISO_IN_ENDP_INTERVAL  (0x02)

//-----------------UAC Up stream channel number select-----all the following options are available and can be selected--------------
//#define UsbAudioFormat_UpStreamChNumIsOne
//#define UsbAudioFormat_UpStreamChNumIsTwo
//#define UsbAudioFormat_UpStreamChNumIsFour
//#define UsbAudioFormat_UpStreamChNumIsSix
#define UsbAudioFormat_UpStreamChNumIsEight

//-----------------UAC Dn stream channel number select-----all the following options are available and can be selected--------------
//#define UsbAudioFormat_DnStreamChNumIsOne
#define UsbAudioFormat_DnStreamChNumIsTwo
//#define UsbAudioFormat_DnStreamChNumIsFour
//#define UsbAudioFormat_DnStreamChNumIsSix
//#define UsbAudioFormat_DnStreamChNumIsEight

#endif
//-----------------------------USB audio config that user can change---------------------------------



//--------------- USB upstream/downstream audio format def --- user should NOT change this part--------------------
#if 1	//---for folding --- not suggested to change this part
#ifdef UsbAudioFormat_UpStreamChNumIsOne
	#define AUDIO_IN_FORMAT_CHANNELS (0x01)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsTwo
	#define AUDIO_IN_FORMAT_CHANNELS (0x02)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsFour
	#define AUDIO_IN_FORMAT_CHANNELS (0x04)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsSix
	#define AUDIO_IN_FORMAT_CHANNELS (0x06)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsEight
	#define AUDIO_IN_FORMAT_CHANNELS (0x08)
#endif

#define AUDIO_IN_SAMPLING_RATE_KHZ (16)
#define AUDIO_IN_FORMAT_BITS (32)
#define AUDIO_IN_FORMAT_SIZE (4)



#ifdef UsbAudioFormat_DnStreamChNumIsOne
	#define AUDIO_OUT_FORMAT_CHANNELS (0x01)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsTwo
	#define AUDIO_OUT_FORMAT_CHANNELS (0x02)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsFour
	#define AUDIO_OUT_FORMAT_CHANNELS (0x04)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsSix
	#define AUDIO_OUT_FORMAT_CHANNELS (0x06)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsEight
	#define AUDIO_OUT_FORMAT_CHANNELS (0x08)
#endif

#define AUDIO_OUT_SAMPLING_RATE_KHZ (16)
#define AUDIO_OUT_FORMAT_BITS (32)
#define AUDIO_OUT_FORMAT_SIZE (4)

#endif	//---for folding
//--------------- USB upstream/downstream audio format def --- user should not change this part--------------------
#endif


//-------------other defines that are not suggested to be changed-------------------
#if 1	//folding
#define EvtFlag_AudioFrameIsReadyToProcess		0x00000001
#define EvtFlag_DspProcessingIsFiinished		0x00000002
#define APP_SEMA42								SEMA42
#define SEMA42_GATE								0U

#define AudioFrameSizeInSamplePerCh 			(128)			//conversa must work with framesize=128, should never change this value, unless Conversa lib's frame size is changed
#endif
//-------------other defines that are not suggested to be changed-------------------



//MCU program must has the exact same struct def as the following --- this h file should be included by both MCU and DSP prj
typedef struct
{
	//part 1 --- one frame of audio buf for each source and sink port
	__attribute__((aligned(32))) S32 PdmInAudioBuf[8][AudioFrameSizeInSamplePerCh];

	__attribute__((aligned(32))) S32 UacUpAudioBuf[AudioFrameSizeInSamplePerCh*8];		//this buffer is channel mixed, and to be used as cir buffer data source
	__attribute__((aligned(32))) S32 I2SLineInBufL[AudioFrameSizeInSamplePerCh];
	__attribute__((aligned(32))) S32 I2SLineInBufR[AudioFrameSizeInSamplePerCh];

	__attribute__((aligned(32))) S32 I2SLineOtBufL[AudioFrameSizeInSamplePerCh];
	__attribute__((aligned(32))) S32 I2SLineOtBufR[AudioFrameSizeInSamplePerCh];

	__attribute__((aligned(32))) S32 BTRxInAudio[AudioFrameSizeInSamplePerCh];	//MCU side writes in the full frame if BT is at 16KHz, writes in half if BT is at 8KHz
	__attribute__((aligned(32))) S32 BTTxOtAudio[AudioFrameSizeInSamplePerCh];	//MCU side takes out the full frame if BT is at 16KHz, takes out half if BT is at 8KHz


	//part 2 --- others
	U32 BtFs;
	U32 CycCnt				[100];
	U32 U32CycCntHistory	[100];
	U32 MonitorInfoArray1	[20];
	U32 MonitorInfoArray2	[20];
	U32 U32ControlPara		[40];
} T_CommonVarSharedByDspAndMcu;


#endif
