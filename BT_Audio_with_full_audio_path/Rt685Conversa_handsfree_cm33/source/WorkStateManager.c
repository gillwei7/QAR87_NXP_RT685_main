/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "stdio.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i2s_dma.h"
//#include "debug_utils.h"

#include "GlobalDef.h"


#include "fsl_mu.h"
#include "fsl_sema42.h"

#include "AudioIoCfg_I2s.h"
#include "AudioIoCfg_Pdm.h"
#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"
//#include "Sweep.h"
#include "MainAudioFlow.h"
#include "WorkStateManager.h"


int AudioPortIsActive_I2SToAmp;
int AudioPortIsActive_I2SToNvt;
int AudioPortIsActive_Pdm;
int AudioPortIsActive_PcmToBt;
int AmpState;		//AmpState_UnConfigured, AmpState_ConfiguredAndSleep, AmpState_ConfiguredAndActive,

int volatile AllowAudioInterfaceReInit_PdmI2S=1;	//maybe this is not necessary --- later could try to remove this protect logic
int volatile AllowAudioInterfaceReInit_Fc25=1;		//maybe this is not necessary --- later could try to remove this protect logic
int WorkStateIsChanged=0;
int RequestToGetIntoHfp=0;
int RequestToGetOutofHfp=0;
TDeviceWorkState DeviceWorkStateCur;
TDeviceWorkState DeviceWorkStatePre;


