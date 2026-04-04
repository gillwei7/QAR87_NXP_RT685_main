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

#include "MainAudioFlow.h"
#include "WorkStateManager.h"
#include "SubFunc.h"
#if UsingQAR87Board == 1
#include "system_status.h"
#include "hal_common.h"
#include "ringtone_handler.h"
#include "scenario_state.h"
#endif

#include "app_handsfree.h"

extern void PRINTF_UsbCom(uint8_t *data, size_t len);

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
static int currentLevel15 = 7;  // 1~15

TDeviceWorkState DeviceWorkStateCur;
TDeviceWorkState DeviceWorkStatePre;

TBtHfpRequest BtHfpRequest;


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
	    "WorkState_Void",		//this is a state that there is no audio interface active at all
	"WorkState_HfpCall",
	"WorkState_HomeVitStandby",
	"WorkState_AudioIoDbg",
		"WorkState_Menu",
	"WorkState_VideoRecording",
		"WorkState_TakePhoto",
	"WorkState_MediaPlayer",
	"WorkState_MusicPlayer",
	"WorkState_Translation",
	"WorkState_AiConversation",
	"WorkState_VideoAi",
	"WorkState_About",

	//-------------------------
	"WorkState_Void_Pre",
	"WorkState_HfpCall_Pre",
	"WorkState_HomeVitStandby_Pre",
	"WorkState_AudioIoDbg_Pre",
	"WorkState_Menu_Pre",
	"WorkState_VideoRecording_Pre",
	"WorkState_TakePhoto_Pre",
	"WorkState_MediaPlayer_Pre",
	"WorkState_MusicPlayer_Pre",
	"WorkState_Translation_Pre",
	"WorkState_AiConversation_Pre",
	"WorkState_VideoAi_Pre",
	"WorkState_About_Pre",
};

// 15-level volume gain table (0.00158f -> 0.999f, equal-dB spacing)
const float MasterVolumeGainTable15[15] =
{
    0.063058f, // Level 1  (-24.00 dB)
    0.076595f, // Level 2  (-22.29 dB)
    0.093045f, // Level 3  (-20.57 dB)
    0.113032f, // Level 4  (-18.86 dB)
    0.137322f, // Level 5  (-17.14 dB)
    0.166849f, // Level 6  (-15.43 dB)
    0.202735f, // Level 7  (-13.71 dB)
    0.246328f, // Level 8  (-12.00 dB)
    0.299249f, // Level 9  (-10.29 dB)
    0.363456f, // Level 10 (-8.57 dB)
    0.441327f, // Level 11 (-6.86 dB)
    0.535755f, // Level 12 (-5.14 dB)
    0.650282f, // Level 13 (-3.43 dB)
    0.789206f, // Level 14 (-1.71 dB)
    0.999000f  // Level 15 (~0 dB)
};



void ChangeMasterVolumeLevel15(int level15)
{
    int index = level15 - 1;  // level 1~16 -> index 0~15

    // Clamp
    if(index < 0) index = 0;
    if(index > 14) index = 14;

    float set_MasterVolumeGain0To1 = MasterVolumeGainTable15[index];

    VarBlockSharedByDspAndMcu.MasterVolumeGain0To1 = set_MasterVolumeGain0To1;

    PRINTF("Volume Change to Level %d : %f\r\n",
    		level15, set_MasterVolumeGain0To1);
}

