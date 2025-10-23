/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __WorkStateManager_h__
#define __WorkStateManager_h__

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
} TDeviceWorkState;

extern TDeviceWorkState DeviceWorkStateCur;

extern int AudioPortIsActive_I2SToAmp;
extern int AudioPortIsActive_I2SToNvt;
extern int AudioPortIsActive_Pdm;
extern int AudioPortIsActive_PcmToBt;
extern int AmpState;


extern int WorkStateIsChanged;
extern int RequestToGetIntoHfp;
extern int RequestToGetOutofHfp;

extern volatile int AllowAudioInterfaceReInit_PdmI2S;
extern volatile int AllowAudioInterfaceReInit_Fc25;

extern void Manager_Task(void *pvParameters);

#endif
