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

#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"
#include "AudioProc_Eap.h"
#include "DspMainAudioFlow.h"


#include "EapTuningConfig.h"

#define APP_AUDIO_RX_SAMPLE_PER_FRAME_MAX       AudioFrameSizeInSamplePerCh_48KHz
PL_INT16 g_processBuff_EAP[LVM_MAX_STREAMS][APP_AUDIO_RX_SAMPLE_PER_FRAME_MAX * LVM_MAX_NUM_CHANNELS];

int EapIsNowBeingUsed;


//the main eap handle for music play and media play
AUDIO_Eap_st				eap_1_Handle;					    // EAP overall structure. If CORE DSP is used then it must be placed in non cached memory section so no volatile is not needed

//decided not to use EAP2, cause EAP2 is simply a mixer when being used for video recording
//the second eap handler together with the first one, being used for video recording
//AUDIO_Eap_st				eap_2_Handle;					    // EAP overall structure. If CORE DSP is used then it must be placed in non cached memory section so no volatile is not needed

#if 1	//folding
status_t toolsDeInterleaveData_16b( PL_INT16*  pIn,
									PL_INT16** ppOuts,
									PL_UINT32  numSamples,
									PL_UINT32  numChannels,
									PL_UINT32  OutIndex)
{
	status_t retStatus = kStatus_Success;

	// process variable
	PL_INT16 offsetSampleIn 	= 0;  		 // sample offset in the input buffer
	PL_UINT32 firstIndex 	    = OutIndex;  // pp_out16b sample Index where to begin the de-interleaving operation
	// loop indice variable
	volatile PL_INT16 iCh 		= 0;
	volatile PL_INT16 iSpl 		= 0;

	/********************/
	/* CHECK PARAMETERS */
	// check for not null output channels to interleave
	for(iCh = numChannels; iCh>0; iCh--)
	{
		if (ppOuts[iCh-1] == NULL)
		{
			return kStatus_NullPointer;
		}
	}

	// check for not null input buffer
	if (pIn == NULL)
	{
		return kStatus_NullPointer;
	}

	/********************/
	/* DE-INTERLEAVE DATA */

	// loop for channels
	for (iCh = 0; iCh < numChannels; iCh++)
	{
		// loop for samples
		for(iSpl=0; iSpl<numSamples; iSpl++)
		{
			offsetSampleIn = iCh + iSpl*numChannels;
			ppOuts[iCh][firstIndex+iSpl] = pIn[offsetSampleIn];
		}
	}

		return retStatus;
}
status_t toolsInterleaveData_16b(	PL_INT16 **ppIns,
						 	 	 	PL_INT16 *pOut,
								 	PL_INT16 NumSamples,
									PL_INT16 NumChannels)
{
	status_t retStatus = kStatus_Success;

	// process variable
	PL_INT16 offsetSampleOut 	= 0;  // sample offset in the output buffer

	// loop indice variable
	volatile PL_INT16 iCh 		= 0;
	volatile PL_INT16 iSpl 		= 0;

	/********************/
	/* CHECK PARAMETERS */

	// check for not null input channels to interleave
	for(iCh = NumChannels; iCh>0; iCh--)
	{
	  	if (ppIns[iCh-1] == NULL)
	  	{
	  		return kStatus_NullPointer;
	  	}
	}

	// check for not null output buffer
	if (pOut ==NULL)
	{
		return kStatus_NullPointer;
	}

	/********************/
	/* INTERLEAVE DATA */
	// loop for channels
	for (iCh = 0; iCh < NumChannels; iCh++)
	{
		// loop for samples
		for(iSpl=0; iSpl<NumSamples; iSpl++)
    	{
			offsetSampleOut = iSpl*NumChannels;

			pOut[offsetSampleOut+iCh] = ppIns[iCh][iSpl];
    	}
    }

    return retStatus;
}
#endif

/**************************************************************************************************/
/*                                                                                                */
/* FUNCTION:            InitEap	and DeInitEap								                      */
/*                                                                                                */
/* DESCRIPTION:																					  */
/*                                                                                                */
/* PARAMETERS:                                                                                    */
/**************************************************************************************************/


