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
#include "SubFunc.h"
#if UsingQAR87Board == 1
#include "system_status.h"
#endif

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
int RequestToGetIntoA2dpPlay=0;
int RequestToGetOutofA2dpPlay=0;
int RequestToGetIntoVideoAI=0;
int RequestToGetOutofVideoAI=0;
int RequestToGetIntoTranslation=0;
int RequestToGetOutofTranslation=0;
int RequestToGetIntoMediaPlayer=0;
int RequestToGetOutofMediaPlayer=0;
int RequestToGetIntoTakePhoto=0;
int RequestToGetOutofTakePhoto=0;
int RequestToGetIntoVideoRecording=0;
int RequestToGetOutofVideoRecording=0;
int RequestToGetIntoMenu=0;
int RequestToGetOutofMenu=0;
int RequestToGetIntoAbout=0;
int RequestToGetOutofAbout=0;
int RequestToGetIntoHome=0;
int RequestToGetOutofHome=0;


TDeviceWorkState DeviceWorkStateCur;
TDeviceWorkState DeviceWorkStatePre;

#if UseEventToControlBtHfp==1
TBtHfpRequest BtHfpRequest;
#endif


int BtA2dpFs_ProvidedFromBtStack;
//U32 AudioPortI2SAndPdmCfg;
/*
AudioPortI2SAndPdmCfg:
bit 0~1: 		I2S_Amp Fs: 0,1,2,3 -> 16K, 32K, 44.1K, 48K
bit 2~3: 		I2S_Amp Bit: 0,1,2 -> disabled, 16, 32bit

bit 4~5:  	I2S_Nvt Fs: 0,1,2,3 -> 16K, 32K, 44.1K, 48K
bit 6~7:  	I2S_Nvt Bit: 0,1,2 -> disabled, 16, 32bit

bit 8~9:   	PDM Fs: 0,1,2,3 -> 16K, 32K, 44.1K, 48K
bit 10~15:  	PDM ch enable
*/