const char *WorkStateName[]=
{
	"WorkState_Void",
	"WorkState_HfpCall",
	"WorkState_HomeVitStandby",
	"WorkState_AudioIoDbg",
	"WorkState_VideoRecording",
	"WorkState_MediaPlayer",
	"WorkState_MusicPlayer",
	"WorkState_Translation",
	"WorkState_AiConversation",
	"WorkState_VideoAi",
};
#if 1	//folding --- all work states init and deinit
void WorkStateDeInit(int WhichState, U32 Opt)
{
	switch(WhichState)
	{
		case WorkState_HfpCall:
			DeInitAudioInterface_HfpCall(0);
			PRINTF_M("    Mcu: WorkState_HfpCall DeInit is done\r\n");
			break;
		case WorkState_HomeVitStandby:
			DeInitAudioInterface_HomeVitStandby(0);
			PRINTF_M("    Mcu: WorkState_HomeVitStandby DeInit is done\r\n");
			break;
		#if EnableWorkState_AudioIoDbg==1
			case WorkState_AudioIoDbg:
				DeInitAudioInterface_AudioIoDebug(0);
				PRINTF_M("    Mcu: WorkState_AudioIoDbg DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_VideoRecording==1
			case WorkState_VideoRecording:
				DeInitAudioInterface_VideoRecording(0);
				PRINTF_M("    Mcu: WorkState_VideoRecording DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_MediaPlayer==1
			case WorkState_MediaPlayer:
				DeInitAudioInterface_MediaPlayer(0);
				PRINTF_M("    Mcu: WorkState_MediaPlayer DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_MusicPlayer==1
			case WorkState_MusicPlayer:
				DeInitAudioInterface_MusicPlayer(0);
				PRINTF_M("    Mcu: WorkState_MusicPlayer DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_Translation==1
			case WorkState_Translation:
				DeInitAudioInterface_Translation(0);
				PRINTF_M("    Mcu: WorkState_Translation DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_AiConversation==1
			case WorkState_AiConversation:
				DeInitAudioInterface_AiConversation(0);
				PRINTF_M("    Mcu: WorkState_AiConversation DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_VideoAi==1
			case WorkState_VideoAi:
				DeInitAudioInterface_VideoAi(0);
				PRINTF_M("    Mcu: WorkState_VideoAi DeInit is done\r\n");
				break;
		#endif
		default:
			break;
	}
}
void WorkStateInit(int WhichState, U32 Opt)
{
	switch(WhichState)
	{
		case WorkState_HfpCall:
			InitAudioInterface_HfpCall(0);
			PRINTF_M("    Mcu: WorkState_HfpCall Init is done\r\n");
			break;
		case WorkState_HomeVitStandby:
			InitAudioInterface_HomeVitStandby(0);
			PRINTF_M("    Mcu: WorkState_HomeVitStandby Init is done\r\n");
			break;
		#if EnableWorkState_AudioIoDbg==1
			case WorkState_AudioIoDbg:
				InitAudioInterface_AudioIoDebug(0);
				PRINTF_M("    Mcu: WorkState_AudioIoDbg Init is done\r\n");
				break;
		#endif
		#if EnableWorkState_VideoRecording==1
			case WorkState_VideoRecording:
				InitAudioInterface_VideoRecording(0);
				PRINTF_M("    Mcu: WorkState_VideoRecording Init is done\r\n");
				break;
		#endif
		#if EnableWorkState_MediaPlayer==1
			case WorkState_MediaPlayer:
				InitAudioInterface_MediaPlayer(0);
				PRINTF_M("    Mcu: WorkState_MediaPlayer Init is done\r\n");
				break;
		#endif
		#if EnableWorkState_MusicPlayer==1
			case WorkState_MusicPlayer:
				InitAudioInterface_MusicPlayer(0);
				PRINTF_M("    Mcu: WorkState_MusicPlayer Init is done\r\n");
				break;
		#endif
		#if EnableWorkState_Translation==1
			case WorkState_Translation:
				InitAudioInterface_Translation(0);
				PRINTF_M("    Mcu: WorkState_Translation Init is done\r\n");
				break;
		#endif
		#if EnableWorkState_AiConversation==1
			case WorkState_AiConversation:
				InitAudioInterface_AiConversation(0);
				PRINTF_M("    Mcu: WorkState_AiConversation Init is done\r\n");
				break;
		#endif
		#if EnableWorkState_VideoAi==1
			case WorkState_VideoAi:
				InitAudioInterface_VideoAi(0);
				PRINTF_M("    Mcu: WorkState_VideoAi Init is done\r\n");
				break;
		#endif
		default:
			break;
	}
}

#endif

#if 1	//folding --- all work sates event processing
void VocEvtProc_HfpCall(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;
		default:
			break;
	}
}
void AppEvtProc_HfpCall()
{

}
void VocEvtProc_HomeVitStandby(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;
		default:
			break;
	}
}
void AppEvtProc_HomeVitStandby()
{

}
void VocEvtProc_AudioIoDbg(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeMainMenu:
			break;
		case ASR_VoiceCommand_MenuDirection_Left:
			break;
		case ASR_VoiceCommand_MenuDirection_Right:
			break;
		case ASR_VoiceCommand_MenuDirection_Next:
			break;
		case ASR_VoiceCommand_MenuSelect_Pickup:
			break;
		case ASR_VoiceCommand_MenuSelect_Enter:
			break;

		default:
			break;
	}
}
void AppEvtProc_AudioIoDbg()
{

}
void VocEvtProc_VideoRecording(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeVideoRecording:
			break;

		default:
			break;
	}
}
void AppEvtProc_VideoRecording()
{

}
void VocEvtProc_MediaPlayer(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeMediaplayer:
			break;
		case ASR_VoiceCommand_Play:
			break;
		case ASR_VoiceCommand_Pause:
			break;
		case ASR_VoiceCommand_Previous:
			break;
		case ASR_VoiceCommand_Next:
			break;
		case ASR_VoiceCommand_VolumeUpMediaplayer:
			break;
		case ASR_VoiceCommand_VolumeDownMediaplayer:
			break;

		default:
			break;
	}
}
void AppEvtProc_MediaPlayer()
{

}
void VocEvtProc_MusicPlayer(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeMusicplayer:
			break;
		case ASR_VoiceCommand_PlayMusic:
			break;
		case ASR_VoiceCommand_PauseMusic:
			break;
		case ASR_VoiceCommand_PreviousSong:
			break;
		case ASR_VoiceCommand_NextSong:
			break;
		case ASR_VoiceCommand_VolumeUpMusicplayer:
			break;
		case ASR_VoiceCommand_VolumeDownMusicplayer:
			break;

		default:
			break;
	}
}
void AppEvtProc_MusicPlayer()
{

}
void VocEvtProc_Translation(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeTranslation:
			break;

		default:
			break;
	}
}
void AppEvtProc_Translation()
{

}
void VocEvtProc_AiConversation(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeAiconversation:
			break;

		default:
			break;
	}
}
void AppEvtProc_AiConversation()
{

}
void VocEvtProc_VideoAi(U32 VoiceCmd)
{
	switch(VoiceCmd)
	{
		case ASR_VoiceCommand_AnswerThePhone:
			break;
		case ASR_VoiceCommand_RejectThePhone:
			break;
		case ASR_VoiceCommand_HangUpThePhone:
			break;
		case ASR_VoiceCommand_TakePhoto:
			break;

		case ASR_VoiceCommand_GoHomeVideoAi:
			break;

		default:
			break;
	}
}
void AppEvtProc_VideoAi()
{

}