status_t DeInitEap( //AUDIO_configParam_st*   p_configParam,
                     AUDIO_Eap_st* 		p_eapDefinition)
{
	status_t   retStatus 		 = kStatus_Success;
	PL_UINT32 i;

	if(!EapIsNowBeingUsed)
		return retStatus;

    for (i = 0; i < LVM_NR_MEMORY_REGIONS; i++)
    {
        if (p_eapDefinition->memTab.Region[i].Size != 0)
        {
        	if(p_eapDefinition->memTab.Region[i].pBaseAddress != NULL)
        		free(p_eapDefinition->memTab.Region[i].pBaseAddress);

        	p_eapDefinition->memTab.Region[i].Size = 0;
        }
    }
	EapIsNowBeingUsed=0;
	CurrentEAPTuningCfg=EapTuningCfg_NoChange;

	return retStatus;
}

status_t initSetEap			(AUDIO_Eap_st* p_eapDefinition);
status_t initCreateEap		(AUDIO_Eap_st* p_eapDefinition);
status_t initSetParamsEap	(AUDIO_Eap_st* p_eapDefinition);

void InitEap(EapTuningCfg_t CfgIdx, int ToPrint_Level)	//OptionWord: byte3,2,1,0:  tbd, tbd, IsStereo, PresetIdx
{
	status_t 	 			 retStatus 	 	 = kStatus_Success;
	status_t 	 			 retStatusFunc   = kStatus_Success;

	if(ToPrint_Level>0)
		PRINTF_M("size fo AUDIO_Eap_st is: %d\r\n", sizeof(AUDIO_Eap_st));

	if(CfgIdx>EapTuningCfg_VideoRecording)
		return;

	//assign more paras in preset tuning files
	switch(CfgIdx)
	{
		case EapTuningCfg_MusicPlay:
				eap_1_Handle.enable						= PL_TRUE;
				eap_1_Handle.pControlParams				= &eapControlParams_48kRx_RT685;		// Eap Control params
				eap_1_Handle.pInstParams				= &eapInstParams_48kRx_RT685;			// Eap Instance params
				#ifdef ALGORITHM_EQNB
					eap_1_Handle.pHeadroomParams		= &eapHeadroomParams_48kRx_RT685;		// EAP headrooms params
				#endif
			break;
		case EapTuningCfg_MediaPlay:
				eap_1_Handle.enable						= PL_TRUE;

				//eap_1_Handle.pControlParams				= &eapControlParams_eapMaxMips;		// Eap Control params
				//eap_1_Handle.pInstParams				= &eapInstParams_eapMaxMips;			// Eap Instance params

				eap_1_Handle.pControlParams				= &eapControlParams_48kRx_RT685;		// Eap Control params
				eap_1_Handle.pInstParams				= &eapInstParams_48kRx_RT685;			// Eap Instance params

				#ifdef ALGORITHM_EQNB
					eap_1_Handle.pHeadroomParams		= &eapHeadroomParams_eapMaxMips;		// EAP headrooms params
				#endif
			break;
		case EapTuningCfg_VideoRecording:
				//eap instance 1 cfg
				eap_1_Handle.enable						= PL_TRUE;
				eap_1_Handle.pControlParams				= &eapControlParams_48kRx_RT685;		// Eap Control params
				eap_1_Handle.pInstParams				= &eapInstParams_48kRx_RT685;			// Eap Instance params
				#ifdef ALGORITHM_EQNB
					eap_1_Handle.pHeadroomParams		= &eapHeadroomParams_48kRx_RT685;		// EAP headrooms params
				#endif

				//eap instance 2 cfg
				//eap_2_Handle.enable						= PL_TRUE;
				//eap_2_Handle.pControlParams				= &eapControlParams_48kRx_RT685;		// Eap Control params
				//eap_2_Handle.pInstParams				= &eapInstParams_48kRx_RT685;			// Eap Instance params
				#ifdef ALGORITHM_EQNB
					//eap_2_Handle.pHeadroomParams		= &eapHeadroomParams_48kRx_RT685;		// EAP headrooms params
				#endif
			break;
		case EapTuningCfg_xxx1:
			break;
		case EapTuningCfg_xxx2:
			break;
		case EapTuningCfg_xxx3:
			break;
		case EapTuningCfg_NoChange:
			break;

		default:
			break;
	}

	#if 1	//---folding
		// GET VERSION INFORMATION
		LVM_ReturnStatus_en 	 eapStatus 		 = LVM_SUCCESS;
		LVM_VersionInfo_st       EAP_VersionInfo = { 0 };            //  EAP Version info structure

					eapStatus = LVM_GetVersionInfo(&EAP_VersionInfo);
					if(eapStatus != LVM_SUCCESS)
					{
						PRINTF("FAIL - EAP LVM_GetVersionInfo error : %d\r\n",eapStatus);
						retStatus = kStatus_Fail;
					}
					if(ToPrint_Level>0)
					{
						PRINTF("DSP init: EAP16 library used:\n\r");
						PRINTF("\tVersion %d.%d.%d\n\r", EAP_VersionInfo.versionMajor, EAP_VersionInfo.versionMedium, EAP_VersionInfo.versionMinor);	//version is 7.0.0
						PRINTF("\tBuild Algorithm available: Mask = 0x%.8X\n\r", EAP_VersionInfo.libAlgorithmdMask);						//mask = 0x3fff7
						if(ToPrint_Level>1)
						{
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_CS_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_CS\n\r");}
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_EQNB_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_EQNB\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_DBE_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_DBE\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_PB_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_PB\n\r"); }		//NOT enabled in this version
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_VC_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_VC\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_VCS_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_VCS\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_TE_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_TE\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_LM_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_LM\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_AVL_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_AVL\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_PSA_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_PSA\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_LIMP_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_LIMP\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_LIMR_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_LIMR\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_XO_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_XO\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_PR_EQNB_MASK)	!= 0) { PRINTF("\t\t\t\t ALGORITHM_PR_EQNB\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_MIXER_MASK)	!= 0) { PRINTF("\t\t\t\t ALGORITHM_MIXER\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_PR_EQFIR_MASK) != 0) { PRINTF("\t\t\t\t ALGORITHM_PR_EQFIR\n\r"); }
							if ((EAP_VersionInfo.libAlgorithmdMask & LVM_MBLM_MASK)		!= 0) { PRINTF("\t\t\t\t ALGORITHM_MBLM\n\r"); }
						}
						PRINTF("\tBuild details: %d\n\r", EAP_VersionInfo.libBuildDetails);		//build = 71
					}
	#endif

	//setup and create EAP instance 1
	int FailCnt=0;

	retStatusFunc = initSetEap(&eap_1_Handle);
	if(retStatusFunc != kStatus_Success)
	{
#ifdef DEBUG_L2
		PRINTF("[initSetEap] Fail\r\n");
#endif
		retStatus = retStatusFunc;
		FailCnt++;
	}


	retStatusFunc = initCreateEap(&eap_1_Handle);
	if(retStatusFunc != kStatus_Success)
	{
#ifdef DEBUG_L2
		PRINTF("FAIL - DSP Init: initCreateEap\r\n");
#endif
		retStatus = retStatusFunc;
		FailCnt++;
	}

	retStatusFunc = initSetParamsEap(&eap_1_Handle);
	if(retStatusFunc != kStatus_Success)
	{
#ifdef DEBUG_L2
		PRINTF("FAIL - DSP Init: setParamsEap\r\n");
#endif
		retStatus = retStatusFunc;
		FailCnt++;
	}



	if(FailCnt)
	{
		while(1)
		{
			PRINTF("RT685 DSP: EAP init is failed\r\n");
		};
	}else
	{
		CurrentEAPTuningCfg=CfgIdx;
		PRINTF("RT685 DSP: EAP init is successful\r\n");
	}

	EapIsNowBeingUsed=1;
}