const char *WorkStateName[]=
{
	"WorkState_Void",
	"WorkState_HfpCall",
	"WorkState_HomeVitStandby",
	"WorkState_Menu",
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
int WorkStateInit(int WhichState, U32 Opt)
{
	switch(WhichState)
	{
		case WorkState_HfpCall_Pre:
			InitAudioInterface_HfpCall(0, 16000, 16);
			PRINTF_M("    Mcu: WorkState_HfpCall Init is done\r\n");
			return(WorkState_HfpCall);
			break;
		case WorkState_HomeVitStandby_Pre:
			InitAudioInterface_HomeVitStandby(AmpFc1Fc3);//in case PDM and Smart amp are required only in HomeStandby mode
			PRINTF_M("    Mcu: WorkState_HomeVitStandby Init is done\r\n");
			return(WorkState_HomeVitStandby);
			break;
		#if EnableWorkState_AudioIoDbg==1
			case WorkState_AudioIoDbg_Pre:
				#if UsingQAR87Board == 1
					InitAudioInterface_AudioIoDebug(BtPcmFc2Fc4_AmpFc1Fc3);//to enable FC5,6 to connect with NT98532
				#else	
					InitAudioInterface_AudioIoDebug(BtPcmFc5Fc2_CodecFc1Fc3);
				#endif
				PRINTF_M("    Mcu: WorkState_AudioIoDbg Init is done\r\n");
				return(WorkState_AudioIoDbg);
				break;
		#endif
		#if EnableWorkState_VideoRecording==1
			case WorkState_VideoRecording_Pre:
				InitAudioInterface_VideoRecording(0);
				PRINTF_M("    Mcu: WorkState_VideoRecording Init is done\r\n");
				return(WorkState_VideoRecording);
				break;
		#endif
		#if EnableWorkState_MediaPlayer==1
			case WorkState_MediaPlayer_Pre:
				InitAudioInterface_MediaPlayer(0);
				PRINTF_M("    Mcu: WorkState_MediaPlayer Init is done\r\n");
				return(WorkState_MediaPlayer);
				break;
		#endif
		#if EnableWorkState_MusicPlayer==1
			case WorkState_MusicPlayer_Pre:
				InitAudioInterface_MusicPlayer(AmpFc1Fc3);//in case smart amp is required only for music play
				PRINTF_M("    Mcu: WorkState_MusicPlayer Init is done\r\n");
				return(WorkState_MusicPlayer);
				break;
		#endif
		#if EnableWorkState_Translation==1
			case WorkState_Translation_Pre:
				InitAudioInterface_Translation(0);
				PRINTF_M("    Mcu: WorkState_Translation Init is done\r\n");
				return(WorkState_Translation);
				break;
		#endif
		#if EnableWorkState_AiConversation==1
			case WorkState_AiConversation_Pre:
				InitAudioInterface_AiConversation(0);
				PRINTF_M("    Mcu: WorkState_AiConversation Init is done\r\n");
				return(WorkState_AiConversation);
				break;
		#endif
		#if EnableWorkState_VideoAi==1
			case WorkState_VideoAi_Pre:
				InitAudioInterface_VideoAi(0);
				PRINTF_M("    Mcu: WorkState_VideoAi Init is done\r\n");
				return(WorkState_VideoAi);
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
	#if UseEventToControlBtHfp==1
		BaseType_t	PriorityTaskWoken = pdFALSE;
		switch(BtHfpRequest)
		{
			case HfpRequest_AudioSetup:
				break;
			case HfpRequest_AudioStart:
				//do nothing, here is actually to make hfp side wait till hfp audio init is done
				xEventGroupSetBits(EvtGrpHdl_StateMangerTaskToBtStack,HfpRequest_AudioStart);
				break;
			case HfpRequest_AudioStop:
				break;
			case HfpRequest_SetCodecAmpVolume:
				//to add
				xEventGroupSetBits(EvtGrpHdl_StateMangerTaskToBtStack,HfpRequest_SetCodecAmpVolume);
				break;
			case HfpRequest_RingToneStart:
				break;
			case HfpRequest_RingToneStop:
				break;
		}
		BtHfpRequest=HfpRequest_None;
	#endif
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
			PRINTF_M("VoiceCmd: take photo\r\n");
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
			PRINTF_M("VoiceCmd: take photo\r\n");
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
			PRINTF_M("VoiceCmd: take photo\r\n");
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
	DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
	DeviceWorkStatePre=WorkState_Void;
	WorkStateIsChanged=1;				//this is to init audio to homevit on startup

	RequestToGetIntoHfp=0;
	RequestToGetOutofHfp=0;
	RequestToGetIntoA2dpPlay=0;
	RequestToGetOutofA2dpPlay=0;

	RequestToGetIntoVideoAI=0;
	RequestToGetOutofVideoAI=0;
	RequestToGetIntoTranslation=0;
	RequestToGetOutofTranslation=0;

	RequestToGetIntoMediaPlayer=0;
	RequestToGetOutofMediaPlayer=0;

	RequestToGetIntoTakePhoto=0;
	RequestToGetOutofTakePhoto=0;
	RequestToGetIntoVideoRecording=0;
	RequestToGetOutofVideoRecording=0;

	RequestToGetIntoMenu=0;
	RequestToGetOutofMenu=0;
	RequestToGetIntoAbout=0;
	RequestToGetOutofAbout=0;


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
		//vTaskDelay(40);
		vTaskDelay(10);

		//-------------------------------step 0, get APP event: button, SPI, sensors, touch, etc---------------------------------------
		//---beg---
		#if 1	//folding
			GenBtnEvt();
		#endif
		//---end---
		//-------------------------------step 0, get APP event: button, SPI, sensors, touch, etc---------------------------------------


		//------------------------step 1, check events for switching work state, and change work state----------------------------------
		//---beg---
		#if 1	//folding
			//check app events for switching work state
			#if 1	//folding
				//getting into/out of HFP
				#if UseEventToControlBtHfp==1
					if(RequestToGetIntoHfp)
					{
						DeviceWorkStatePre=DeviceWorkStateCur;
					DeviceWorkStateCur=WorkState_HfpCall_Pre;
						WorkStateIsChanged=1;
						RequestToGetIntoHfp=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}

					if(RequestToGetOutofHfp)
					{
					DeviceWorkStateCur=DeviceWorkStatePre + (WorkState_Void_Pre - WorkState_Void);		//this gives _pre
						DeviceWorkStatePre=WorkState_HfpCall;
						WorkStateIsChanged=1;
						RequestToGetOutofHfp=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
				#endif

				#if EnableWorkState_MusicPlayer==1
					//getting into/out of A2dp
					if(RequestToGetIntoA2dpPlay && !RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur!=WorkState_MusicPlayer)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_MusicPlayer_Pre;
							WorkStateIsChanged=1;
							ss_set_music_status(1);

						}
						RequestToGetIntoA2dpPlay=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofA2dpPlay)
					{
						if(DeviceWorkStateCur==WorkState_MusicPlayer)
						{
							DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							DeviceWorkStatePre=WorkState_MusicPlayer;
							WorkStateIsChanged=1;
							ss_set_music_status(0);

						}
						RequestToGetOutofA2dpPlay=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}

					#if 0
						//this part is to check switching when A2dp BT is NOT added
						if((DeviceWorkStateCur!=WorkState_MusicPlayer)&&(VarBlockSharedByDspAndMcu.CurVoiceMenu==ASR_Menu_MusicPlayer))
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_MusicPlayer_Pre;
							WorkStateIsChanged=1;
							//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
						if((DeviceWorkStateCur==WorkState_MusicPlayer)&&(VarBlockSharedByDspAndMcu.CurVoiceMenu!=ASR_Menu_MusicPlayer))
						{
							DeviceWorkStateCur=DeviceWorkStatePre + (WorkState_Void_Pre - WorkState_Void);		//this gives _pre
							DeviceWorkStatePre=WorkState_MusicPlayer;
							WorkStateIsChanged=1;
							//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
						}
					#endif
				#endif
				#if EnableWorkState_MediaPlayer == 1
					if(RequestToGetIntoMediaPlayer && !RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur!=WorkState_MediaPlayer)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_MediaPlayer_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoMediaPlayer=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofMediaPlayer)
					{
						if(DeviceWorkStateCur==WorkState_MediaPlayer)
						{
							DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							DeviceWorkStatePre=WorkState_MediaPlayer;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofMediaPlayer=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
				#endif // #if EnableWorkState_MediaPlayer == 1

				#if EnableWorkState_Translation == 1
					if(RequestToGetIntoTranslation && !RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur!=WorkState_Translation)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_Translation_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoTranslation=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofTranslation)
					{
						if(DeviceWorkStateCur==WorkState_Translation)
						{
							DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							DeviceWorkStatePre=WorkState_Translation;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofTranslation=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
				#endif // #if EnableWorkState_Translation == 1

				#if EnableWorkState_VideoAi == 1
					if(RequestToGetIntoVideoAI && !RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur!=WorkState_VideoAi)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_VideoAi_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoVideoAI=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofVideoAI)
					{
						if(DeviceWorkStateCur==WorkState_VideoAi)
						{
							DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							DeviceWorkStatePre=WorkState_VideoAi;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofVideoAI=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
				#endif // #if EnableWorkState_VideoAi == 1

				#if EnableWorkState_VideoRecording == 1
					if(RequestToGetIntoVideoRecording && !RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur!=WorkState_VideoRecording)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_VideoRecording_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoVideoRecording=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofVideoRecording)
					{
						if(DeviceWorkStateCur==WorkState_VideoRecording)
						{
							DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							DeviceWorkStatePre=WorkState_VideoRecording;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofVideoRecording=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
				#endif // #if EnableWorkState_VideoRecording == 1

					if(RequestToGetIntoMenu)
					{
						if(DeviceWorkStateCur!=WorkState_Menu)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_Menu_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoMenu=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur==WorkState_Menu)
						{
							DeviceWorkStateCur=DeviceWorkStatePre + (WorkState_Void_Pre - WorkState_Void);		//this gives _pre
							DeviceWorkStatePre=WorkState_Menu;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofMenu=0;
						//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}


			#endif

			//check voice events for switching work state
			if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent!=ASR_Menu_None)
			{
				//process voice event for switching work state
				switch(VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent)
				{
					case ASR_Menu_Home:
						//go to other work state from HomeVit
						#if EnableWorkState_AudioIoDbg==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_OpenMenu)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_AudioIoDbg_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						#if EnableWorkState_VideoRecording==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartRecording)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						#if EnableWorkState_MediaPlayer==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoMediaPlayer)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
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
								DeviceWorkStateCur=WorkState_Translation_Pre;
								WorkStateIsChanged=1;
								PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						#if EnableWorkState_AiConversation==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartAiConversation)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_AiConversation_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						#if EnableWorkState_VideoAi==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartVideoAi)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_VideoAi_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
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
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						break;
					case ASR_Menu_VideoRecording:
						#if EnableWorkState_VideoRecording==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoRecording)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						break;
					case ASR_Menu_MediaPlayer:
						#if EnableWorkState_MediaPlayer==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMediaplayer)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						break;
					case ASR_Menu_MusicPlayer:
						#if EnableWorkState_MusicPlayer==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMusicplayer)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						break;
					case ASR_Menu_Translation:
						#if EnableWorkState_Translation==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeTranslation)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						break;
					case ASR_Menu_AiConversation:
						#if EnableWorkState_AiConversation==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeAiconversation)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif
						break;
					case ASR_Menu_VidioAi:
						#if EnableWorkState_VideoAi==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoAi)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								//PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
							}
						#endif

						break;
					default:
						break;
				}
				VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent=ASR_Menu_None;
			}

			//work state change --- init and deinit,
			if(WorkStateIsChanged)
			{
				//wait till it is the proper time to re-init audio interface
				while(1)
				{
					if((AllowAudioInterfaceReInit_PdmI2S)&&(AllowAudioInterfaceReInit_Fc25))
							break;
				}

				WorkStateDeInit(DeviceWorkStatePre,  0);
				DeviceWorkStateCur = WorkStateInit  (DeviceWorkStateCur,  0);

				// Send SPI command to SoC
				send_state_to_soc();

				PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
				WorkStateIsChanged=0;
			}
		#endif
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------

		//------------------------------------step 2, voice event and app event handling----------------------------------------------
		//---beg---
		//process app event and voice event for current work state
		#if 1	//folding
			if(VarBlockSharedByDspAndMcu.WWIsDetected)
			{
				PRINTF_M("VoiceCmd: Hey Quanta is detected!\r\n");
				VarBlockSharedByDspAndMcu.WWIsDetected=0;
			}

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
		#endif
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------
	}
}