void ChangeMasterVolumeLevel15_UpDown(int direction)
{
    /* direction: +1 = volume up, -1 = volume down */
    if(direction > 0)
    {
        currentLevel15++;
    }
    else if(direction <= 0)
    {
        currentLevel15--;
    }

    /* Clamp */
    if(currentLevel15 < 1)  currentLevel15 = 1;
    if(currentLevel15 > 15) currentLevel15 = 15;

    int index = currentLevel15 - 1;

    float set_MasterVolumeGain0To1 =
        MasterVolumeGainTable15[index];

    VarBlockSharedByDspAndMcu.MasterVolumeGain0To1 =
        set_MasterVolumeGain0To1;

    PRINTF("Volume Change %s -> Level %d : %f\r\n",
           (direction > 0) ? "UP" : "DOWN",
           currentLevel15,
           set_MasterVolumeGain0To1);
}

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
//		case WorkState_Menu:
//			DeInitAudioInterface_HomeVitStandby(0);
//			PRINTF_M("    Mcu: WorkState_HomeVitStandby(Menu) DeInit is done\r\n");
//			break;
//		case WorkState_About:
//			DeInitAudioInterface_HomeVitStandby(0);
//			PRINTF_M("    Mcu: WorkState_HomeVitStandby(About) DeInit is done\r\n");
//			break;
		#if EnableWorkState_AudioIoDbg==1
			case WorkState_AudioIoDbg:
				DeInitAudioInterface_AudioIoDebug(0);
				PRINTF_M("    Mcu: WorkState_AudioIoDbg DeInit is done\r\n");
				break;
		#endif
		#if EnableWorkState_VideoRecording==1
			case WorkState_VideoRecording:
				DeInitAudioInterface_VideoRecording(0);
				set_ringtone_state(Ringtone_StopRecording);
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
				set_ringtone_state(Ringtone_StopTranslation);

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
				set_ringtone_state(Ringtone_StopVideoAI);
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
			InitAudioInterface_HfpCall(0);
			PRINTF_M("    Mcu: WorkState_HfpCall Init is done\r\n");
			return(WorkState_HfpCall);
			break;
		case WorkState_HomeVitStandby_Pre:
			InitAudioInterface_HomeVitStandby(0);
			PRINTF_M("    Mcu: WorkState_HomeVitStandby Init is done\r\n");
			return(WorkState_HomeVitStandby);
			break;
		#if EnableWorkState_AudioIoDbg==1
			case WorkState_AudioIoDbg_Pre:
				InitAudioInterface_AudioIoDebug(0);
				PRINTF_M("    Mcu: WorkState_AudioIoDbg Init is done\r\n");
				return(WorkState_AudioIoDbg);
				break;
		#endif
		#if EnableWorkState_VideoRecording==1
			case WorkState_VideoRecording_Pre:
				InitAudioInterface_VideoRecording(0);
				set_ringtone_state(Ringtone_StartRecording);
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
				set_ringtone_state(Ringtone_StartTranslation);
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
				set_ringtone_state(Ringtone_StartVideoAI);
				PRINTF_M("    Mcu: WorkState_VideoAi Init is done\r\n");
				return(WorkState_VideoAi);
				break;
		#endif
