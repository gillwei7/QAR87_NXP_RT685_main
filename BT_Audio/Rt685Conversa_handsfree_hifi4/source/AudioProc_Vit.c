/*
 * Copyright 2018-2019 NXP
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

#define CORE_MCU
#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"


#include "VIT_Model_en.h"
//#include "VIT_Model_en_2.h"


/*******************************************************************************
 * Variables
 ******************************************************************************/

XosThread 		  g_audioVitTask_thread;			 							// Audio task thread
PL_UINT8	 	  g_audioVitTask_stack 	[DSP_AUDIO_VIT_THREAD_STACK_SIZE_BYTE];	// Audio task memory stack allocated
XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.
XosMutex 		  g_audio_vitBufferMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task

T_CircularAudioBuf_S16  VitCircBuff;
S16 					VitCircBuff_DataArea[APP_VIT_FRAME_SIZE_InSAMPLEs*2+APP_VIT_FRAME_SIZE_InSAMPLEs];	//cirbuffer size=APP_VIT_FRAME_SIZE_InSAMPLEs*2, extra read space for 1 APP_VIT_FRAME_SIZE_InSAMPLEs frame
VIT_DetectionStatus_en 	g_vitDetectionResult = 0;   // VIT detection results
PL_UINT16 				g_vitVcDetectionId = 0;     // VIT voice command detection results ID


PL_UINT32 g_vitFramecount 	 = 0;
static PL_UINT32 g_vitDetectionFrame = 0;


/*******************************************************************************
 * Code
 ******************************************************************************/