#if TestAlgoInitAndDeInit==1
void TestHeap_EapInitAndDeInit(int l)
{
	void *HeapPtr1;
	void *HeapPtr2;
	HeapPtr1=GetCurrentHeapTail(3000);
	int TestCnt=l;

	while(1)
	{
		InitEap(EapTuningCfg_MusicPlay, 1);
		DeInitEap(&eap_1_Handle);

		HeapPtr2=GetCurrentHeapTail(3000);
		if(HeapPtr1!=HeapPtr2)
		{
			PRINTF("RT685 DSP: heap base address was, %x\r\n",    (U32)HeapPtr1);
			PRINTF("RT685 DSP: heap base address now is, %x\r\n", (U32)HeapPtr2);
			PRINTF("RT685 DSP: DeInitEap is NOT successful \r\n");
			while(1) {};
		}

		delay_ms(100);
		TestCnt--;
		if(!TestCnt)
			break;
	}
	PRINTF("RT685 DSP: TestHeap_EapInitAndDeInit is successful \r\n");
}
#endif

/**************************************************************************************************/
/*                                                                                                */
/* FUNCTION:            initSetEap												                      */
/*                                                                                                */
/* DESCRIPTION:																					  */
/*                                                                                                */
/* PARAMETERS:                                                                                    */
/**************************************************************************************************/
status_t initSetEap( //AUDIO_configParam_st*   p_configParam,
                     AUDIO_Eap_st* 		p_eapDefinition)
{
	status_t   retStatus 		 = kStatus_Success;
	LVM_ReturnStatus_en eapStatus = LVM_SUCCESS;
	PL_UINT32 i;
	PL_UINT32 temp32;
	eapStatus = LVM_GetMemoryTable(	PL_NULL,
                                    &p_eapDefinition->memTab,
                                    p_eapDefinition->pInstParams);
    if (eapStatus == LVM_NULLADDRESS)
    {
#ifdef DEBUG_L2
        PRINTF("Memory table parameter error - null pointer\r\n");
#endif // DEBUG_L2
        retStatus = kStatus_Fail;
    }
    if (eapStatus == LVM_OUTOFRANGE)
    {
#ifdef DEBUG_L2
        PRINTF("Memory table parameter error - out of range\r\n");
#endif // DEBUG_L2
        retStatus = kStatus_Fail;
    }
	if (eapStatus != LVM_SUCCESS)
    {
        PRINTF("FAIL - EAP Get Memory Table error No: %d\r\n",eapStatus);
        retStatus = kStatus_Fail;
        return retStatus;
    }

    for (i = 0; i < LVM_NR_MEMORY_REGIONS; i++)
    {
        /* Log the memory size */
#ifdef DEBUG_L2
        PRINTF(" Memory region %d, size %d in Bytes\r\n",(LVM_INT16)i,p_eapDefinition->memTab.Region[i].Size);
#endif // DEBUG_L2
        if (p_eapDefinition->memTab.Region[i].Size != 0)
        {
			temp32 =(PL_UINT32) malloc(p_eapDefinition->memTab.Region[i].Size);
            p_eapDefinition->memTab.Region[i].pBaseAddress = (PL_INT8 *)(temp32);
#ifdef DEBUG_L2
            PRINTF(" Memory region address %p\r\n",p_eapDefinition->memTab.Region[i].pBaseAddress);
#endif // DEBUG_L2
        }
    }

	return retStatus;
}