//		#if EnableWorkState_Menu==1
//			case WorkState_Menu_Pre:
////				InitAudioInterface_HomeVitStandby(0);
//				PRINTF_M("    Mcu: WorkState_HomeVitStandby(Menu) Init is done\r\n");
//				return(WorkState_Menu);
//				break;
//		#endif
//		#if EnableWorkState_About==1
//			case WorkState_About_Pre:
////				InitAudioInterface_HomeVitStandby(0);
//				PRINTF_M("    Mcu: WorkState_HomeVitStandby(About) Init is done\r\n");
//				return(WorkState_About);
//				break;
//		#endif

		default:
			return(WhichState);
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
			PRINTF_M("VoiceCmd: take photo\r\n");
			break;
		default:
			break;
	}
}
void AppEvtProc_HfpCall()
{
	BaseType_t	PriorityTaskWoken = pdFALSE;
	switch(BtHfpRequest)
	{
		case HfpRequest_AudioSetup:
			break;
		case HfpRequest_AudioStart:
			sco_audio_start();
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
	//if a change volume event is detected, change master volume here, then DSP side will follow
	//VarBlockSharedByDspAndMcu.MasterVolumeGain0To1=0.999f;
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
		//write a control request to DSP to let EAP change volume --- later to test
			VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdx_McuCmdToDsp]		=McuToDspReqeust_IncMasterVol;
			VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdx_McuCmdToDspPara1]	=VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent;
			VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdx_McuCmdToDspPara2]	=VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName;
			break;
		case ASR_VoiceCommand_VolumeDownMusicplayer:
		//write a control request to DSP to let EAP change volume --- later to test
			VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdx_McuCmdToDsp]		=McuToDspReqeust_SetMasterVol;
			VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdx_McuCmdToDspPara1]	=VarBlockSharedByDspAndMcu.CurrentVoiceCommandIntent;
			VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdx_McuCmdToDspPara2]	=VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName;
			break;

		default:
			break;
	}
}
void AppEvtProc_MusicPlayer()
{
	//if a change volume event is detected, change master volume here, then DSP side will follow

	static int TstCnt=0, OpusCount=0;

	#if 1 //B36932 for debug only
		//for demo volume control only
		//close this ++ to have stable master volume
		//TstCnt++;		//this is just a test to watch mater volume changes have effect (on the UAC up streaming channel 1 2)

		if(TstCnt%600 ==1)
		{
			VarBlockSharedByDspAndMcu.MasterVolumeGain0To1=0.199f;
			//VarBlockSharedByDspAndMcu.NeedToStartPlayOpus=1;
			//VarBlockSharedByDspAndMcu.PlayOpusFileIdx=OpusCount++;
			//if(OpusCount>12)
			//	OpusCount=0;
		}
		if(TstCnt%600 ==301)
		{
			VarBlockSharedByDspAndMcu.MasterVolumeGain0To1=0.999f;
		}
	#endif
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

#if DspPrintsToMcuThenMcuPrintsToUsbCom==1
int IncreasePrintBufIdx(int Idx, int l)
{
	Idx++;
	if(Idx >= l)
		Idx=0;
	return Idx;
}
int FreeSpaceOfThePrintBuf(int WIdx, int RIdx, int l)
{
	if(WIdx==RIdx)
		return (l);
	if(WIdx>RIdx)
		return(l-(WIdx-RIdx));
	else
		return((RIdx-WIdx)-1);
}
int UsedSpaceOfThePrintBuf(int WIdx, int RIdx, int l)
{
	if(WIdx==RIdx)
		return (0);
	if(WIdx>RIdx)
		return(WIdx-RIdx);
	else
		return(l+1-(RIdx-WIdx));
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

	VarBlockSharedByDspAndMcu.MasterVolumeGain0To1 = MasterVolumeGainTable15[currentLevel15];		//must set an initial value

	spi_command_handler_init();
	button_init();

	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(40));
		//wait till everything is ready for manager running
		if(1)
			break;
	}

	MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, 1);		//DSP side waits for 2 MSGs before DSP goes on --- this is the second

	/* Wait DSP core is Boot Up */
	while (BOOT_FLAG_2 != MU_GetFlags(APP_MU))
	{
		delay_ms(1);
	};


	while(1)
	{
		//vTaskDelay(40);
		//vTaskDelay(10);
		vTaskDelay(pdMS_TO_TICKS(10));

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
				if((RequestToGetIntoHfp)&&(DeviceWorkStateCur!=WorkState_HfpCall))
				{
					DeviceWorkStatePre=DeviceWorkStateCur;
					DeviceWorkStateCur=WorkState_HfpCall_Pre;
					WorkStateIsChanged=1;
					RequestToGetIntoHfp=0;
				}

				//if((RequestToGetOutofHfp)&&(DeviceWorkStateCur==WorkState_HfpCall))
				if(RequestToGetOutofHfp)
				{
					DeviceWorkStateCur=DeviceWorkStatePre + (WorkState_Void_Pre - WorkState_Void);		//this gives _pre
					DeviceWorkStatePre=WorkState_HfpCall;
					WorkStateIsChanged=1;
					RequestToGetOutofHfp=0;
				}

				#if EnableWorkState_MusicPlayer==1
					//getting into/out of A2dp
					if(RequestToGetIntoA2dpPlay && !RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur!=WorkState_MusicPlayer)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_MusicPlayer_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoA2dpPlay=0;
					}
					if(RequestToGetOutofA2dpPlay)
					{
						if(DeviceWorkStateCur==WorkState_MusicPlayer)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TAKE_PHOTO) {
								DeviceWorkStateCur=WorkState_TakePhoto_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}

							DeviceWorkStatePre=WorkState_MusicPlayer;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofA2dpPlay=0;
					}
					#if 0
						//this part is to check switching according to the request from DSP side --- evk board button press makes the request
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
					if(RequestToGetIntoMediaPlayer)
					{
						if(DeviceWorkStateCur!=WorkState_MediaPlayer)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_MediaPlayer_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoMediaPlayer=0;
					}
					if(RequestToGetOutofMediaPlayer)
					{
						if(DeviceWorkStateCur==WorkState_MediaPlayer)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TAKE_PHOTO) {
								DeviceWorkStateCur=WorkState_TakePhoto_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_MediaPlayer;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofMediaPlayer=0;
					}
				#endif // #if EnableWorkState_MediaPlayer == 1

				#if EnableWorkState_Translation == 1
					if(RequestToGetIntoTranslation)
					{
						if(DeviceWorkStateCur!=WorkState_Translation)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_Translation_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoTranslation=0;
					}
					if(RequestToGetOutofTranslation)
					{
						if(DeviceWorkStateCur==WorkState_Translation)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TAKE_PHOTO) {
								DeviceWorkStateCur=WorkState_TakePhoto_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_Translation;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofTranslation=0;
					}
				#endif // #if EnableWorkState_Translation == 1

				#if EnableWorkState_VideoAi == 1
					if(RequestToGetIntoVideoAI)
					{
						if(DeviceWorkStateCur!=WorkState_VideoAi)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_VideoAi_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoVideoAI=0;
					}
					if(RequestToGetOutofVideoAI)
					{
						if(DeviceWorkStateCur==WorkState_VideoAi)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TAKE_PHOTO) {
								DeviceWorkStateCur=WorkState_TakePhoto_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_VideoAi;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofVideoAI=0;
					}
				#endif // #if EnableWorkState_VideoAi == 1

				#if EnableWorkState_VideoRecording == 1
					if(RequestToGetIntoVideoRecording)
					{
						if(DeviceWorkStateCur!=WorkState_VideoRecording)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_VideoRecording_Pre;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoVideoRecording=0;
					}
					if(RequestToGetOutofVideoRecording)
					{
						if(DeviceWorkStateCur==WorkState_VideoRecording)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TAKE_PHOTO) {
								DeviceWorkStateCur=WorkState_TakePhoto_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_VideoRecording;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofVideoRecording=0;
					}
				#endif // #if EnableWorkState_VideoRecording == 1

				#if EnableWorkState_TakePhoto == 1
					if(RequestToGetIntoTakePhoto)
					{
						if(DeviceWorkStateCur!=WorkState_TakePhoto)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_TakePhoto;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoTakePhoto=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
					if(RequestToGetOutofTakePhoto)
					{
						if(DeviceWorkStateCur==WorkState_TakePhoto)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_TakePhoto;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofTakePhoto=0;
						PRINTF_M("    Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
					}
				#endif // #if EnableWorkState_TakePhoto == 1

					if(RequestToGetIntoAbout)
					{
						if(DeviceWorkStateCur!=WorkState_About)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_About;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoAbout=0;
					}
					if(RequestToGetOutofAbout)
					{
						if(DeviceWorkStateCur==WorkState_About)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MENU) {
								DeviceWorkStateCur=WorkState_Menu;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_About;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofAbout=0;
					}

					if(RequestToGetIntoMenu)
					{
						if(DeviceWorkStateCur!=WorkState_Menu)
						{
							DeviceWorkStatePre=DeviceWorkStateCur;
							DeviceWorkStateCur=WorkState_Menu;
							WorkStateIsChanged=1;
						}
						RequestToGetIntoMenu=0;
					}
					if(RequestToGetOutofMenu)
					{
						if(DeviceWorkStateCur==WorkState_Menu)
						{
							if (get_scenario_state() == SCENARIO_STATE_HOME) {
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_ABOUT) {
								DeviceWorkStateCur=WorkState_About;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_MEDIA_PLAYER) {
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_RECORDING) {
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_VIDEO_AI) {
								DeviceWorkStateCur=WorkState_VideoAi_Pre;		//this gives _pre
							} else if (get_scenario_state() == SCENARIO_STATE_TRANSLATION) {
								DeviceWorkStateCur=WorkState_Translation_Pre;		//this gives _pre
							}
							DeviceWorkStatePre=WorkState_Menu;
							WorkStateIsChanged=1;
						}
						RequestToGetOutofMenu=0;
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
							}
						#endif
						#if EnableWorkState_VideoRecording==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartRecording)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_VideoRecording_Pre;
								WorkStateIsChanged=1;
								set_ringtone_state(Ringtone_StartRecording);
							}
						#endif
						#if EnableWorkState_MediaPlayer==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoMediaPlayer)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_MediaPlayer_Pre;
								WorkStateIsChanged=1;
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
								set_ringtone_state(Ringtone_StartTranslation);
							}
						#endif
						#if EnableWorkState_AiConversation==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartAiConversation)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_AiConversation_Pre;
								WorkStateIsChanged=1;
							}
						#endif
						#if EnableWorkState_VideoAi==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_StartVideoAi)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_VideoAi_Pre;
								WorkStateIsChanged=1;
								set_ringtone_state(Ringtone_StartVideoAI);
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
								set_ringtone_state(Ringtone_StopRecording);
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
								set_ringtone_state(Ringtone_StopTranslation);
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
							}
						#endif
						break;
					case ASR_Menu_VideoAi:
						#if EnableWorkState_VideoAi==1
							if(VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoAi)
							{
								VarBlockSharedByDspAndMcu.CurrentVoiceCommandTagName=ASR_VoiceCommand_None;
								DeviceWorkStatePre=DeviceWorkStateCur;
								DeviceWorkStateCur=WorkState_HomeVitStandby_Pre;
								WorkStateIsChanged=1;
								set_ringtone_state(Ringtone_StopVideoAI);
							}
						#endif
						break;
					default:
						break;
				}
				PRINTF_M("Voice events for switching work state\r\n");
				PRINTF_M("Mcu: Pre in: %s\r\n",WorkStateName[DeviceWorkStatePre]);
				PRINTF_M("Mcu: Now in: %s\r\n",WorkStateName[DeviceWorkStateCur]);
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
				// If it is placed between deinit and init , it will cause SPI failure
