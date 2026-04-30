/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __WorkStateManager_h__
#define __WorkStateManager_h__







//---------------------------------defines for AudioPortI2SAndPdmCfg related-----------------------------------------
//---beg---
#if 0	//--- decided not to use the following part
/*
AudioPortI2SAndPdmCfg:
bit 0~1: 		I2S_Amp Fs: 0,1,2,3 -> 16K, 32K, 44.1K, 48K
bit 2~3: 		I2S_Amp Bit: 0,1,2 -> disabled, 16, 32bit

bit 4~5:  	I2S_Nvt Fs: 0,1,2,3 -> 16K, 32K, 44.1K, 48K
bit 6~7:  	I2S_Nvt Bit: 0,1,2 -> disabled, 16, 32bit

bit 8~9:   	PDM Fs: 0,1,2,3 -> 16K, 32K, 44.1K, 48K
bit 10~15:  	PDM ch enable
*/

#define AudioPortI2SAndPdmCfg_GetAmpI2SFs(x)  (((x) & 0x03) >> 0)
#define AudioPortI2SAndPdmCfg_GetAmpI2SBit(x) (((x) & 0x0c) >> 2)

#define AudioPortI2SAndPdmCfg_GetNvtI2SFs(x)  (((x) & 0x30) >> 4)
#define AudioPortI2SAndPdmCfg_GetNvtI2SBit(x) (((x) & 0xc0) >> 6)

#define AudioPortI2SAndPdmCfg_GetPdmFs(x)        (((x) & 0x0300) >> 8)
#define AudioPortI2SAndPdmCfg_GetPdmEnableBit(x) (((x) & 0xfc00) >>10)

#define AudioPortI2SAndPdmCfg_SetAmpI2SFs(x, y)  x=(((x) & ~0x03) | (((y) & 0x03) << 0))
#define AudioPortI2SAndPdmCfg_SetAmpI2SBit(x, y) x=(((x) & ~0x0C) | (((y) & 0x03) << 2))

#define AudioPortI2SAndPdmCfg_SetNvtI2SFs(x, y)  x=(((x) & ~0x30) | (((y) & 0x03) << 4))
#define AudioPortI2SAndPdmCfg_SetNvtI2SBit(x, y) x=(((x) & ~0xC0) | (((y) & 0x03) << 6))

#define AudioPortI2SAndPdmCfg_SetPdmFs(x, y)        x=(((x) & ~0x0300) | (((y) & 0x03) << 8))
#define AudioPortI2SAndPdmCfg_SetPdmEnableBit(x, y) x=(((x) & ~0xfc00) | (((y) & 0x3f) <<10))

enum
{
	Fs_16000  = 0,
	Fs_32000,
	Fs_44100,
	Fs_48000,
};

enum
{
	BitWidth_I2SIsDisabled = 0,
	BitWidth_16,
	BitWidth_32,
};

extern U32 AudioPortI2SAndPdmCfg;
#endif
extern int BtA2dpFs_ProvidedFromBtStack;
//---end---
//---------------------------------defines for AudioPortI2SAndPdmCfg related-----------------------------------------






//---------------------------------defines for HFP stack request and events-----------------------------------------
//---beg---
typedef enum
{
	HfpRequest_None  = 0,
	HfpRequest_AudioSetup,
	HfpRequest_AudioStart,
	HfpRequest_AudioStop,
	HfpRequest_SetCodecAmpVolume,
	HfpRequest_RingToneStart,
	HfpRequest_RingToneStop,
} TBtHfpRequest;

extern TBtHfpRequest BtHfpRequest;
extern EventGroupHandle_t EvtGrpHdl_StateMangerTaskToBtStack;

//---end---
//---------------------------------defines for HFP stack request and events-----------------------------------------






#define EnableWorkState_AudioIoDbg							1
#define EnableWorkState_VideoRecording						1
#define EnableWorkState_MediaPlayer							1
#define EnableWorkState_MusicPlayer							1
#define EnableWorkState_Translation							1
#define EnableWorkState_AiConversation						1
#define EnableWorkState_VideoAi								1
										/*
										Note:
											HfpCall			--- always enabled
											HomeVitStandby  --- always enabled
										*/

typedef enum
{
	AmpState_UnConfigured  = 0,
	AmpState_ConfiguredAndSleep,
	AmpState_ConfiguredAndActive,
} TAmpState;


typedef enum
{
    WorkState_Void  = 0,		//this is a state that there is no audio interface active at all
	WorkState_HfpCall,
	WorkState_HomeVitStandby,
	WorkState_AudioIoDbg,
	WorkState_VideoRecording,
	WorkState_MediaPlayer,
	WorkState_MusicPlayer,
	WorkState_Translation,
	WorkState_AiConversation,
	WorkState_VideoAi,

	//-------------------------
	WorkState_Void_Pre,
	WorkState_HfpCall_Pre,
	WorkState_HomeVitStandby_Pre,
	WorkState_AudioIoDbg_Pre,
	WorkState_VideoRecording_Pre,
	WorkState_MediaPlayer_Pre,
	WorkState_MusicPlayer_Pre,
	WorkState_Translation_Pre,
	WorkState_AiConversation_Pre,
	WorkState_VideoAi_Pre,

} TDeviceWorkState;

extern uint8_t domainId;

extern TDeviceWorkState DeviceWorkStateCur;

extern int AudioPortIsActive_I2SToAmp;
extern int AudioPortIsActive_I2SToNvt;
extern int AudioPortIsActive_Pdm;
extern int AudioPortIsActive_PcmToBt;
extern int AmpState;


extern int WorkStateIsChanged;
extern int RequestToGetIntoHfp;
extern int RequestToGetOutofHfp;
extern int RequestToGetIntoA2dpPlay;
extern int RequestToGetOutofA2dpPlay;

extern void sco_audio_start(void);
extern volatile int AllowAudioInterfaceReInit_PdmI2S;
extern volatile int AllowAudioInterfaceReInit_Fc25;

extern void Manager_Task(void *pvParameters);

#endif