/*************************************************************************************************/
/*                                                                                               */
/* FUNCTION: initCreateEap			   											 			 */
/*                                                                                               */
/* DESCRIPTION: create eap instance 														 */
/*                                                                                               */
/* PARAMETERS:                                                                                   */
/*************************************************************************************************/

status_t initCreateEap( AUDIO_Eap_st* p_eapDefinition )
{
	status_t   retStatus 		 = kStatus_Success;
	LVM_ReturnStatus_en eapStatus = LVM_SUCCESS;

	eapStatus = LVM_GetInstanceHandle( &p_eapDefinition->hInstance,              	/* Init sets the instance handle */
    									&p_eapDefinition->memTab,
    									p_eapDefinition->pInstParams);

    if (eapStatus == LVM_NULLADDRESS)
    {
#ifdef DEBUG_L2
        PRINTF("Get instance handle parameter error - null pointer\r\n");
#endif // DEBUG_L2
        retStatus = kStatus_Fail;
    }
    if (eapStatus == LVM_OUTOFRANGE)
    {
#ifdef DEBUG_L2
        PRINTF("Get instance handle parameter error - out of range\r\n");
#endif // DEBUG_L2
        retStatus = kStatus_Fail;
    }
	if (eapStatus == LVM_INVALIDNXPPLATFORM)
	{
#ifdef DEBUG_L2
		PRINTF("Invalid Platform\r\n");
#endif // DEBUG_L2
		retStatus = kStatus_Fail;
	}
    // PRINTF("[LVM_GetInstanceHandle] Done\r\n");

#ifdef ALGORITHM_EQNB
    // set headroom param config
    eapStatus = LVM_SetHeadroomParams(p_eapDefinition->hInstance, p_eapDefinition->pHeadroomParams);
    if (eapStatus != LVM_SUCCESS)
    {
#ifdef DEBUG_L2
        PRINTF("Set Headrooms Parameters error\r\n");
#endif // DEBUG_L2
        retStatus = kStatus_Fail;
    }
    // PRINTF("[LVM_SetHeadroomParams] Done\r\n");
#endif

	return retStatus;
}