//				send_state_to_soc();
				PRINTF_M("WorkStateIsChanged\r\n");
				PRINTF_M("Mcu: Pre in: %s\r\n",WorkStateName[DeviceWorkStatePre]);
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

		#if DspPrintsToMcuThenMcuPrintsToUsbCom==1
			//check if there is the print request from DSP --- now only USB COM print is available
			if(UsedSpaceOfThePrintBuf(VarBlockSharedByDspAndMcu.DspPrintBufWrIdx, VarBlockSharedByDspAndMcu.DspPrintBufRdIdx, DspPrintBufLength))
			{
				#if PRINTF_GoesToUsbCom==1
					PRINTF_UsbCom((U8 *)&VarBlockSharedByDspAndMcu.DspPrintBuf[VarBlockSharedByDspAndMcu.DspPrintBufRdIdx][1], VarBlockSharedByDspAndMcu.DspPrintBuf[VarBlockSharedByDspAndMcu.DspPrintBufRdIdx][0]);
				#endif
					VarBlockSharedByDspAndMcu.DspPrintBufRdIdx=IncreasePrintBufIdx(VarBlockSharedByDspAndMcu.DspPrintBufRdIdx,DspPrintBufLength);
			}
		#endif
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------
		//------------------------------------step 3, i2c devices handling----------------------------------------------
		//---beg---
			i2c_device_handler();
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------

		//------------------------------------step 4, spi commands handling----------------------------------------------
		//---beg---
			spi_command_handler();
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------
		//------------------------------------step 5, connection handling----------------------------------------------
		//---beg---
			connect_handler();
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------
		//------------------------------------step 6, ringtone handling----------------------------------------------
		//---beg---
			ringtone_handler();
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------
		//------------------------------------step 7, button handling----------------------------------------------
		//---beg---
			button_handler();
		//---end---
		//----------------------------------------------------------------------------------------------------------------------------

	}
}