#endif


void Manager_Task(void *pvParameters)
{

	DeviceWorkStateCur=WorkState_HomeVitStandby;
	DeviceWorkStatePre=WorkState_HomeVitStandby;

	WorkStateIsChanged=0;
	RequestToGetIntoHfp=0;
	RequestToGetOutofHfp=0;

	AllowAudioInterfaceReInit_PdmI2S=1;
	AllowAudioInterfaceReInit_Fc25=1;

	AudioPortIsActive_I2SToAmp=0;
	AudioPortIsActive_I2SToNvt=0;
	AudioPortIsActive_Pdm=0;
	AudioPortIsActive_PcmToBt=0;
	AmpState=AmpState_UnConfigured;

	while(1)
	{
		vTaskDelay(40);
		//wait till everything is ready for manager running
		if(1)
			break;
	}

	while(1)
	{
		vTaskDelay(40);

		//------------------------step 1, check events for switching work state, and change work state----------------------------------
		//---beg---

		//check app events for switching workstate
		#if 1	//folding
			if(RequestToGetIntoHfp)
			{
				DeviceWorkStatePre=DeviceWorkStateCur;
				DeviceWorkStateCur=WorkState_HfpCall;
				WorkStateIsChanged=1;
				RequestToGetIntoHfp=0;
				PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
			}

			if(RequestToGetOutofHfp)
			{
				DeviceWorkStateCur=DeviceWorkStatePre;
				DeviceWorkStatePre=WorkState_HfpCall;
				WorkStateIsChanged=1;
				RequestToGetOutofHfp=0;
				PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
			}

			#if EnableWorkState_MusicPlayer==1
				if((DeviceWorkStateCur!=WorkState_MusicPlayer)&&(VarBlockSharedByDspAndMcu.CurVoiceMenu==ASR_Menu_MusicPlayer))
				{
					DeviceWorkStatePre=DeviceWorkStateCur;
					DeviceWorkStateCur=WorkState_MusicPlayer;
					WorkStateIsChanged=1;
					PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
				}
				if((DeviceWorkStateCur==WorkState_MusicPlayer)&&(VarBlockSharedByDspAndMcu.CurVoiceMenu!=ASR_Menu_MusicPlayer))
				{
					DeviceWorkStateCur=DeviceWorkStatePre;
					DeviceWorkStatePre=WorkState_MusicPlayer;
					WorkStateIsChanged=1;
					PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
				}
			#endif
		#endif

		//check voice events for switching workstate
		if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent!=ASR_Menu_None)
		{
			//process voice event for switching work state
			switch(VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent)
			{
				case ASR_Menu_Home:
					//go to other workstate from HomeVit
					#if EnableWorkState_AudioIoDbg==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_OpenMenu)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_AudioIoDbg;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					#if EnableWorkState_VideoRecording==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartRecording)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_VideoRecording;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					#if EnableWorkState_MediaPlayer==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoMediaPlayer)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_MediaPlayer;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					#if EnableWorkState_MusicPlayer==1
						//note: go to music player is not done by voice command
					#endif
					#if EnableWorkState_Translation==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartTranslation)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_Translation;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					#if EnableWorkState_AiConversation==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartAiConversation)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_AiConversation;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					#if EnableWorkState_VideoAi==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartVideoAi)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_VideoAi;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;

				//go back to HomeVit from other workstates
				case ASR_Menu_MainMenu:
					#if EnableWorkState_AudioIoDbg==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMainMenu)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				case ASR_Menu_VideoRecording:
					#if EnableWorkState_VideoRecording==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoRecording)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				case ASR_Menu_MediaPlayer:
					#if EnableWorkState_MediaPlayer==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMediaplayer)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				case ASR_Menu_MusicPlayer:
					#if EnableWorkState_MusicPlayer==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMusicplayer)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				case ASR_Menu_Translation:
					#if EnableWorkState_Translation==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeTranslation)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				case ASR_Menu_AiConversation:
					#if EnableWorkState_AiConversation==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeAiconversation)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				case ASR_Menu_VidioAi:
					#if EnableWorkState_VideoAi==1
						if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoAi)
						{
							VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_HomeVitStandby;
							WorkStateIsChanged=1;
							PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
					break;
				default:
					break;
			}
			VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent=ASR_Menu_None;
		}

		//workstate change --- init and deinit,
		if(WorkStateIsChanged)
		{
			//wait till it is the proper time to re-init audio interface
			while(1)
			{
				if((AllowAudioInterfaceReInit_PdmI2S)&&(AllowAudioInterfaceReInit_Fc25))
						break;
			}
			#if 0
				WorkStateDeInit(DeviceWorkStatePre,  0);
				WorkStateInit  (DeviceWorkStateCur,  0);
			#else
				//temp disable some of the workstate switching
				if(DeviceWorkStateCur==WorkState_AudioIoDbg)
				{
					//now, only test going to audio IO debug
					WorkStateDeInit(DeviceWorkStatePre,  0);
					WorkStateInit  (DeviceWorkStateCur,  0);
				}
			#endif
			WorkStateIsChanged=0;
		}
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------

		//------------------------------------step 2, voice event and app event handling----------------------------------------------
		//---beg---
		//process app event and voice event for current work state
		switch(DeviceWorkStateCur)
		{
			case WorkState_HfpCall:
				if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
				{
					VocEvtProc_HfpCall(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
					VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
				}
				AppEvtProc_HfpCall();
				break;
			case WorkState_HomeVitStandby:
				if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
				{
					VocEvtProc_HomeVitStandby(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
					VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
				}
				AppEvtProc_HomeVitStandby();
				break;
			#if EnableWorkState_AudioIoDbg==1
				case WorkState_AudioIoDbg:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_AudioIoDbg(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_AudioIoDbg();
					break;
			#endif
			#if EnableWorkState_VideoRecording==1
				case WorkState_VideoRecording:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_VideoRecording(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_VideoRecording();
					break;
			#endif
			#if EnableWorkState_MediaPlayer==1
				case WorkState_MediaPlayer:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_MediaPlayer(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_MediaPlayer();
					break;
			#endif
			#if EnableWorkState_MusicPlayer==1
				case WorkState_MusicPlayer:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_MusicPlayer(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_MusicPlayer();
					break;
			#endif
			#if EnableWorkState_Translation==1
				case WorkState_Translation:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_Translation(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_Translation();
					break;
			#endif
			#if EnableWorkState_AiConversation==1
				case WorkState_AiConversation:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_AiConversation(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_AiConversation();
					break;
			#endif
			#if EnableWorkState_VideoAi==1
				case WorkState_VideoAi:
					if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName!=ASR_VoiceCommand_None)
					{
						VocEvtProc_VideoAi(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName);
						VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
					}
					AppEvtProc_VideoAi();
					break;
			#endif
			default:
				break;
		}
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------


	}
}