status_t initSetParamsEap(  AUDIO_Eap_st*    p_eapDefinition )
{
	status_t   			retStatus 		 	= kStatus_Success;
	LVM_ReturnStatus_en eapStatus 			= LVM_SUCCESS;
	LVM_ControlParams_t currentParams;
	PL_BOOL 			clearBuffers 		= PL_FALSE;

	/******************************************************************************
	 Call get control parameters
	 	- Compare new to current params to determine if buffer clearing necessary
	 *******************************************************************************/
#ifdef DEBUG_L2
	PRINTF("DSP initSetParamsEap: LVM_GetControlParameters()\r\n");
#endif

	currentParams.BuildStructAlignCheck = LVM_BUILD_STRUCT_CHECK_VALUE;
	eapStatus = LVM_GetControlParameters(p_eapDefinition->hInstance, &currentParams);

	if (eapStatus != LVM_SUCCESS)
	{
		PRINTF("FAIL - EAP GetControlParameters error : %d\r\n",eapStatus);
		retStatus = kStatus_Fail;
	}

	// If general EAP settings are changed a buffer clearing is necessary
	if (
		   (currentParams.OperatingMode 	  != p_eapDefinition->pControlParams->OperatingMode)
		|| (currentParams.SampleRate 		  != p_eapDefinition->pControlParams->SampleRate)
		|| (currentParams.SourceFormat 		  != p_eapDefinition->pControlParams->SourceFormat)
		|| (currentParams.SinkFormat 		  != p_eapDefinition->pControlParams->SinkFormat)
		|| (currentParams.SpeakerType 		  != p_eapDefinition->pControlParams->SpeakerType)
		)
	{
		clearBuffers = PL_TRUE;
	}

	/******************************************************************************
	 Call set control parameters
	 	- propagate the configuration to EAP
	 *******************************************************************************/
#ifdef DEBUG_L2
	PRINTF("DSP initSetParamsEap: LVM_SetControlParameters()\r\n");
#endif

	eapStatus = LVM_SetControlParameters(p_eapDefinition->hInstance, p_eapDefinition->pControlParams);

	if (eapStatus != LVM_SUCCESS)
	{
		PRINTF("FAIL - EAP SetControlParameters error : %d\r\n",eapStatus);
		retStatus = kStatus_Fail;
	}

	/******************************************************************************
	 Call clear audio buffer
	 	- clear the audio buffer
	 	- clear the history
	 	- LVM_ClearAudioBuffers musn't be called if only algorithm parameter is updated
	 	- LVM_ClearAudioBuffers can be called when enabling or disabling algorithm
	 	- LVM_ClearAudioBuffers need to be call when disabling AVL algorithm
	 *******************************************************************************/

	if(clearBuffers == PL_TRUE)
	{
	#ifdef DEBUG_L2
		PRINTF("DSP initSetParamsEap: LVM_ClearAudioBuffers()\r\n");
	#endif

		eapStatus = LVM_ClearAudioBuffers(p_eapDefinition->hInstance);

		if (eapStatus != LVM_SUCCESS)
		{
			PRINTF("FAIL - EAP ClearAudioBuffers error : %d\r\n",eapStatus);
			retStatus = kStatus_Fail;
		}
	}

	p_eapDefinition->hInstance = p_eapDefinition->hInstance;

	return retStatus;
}


