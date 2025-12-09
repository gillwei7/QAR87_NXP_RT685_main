/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <xtensa/config/core.h>
#include <xtensa/xos.h>

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"

#include "fsl_dma.h"
#include "fsl_mu.h"
#include "fsl_sema42.h"

#include "pin_mux.h"
#include "board_hifi4.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"

#include "GlobalDef.h"
#include "SubFunc.h"
#include "Sweep.h"
#include "TestDspFunctions.h"

#include "GainingAndMixing.h"
#include "IIR.h"
#include "SRCProc.h"

#include "CircularBufManagement.h"

#define CORE_DSP
#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"


//#include "VIT_Model_en.h"
//#include "VIT_Model_en_2.h"
#include "VIT_Model_en_Quanta.h"

#define MASK_SIE

#if defined(MASK_SIE)
#include "VIT_Intent_Slot_Mask.h"
#endif


/*******************************************************************************
 * Variables
 ******************************************************************************/

//XosThread 		  g_audioVitTask_thread;			 							// Audio task thread
//PL_UINT8	 	  g_audioVitTask_stack 	[DSP_AUDIO_VIT_THREAD_STACK_SIZE_BYTE];	// Audio task memory stack allocated

XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.
XosMutex 		  g_audio_vitBufferMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task
XosMutex 		  g_audio_SbcDecoderMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task
XosMutex 		  g_audio_OpusDecoderMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task

T_CircularAudioBuf_S16  VitCircBuff;
T_CircularAudioBuf_S16  VitCircBuff_RawMic;
S16 					VitCircBuff_DataArea[APP_VIT_FRAME_SIZE_InSAMPLEs*2+APP_VIT_FRAME_SIZE_InSAMPLEs];	//cirbuffer size=APP_VIT_FRAME_SIZE_InSAMPLEs*2, extra read space for 1 APP_VIT_FRAME_SIZE_InSAMPLEs frame
S16 					VitCircBuff_DataArea_RawMic[APP_VIT_FRAME_SIZE_InSAMPLEs*2+APP_VIT_FRAME_SIZE_InSAMPLEs];	//cirbuffer size=APP_VIT_FRAME_SIZE_InSAMPLEs*2, extra read space for 1 APP_VIT_FRAME_SIZE_InSAMPLEs frame
VIT_DetectionStatus_en 	g_vitDetectionResult = 0;   // VIT detection results
PL_UINT16 				g_vitVcDetectionId = 0;     // VIT voice command detection results ID

volatile uint32_t g_wakeWordLength  = 0;
PL_UINT32 g_vitFramecount 	 = 0;
static PL_UINT32 g_vitDetectionFrame = 0;
VIT_Intent_st SpeechIntent;
uint32_t g_sampleCount = 0;

const char *CurrentIntentName;
const char *CurrentTagName;
const char *CurrentTagValue1;
//const char *CurrentTagValue2;
//VoiceCommandItem_t CurrentVoiceCommandTagValue;

#if ASE_DetectDisplayInUacWav==1
ASR_WavPulseType ASR_WavPulse=ASR_WavPulse_NothingDetected;
#endif

int InitalVitMaskSetupIsDone=0;

const char *MenuItemName[]=
{
	"ASR_Menu_None",
	"ASR_Menu_Home",
	"ASR_Menu_VideoAi",
	"ASR_Menu_AiConversation",
	"ASR_Menu_Translation",
	"ASR_Menu_VideoRecording",
	"ASR_Menu_MediaPlayer",
	"ASR_Menu_PhoneCall",
	"ASR_Menu_MusicPlayer",
	"ASR_Menu_MainMenu",
};

/*******************************************************************************
 * Code
 ******************************************************************************/