status_t swProcessVIT( AUDIO_vit_st* 			 p_definitionVIT,
					   PL_INT16*     			 p_inputAudioData,
					   PL_INT32	     			 inputAudioDataSize_sample,
					   VIT_DetectionStatus_en* 	 p_vitDetectionResult,
					   PL_UINT16* 	 			 p_vitVcDetectionId)
{
	status_t   				retStatus 		 		= kStatus_Success;

	VIT_Handle_t*          	p_VITHandle			  	= &p_definitionVIT->vitHandle;
	//VIT_ControlParams_st*  	p_VITControlParamConfig = &p_definitionVIT->vitControlParams;

	VIT_ReturnStatus_en 	VIT_Status				= VIT_SUCCESS;
	VIT_DetectionStatus_en 	VIT_DetectionResults 	= VIT_NO_DETECTION;	// VIT detection result
	VIT_VoiceCommand_st 	VoiceCommand;                   	        // VIT Voice Command info
	VIT_WakeWord_st 		WakeWord;                    		        // VIT Wakeword info

	/***************************/
	/* CHECK PARAMETER POINTER */
	/***************************/
	// Check input pointer
	if ( (p_inputAudioData == NULL) || (p_definitionVIT == NULL) )
	{
		return kStatus_NullPointer;
	}

	/***************************/
	/* VIT PROCESS
	 *
	 *    VIT process:
	 *    	- VIT process
	 *    	- Check VIT detection results
	 *    	- Share WW and Command detection
	 */

	VIT_Status = VIT_Process( *p_VITHandle, p_inputAudioData, &VIT_DetectionResults );

	if (VIT_Status != VIT_SUCCESS)
	{
		PRINTF("VIT_Process error : %d\n", VIT_Status);
		return VIT_SYSTEM_ERROR;                                            // will stop processing VIT and go directly to MEM free
	}

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
				}
			}
		}
	}
	/* Check if Voice Command was detected */
	else if (VIT_DetectionResults == VIT_VC_DETECTED)
	{
		// Retrieve id of the Voice Command detected
		// String of the Command can also be retrieved (when WW and CMDs strings are integrated in Model)
		VIT_Status = VIT_GetVoiceCommandFound( *p_VITHandle, &VoiceCommand );

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

	g_vitFramecount++;

	*p_vitDetectionResult = VIT_DetectionResults; // get VIT detection results for further analysis

	return retStatus;
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
        	PRINTF("\n    Voice commands supported : \r\n");
			ptr = Model_Info.pVoiceCmds_List;
			if (ptr != PL_NULL)
			{
				for (PL_UINT16 i = 0; i < Model_Info.NbOfVoiceCmds; i++)
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
	VIT_ControlParams_st*  p_VITControlParamConfig  = &p_definitionVIT->vitControlParams;


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
	p_VITControlParamConfig->OperatingMode     = p_definitionVIT->vitConfig.operatingMode;	//Set VIT operating mode
	p_VITControlParamConfig->Command_Time_Span = p_definitionVIT->vitConfig.timeSpan; 	    // Set VIT time span
	p_VITControlParamConfig->Feature_LowRes    = PL_FALSE;									// Set VIT low res in false

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
	if( p_VITControlParamConfig->Command_Time_Span > 5.0 )
	{
		PRINTF("FAIL - DSP: TIme Span => VIT  parameter (%f) grater than VIT library time span(%f)\r\n", p_VITControlParamConfig->Command_Time_Span, 5.0);
		retStatus = kStatus_NotCompatible;
	}

	return retStatus;

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
	static PL_INT8  *pMemory[PL_NR_MEMORY_REGIONS];

	//settings were done in MCU code, in the orignal demo
	p_definitionVIT->vitConfig.vitInputSelect = AUDIO_VIT_USE_CONVERSA_BFOUT;		// VIT uses Conversa TxOut as input
	#ifdef WHO_IS_TALKING_PRESENT
		p_definitionVIT->vitConfig.operatingMode  = VIT_WAKEWORD_ENABLE ;			// Enable VIT for WW recognition. It will be update to CMD detection by the application code when a WW is detected and valid
	#else
		p_definitionVIT->vitConfig.operatingMode  = VIT_WAKEWORD_ENABLE + VIT_VOICECMD_ENABLE;	// Enable VIT for WW and VCMD recognition.
	#endif
	p_definitionVIT->vitConfig.framesize	  = VIT_SAMPLES_PER_30MS_FRAME;		// Set VIT framesize (160 (10ms) or 480(30ms))
	p_definitionVIT->vitConfig.timeSpan       = 5.0;								// Set VIT time span for command detection
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

	PRINTF("\n    VIT library:\r\n");

	// get VIT library information
	VIT_Status = VIT_GetLibInfo(&Lib_Info);
	if (VIT_Status != VIT_SUCCESS)
	{
		PRINTF("    VIT_GetLibInfo error : %d\n", VIT_Status);
		retStatus = kStatus_Fail;								// We can exit from here since memory is not allocated yet

	}
	PRINTF("    VIT Library v%04x\r\n", Lib_Info.VIT_LIB_Release);

	// set VIT model

	p_VITModel->vitModelMem = VIT_MODEL_IN_SLOW_MEM;					// VIT model is stored in RAM
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

   	/*
	 *  Allocate memory for VIT instance
	 *     - Get required VIT instance memory size according VIT parameters
	 *     - Reserved required memory for VIT instance
	 *     - Check VIT instance memory allocation pass
	 */

	// get VIT memory table
	/*
	 * VIT Inst Params:
	 * 		- SampleRate_Hz    = VIT_SAMPLE_RATE;
	 *		- SamplesPerFrame  = VIT_SAMPLES_PER_30MS_FRAME;
	 *		- NumberOfChannel  = VIT_MAX_CHANNELS;
	 *		- DeviceId         = DEVICE_ID;
	 *		- APIVersion       = VIT_API_VERSION;
	 * */
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

	// set VIT control parameters
	/*
	 * VIT Control Params:
	 * 		- OperatingMode     = ( VIT_WAKEWORD_ENABLE | VIT_VOICECMD_ENABLE )
	 *		- Feature_LowRes    = PL_FALSE;
	 *		- Command_Time_Span = VIT_COMMAND_TIME_SPAN;
	 * */
	if (!InitPhase_Error)
	{
		VIT_Status = VIT_SetControlParameters( *p_VITHandle, p_VITControlParamConfig );
		if (VIT_Status != VIT_SUCCESS)
		{
			InitPhase_Error = PL_TRUE;
			PRINTF("VIT_SetControlParameters error : %d\r\n", VIT_Status);
		}
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
	initCreateVIT(&vitPluginParams);
}

#if 0
extern void XOS_INIT(void);
extern void XOS_EnableMuIntr(void);
int audioProcessVitTask(void *arg, int wake_value)
{
	status_t retStatus	   = kStatus_Success;
	status_t retStatusFunc = kStatus_Success;
	int32_t	 retStatusXos  = XOS_OK;
	VIT_ReturnStatus_en VIT_Status;

#if 1

	AUDIO_vit_st*       p_swIpVIT_handle    	 = &vitPluginParams;
	PL_INT16*           p_conversaToVitBuff_16b  = NULL;
	XOS_EnableMuIntr();

	PRINTF("\n*************************************\r\n");
	PRINTF("[DSP audioVITProcessTask] is running...\r\n");

	while (retStatusXos == XOS_OK)
	{
		retStatusXos = xos_sem_tryget( &g_audioTask_audioVitProcessSemaphore );    // try to decrement the audio task main semaphore counter and return error when semaphore count = 0
	}
	PRINTF("*************************************\r\n\n");

	while(1)
	{

		/*
		 *  WAIT MCU ASK DSP TO RUN AN AUDIO PROCESS
		 *
		 *      - wait until g_audioTask_mainSemaphore > 0
		 */
		retStatusXos = xos_sem_get( &g_audioTask_audioVitProcessSemaphore );   	// Wait until the audio vit task semaphore can be decremented
		if ( retStatusXos != XOS_OK )
		{
			PRINTF("FAIL - [DSP audioProcessTask]: Semaphore mechanism (error = %d)\r\n",retStatusXos);
			retStatus = kStatus_Fail;
		}

		//xos_mutex_lock(&g_audio_vitBufferMutex);
		p_conversaToVitBuff_16b = CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff, p_swIpVIT_handle->vitConfig.framesize);
		//xos_mutex_unlock(&g_audio_vitBufferMutex);

		/********************************************************************
		 * VIT PROCESS
		 *		- Copy current samples from source
		 *		- Check if there are enogugh samples to run VIT
		 *		- Run VIT process
		 *		- Send message to MCU to indicate VIT process finished
		 *******************************************************************/
		DbgPin8Up();
		VIT_Status = swProcessVIT( p_swIpVIT_handle,
								   p_conversaToVitBuff_16b,
								   p_swIpVIT_handle->vitConfig.framesize,
								   &g_vitDetectionResult,
								   &g_vitVcDetectionId
								 );
		DbgPin8Dn();

		if (VIT_Status != VIT_SUCCESS)
		{
			PRINTF("  VIT_Process error : %d\n", VIT_Status);
			retStatusFunc = kStatus_Fail;
		}

		/* handle return error code */
		if (retStatusFunc != kStatus_Success)
		{
			PRINTF("[AUDIO_vitTask] FAIL: VIT process return error code = %d\r\n", retStatusFunc);
			retStatus = retStatusFunc;
		}


		#if 1
			if(!(g_vitFramecount%20))
			{
				SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
					PRINTF("RT685 DSP: active \r\n");
				SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
			}
		#endif
	}
#endif

}

extern void APP_MU_IRQHandler(void);
void ConfigAndStartVitTask(void)
{
    PL_INT32 retStatusXos  = XOS_OK;
    status_t retStatus     = kStatus_Success;


	/* Create audio task semaphore */
	retStatusXos = xos_sem_create(	&g_audioTask_audioVitProcessSemaphore,  	// pointer to audio vit task semaphore
									XOS_SEM_WAIT_PRIORITY,						// Wake waiters in priority order
									0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create audio VIT task semaphore (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	else
	{
	}

	/* Create VIT bufer access mutex */
	retStatusXos =xos_mutex_create( &g_audio_vitBufferMutex,							// pointer to vit buffer mutex read/write for access
									XOS_MUTEX_WAIT_FIFO,
									0);

	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create audio VIT mutex (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	else
	{
	}

	/* create XOS DSP VIT thread */
	//p_audioDefinition_handle->audioWorkFlow_handle.p_dspAudioTaskHandle = &g_audioVitTask_thread;			// set audio task thread memory address

	retStatusXos = xos_thread_create(	&g_audioVitTask_thread,												// thread memory address,
										NULL,																// reserved
										audioProcessVitTask,												// thread entry function
										NULL,																// reserved
										"DSP VIT audio thread",												// name of the thread
										g_audioVitTask_stack,												// allocate memory pointer for this thread
										DSP_AUDIO_VIT_THREAD_STACK_SIZE_BYTE,								// allocate memory size for this thread in byte
										DSP_AUDIO_VIT_THREAD_PRIORITY,										// thread priority, higher numbers are higher priority
										0,																	// extra parameters
										0);																	// no option for this thread
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create audio VIT task thread (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	else
	{
	}

	/* Start XOS scheduler */
    if (retStatus == kStatus_Success)
    {
		PRINTF("DSP XOS is started\r\n");
    	xos_start(0);
    }


}

#endif