#define DEBUG_L3
status_t swProcessEap( AUDIO_Eap_st*    p_eapDefinition,
                        PL_INT8**   	pp_inputAudioData,
                        PL_INT8**   	pp_outputAudioData,
                        PL_INT32		nSamplePerChannel,
                        PL_INT32        audioTime
                        )
{
	status_t   retStatus 		 = kStatus_Success;
    LVM_ReturnStatus_en eapStatus = LVM_SUCCESS;
    PL_INT32 iStream;
    PL_INT16* pp_processEap[LVM_MAX_STREAMS];

	/***************************/
	/* CHECK PARAMETER POINTER */
	/***************************/
	// Check input pointer
	if ( (pp_inputAudioData == PL_NULL) || (pp_outputAudioData == PL_NULL))
	{
		return kStatus_NullPointer;
	}

    for(iStream = 0; iStream < p_eapDefinition->pControlParams->MIXER_NumStreams; iStream++)
    {
        pp_processEap[iStream] = &g_processBuff_EAP[iStream][0];
    }


	/***************************/
    /* PREPARE INPUT DATA
	 * - Interleave for EAP Process
	 */
	/***************************/
	if (p_eapDefinition->pControlParams->MIXER_NumStreams > 1)
	{
		//mixer is used
        if(p_eapDefinition->pControlParams->SinkFormat == LVM_STEREO)
        {
        	//stereo, need to interleave
    		for(iStream = 0; iStream < p_eapDefinition->pControlParams->MIXER_NumStreams; iStream++)
    		{
    			toolsInterleaveData_16b((PL_INT16**)&pp_inputAudioData[iStream * 2],	//src
    									(PL_INT16*)pp_processEap[iStream],				//dst
    									nSamplePerChannel,
    									2);
    		}
        }
        else if(p_eapDefinition->pControlParams->SinkFormat == LVM_MONO)
        {
        	//mono, directly copy
    		for(iStream = 0; iStream < p_eapDefinition->pControlParams->MIXER_NumStreams; iStream++)
    			memcpy( (PL_INT8*)pp_processEap[iStream], (const PL_INT8*)pp_inputAudioData[iStream], nSamplePerChannel*2); //2 bytes per sample
        }
        else
        {
        	retStatus = kStatus_OutOfRange;
        	return retStatus;
        }
	}
	else
	{
        if(p_eapDefinition->pControlParams->SinkFormat == LVM_STEREO)
        {
        	//stereo, need to interleave
    		// Only 1 stream so no mixer used
    		toolsInterleaveData_16b((PL_INT16**)&pp_inputAudioData[0],	//src
    								(PL_INT16*)pp_processEap[0],		//dst
    								nSamplePerChannel,
    								2);
        }
        else if(p_eapDefinition->pControlParams->SinkFormat == LVM_MONO)
        {
        	//mono, directly copy
        	memcpy( (PL_INT8*)pp_processEap[0], (const PL_INT8*)pp_inputAudioData[0], nSamplePerChannel*2); //2 bytes per sample
        }
        else
        {
        	retStatus = kStatus_OutOfRange;
        	return retStatus;
        }
	}

	/***************************/
    /* EAP PROCESS
	 *
	 */
	/***************************/
#ifdef MIPS_MEASURE_GPIO
    BOARD_APP_GPIO_P0_19_ON(); // GPIO turn on
#endif

    eapStatus = LVM_Process( p_eapDefinition->hInstance,                /* Instance handle */
                             (const PL_INT16**)pp_processEap,      		/* Input buffer */
                             (PL_INT16**)pp_processEap,                 /* Output buffer */
                             nSamplePerChannel,                         /* Number of samples to process per channel (Ex: If stereo the total amount of sample is 2 x ReadSamplesPerChannel)*/
                             audioTime);                                /* Audio Time */

#ifdef MIPS_MEASURE_GPIO
    BOARD_APP_GPIO_P0_19_OFF(); // GPIO turn off
#endif

    // Check retStatus
    if (eapStatus == LVM_NULLADDRESS)
    {
#ifdef DEBUG_L3
        PRINTF("Process parameter error - null pointer\n");
#endif // DEBUG_L3
        retStatus = kStatus_Fail;
    	return retStatus;
    }
    if (eapStatus == LVM_INVALIDNUMSAMPLES)
    {
#ifdef DEBUG_L3
        PRINTF("Invalid number of samples, check value or range \n");
#endif // DEBUG_L3
        retStatus = kStatus_Fail;
    	return retStatus;
    }
    if (eapStatus == LVM_ALIGNMENTERROR)
    {
#ifdef DEBUG_L3
        PRINTF("Alignment error, check input and/or output buffer alignment \n");
#endif // DEBUG_L3
        retStatus = kStatus_Fail;
    	return retStatus;
    }
    if(eapStatus != LVM_SUCCESS)
    {
#ifdef DEBUG_L3
        PRINTF("Error while processing: %d\n", (LVM_INT32)eapStatus);
#endif // DEBUG_L3
        retStatus = kStatus_Fail;
    	return retStatus;
    }

    /***************************/
    /* PREPARE OUTPUT DATA
	 * - Deinterleave for Audio Framework
	 */
	/***************************/

    if(p_eapDefinition->pControlParams->XO_OperatingMode == LVM_XO_ON)
    {
    	//CROSSOVER is enabled, De-interleave only the Low Band signal to send it to App output
     	if(p_eapDefinition->pControlParams->SinkFormat == LVM_STEREO)
 		{
 			toolsDeInterleaveData_16b(  (PL_INT16*)pp_processEap[1],		//src --- taken from pp_processEap[2]
 										(PL_INT16**)&pp_outputAudioData[0],	//dst
 										 nSamplePerChannel,
 										 2,
 										 0);
 		}
 		else if(p_eapDefinition->pControlParams->SinkFormat == LVM_MONO)
 		{
 			memcpy( (PL_INT8*)pp_outputAudioData[1], (const PL_INT8*)pp_processEap[1], nSamplePerChannel*2); //2 bytes per sample
 		}
 		else
 		{
 			retStatus = kStatus_OutOfRange;
 		}
     }
     else
     {
    	 //CROSSOVER is disabled, De-interleave the signal to send it to App output
         if(p_eapDefinition->pControlParams->SinkFormat == LVM_STEREO)
         {
             toolsDeInterleaveData_16b(  (PL_INT16*)pp_processEap[0],		//src --- taken from pp_processEap[0]
                                         (PL_INT16**)&pp_outputAudioData[0],//dst
                                         nSamplePerChannel,
                                         2,
                                         0);
         }
         else if(p_eapDefinition->pControlParams->SinkFormat == LVM_MONO)
         {
         	memcpy( (PL_INT8*)pp_outputAudioData[1], (const PL_INT8*)pp_processEap[0], nSamplePerChannel*2); //2 bytes per sample
         }
         else
         {
         	retStatus = kStatus_OutOfRange;
         }
     }

	return retStatus;
}