void IdentifyCurrentVoiceCommand(void)
{
	if(!strcmp("Menu",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_MainMenu;
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeMainMenu;
			PRINTF("ASR_VoiceCommand_GoHomeMainMenu\r\n");
		}
		if(!strcmp("direction",CurrentTagName))
		{
			if(!strcmp("left",CurrentTagValue1))
			{
				PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_MenuDirection_Left;
				PRINTF("ASR_VoiceCommand_MenuDirection_Left\r\n");
			}else
				if(!strcmp("right",CurrentTagValue1))
				{
					PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_MenuDirection_Right;
					PRINTF("ASR_VoiceCommand_MenuDirection_Right\r\n");
				}else
					if(!strcmp("next",CurrentTagValue1))
					{
						PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_MenuDirection_Next;
						PRINTF("ASR_VoiceCommand_MenuDirection_Next\r\n");
					}
		}
		if(!strcmp("select",CurrentTagName))
		{
			if(!strcmp("pickup",CurrentTagValue1))
			{
				PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_MenuSelect_Pickup;
				PRINTF("ASR_VoiceCommand_MenuDirection_Pickup\r\n");
			}else
				if(!strcmp("enter",CurrentTagValue1))
				{
					PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_MenuSelect_Enter;
					PRINTF("ASR_VoiceCommand_MenuDirection_Enter\r\n");
				}
		}
	}
	if(!strcmp("AIConversation",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_AiConversation;
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeAiconversation;
			PRINTF("ASR_VoiceCommand_GoHomeAiconversation\r\n");
		}
	}
	if(!strcmp("Call",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_PhoneCall;
		if(!strcmp("answer",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_AnswerThePhone;
			PRINTF("ASR_VoiceCommand_AnswerThePhone\r\n");
		}
		if(!strcmp("reject",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_RejectThePhone;
			PRINTF("ASR_VoiceCommand_RejectThePhone\r\n");
		}
		if(!strcmp("hang_up",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_HangUpThePhone;
			PRINTF("ASR_VoiceCommand_HangUpThePhone\r\n");
		}
	}
	if(!strcmp("Home",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_Home;
		if(!strcmp("Translation",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_StartTranslation;
			PRINTF("ASR_VoiceCommand_StartTranslation\r\n");
		}
		if(!strcmp("VideoAI",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_StartVideoAi;
			PRINTF("ASR_VoiceCommand_StartVideoAi\r\n");
		}
		if(!strcmp("AIConversation",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_StartAiConversation;
			PRINTF("ASR_VoiceCommand_StartAiConversation\r\n");
		}
		if(!strcmp("MediaPlayer",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoMediaPlayer;
			PRINTF("ASR_VoiceCommand_GoMediaPlayer\r\n");
		}
		if(!strcmp("Photo",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_TakePhoto;
			PRINTF("ASR_VoiceCommand_TakePhotoOrPicture\r\n");
		}
		if(!strcmp("VideoRecording",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_StartRecording;
			PRINTF("ASR_VoiceCommand_StartRecording\r\n");
		}
		if(!strcmp("Menu",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_OpenMenu;
			PRINTF("ASR_VoiceCommand_OpenMenu\r\n");
		}
	}
	if(!strcmp("MediaPlayer",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_MediaPlayer;
		if(!strcmp("play",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_Play;
			PRINTF("ASR_VoiceCommand_Play\r\n");
		}
		if(!strcmp("pause",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_Pause;
			PRINTF("ASR_VoiceCommand_Pause\r\n");
		}
		if(!strcmp("previous",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_Previous;
			PRINTF("ASR_VoiceCommand_Previous\r\n");
		}
		if(!strcmp("next",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_Next;
			PRINTF("ASR_VoiceCommand_Next\r\n");
		}
		if(!strcmp("up",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_VolumeUpMediaplayer;
			PRINTF("ASR_VoiceCommand_VolumeUpMediaplayer\r\n");
		}
		if(!strcmp("down",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_VolumeDownMediaplayer;
			PRINTF("ASR_VoiceCommand_VolumeDownMediaplayer\r\n");
		}
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeMediaplayer;
			PRINTF("ASR_VoiceCommand_GoHomeMediaplayer\r\n");
		}
	}
	if(!strcmp("MusicPlayer",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_MusicPlayer;
		if(!strcmp("play",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_PlayMusic;
			PRINTF("ASR_VoiceCommand_PlayMusic\r\n");
		}
		if(!strcmp("pause",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_PauseMusic;
			PRINTF("ASR_VoiceCommand_PauseMusic\r\n");
		}
		if(!strcmp("previous",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_PreviousSong;
			PRINTF("ASR_VoiceCommand_PreviousSong\r\n");
		}
		if(!strcmp("next",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_NextSong;
			PRINTF("ASR_VoiceCommand_NextSong\r\n");
		}
		if(!strcmp("up",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_VolumeUpMusicplayer;
			PRINTF("ASR_VoiceCommand_VolumeUpMusicplayer\r\n");
		}
		if(!strcmp("down",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_VolumeDownMusicplayer;
			PRINTF("ASR_VoiceCommand_VolumeDownMusicplayer\r\n");
		}
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeMusicplayer;
			PRINTF("ASR_VoiceCommand_GoHomeMusicplayer\r\n");
		}
	}
	if(!strcmp("Translation",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_Translation;
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeTranslation;
			PRINTF("ASR_VoiceCommand_GoHomeTranslation\r\n");
		}
	}
	if(!strcmp("VideoAI",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_VideoAi;
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeVideoAi;
			PRINTF("ASR_VoiceCommand_GoHomeVideoAi\r\n");
		}
	}
	if(!strcmp("VideoRecording",CurrentIntentName))
	{
		PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent=ASR_Menu_VideoRecording;
		if(!strcmp("Home",CurrentTagName))
		{
			PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName=ASR_VoiceCommand_GoHomeVideoRecording;
			PRINTF("ASR_VoiceCommand_GoHomeVideoRecording\r\n");
		}
	}

	#if defined(MASK_SIE)
		if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent!=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu)
			PRINTF("Internal Menu logic wrong, this should not happen\r\n");
	#endif
}

AUDIO_vit_st	vitPluginParams;

VIT_ReturnStatus_en VIT_ModelInfo( VIT_OperatingMode_en operatingMode )
{
    VIT_ReturnStatus_en     VIT_Status;                             /* Function call status */
    /*
    *   VIT Get Model Info (OPTIONAL)
    *       To retrieve information on the VIT_Model registered in VIT:
    *               - Model Release Number, number of commands supported
    *               - WakeWords supported (when info is present)
    *               - list of commands (when info is present)
    *
    */
    VIT_ModelInfo_st Model_Info;
    VIT_Status = VIT_GetModelInfo(&Model_Info);
    if (VIT_Status != VIT_SUCCESS)
    {
        PRINTF("	VIT_GetModelInfo error : %d\r\n", VIT_Status);
        return VIT_INVALID_MODEL;
    }

    if (!Model_Info.WW_VoiceCmds_Strings)               // Check here if Model is containing WW and CMDs strings
    {
        PRINTF("    VIT_Model integrating WakeWord and Voice Commands strings : NO\r\n");
    }
    else
    {
        const char* ptr;

        PRINTF("    VIT_Model integrating WakeWord and Voice Commands strings : YES\r\n");
        PRINTF("    WakeWords supported : \r\n");
        ptr = Model_Info.pWakeWord_List;
        if (ptr != PL_NULL)
        {
            for (PL_UINT16 i = 0; i < Model_Info.NbOfWakeWords; i++)
            {
                PRINTF("     '%s' \r\n", ptr);
                ptr += strlen(ptr) + 1;                 // to consider NULL char
            }
        }

        // Voice command mode is now set only if wake word detected TODO
        //if( ( operatingMode & VIT_VOICECMD_ENABLE ) == VIT_VOICECMD_ENABLE  )
        //{
        	PRINTF("\n    Intent slot tag names supported : \r\n");
			ptr = Model_Info.pIntentSlotTag_List;
			if (ptr != PL_NULL)
			{
				for (PL_UINT16 i = 0; i < Model_Info.NbOfIntentSlotTag; i++)
				{
					PRINTF("     '%s' \r\n", ptr);
					ptr += strlen(ptr) + 1;                 // to consider NULL char
				}
			}
        //}

        PRINTF("\r\n");
    }
    return VIT_SUCCESS;
}

status_t initSetVITParam( AUDIO_vit_st* p_definitionVIT )
{
	status_t   retStatus 		 = kStatus_Success;

	VIT_InstanceParams_st* p_VITInstParamConfig 	= &p_definitionVIT->vitInstParams;
	//VIT_ControlParams_st*  p_VITControlParamConfig  = &p_definitionVIT->vitControlParams;


	/*******************************/
	/* SET VIT INSTANCE PARAMETERS */
	p_VITInstParamConfig->SampleRate_Hz		= 16000;			// Set VIT sample rate
	p_VITInstParamConfig->SamplesPerFrame	= p_definitionVIT->vitConfig.framesize;	// Set VIT samples per frame
	p_VITInstParamConfig->DeviceId			= VIT_IMXRT600;							// Set Device ID
	p_VITInstParamConfig->APIVersion		= VIT_API_VERSION;						// Set VIT library version

	if(		AUDIO_VIT_USE_CONVERSA_TXOUT == p_definitionVIT->vitConfig.vitInputSelect  // If VIT uses Conversa TxOut or Conversa BfOut
		||  AUDIO_VIT_USE_CONVERSA_BFOUT == p_definitionVIT->vitConfig.vitInputSelect
	  )
	{
		p_VITInstParamConfig->NumberOfChannel	= 1;									// Set number of input channels used
	}
	else
	{
		p_VITInstParamConfig->NumberOfChannel	= 1;//p_configParamTx->channelNumber;		// Set number of input channels used in the Tx path
	}

	/******************************/
	/* SET VIT CONTROL PARAMETERS */
	//p_VITControlParamConfig->OperatingMode     = p_definitionVIT->vitConfig.operatingMode;	//Set VIT operating mode
	//p_VITControlParamConfig->Command_Time_Span = p_definitionVIT->vitConfig.timeSpan; 	    // Set VIT time span
	//p_VITControlParamConfig->Feature_LowRes    = PL_FALSE;									// Set VIT low res in false

	/******************************/
	/* SET VIT HANDLE             */
	p_definitionVIT->vitHandle = NULL;									// force VIT Handle to null address for correct memory initialization

	/* At this step VIT instance is ready to be created */
	return retStatus;
}

status_t initCheckDefinitionParameterVIT( AUDIO_vit_st* p_definitionVIT )
{
	status_t retStatus 		 		= kStatus_Success;

	VIT_InstanceParams_st* p_VITInstParamConfig    = &p_definitionVIT->vitInstParams;
	VIT_ControlParams_st*  p_VITControlParamConfig = &p_definitionVIT->vitControlParams;

	/***************************/
	/* CHECK PARAMETER POINTER */
	if (p_definitionVIT == PL_NULL)
	{
		PRINTF("FAIL - initCheckDefinitionParameterVIT NULL input pointer 1");
		return kStatus_NullPointer;
	}

	/***************************/
	/* CHECK VIT MODEL POINTER */
	if (p_definitionVIT->vitModel.p_address == PL_NULL)
	{
		PRINTF("FAIL - initCheckDefinitionParameterVIT NULL input pointer 2");
		return kStatus_NullPointer;
	}

	/***********************************************************************/
	/* CHECK PARAMETERS MATCH WITH VIT LIB LIMITATION */

	// Check sample rate
	if( p_VITInstParamConfig->SampleRate_Hz != VIT_SAMPLE_RATE )
	{
		PRINTF("FAIL - DSP: Samples sate => VIT  parameter (%d) not equal to VIT library sample rate(%d)\r\n", p_VITInstParamConfig->SampleRate_Hz, VIT_SAMPLE_RATE);
		retStatus = kStatus_NotCompatible;
	}

	// Check samples per frame
	if( p_VITInstParamConfig->SamplesPerFrame != VIT_SAMPLES_PER_30MS_FRAME )
	{
		PRINTF("FAIL - DSP: Samples per frame => VIT parameter (%d) not equal to VIT library samples per frame (%d)\r\n", p_VITInstParamConfig->SamplesPerFrame, VIT_SAMPLES_PER_30MS_FRAME);
		retStatus = kStatus_NotCompatible;
	}

	// Check samples number of channels
	if( p_VITInstParamConfig->NumberOfChannel > VIT_MAX_NUMBER_OF_CHANNEL )
	{
		PRINTF("FAIL - DSP: Num. of Channels => VIT parameter (%d) not equal to VIT library number of channels (%d)\r\n", p_VITInstParamConfig->NumberOfChannel, VIT_MAX_NUMBER_OF_CHANNEL);
		retStatus = kStatus_NotCompatible;
	}

	// Check device Id
	if( p_VITInstParamConfig->DeviceId != VIT_IMXRT600 )
	{
		PRINTF("FAIL - DSP: Device ID => VIT  parameter (%d) not equal to VIT library device ID (%d)\r\n", p_VITInstParamConfig->DeviceId, VIT_IMXRT600);
		retStatus = kStatus_NotCompatible;
	}

	// Check API version
	if( p_VITInstParamConfig->APIVersion != VIT_API_VERSION )
	{
		PRINTF("FAIL - DSP: API version => VIT  parameter (%d) not equal to VIT library API version (%d)\r\n", p_VITInstParamConfig->APIVersion, VIT_API_VERSION);
		retStatus = kStatus_NotCompatible;
	}

	// Check operating mode
	if( (p_VITControlParamConfig->OperatingMode & ( VIT_WAKEWORD_ENABLE | VIT_VOICECMD_ENABLE) ) == 0)
	{
		PRINTF("FAIL - DSP: Operating mode => VIT  parameter (%d) not equal to VIT library operating mode(%d)\r\n", p_VITControlParamConfig->OperatingMode, ( VIT_WAKEWORD_ENABLE | VIT_VOICECMD_ENABLE ));
		retStatus = kStatus_NotCompatible;
	}

	// Check time span
	if( p_VITControlParamConfig->Command_Time_Span > VIT_COMMAND_TIME_SPAN )
	{
		PRINTF("FAIL - DSP: TIme Span => VIT  parameter (%f) grater than VIT library time span(%f)\r\n", p_VITControlParamConfig->Command_Time_Span, 5.0);
		retStatus = kStatus_NotCompatible;
	}

	return retStatus;

}

static asr_session_t s_asrSession = ASR_SESSION_STOPPED;
static PL_INT8  *pMemory[PL_NR_MEMORY_REGIONS];

static void asr_set_state(asr_session_t state, VIT_Handle_t* VITHandlePtr, VIT_ControlParams_st* CntrParaPtr)
{
    VIT_ReturnStatus_en VIT_Status = VIT_ERROR_UNDEFINED;
    s_asrSession                   = state;

    switch (state)
    {
        case ASR_SESSION_STOPPED:
            PRINTF("[ASR] Session stopped\r\n");
            CntrParaPtr->OperatingMode = VIT_LPVAD_ENABLE;
            break;

        case ASR_SESSION_WAKE_WORD:
        	CntrParaPtr->OperatingMode = VIT_OPERATING_MODE_WW;
            break;

        case ASR_SESSION_INTENT:
        	CntrParaPtr->OperatingMode = VIT_OPERATING_MODE_S2I;
            break;

        default:
            PRINTF("Unknown state %d\r\n", state);
            break;
    }

    if (s_asrSession != ASR_SESSION_STOPPED)
    {
    	CntrParaPtr->Feature_LowRes    = PL_FALSE;
    	CntrParaPtr->Command_Time_Span = VIT_COMMAND_TIME_SPAN;

    	CntrParaPtr->Input_Noise_Floor = -90;
    	CntrParaPtr->Noise_Floor_Threshold = 0;
    	CntrParaPtr->WakeWordDelayRecovering = PL_FALSE;

        VIT_Status = VIT_SetControlParameters(*VITHandlePtr, CntrParaPtr);
#if SELF_WAKE_UP_PROTECTION
        VIT_Status = VIT_SetControlParameters(VITHandleSelfWake, &VITControlParams);
#endif /* SELF_WAKE_UP_PROTECTION */

        if (VIT_Status != VIT_SUCCESS)
        {
            configPRINTF("[ASR] %d state failed %d\r\n", state, VIT_Status);
        }
    }
}

static VIT_ReturnStatus_en VIT_Deinit(AUDIO_vit_st* p_definitionVIT)
{
    VIT_ReturnStatus_en VIT_Status = VIT_SUCCESS;
#if 1
    for (int i = 0; i < PL_NR_MEMORY_REGIONS; i++)
    {
		free(pMemory[i]);
		PRINTF("RT685 DSP: VIT memory region %d is freed: address: %x\r\n", i, (void *)pMemory[i]);
    }
#else

	PL_MemoryTable_st*	   		p_VITMemoryTable		= &p_definitionVIT->vitMemoryTable;
	VIT_InstanceParams_st* 		p_VITInstParamConfig    = &p_definitionVIT->vitInstParams;
	VIT_Handle_t*          		p_VITHandle			    = &p_definitionVIT->vitHandle;


    VIT_Status = VIT_GetMemoryTable(p_VITHandle,p_VITMemoryTable, p_VITInstParamConfig);

    if (VIT_Status != VIT_SUCCESS)
    {
        PRINTF("VIT_GetMemoryTable error: %d\r\n", VIT_Status);
    }

    if (VIT_Status == VIT_SUCCESS)
    {
        // Free the MEM tables
        for (int i = 0; i < PL_NR_MEMORY_REGIONS; i++)
        {
			if ((p_VITMemoryTable->Region[i].Size != 0)&&(p_VITMemoryTable->Region[i].pBaseAddress!=NULL))
			{
				//memset(pMemory[i], 0, p_VITMemoryTable->Region[i].Size);
				PRINTF("RT685 DSP: VIT memory region %d is freed: address: %x, size is %d\r\n", i, (void *)p_VITMemoryTable->Region[i].pBaseAddress, p_VITMemoryTable->Region[i].Size);
				free(p_VITMemoryTable->Region[i].pBaseAddress);
				//pMemory[i] = NULL;
			}
			#if SELF_WAKE_UP_PROTECTION
					if (VITMemoryTableSelfWake.Region[i].Size != 0)
					{
						memset(pMemorySelfWake[i], 0, VITMemoryTableSelfWake.Region[i].Size);
						pMemorySelfWake[i] = NULL;
					}
			#endif /* SELF_WAKE_UP_PROTECTION */
        }
    }
#endif
    return VIT_Status;
}

void initCreateVIT( AUDIO_vit_st* p_definitionVIT )
{
	status_t   			retStatus 		 	= kStatus_Success;
	VIT_ReturnStatus_en VIT_Status  		= VIT_SUCCESS;
	VIT_LibInfo_st 		Lib_Info;

	PL_UINT16 		order[PL_NR_MEMORY_REGIONS];
	PL_INT16  		i, j, minIdx;         						// loop index
	PL_INT32 		temp32;     								// temporary address
	PL_BOOL 		InitPhase_Error					= PL_FALSE;

	//settings were done in MCU code, in the orignal demo
	p_definitionVIT->vitConfig.vitInputSelect = AUDIO_VIT_USE_CONVERSA_BFOUT;		// VIT uses Conversa TxOut as input
	#ifdef WHO_IS_TALKING_PRESENT
		//p_definitionVIT->vitConfig.operatingMode  = VIT_WAKEWORD_ENABLE ;			// Enable VIT for WW recognition. It will be update to CMD detection by the application code when a WW is detected and valid
	#else
		//p_definitionVIT->vitConfig.operatingMode  = VIT_WAKEWORD_ENABLE + VIT_VOICECMD_ENABLE;	// Enable VIT for WW and VCMD recognition.
	#endif
	p_definitionVIT->vitConfig.framesize	  = VIT_SAMPLES_PER_30MS_FRAME;		// Set VIT framesize (160 (10ms) or 480(30ms))
	//p_definitionVIT->vitConfig.timeSpan       = 5.0;								// Set VIT time span for command detection
	p_definitionVIT->vitConfig.num_mic        = 1;


	initSetVITParam(p_definitionVIT);

	PL_MemoryTable_st*	   		p_VITMemoryTable		= &p_definitionVIT->vitMemoryTable;
	VIT_InstanceParams_st* 		p_VITInstParamConfig    = &p_definitionVIT->vitInstParams;
	VIT_ControlParams_st*  		p_VITControlParamConfig = &p_definitionVIT->vitControlParams;
	AUDIO_VITModel_st*     		p_VITModel			    = &p_definitionVIT->vitModel;
	VIT_Handle_t*          		p_VITHandle			    = &p_definitionVIT->vitHandle;
	VIT_parameter_config_t * 	p_VITConfig   			= &p_definitionVIT->vitConfig;


	/***************************/
	/* CHECK PARAMETER POINTER */
	if ( p_definitionVIT == NULL )
	{
		return;
	}

	// get VIT library information
	VIT_Status = VIT_GetLibInfo(&Lib_Info);
	if (VIT_Status != VIT_SUCCESS)
	{
		PRINTF("    VIT_GetLibInfo error : %d\n", VIT_Status);
		retStatus = kStatus_Fail;								// We can exit from here since memory is not allocated yet
	}
	PRINTF("    VIT Library v%04x\r\n", Lib_Info.VIT_LIB_Release);

	// set VIT model

	p_VITModel->vitModelMem = VIT_MODEL_IN_FAST_MEM;					// VIT model is stored in RAM
	//p_VITModel->vitModelMem = VIT_MODEL_IN_SLOW_MEM;					// VIT model is stored in RAM
	p_VITModel->p_address   = (const PL_UINT8*)&VIT_Model_en[0];		// VIT model address
	p_VITModel->size_byte   = sizeof(VIT_Model_en);				    // VIT model size in bytes

	if( p_VITModel->vitModelMem == VIT_MODEL_IN_SLOW_MEM )
	{
		PRINTF("    \r\nVIT model address: 0x%x (VIT_MODEL_IN_SLOW_MEM)\r\n", p_VITModel->p_address);
		VIT_Status = VIT_SetModel(p_VITModel->p_address, p_VITModel->vitModelMem);
		if (VIT_Status != VIT_SUCCESS)
		{
			retStatus = kStatus_Fail;       						// We can exit from here since memory is not allocated yet
			PRINTF("FAIL - DSP: VIT_SetModel (%d)\r\n", VIT_Status);
		}
	}
	else if( p_VITModel->vitModelMem == VIT_MODEL_IN_FAST_MEM )
	{
		PRINTF("    \r\nVIT model address: 0x%x (VIT_MODEL_IN_FAST_MEM)\r\n",VIT_Model_en);
		VIT_Status = VIT_SetModel(VIT_Model_en, VIT_MODEL_IN_FAST_MEM);
		if (VIT_Status != VIT_SUCCESS)
		{
			retStatus = kStatus_Fail;       						// We can exit from here since memory is not allocated yet
			PRINTF("FAIL - DSP: VIT_SetModel (%d)\r\n", VIT_Status);
		}
	}

	VIT_Status = VIT_ModelInfo( p_VITConfig->operatingMode );
	if (VIT_Status != VIT_SUCCESS)
	{
		retStatus = kStatus_Fail;								// We can exit from here since memory is not allocated yet
		PRINTF("FAIL - DSP: VIT_GetModelInfo Invalid VIT model \r\n");
	}

	VIT_Status = VIT_GetMemoryTable( PL_NULL, p_VITMemoryTable, p_VITInstParamConfig );
	if (VIT_Status != VIT_SUCCESS)
	{
		retStatus = kStatus_Fail;
		PRINTF("FAIL - DSP: VIT_GetMemoryTable error : %d\r\n", VIT_Status);
	}

	// initialize order variable
	for (i = 0; i < PL_NR_MEMORY_REGIONS; i++)
	{
		order[i] = i;
	}

	// Sort region indexes by region size
	for (i = 0; i < (PL_NR_MEMORY_REGIONS - 1); i++)
	{
		minIdx = i;
		for (j = i + 1; j < PL_NR_MEMORY_REGIONS; j++)
			if (p_VITMemoryTable->Region[order[j]].Size < p_VITMemoryTable->Region[order[minIdx]].Size)
				minIdx = j;

		/* Swap indexes */
		temp32        = order[minIdx];
		order[minIdx] = order[i];
		order[i]      = temp32;
	}


	// Reserve memory space : Malloc for each memory type
    for (j = (PL_NR_MEMORY_REGIONS - 1); j >= 0; j--)
    {
        if (p_VITMemoryTable->Region[order[j]].Size != 0)
        {
            // reserve memory space
            // NB: p_VITMemoryTable->Region[PL_MEMREGION_PERSISTENT_FAST_DATA] should be allocated
            //      in the fastest memory of the platform (when possible) - this is not the case in this example.
            pMemory[j] = malloc(p_VITMemoryTable->Region[order[j]].Size + MEMORY_ALIGNMENT);
            if (!pMemory[j])
            {
                return;
            }
            p_VITMemoryTable->Region[order[j]].pBaseAddress = (void *)pMemory[j];
        	PRINTF("RT685 DSP: VIT memory region %d is allocated: address: %x, size is %d\r\n", j, (void *)pMemory[j], p_VITMemoryTable->Region[order[j]].Size);
        }
    }

    // create VIT Instance
	VIT_Status = VIT_GetInstanceHandle( p_VITHandle,
										p_VITMemoryTable,
										p_VITInstParamConfig );
	if (VIT_Status != VIT_SUCCESS)
	{
		InitPhase_Error = PL_TRUE;
		PRINTF("VIT_GetInstanceHandle error : %d\r\n", VIT_Status);
	}


	// test the reset (OPTIONAL)
	if (!InitPhase_Error)
	{
		VIT_Status = VIT_ResetInstance(*p_VITHandle);
		if (VIT_Status != VIT_SUCCESS)
		{
			InitPhase_Error = PL_TRUE;
			PRINTF("VIT_ResetInstance error : %d\r\n", VIT_Status);
		}
	}

#if SELF_WAKE_UP_PROTECTION
    if (VIT_SUCCESS == VIT_Status)
    {
        VIT_Status = initialize_asr_self_wake_up();
    }
#endif /* SELF_WAKE_UP_PROTECTION */

    if (!InitPhase_Error)
    {
		#if 0
			/* Set and Apply VIT control parameters */
			if (appAsrShellCommands.asrMode == ASR_MODE_CMD_ONLY)
			{
				//asr_session_t state, VIT_Handle_t* VITHandlePtr, VIT_ControlParams_st* CntrParaPtr
				asr_set_state(ASR_SESSION_INTENT,p_VITHandle,p_VITControlParamConfig);
			}
			else
			{
				//asr_session_t state, VIT_Handle_t* VITHandlePtr, VIT_ControlParams_st* CntrParaPtr
				asr_set_state(ASR_SESSION_WAKE_WORD,p_VITHandle,p_VITControlParamConfig);
			}
		#else
	    	//asr_session_t state, VIT_Handle_t* VITHandlePtr, VIT_ControlParams_st* CntrParaPtr
	        asr_set_state(ASR_SESSION_WAKE_WORD,p_VITHandle,p_VITControlParamConfig);
		#endif
    }

	if( p_definitionVIT->vitConfig.vitInputSelect == AUDIO_VIT_USE_RAW_MIC )
	{
		PRINTF("    VIT input: microphone raw data\r\n");
	}
	else if( p_definitionVIT->vitConfig.vitInputSelect == AUDIO_VIT_USE_CONVERSA_TXOUT )
	{
		PRINTF("    VIT input: Conversa TxOut\r\n");
	}
	else if( p_definitionVIT->vitConfig.vitInputSelect == AUDIO_VIT_USE_CONVERSA_BFOUT )
	{
		PRINTF("    VIT input: Conversa BfOut\r\n");
	}
	else if( p_definitionVIT->vitConfig.vitInputSelect == AUDIO_VIT_USE_CONVERSA_AECOUT )
	{
		PRINTF("    VIT input: Conversa AEC Out\r\n");
	}
	else if( p_definitionVIT->vitConfig.vitInputSelect == AUDIO_VIT_USE_CONVERSA_NLPOUT )
	{
		PRINTF("    VIT input: Conversa NLP Out\r\n");
	}

	/* WHO IS TALKING BEAM */
#ifdef CONVERSA_PRESENT
#ifdef WHO_IS_TALKING_PRESENT
	PRINTF("    VIT wake word whoIsTalking: Threshold at %.2f\r\n", ((PL_FLOAT)(WHO_IS_TALKING_BEAM_THRESHOLD) / 100));
#endif
#endif

	// check initialization error
	if (!InitPhase_Error)
	{
	    InitCirAudioBuf_S16(&VitCircBuff, VitCircBuff_DataArea, (APP_VIT_FRAME_SIZE_InSAMPLEs*2));
	    InitCirAudioBuf_S16(&VitCircBuff_RawMic, VitCircBuff_DataArea_RawMic, (APP_VIT_FRAME_SIZE_InSAMPLEs*2));
	}

	// check initialization error
	if (InitPhase_Error)
	{
		retStatus = kStatus_Fail;	// if error in initialization, return fail
	}
	/* At this step VIT instance is created */
	initCheckDefinitionParameterVIT(p_definitionVIT);	//if no printing in it, then good
}

void InitVit(void)
{
	VIT_ReturnStatus_en 	VIT_Status				= VIT_SUCCESS;
	#if 0
		//only a test, can be closed
		void *HeapPtr1;
		void *HeapPtr2;

		HeapPtr1=GetCurrentHeapTail(3000);
		initCreateVIT(&vitPluginParams);
		VIT_Deinit(&vitPluginParams);
		HeapPtr2=GetCurrentHeapTail(3000);

		if(HeapPtr1==HeapPtr2)
		{
			//PRINTF("RT685 DSP: heap base address was, %x\r\n",    (U32)HeapPtr1);
			//PRINTF("RT685 DSP: heap base address now is, %x\r\n", (U32)HeapPtr2);
			PRINTF("RT685 DSP: VIT_Deinit is successful \r\n");
		}else
		{
			PRINTF("RT685 DSP: heap base address was, %x\r\n",    (U32)HeapPtr1);
			PRINTF("RT685 DSP: heap base address now is, %x\r\n", (U32)HeapPtr2);
			PRINTF("RT685 DSP: VIT_Deinit is NOT successful \r\n");
		}
		PRINTF("RT685 DSP: heap base address, %x\r\n", (U32)HeapPtr2);
	#endif

	InitalVitMaskSetupIsDone=0;
	PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
	PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=ASR_Menu_Home;
	initCreateVIT(&vitPluginParams);
}

status_t swProcessVIT( AUDIO_vit_st* 			 p_definitionVIT,
					   PL_INT16*     			 p_inputAudioData,
					   PL_INT16*     			 p_inputAudioData_RawMic,
					   PL_INT32	     			 inputAudioDataSize_sample,
					   VIT_DetectionStatus_en* 	 p_vitDetectionResult,
					   PL_UINT16* 	 			 p_vitVcDetectionId)
{
	status_t   				retStatus 		 		= kStatus_Success;

	VIT_Handle_t*          	p_VITHandle			  	= &p_definitionVIT->vitHandle;
	VIT_ControlParams_st*  	p_VITControlParamConfig = &p_definitionVIT->vitControlParams;

	VIT_ReturnStatus_en 	VIT_Status				= VIT_SUCCESS;
	VIT_DetectionStatus_en 	VIT_DetectionResults 	= VIT_NO_DETECTION;	// VIT detection result
    static VIT_WakeWord_st s_WakeWord;

	/***************************/
	/* CHECK PARAMETER POINTER */
	/***************************/
	// Check input pointer
	if ( (p_inputAudioData == NULL) || (p_definitionVIT == NULL) )
	{
		return kStatus_NullPointer;
	}

#if defined(MASK_SIE)
	// read hedaer file regarding intent device status
	//const PL_BOOL            *pIntentSlotMask = VIT_Intent_Slot_Mask_AIConversation;
#endif

	/***************************/
	/* VIT PROCESS
	 *
	 *    VIT process:
	 *    	- VIT process
	 *    	- Check VIT detection results
	 *    	- Share WW and Command detection
	 */
	CurrentTagValue1=NULL;

	VIT_Status = VIT_Process( *p_VITHandle, p_inputAudioData_RawMic, p_inputAudioData, &VIT_DetectionResults );

	if (VIT_Status != VIT_SUCCESS)
	{
		PRINTF("VIT_Process error : %d\n", VIT_Status);
		return VIT_SYSTEM_ERROR;                                            // will stop processing VIT and go directly to MEM free
	}
#if 1
    else if ((VIT_DetectionResults == VIT_WW_DETECTED)
            || (VIT_DetectionResults == VIT_INTENT_DETECTED))
    {
        if (VIT_DetectionResults == VIT_WW_DETECTED)
        {
            VIT_Status = VIT_GetWakeWordFound(*p_VITHandle, &s_WakeWord);

            if (VIT_Status != VIT_SUCCESS)
            {
                PRINTF("VIT_GetWakeWordFound error: %d\r\n", VIT_Status);
            }
            else if (s_WakeWord.Id > 0)
            {
            	//this is to start playing an OPUS file as the WW detecting prompt sound --- a short quick OPUS sound
            	CommandToPlayWakeSound();
				//this is to let MCU side print the WW detected event
				PtrVarBlockSharedByDspAndMcu->WWIsDetected=1;


                g_wakeWordLength = s_WakeWord.StartOffset;

                PRINTF("[ASR] Wake Word: %s(%d) --- In %s menu\r\n", (s_WakeWord.pName == PL_NULL) ? "UNDEF" : s_WakeWord.pName, s_WakeWord.Id, MenuItemName[PtrVarBlockSharedByDspAndMcu->CurVoiceMenu]);
				#if ASE_DetectDisplayInUacWav==1
					ASR_WavPulse=ASR_WavPulse_WakeWordDetected;
				#endif
                //configPRINTF(("VIT_Process max MIPS: %d\r\n", Cyc2Max));

				#if 1
                	asr_set_state(ASR_SESSION_INTENT,p_VITHandle, p_VITControlParamConfig);
					//xTaskNotify(appTaskHandle, kWakeWordDetected, eSetBits); 	//inform other task a wake word is detected

					#if defined(MASK_SIE)
						if(!InitalVitMaskSetupIsDone)
						{
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");

							InitalVitMaskSetupIsDone=1;
						}
					#endif

				#else
					/* VIT supports only one language at a time so it does not offer the detected
					 * language because it is the one used during initialization. */
					oob_demo_control.language   = appAsrShellCommands.activeLanguage;
					oob_demo_control.commandSet = appAsrShellCommands.demo;

					if ((appAsrShellCommands.asrMode == ASR_MODE_WW_AND_CMD) || (appAsrShellCommands.asrMode == ASR_MODE_WW_AND_MUL_CMD))
					{
						asr_set_state(ASR_SESSION_INTENT);
					}

					// Notify App Task Wake Word Detected
					xTaskNotify(appTaskHandle, kWakeWordDetected, eSetBits);
				#endif

            }
        }
        else if (VIT_DetectionResults == VIT_INTENT_DETECTED)
        {
            /* Retrieve the Voice Speech Intent detected */
            VIT_Status = VIT_GetIntentFound(*p_VITHandle, &SpeechIntent);
            if (VIT_Status != VIT_SUCCESS)
            {
                PRINTF("VIT_GetSpeechIntentFound error: %d\r\n", VIT_Status);
            }
            else
            {
                if (SpeechIntent.Slot_Tag_count != 0)
                {
                    //if (APP_LAYER_FilterIntent() == false)
                	if(1)
                    {
                        for (PL_INT16 i=(SpeechIntent.Slot_Tag_count - 1); i>=0; i--)
                        {
                        	PRINTF("Intent:                        %s\r\n", SpeechIntent.pIntent[i]);
                        	CurrentIntentName=                              SpeechIntent.pIntent[i];
                        	PRINTF("TagName:                       %s\r\n", SpeechIntent.pSlot_Tag[i]);
                        	CurrentTagName=                                 SpeechIntent.pSlot_Tag[i];
                        	PRINTF("TagValue_count:                %d\r\n", SpeechIntent.Slot_Tag_Value_count[i]);
                            for (PL_INT16 j=(SpeechIntent.Slot_Tag_Value_count[i] - 1); j>=0; j--)
                            {
                            	PRINTF("Tag value:                     %s\r\n", SpeechIntent.pSlot_Tag_Value[(i*MAX_NUMBER_WORDS_PER_SLOT_TAG_VALUE)+j]);

                            	//assign the string to CurrentTagValue1 only for the first time in this loop
                            	if(CurrentTagValue1==NULL)
                            		CurrentTagValue1=SpeechIntent.pSlot_Tag_Value[(i*MAX_NUMBER_WORDS_PER_SLOT_TAG_VALUE)+j];
                            }
                        }

                        IdentifyCurrentVoiceCommand();

                        g_sampleCount = 0;

						#if ASE_DetectDisplayInUacWav==1
							ASR_WavPulse=ASR_WavPulse_VoiceCmdDetected;
						#endif

						#if 1
							asr_set_state(ASR_SESSION_WAKE_WORD,p_VITHandle, p_VITControlParamConfig);
							//xTaskNotify(appTaskHandle, kVoiceCommandDetected, eSetBits);	//inform other task a voice command is detected
						#else
							if (appAsrShellCommands.asrMode == ASR_MODE_WW_AND_MUL_CMD)
							{
								asr_set_state(ASR_SESSION_INTENT,p_VITHandle, p_VITControlParamConfig);
							}
							else if ((appAsrShellCommands.asrMode == ASR_MODE_WW_AND_CMD) ||
									 (appAsrShellCommands.asrMode == ASR_MODE_PTT))
							{
								asr_set_state(ASR_SESSION_WAKE_WORD,p_VITHandle, p_VITControlParamConfig);
							}

							xTaskNotify(appTaskHandle, kVoiceCommandDetected, eSetBits);
						#endif
                    }
                    else
                    {
                        PRINTF("\r\nINTENT filtered: %s\r\n", SpeechIntent.pIntent[(SpeechIntent.Slot_Tag_count - 1)]);
                    }
                }
            }
        }
    }

	if (s_asrSession == ASR_SESSION_INTENT)
	{
		g_sampleCount += VIT_SAMPLES_PER_30MS_FRAME;

		if (g_sampleCount > 16000 / 1000 * 5000)
		{
			g_sampleCount = 0;

			asr_set_state(ASR_SESSION_WAKE_WORD,p_VITHandle, p_VITControlParamConfig);
			PRINTF("No voice command is detected. Time is out.\r\n");

			// Notify App Task Timeout
			//xTaskNotify(appTaskHandle, kTimeOut, eSetBits);
		}
	}
#if 0
		// reinitialize the ASR engine if language set was changed
		if (appAsrShellCommands.asrCfg & (ASR_CFG_DEMO_LANGUAGE_CHANGED | ASR_CFG_CMD_INFERENCE_ENGINE_CHANGED))
		{
			VIT_Deinit();
			VIT_Init();
			appAsrShellCommands.asrCfg &= ~(ASR_CFG_DEMO_LANGUAGE_CHANGED | ASR_CFG_CMD_INFERENCE_ENGINE_CHANGED);
			oob_demo_control.language   = appAsrShellCommands.activeLanguage;

			xTaskNotify(appTaskHandle, kAsrModelChanged, eSetBits);
		}

		if (appAsrShellCommands.asrCfg & ASR_CFG_MODE_CHANGED)
		{
			appAsrShellCommands.asrCfg &= ~ASR_CFG_MODE_CHANGED;

			if(appAsrShellCommands.asrMode == ASR_MODE_CMD_ONLY)
			{
				asr_set_state(ASR_SESSION_INTENT,p_VITHandle, p_VITControlParamConfig);
			}
			else
			{
				asr_set_state(ASR_SESSION_WAKE_WORD,p_VITHandle, p_VITControlParamConfig);
			}
		}
	#endif

#else
	VIT_VoiceCommand_st 	VoiceCommand;                   	        // VIT Voice Command info
	VIT_WakeWord_st 		WakeWord;                    		        // VIT Wakeword info

	/* Check if Wake Word was detected */
	if (VIT_DetectionResults == VIT_WW_DETECTED)
	{
		g_vitDetectionFrame = g_vitFramecount;
		VIT_Status = VIT_GetWakeWordFound( *p_VITHandle, &WakeWord);

		if (VIT_Status != VIT_SUCCESS)
		{
			PRINTF("  VIT_GetWakeWordFound error : %d\r\n", VIT_Status);
			return VIT_SYSTEM_ERROR;
		}
		else
		{
			if(0 != WakeWord.Id)
			{
				// Retrieve WakeWord Name : OPTIONAL
				// Check first if WakeWord string is present
				if (WakeWord.pName != PL_NULL)
				{
				#ifdef WHO_IS_TALKING_PRESENT
					// check if speech is inside the beam
					// compute whoIsTalkingBeam in the WakeWord detection offset
					g_whoIsTalkingBeam_WWOffsetRange = process_whoIsTalkingBeam_offsetRange(&g_conversaFBfPfWeight_CircBuffInst[0],	    // circular buffer instance
																						    WakeWord.StartOffset, WakeWord.EndOffset);  // offset range
					if (g_whoIsTalkingBeam_WWOffsetRange == APP_WHO_IS_TALKING_NOT_VALID)
					{
						PRINTF(" - OUT\r\n");				// WW detected + not a valid speech
					}
					else
					{
						// FORCE VIT CMD MODE
						p_VITControlParamConfig->OperatingMode = VIT_VOICECMD_ENABLE;
						VIT_Status = VIT_SetControlParameters( *p_VITHandle, p_VITControlParamConfig );   // move VIT to command mode
						if (VIT_Status != VIT_SUCCESS)
						{
							PRINTF("VIT_SetControlParameters error : %d\r\n", VIT_Status);
						}

						//PRINT WAKEWORD ID
						PRINTF(" - %s ",  WakeWord.pName);  // WW detected + valid speech
					}
				#else
					PRINTF(" - %s ", WakeWord.pName);
				#endif
				#if defined(MASK_SIE)
				// VIT Configure Intent Slot Mask
				VIT_Status = VIT_ConfigureIntentSlotMask (p_VITHandle, pIntentSlotMask, sizeof(VIT_Intent_Slot_Mask_AIConversation)/sizeof(PL_BOOL));
				if (VIT_Status != VIT_SUCCESS)
				{
					//TestApp_VIT_HandleError("VIT_ConfigureIntentMask", Status);
				}
				#endif
				}
			}
		}
	}
	/* Check if Voice Command was detected */
	else if (VIT_DetectionResults == VIT_VC_DETECTED)
	{
		// Retrieve id of the Voice Command detected
		// String of the Command can also be retrieved (when WW and CMDs strings are integrated in Model)

		//VIT_Status = VIT_GetVoiceCommandFound( *p_VITHandle, &VoiceCommand );

		VIT_Status = VIT_GetIntentFound( *p_VITHandle, &VoiceCommand );


		//VIT_ReturnStatus_en VIT_GetIntentFound ( VIT_Handle_t pVIT_Instance,      VIT_Intent_st *pSpeechIntent        );


		if (VIT_Status != VIT_SUCCESS)
		{
			PRINTF("  VIT_GetVoiceCommandFound error : %d\r\n", VIT_Status);
			return VIT_SYSTEM_ERROR;                                              // will stop processing VIT and go directly to MEM free
		}
		else
		{
			//PRINTF(" - Voice Command detected %d", VoiceCommand.Id);

			// Retrieve CMD Name : OPTIONAL
			// Check first if CMD string is present
			if (VoiceCommand.pName != PL_NULL)
			{
			#ifdef WHO_IS_TALKING_PRESENT
				// FORCE VIT WW MODE
				p_VITControlParamConfig->OperatingMode = VIT_WAKEWORD_ENABLE;
				VIT_Status = VIT_SetControlParameters( *p_VITHandle, p_VITControlParamConfig );	// move to wake word mode
				if (VIT_Status != VIT_SUCCESS)
				{
					PRINTF("VIT_SetControlParameters error : %d\r\n", VIT_Status);
				}
			#else
				PRINTF(">> %s\r\n", VoiceCommand.pName);
			#endif
			}
			else
			{
				PRINTF("\r\n");
			}
			*p_vitVcDetectionId = VoiceCommand.Id;
		}
	}
#endif


	#if defined(MASK_SIE)
		if(CurrentTagValue1!=NULL)
		{
			//update PtrVarBlockSharedByDspAndMcu->PreVoiceMenu and PtrVarBlockSharedByDspAndMcu->CurVoiceMenu, and set new voice command mask
			switch(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandIntent)
			{
				case ASR_Menu_Home:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_TakePhoto)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							//take photo is not a sub menu anymore
							/*
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Photo, sizeof(VIT_Intent_Slot_Mask_Photo)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in TakePhoto menu\r\n");
							*/
						}
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_StartRecording)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_VideoRecording;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_VideoRecording, sizeof(VIT_Intent_Slot_Mask_VideoRecording)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in VideoRecording menu\r\n");
						}
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_StartTranslation)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Translation;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Translation, sizeof(VIT_Intent_Slot_Mask_Translation)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Translation menu\r\n");
						}
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_StartVideoAi)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_VideoAi;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_VideoAI, sizeof(VIT_Intent_Slot_Mask_VideoAI)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in VideoAi menu\r\n");
						}
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_StartAiConversation)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_AiConversation;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_AIConversation, sizeof(VIT_Intent_Slot_Mask_AIConversation)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in AiConversation menu\r\n");
						}
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoMediaPlayer)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_MediaPlayer;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_MediaPlayer, sizeof(VIT_Intent_Slot_Mask_MediaPlayer)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in MediaPlayer menu\r\n");
						}
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_OpenMenu)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_MainMenu;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Menu, sizeof(VIT_Intent_Slot_Mask_Menu)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in MainMenu menu\r\n");
						}
					break;
				case ASR_Menu_VideoAi:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoAi)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;
				case ASR_Menu_AiConversation:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeAiconversation)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;
				case ASR_Menu_Translation:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeTranslation)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;
				case ASR_Menu_VideoRecording:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeVideoRecording)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;
				case ASR_Menu_MediaPlayer:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMediaplayer)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;
				case ASR_Menu_PhoneCall:
					if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_RejectThePhone)
					{
						PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
						PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
						VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
						if (VIT_Status != VIT_SUCCESS)
							PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
						else
							PRINTF("Now in Home menu\r\n");
					}
					if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_HangUpThePhone)
					{
						PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
						PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
						VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
						if (VIT_Status != VIT_SUCCESS)
							PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
						else
							PRINTF("Now in Home menu\r\n");
					}
					break;
				case ASR_Menu_MusicPlayer:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMusicplayer)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;

				case ASR_Menu_MainMenu:
						if(PtrVarBlockSharedByDspAndMcu->CurrentVoiceCommandTagName==ASR_VoiceCommand_GoHomeMainMenu)
						{
							PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
							PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
							VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
							if (VIT_Status != VIT_SUCCESS)
								PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
							else
								PRINTF("Now in Home menu\r\n");
						}
					break;
			}

			CurrentTagValue1=NULL;
		}





		if(PtrVarBlockSharedByDspAndMcu->U32ControlPara[0])
		{
			//user button 1 is pressed --- go to musicplayer menu
			PtrVarBlockSharedByDspAndMcu->U32ControlPara[0]=0;

			if(PtrVarBlockSharedByDspAndMcu->CurVoiceMenu==ASR_Menu_Home)
			{
				PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
				PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_MusicPlayer;
				VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_MusicPlayer, sizeof(VIT_Intent_Slot_Mask_MusicPlayer)/sizeof(PL_BOOL));
				if (VIT_Status != VIT_SUCCESS)
					PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
				else
					PRINTF("Now in MusicPlayer menu\r\n");
			}else
			if(PtrVarBlockSharedByDspAndMcu->CurVoiceMenu==ASR_Menu_MusicPlayer)
			{
				PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
				PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
				VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
				if (VIT_Status != VIT_SUCCESS)
					PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
				else
					PRINTF("Now in Home menu\r\n");
			}

		}
		if(PtrVarBlockSharedByDspAndMcu->U32ControlPara[1])
		{
			//user button 2 is pressed --- go to phonecall menu
			PtrVarBlockSharedByDspAndMcu->U32ControlPara[1]=0;

			if(PtrVarBlockSharedByDspAndMcu->CurVoiceMenu==ASR_Menu_Home)
			{
				PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
				PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_PhoneCall;
				VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Call, sizeof(VIT_Intent_Slot_Mask_Call)/sizeof(PL_BOOL));
				if (VIT_Status != VIT_SUCCESS)
					PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
				else
					PRINTF("Now in PhoneCall menu\r\n");
			}else
			if(PtrVarBlockSharedByDspAndMcu->CurVoiceMenu==ASR_Menu_PhoneCall)
			{
				PtrVarBlockSharedByDspAndMcu->PreVoiceMenu=PtrVarBlockSharedByDspAndMcu->CurVoiceMenu;
				PtrVarBlockSharedByDspAndMcu->CurVoiceMenu=ASR_Menu_Home;
				VIT_Status = VIT_ConfigureIntentSlotMask (*p_VITHandle, VIT_Intent_Slot_Mask_Home, sizeof(VIT_Intent_Slot_Mask_Home)/sizeof(PL_BOOL));
				if (VIT_Status != VIT_SUCCESS)
					PRINTF("VIT_ConfigureIntentMask error %d\r\n", VIT_Status);
				else
					PRINTF("Now in Home menu\r\n");
			}
		}
	#endif

	g_vitFramecount++;

	*p_vitDetectionResult = VIT_DetectionResults; // get VIT detection results for further analysis

	return retStatus;
}