PL_BOOL				EapRefusesStartingANewPromptPlaying = PL_FALSE;		// Whether a notification sound is playing --- doesn't seem to be useful --- later to be deleted
PL_BOOL 			EapAllowsWritingStrm1AsPromptAudio  = PL_FALSE;		// After volume transition callback, tell Audio task to send sound file buffers to EAP

//PL_BOOL				g_playNotification		= PL_FALSE;		// Tell EAP to play notification sound
//PL_UINT8			g_notificationIndex 	= 0U;			// Which notification sound is playing
//PL_UINT32			g_fileLenBytes 			= 0U;			// Which notification sound is playing

//this function is not used --- for reference ONLY
#if 0
status_t swProcessEAPSetCommand(AUDIO_Eap_st*    p_eapDefinition)
{
	if (p_eapDefinition == PL_NULL)
	{
		return kStatus_NullPointer;
	}
	status_t   			retStatus = kStatus_Success;
	LVM_ReturnStatus_en retStatusEAP = LVM_SUCCESS;
	LVM_ControlParams_t currentParams;

	/*
	 *	Call get  EAP control parameters
	 */
	currentParams.BuildStructAlignCheck = LVM_BUILD_STRUCT_CHECK_VALUE;

	retStatusEAP = LVM_GetControlParameters(p_eapDefinition->hInstance, &currentParams);
	if(retStatusEAP != LVM_SUCCESS)
	{
		PRINTF("LVM_GetControlParameters: fail with error %i \r\n",retStatusEAP );
		retStatus = kStatus_Fail;
	}
	/*
	 *	Handle VIT command detection
	 */

	if(PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_McuCmdToDsp])
	{
		switch(PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_McuCmdToDspPara2])
		{
		/*
		      1    : PLAY MUSIC
			  2    : STOP MUSIC
			  3    : TAKE PICTURE
			  4    : VOLUME UP
			  5    : VOLUME DOWN
			  6    : MUTE SPEAKER
			  7    : PLAY SPEAKER
		 */
		case 1: //PLAY MUSIC (Enable EAP effect)
			currentParams.OperatingMode = LVM_MODE_ON;
			retStatusEAP = LVM_SetControlParameters(p_eapDefinition->hInstance, &currentParams);
			break;
		case 2: //STOP MUSIC (Disable EAP effect)
			currentParams.OperatingMode = LVM_MODE_OFF;
			retStatusEAP = LVM_SetControlParameters(p_eapDefinition->hInstance, &currentParams);
			break;
		case 3: //TAKE PICTURE
			/* No action on EAP */
			EapRefusesStartingANewPromptPlaying = PL_TRUE;
//			vitDetectionResult = 0;
			g_notificationIndex = 0U; // index for Camera click notification file
			g_playNotification = PL_TRUE;
			currentParams.pMIXER_StreamParams[0].TargetGain 		= (LVM_MIXER_VOLUME_MAX / 16); 		// Set audio playback stream to -24dB
			currentParams.pMIXER_StreamParams[0].TimeConstantMs 	= NOTIF_VOLUME_TRANSITION_TIME_MS;	// Time to arrive to target volume
			currentParams.pMIXER_StreamParams[0].pCallback 			= (LVM_Callback)eapNotificationCallbackDown;

			currentParams.pMIXER_StreamParams[2].TargetGain 		= (LVM_MIXER_VOLUME_MAX / 16); 		// Set audio playback stream to -24dB
			currentParams.pMIXER_StreamParams[2].TimeConstantMs 	= NOTIF_VOLUME_TRANSITION_TIME_MS;	// Time to arrive to target volume

			retStatusEAP = LVM_SetControlParameters(p_eapDefinition->hInstance,	&currentParams);
			break;
		case 4: //VOLUME UP
			if (currentParams.VC_EffectLevel <= -3)
			{
				currentParams.VC_EffectLevel +=3;
				retStatusEAP = LVM_SetVolumeNoSmoothing(p_eapDefinition->hInstance, &currentParams);
			}
			break;
		case 5: //VOLUME DOWN
			if (currentParams.VC_EffectLevel >= -18)
			{
				currentParams.VC_EffectLevel -=3;
				retStatusEAP = LVM_SetVolumeNoSmoothing(p_eapDefinition->hInstance, &currentParams);
			}
			break;
		case 6: //MUTE SPEAKER
			currentParams.VC_EffectLevel = -96;
			retStatusEAP = LVM_SetControlParameters(p_eapDefinition->hInstance, &currentParams);
			break;
		case 7: //PLAY SPEAKER
			currentParams.VC_EffectLevel = 0;
			retStatusEAP = LVM_SetControlParameters(p_eapDefinition->hInstance, &currentParams);
			break;
		default:
			break;
		}
		if(retStatusEAP != LVM_SUCCESS)
		{
			PRINTF("LVM_SetControlParameters : fail with error %i \r\n",retStatusEAP );
			retStatus = kStatus_Fail;
		}

		PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_McuCmdToDsp]=0;
	}

	return retStatus;
}
#endif

/*************************************************************************/
/* EAP Specific 														 */
/*************************************************************************/

void eapNotificationCallbackDown(void* Instance, void* UserData, short int CallbackArg)
{
	EapAllowsWritingStrm1AsPromptAudio = PL_TRUE;
	PRINTF("eapNotificationCallbackDown\r\n");
}

void eapNotificationCallbackUp(void* Instance, void* UserData, short int CallbackArg)
{
	EapRefusesStartingANewPromptPlaying = PL_FALSE;
	PRINTF("eapNotificationCallbackUp\r\n");
}

void EapProcess(S16 **DstPtrs, S16 **SrcPtrs, int SampleNum, EapTuningCfg_t EapTuneCfg)
{

	//Note: in this project, stream[2] (the third) stream is not needed --- we feed in zeros

	/*******************************************************************************
	 * EAP Process
	 *
	 * this is the mapping of input channels to be mixed internally by EAP mixer:
	 *                                                                    ________
	 * pp_processBuff_Rx_In_8b[0] = far end left ----------------------->|        |
	 * pp_processBuff_Rx_In_8b[1] = far end right----------------------->|        |
	 *                                                                   |        |------> pp_processBuff_Rx_Out_8b[0] = EAP out left
	 * pp_processBuff_Rx_In_8b[2] = local audio message/chime left------>|  EAP   |
	 * pp_processBuff_Rx_In_8b[2] = local audio message/chime right----->|16-bits |
	 *                                                                   |  48k   |------> pp_processBuff_Rx_Out_8b[0] = EAP out right
	 * pp_processBuff_Rx_In_8b[3] = DP converter left ------------------>|        |
	 * pp_processBuff_Rx_In_8b[4] = DP converter right ----------------->|________|
	 *
	 *******************************************************************************/


	if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg==EapTuningCfg_NoChange)
	{
		switch(EapTuneCfg)
		{
			case EapTuningCfg_MusicPlay:
					//single EAP instance
					swProcessEap(&eap_1_Handle,
								(PL_INT8**)   	SrcPtrs,
								(PL_INT8**)   	DstPtrs,
												SampleNum,
												0
								);
			break;
			case EapTuningCfg_MediaPlay:
					//single EAP instance
					swProcessEap(&eap_1_Handle,
								(PL_INT8**)   	SrcPtrs,
								(PL_INT8**)   	DstPtrs,
												SampleNum,
												0
								);
				break;
			case EapTuningCfg_VideoRecording:
					#if 0
						//dual EAP instances --- decided not to use 2 EAP instances for video recording
						swProcessEap(&eap_1_Handle,
									(PL_INT8**)   	&SrcPtrs[0],
									(PL_INT8**)   	&DstPtrs[0],
													SampleNum,
													0
									);
						swProcessEap(&eap_2_Handle,
									(PL_INT8**)   	&SrcPtrs[1],
									(PL_INT8**)   	&DstPtrs[1],
													SampleNum,
													0
									);
					#else
						//single EAP instance
						swProcessEap(&eap_1_Handle,
									(PL_INT8**)   	SrcPtrs,
									(PL_INT8**)   	DstPtrs,
													SampleNum,
													0
									);
					#endif
				break;

			default:
				break;
		}
	}else
	{
		//bypass EAP --- directly copy src audio data to dst
		memcpy(DstPtrs[0],SrcPtrs[0],sizeof(S16)*SampleNum);
		memcpy(DstPtrs[1],SrcPtrs[1],sizeof(S16)*SampleNum);
	}
}



