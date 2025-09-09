/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <xtensa/config/core.h>
#include <xtensa/xos.h>

/* StdLib */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>

/* McuExpresso */
#include "fsl_gpio.h"
#include "fsl_debug_console.h"
#include "fsl_common.h"
#include "GlobalDef.h"
#include "SubFunc.h"
#include "AudioProc_Conversa.h"
#include "Sweep.h"
#include "CircularBufManagement.h"

#include "GainingAndMixing.h"
#include "IIR.h"

#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"

#include "fsl_sema42.h"
#include "SRCProc.h"


#define CORE_MCU
#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"

//tuning parameter file
#include "conversaParam7_3_1_q1bcMic034_Fb128_16kHz.h"

// Se model small for 16k
#include "ml_i16xi8_GDFT128-128-768_16kHz.h"			// Conversa Speech enhancement Model small at 16Khz

#define UsePrimitiveSimpleSrc		0


uint32_t frmcount;

nxp_conversa_plugin_t conversaPluginParams;
__attribute__((__section__(".control_reg_addr")))
PL_UINT32 s_conversaControlBlockAddress;

__attribute__((__section__("NonCacheable")))
uint8_t s_conversaCriticalmemory[76800];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 BtRxInBuf [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 BtTxOtBuf [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 SpkOtBufL [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 SpkOtBufR [AudioFrameSizeInSamplePerCh];


__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 RawMic32BitBuf0 [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 RawMic32BitBuf1 [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 RawMic32BitBuf2 [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 RawMic32BitBuf3 [AudioFrameSizeInSamplePerCh];


__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 ConversaTxOut32BitBuf [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AecOut32BitBuf [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 BfOut32BitBuf [AudioFrameSizeInSamplePerCh];

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 NlpOut32BitBuf [AudioFrameSizeInSamplePerCh];




/*******************************************************************************************/
void printConversaPluginParam ( nxp_conversa_plugin_t* p_conversaPluginParams )
{
	PRINTF("\n\tConversa plug-in parameters:\r\n");
	PRINTF("\t\t.memory_base_address                     = 0x%x\r\n",   p_conversaPluginParams->memory_base_address);
	PRINTF("\t\t.memory_size_bytes                       = %d\r\n",   	p_conversaPluginParams->memory_size_bytes);
    PRINTF("\t\t.critical_memory_base_address            = 0x%x\r\n",   p_conversaPluginParams->critical_memory_base_address);
    PRINTF("\t\t.critical_memory_size_bytes              = %d\r\n",     p_conversaPluginParams->critical_memory_size_bytes);
    PRINTF("\t\t.config.num_mics                         = %d\r\n",     p_conversaPluginParams->config.num_mics);
    PRINTF("\t\t.config.num_mics_bf                      = %d\r\n",     p_conversaPluginParams->config.num_mics_bf);
	PRINTF("\t\t.config.num_rx_in                        = %d\r\n",     p_conversaPluginParams->config.num_rx_in);
	PRINTF("\t\t.config.num_spks                         = %d\r\n",     p_conversaPluginParams->config.num_spks);
	PRINTF("\t\t.config.sample_rate                      = %d\r\n",     p_conversaPluginParams->config.sample_rate);
	PRINTF("\t\t.config.framesize                        = %d\r\n",     p_conversaPluginParams->config.framesize);
	PRINTF("\t\t.config.num_bands                        = %d\r\n",     p_conversaPluginParams->config.num_bands);
	PRINTF("\t\t.config.fb_lp                            = %d\r\n",     p_conversaPluginParams->config.fb_lp);
	PRINTF("\t\t.config.create_aec                       = %d\r\n",     p_conversaPluginParams->config.create_aec);
	PRINTF("\t\t.config.aec_filter_length_ms             = %4.4f\r\n",  p_conversaPluginParams->config.aec_filter_length_ms);
	PRINTF("\t\t.config.aec_uses_current_sensing         = %d\r\n",     p_conversaPluginParams->config.aec_uses_current_sensing);
	PRINTF("\t\t.config.create_bf                        = %d\r\n",     p_conversaPluginParams->config.create_bf);
	PRINTF("\t\t.config.create_doa                       = %d\r\n",     p_conversaPluginParams->config.create_doa);
	PRINTF("\t\t.config.create_se_model                  = %d\r\n",     p_conversaPluginParams->config.create_se_model);
	PRINTF("\t\t.config.se_model_ptr                     = 0x%x\r\n",   p_conversaPluginParams->config.se_model_ptr);
	PRINTF("\t\t.config.se_model_size                    = %d\r\n",     p_conversaPluginParams->config.se_model_size);
	PRINTF("\t\t.config.se_model_is_permuted             = %d\r\n",     p_conversaPluginParams->config.se_model_is_permuted);
	PRINTF("\t\t.config.enable_tuning_tool_signal_buffer = %d\r\n",     p_conversaPluginParams->config.enable_tuning_tool_signal_buffer);
	PRINTF("\t\t.config.device_id                        = %d\r\n\n",   p_conversaPluginParams->config.device_id);
}

void initSetConversaInstParam(nxp_conversa_plugin_t *p_conversaPluginParams)
{

	PL_UINT32* 					  p_model_blob_header_32b = PL_NULL;		// 32 bit pointer on the Conversa Se Model
	nxp_conversa_plugin_config_t* p_conversaPluginConfig  = &p_conversaPluginParams->config;

	/***************************/
	/* SET CONVERSA PARAMETERS */
	p_conversaPluginConfig->sample_rate = 16000;
	p_conversaPluginConfig->framesize 	= AudioFrameSizeInSamplePerCh;

 	/*
	 *  Conversa setup:
	 *  	- Se model (address and size)
	 *  	- number of bands
	 *  	- filter bank windows
	 */

	// if Conversa Se Model is enabled
	p_conversaPluginConfig->create_se_model			= CONVERSA_ENABLE;		// Conversa ML enable
	p_conversaPluginConfig->se_model_is_permuted	= CONVERSA_DISABLE; 	// disable for Non-permuted model. Model is copied by Conversa library to internal Conversa heap memory and permuted
																			// enable for Permuted model. Model is directly used by Conversa library without internal copy. Less internal Conversa heap memory is required.

	p_conversaPluginConfig->se_model_size 	= sizeof(model_blob_ml_i16xi8_128_128_768_16kHz);
	p_conversaPluginConfig->se_model_ptr 	= (uint8_t*)model_blob_ml_i16xi8_128_128_768_16kHz;

 	// Check model address and size
 	if (   ( p_conversaPluginConfig->se_model_ptr  == PL_NULL)
 		|| ( p_conversaPluginConfig->se_model_size == 0)
	   )
 	{
		PRINTF("FAIL - DSP process init: Conversa Model or model size is not correct. SE model pointer address = 0x%x  - SE model size = %d bytes\r\n",   p_conversaPluginConfig->se_model_ptr, p_conversaPluginConfig->se_model_size);
 		return;
 	}
	else
	{
		/* Get data from model header informations */
		p_model_blob_header_32b = (PL_UINT32*) p_conversaPluginConfig->se_model_ptr;

		p_conversaPluginConfig->sample_rate = p_model_blob_header_32b [SE_MODEL_U32_OFFSET_SAMPLE_RATE];  // Conversa sample rate
		p_conversaPluginConfig->framesize 	= p_model_blob_header_32b [SE_MODEL_U32_OFFSET_FRAMESIZE];	  // Conversa framesize
		p_conversaPluginConfig->num_bands   = p_model_blob_header_32b [SE_MODEL_U32_OFFSET_NUM_BANDS];    // Extract number of band from header
		p_conversaPluginConfig->fb_lp 	    = p_model_blob_header_32b [SE_MODEL_U32_OFFSET_FB_LP];	      // Extract filter bank windows from header
	}

	/*
	 * Conversa In/Out parameters
	 */
	p_conversaPluginConfig->num_rx_in   = 1;								// Conversa Rx input channel number is always 1. (rx_in = 1 for calling operation, rx_in = 2 for gaming headset operation)
	p_conversaPluginConfig->num_spks    = 1;  							 	// Conversa Rx output channel number is always 1. (num_spk = 1 for when having mono output, num_spk = 2 when using internal split filter (woofer and tweeter))
	p_conversaPluginConfig->num_mics    = 3;
	p_conversaPluginConfig->num_mics_bf = 3;

	/*
	 * Conversa Direction Of Arrival (DOA) and Beam Former (BF)
	 */

	//for glasses:
	p_conversaPluginConfig->create_doa 			= CONVERSA_DISABLE;							// Conversa DOA disable (direction of arrival) process
	p_conversaPluginConfig->create_bf 			= CONVERSA_BF_MODE_ADAPTIVE_OR_FIX;		// Conversa beam former mode (select adaptive + fix steering available)
	//p_conversaPluginConfig->create_bf           = CONVERSA_BF_MODE_ADAPTIVE;

	/*
	 * Audio Echo Canceler (AEC)
	 */
	if (p_conversaPluginConfig->num_rx_in > 0)													// if more Rx input exist then enable the Audio Echo Canceler
	{
		p_conversaPluginConfig->create_aec 				= CONVERSA_ENABLE;				   		// Conversa AEC enable
	}
	else																						// else if no Rx input exist then disable Audio Echo Canceler
	{
		p_conversaPluginConfig->create_aec 				= CONVERSA_DISABLE;				   		// Conversa AEC disable
	}

	/*
	 *  AEC filter length
	 */
	//for glasses
	p_conversaPluginConfig->aec_filter_length_ms = NXP_CONVERSA_AEC_FILTER_LENGTH_MIN_MS;				// Conversa AEC filter length

	/* Current sensing */
    p_conversaPluginConfig->aec_uses_current_sensing 	= NXP_CONVERSA_AEC_REFERENCE;			 // Conversa AEC reference input channel type

	p_conversaPluginConfig->device_id 					= Device_IMXRT600_HIFI4;				 // Conversa library made for I.MX RT600 Hifi core

	p_conversaPluginConfig->enable_tuning_tool_signal_buffer = CONVERSA_ENABLE;					 // Tuning tool buffer can be disabled after tuning to save memory

	/* Select Conversa internal signal to be computed for extraction (More internal signal available or extraction more MIPS consumption)*/
	p_conversaPluginConfig->internal_output_selector = NXP_AEC_OUT + NXP_BF_OUT + NXP_NLP_OUT; // Select AEC output + Beam former output + NLP output.

    /* Memory handle */
	// Non cached Conversa memory address reset
	p_conversaPluginParams->critical_memory_base_address = NULL;
	p_conversaPluginParams->critical_memory_size_bytes   = 0;
	// Cached Conversa memory address reset
	p_conversaPluginParams->memory_base_address		   = NULL;
	p_conversaPluginParams->memory_size_bytes			   = 0;
}

//status_t initCreateConversa( AUDIO_conversa_st* p_definitionConversa )
void InitConversa(void)
{

	NXP_STATUS retStatusConversa = OK;

	initSetConversaInstParam(&conversaPluginParams);

	// conversa version
	PL_UINT32 conversaVersionMajor;
	PL_UINT32 conversaVersionMinor;
	PL_UINT32 conversaVersionPatch;

	conversa_memory_req_t conversaMemReq; // Memory size (Cache and non cached) required by conversa in bytes

	PRINTF("\n    Conversa library:\r\n");

	/*
	 *  Allocate memory for Conversa instance
	 *     - Get required Conversa instance memory size according Conversa parameters
	 *     - Reserved required memory for Conversa instance
	 *     - Check Convera instance memory allocation pass
	 */

	/* Get required Conversa instance memory size according Conversa parameters */
	conversaMemReq = NxpConversa_Plugin_GetRequiredHeapMemoryBytes(&conversaPluginParams);


	conversaPluginParams.memory_size_bytes 		  = conversaMemReq.NonCritical_mem;		// set required memory that can be cached or non cached
	conversaPluginParams.critical_memory_size_bytes = conversaMemReq.Critical_mem;    // set required non cached memory

	PRINTF("\t-Conversa memory size required: %i bytes\r\n", conversaMemReq.NonCritical_mem);
	PRINTF("\t-Conversa critical memory size required: %i bytes\r\n", conversaMemReq.Critical_mem);

	/* Reserved required memory for Conversa instance */
	conversaPluginParams.memory_base_address = malloc(conversaPluginParams.memory_size_bytes);   // Get Conversa memory base address from malloc function

	/* Check Convera instance memory allocation pass */
	if (conversaPluginParams.memory_base_address != PL_NULL)				// if Conversa memory allocation pass
	{
		PRINTF("\t-Conversa memory allocation: PASS, structure at adr 0x%8.8X\r\n", conversaPluginParams.memory_base_address);
	}
	else																						// else if Conversa memory allocation fail
	{
		PRINTF("\tFAIL - DSP process init: Conversa memory allocation failed \r\n");
		printConversaPluginParam (&conversaPluginParams);  // print conversaPluginParams parameters
		return;
	}

	PRINTF("\t-Conversa critical memory size required: %i bytes\r\n", conversaPluginParams.critical_memory_size_bytes);

	/* Reserved required memory for Conversa instance */
	conversaPluginParams.critical_memory_base_address = (void *) s_conversaCriticalmemory;   // Get Conversa memory base address from malloc function

	/* Check Convera instance memory allocation pass */
	if (conversaPluginParams.critical_memory_base_address != PL_NULL)				// if Conversa memory allocation pass
	{
		PRINTF("\t-Conversa critical memory allocation: PASS, structure at adr 0x%8.8X\r\n", conversaPluginParams.critical_memory_base_address);
	}
	else																						// else if Conversa memory allocation fail
	{
		PRINTF("\tFAIL - DSP process init: Conversa critical memory allocation failed \r\n");
		printConversaPluginParam (&conversaPluginParams);  // print conversaPluginParams parameters
		return;
	}

	/********************************/
	/* CREATE SET CONVERSA INSTANCE */
	retStatusConversa = NxpConversa_Plugin_Create(&conversaPluginParams);
	if (retStatusConversa != OK)
	{
		PRINTF("\tFAIL - DSP process init: Conversa failed to create instance (error code = %i)\r\n", retStatusConversa);
		printConversaPluginParam (&conversaPluginParams);  // print conversaPluginParams parameters
		return;
	}

	/******************************/
	/* DISPAY CONVERSA VERSION    */
	retStatusConversa = NxpConversa_Plugin_GetLibVersion(	&conversaPluginParams,
														    (uint32_t*) &conversaVersionMajor,
															(uint32_t*) &conversaVersionMinor,
															(uint32_t*) &conversaVersionPatch);
	if (retStatusConversa != OK)
	{
		PRINTF("\tFAIL - DSP process init: Conversa failed to get lib version (error code = %i)\r\n", retStatusConversa);
		return;
	}
	else
	{
		PRINTF("\t-Conversa version: %i.%i.%i \r\n",conversaVersionMajor,conversaVersionMinor,conversaVersionPatch);
	}

	/************************/
	/* INITIALSIE CONVERSA 	*/
	retStatusConversa = NxpConversa_Plugin_Init(&conversaPluginParams);
	if (retStatusConversa != OK)
	{
		PRINTF("\tFAIL - DSP process init: Conversa failed to init (error code = %i)\r\n", retStatusConversa);
		printConversaPluginParam (&conversaPluginParams);  // print conversaPluginParams parameters
		return;
	}

	/*****************************/
	/* GET CONVERSA DATA ADDRESS
	 *  Get the address of the tuning structure
	 */

	s_conversaControlBlockAddress = (PL_UINT32) NxpConversa_Plugin_GetControlDataAddress( &conversaPluginParams );	// Get the the Conversa parameter structure address. This address must be in non cacheable aera

	PRINTF("\t-Conversa control data address ( to be used for the tuning tool 'Control Addr. (hex)' ) = 0x%x\r\n", &s_conversaControlBlockAddress);
	if (s_conversaControlBlockAddress < RT600_CACHE_MEM_START_ADR)
	{
		PRINTF("\t-FAIL - DSP process init: True Conversa control data address = 0x%x is located in a memory cached section\r\n", s_conversaControlBlockAddress); // ensure to provide a section called "NonCacheable" memory section in the linker script. Conversa library is using it.
		//return;
	}

	/******************************/
	/* UPDATE CONVERSA PARAMETERS */
	/* 	 Load conversa parameters from .c configuration file
	 */
	const conversa_parameter_config_t* 		p_address=&conversaParam7_3_1_q1bcMic034_Fb128_16kHz;
	retStatusConversa = NxpConversa_Plugin_SetParameters( &conversaPluginParams,
														  (const char*)      p_address->info_str,	   	// string information associated to the tuning file
														  sizeof            (p_address->info_str),	   				// size of the string information
														  (const uint32_t*) &p_address->parameter_data,	// tuning parameters to be set
														                     p_address->parameter_data_size );
	if (retStatusConversa != OK)
	{
		PRINTF("\tFAIL - DSP process init: Conversa failed to set parameter files (error code = %i)\r\n", retStatusConversa);
		return;
	}
	/* At this step Conversa instance is created */
}

void Init_AudioProcessOnEachFrame(void)
{

}
extern T_CircularAudioBuf_S16  VitCircBuff;
extern XosSem g_audioTask_audioVitProcessSemaphore;

__attribute__((__section__(".iram.text")))
void AudioProcessOnEachFrame(void)
{
	DbgPin7Up();

	int OutSampleNum;

	S32 *Ptr_Mic0=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[0];
	S32 *Ptr_Mic1=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[1];
	S32 *Ptr_Mic2=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[2];
	S32 *Ptr_Mic3=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[3];
	S32 *Ptr_Mic4=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[4];
	S32 *Ptr_Mic5=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[5];
	S32 *Ptr_Mic6=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[6];
	S32 *Ptr_Mic7=PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[7];

	S16 i;

	//generate sine tone for debugging purpose
	#if 0
		//sweeping signal overwrites USB down streaming L and R
		CycCnt1=read_ccount();
		GenerateSineTone          (&SineToneGenerator1, (float *)RawMic32BitBuf1, AudioFrameSizeInSamplePerCh,1);
		CycCnt2=read_ccount(); PtrVarBlockSharedByDspAndMcu->U32TmpArray[1]= CycCnt2-CycCnt1;
		CycCnt1=read_ccount();
		GenerateSineToneSingleFreq(&SineToneGenerator2, (float *)RawMic32BitBuf2, AudioFrameSizeInSamplePerCh,1);
		CycCnt2=read_ccount(); PtrVarBlockSharedByDspAndMcu->U32TmpArray[2]= CycCnt2-CycCnt1;

		vec_float2int((int *)RawMic32BitBuf1, (const float *)RawMic32BitBuf1,	-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)RawMic32BitBuf2, (const float *)RawMic32BitBuf2,	-31,  AudioFrameSizeInSamplePerCh);
	#endif


	#if 1	//folding --- step1: get samples from BT Dn cir buffer, and convert all mic input buffers to float
		if(PtrVarBlockSharedByDspAndMcu->BtFs==8000)
		{
			#if UsePrimitiveSimpleSrc==1
				for(int i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
				{
					BtRxInBuf[2*i+0]=PtrVarBlockSharedByDspAndMcu->BTRxInAudio[i];
					BtRxInBuf[2*i+1]=PtrVarBlockSharedByDspAndMcu->BTRxInAudio[i];
				}
			#else
				//int ProcCadenceAsrc(xa_codec_handle_t *xa_process_handle, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum)
				ProcCadenceAsrc( &SRC_process_Ref_handle,    BtRxInBuf, PtrVarBlockSharedByDspAndMcu->BTRxInAudio,  AudioFrameSizeInSamplePerCh/2,    &OutSampleNum  );
			#endif

			vec_int2float((float *)BtRxInBuf, (const int *)BtRxInBuf,	                            -31,  AudioFrameSizeInSamplePerCh);
		}else if(PtrVarBlockSharedByDspAndMcu->BtFs==16000)
		{
			vec_int2float((float *)BtRxInBuf, (const int *)PtrVarBlockSharedByDspAndMcu->BTRxInAudio,	-31,  AudioFrameSizeInSamplePerCh);
		}else
		{
			//should never come here
			PRINTF("DSP: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}

		vec_int2float((float *)Ptr_Mic0, (const int *)Ptr_Mic0,	-31,  AudioFrameSizeInSamplePerCh);
		//vec_int2float((float *)Ptr_Mic1, (const int *)Ptr_Mic1,	-31,  AudioFrameSizeInSamplePerCh);
		vec_int2float((float *)Ptr_Mic2, (const int *)Ptr_Mic2,	-31,  AudioFrameSizeInSamplePerCh);
		//vec_int2float((float *)Ptr_Mic3, (const int *)Ptr_Mic3,	-31,  AudioFrameSizeInSamplePerCh);
		vec_int2float((float *)Ptr_Mic4, (const int *)Ptr_Mic4,	-31,  AudioFrameSizeInSamplePerCh);
		//vec_int2float((float *)Ptr_Mic5, (const int *)Ptr_Mic5,	-31,  AudioFrameSizeInSamplePerCh);
		vec_int2float((float *)Ptr_Mic6, (const int *)Ptr_Mic6,	-31,  AudioFrameSizeInSamplePerCh);
		//vec_int2float((float *)Ptr_Mic7, (const int *)Ptr_Mic7,	-31,  AudioFrameSizeInSamplePerCh);

		memcpy(RawMic32BitBuf0,Ptr_Mic0,sizeof(S32)*AudioFrameSizeInSamplePerCh);
		memcpy(RawMic32BitBuf1,Ptr_Mic2,sizeof(S32)*AudioFrameSizeInSamplePerCh);
		memcpy(RawMic32BitBuf2,Ptr_Mic4,sizeof(S32)*AudioFrameSizeInSamplePerCh);
		memcpy(RawMic32BitBuf3,Ptr_Mic6,sizeof(S32)*AudioFrameSizeInSamplePerCh);
	#endif

	#if 1	//folding --- step2: Conversa processing
		//real conversa
		NXP_STATUS retStatusConversa = OK;
		status_t   retStatus 		 = kStatus_Success;

		S32 VitInTmpBuf32Bit [AudioFrameSizeInSamplePerCh];
		S16 VitInTmpBuf16Bit [AudioFrameSizeInSamplePerCh];

		PL_FLOAT*  pp_inputAudioData_Tx_FLT [8];
		PL_FLOAT*  pp_inputAudioData_Rx_FLT [8];
		PL_FLOAT** p_currentSensingInput 		= NULL;		// current sensing input
		PL_FLOAT*  p_outputAudioData_Tx_FLT 	= NULL; 	// conversa out pointer
		PL_FLOAT*  ConversaTmpOutputPtr			= NULL;
		PL_FLOAT** pp_outputAudioData_Rx_FLT 	= NULL;

		pp_inputAudioData_Tx_FLT[0]=(float *)Ptr_Mic2;
		pp_inputAudioData_Tx_FLT[1]=(float *)Ptr_Mic4;
		pp_inputAudioData_Tx_FLT[2]=(float *)Ptr_Mic6;

		//memset(BtRxInBuf,0,sizeof(BtRxInBuf));
		pp_inputAudioData_Rx_FLT[0]=(float *)BtRxInBuf;

		retStatusConversa = NxpConversa_Plugin_Process( &conversaPluginParams,
													    pp_inputAudioData_Tx_FLT,
														pp_inputAudioData_Rx_FLT,
														p_currentSensingInput );

		if (retStatusConversa != OK) 						// if return status not OK
		{
			if (retStatusConversa == LICENSE_EXPIRED)       // if license expired occurs
			{
				PRINTF("FAIL: conversa LICENSE_EXPIRED error\r\n");
				retStatus = kStatus_LicenseError;
			}
			else
			{
				PRINTF("FAIL: conversa process error %d\r\n",retStatusConversa);
				retStatus = kStatus_Fail;
			}
		}

		p_outputAudioData_Tx_FLT = NxpConversa_Plugin_GetTxOut(&conversaPluginParams);
		if (p_outputAudioData_Tx_FLT != PL_NULL)
		{
			memcpy(ConversaTxOut32BitBuf, p_outputAudioData_Tx_FLT, sizeof(float)*AudioFrameSizeInSamplePerCh);
		}
		else
		{
			PRINTF("FAIL: conversaProcess return NULL TxOut pointer\r\n");
			retStatus = kStatus_NullPointer;
		}

		pp_outputAudioData_Rx_FLT = NxpConversa_Plugin_GetRxOut(&conversaPluginParams);
		if (pp_outputAudioData_Rx_FLT[0] != PL_NULL)
		{
		}
		else
		{
			PRINTF("FAIL: conversaProcess return NULL RxOut pointer\r\n");
			retStatus = kStatus_OutOfRange;
		}


		//------------------take out internal output streaming from conversa and put to VIT input cir buffer, with converting float --> int --> short in-------
		//---beg---
		#if 1
			ConversaTmpOutputPtr = NxpConversa_Plugin_GetTxAecOut(&conversaPluginParams, 0);
			if(ConversaTmpOutputPtr!=PL_NULL)
				memcpy(AecOut32BitBuf, ConversaTmpOutputPtr, sizeof(float)*AudioFrameSizeInSamplePerCh);
		#endif

		#if 1
			ConversaTmpOutputPtr = NxpConversa_Plugin_GetTxBfOut(&conversaPluginParams);
			if(ConversaTmpOutputPtr!=PL_NULL)
				memcpy(BfOut32BitBuf, ConversaTmpOutputPtr, sizeof(float)*AudioFrameSizeInSamplePerCh);
		#endif

		#if 1
			ConversaTmpOutputPtr = NxpConversa_Plugin_GetTxNlpOut(&conversaPluginParams);
			if(ConversaTmpOutputPtr!=PL_NULL)
				memcpy(NlpOut32BitBuf, ConversaTmpOutputPtr, sizeof(float)*AudioFrameSizeInSamplePerCh);
		#endif

		//select one of the following 5
		vec_float2int((int *)VitInTmpBuf32Bit, (const float *)BfOut32BitBuf,		-31,  AudioFrameSizeInSamplePerCh);		//use conversa bf out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)AecOut32BitBuf,		-31,  AudioFrameSizeInSamplePerCh);		//use conversa aec out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)NlpOut32BitBuf,		-31,  AudioFrameSizeInSamplePerCh);		//use conversa nlp out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)ConversaTxOut32BitBuf,-31,  AudioFrameSizeInSamplePerCh);	//use conversa final tx out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)RawMic32BitBuf0,		-31,  AudioFrameSizeInSamplePerCh);	//use raw mic 0 bf out as VIT input

		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
			VitInTmpBuf16Bit[i]=(VitInTmpBuf32Bit[i]>>16);

		if(CirAudioBuf_SpaceAvailableInSamples_S16(&VitCircBuff) >= AudioFrameSizeInSamplePerCh)
		{
			//PRINTF(".\r\n");
			CirAudioBuf_WriteSamples_S16(&VitCircBuff, AudioFrameSizeInSamplePerCh, VitInTmpBuf16Bit);
		}

		//trigger VIT task to step on
		if(CirAudioBuf_SpaceOccupiedInSamples_S16(&VitCircBuff) >= VIT_SAMPLES_PER_30MS_FRAME)
		{
			xos_sem_put( &g_audioTask_audioVitProcessSemaphore );  	// Audio process semaphore put
			//PRINTF("1\r\n");
		}
		//---end---
		//------------------take out internal output streaming from conversa and put to VIT input cir buffer, with converting float --> int --> short in-------



		//real conversa rx out --> to spk out L
		float *FltDstPtr=(float *)SpkOtBufL;
		float *FltSrcPtr=(float *)pp_outputAudioData_Rx_FLT[0];
		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			*FltDstPtr++=*FltSrcPtr++;
		}

		//raw mic1 signal --> to spk out R --- can be changed to other interested audio source
		FltDstPtr=(float *)SpkOtBufR;
		FltSrcPtr=(float *)RawMic32BitBuf0;
		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			*FltDstPtr++=*FltSrcPtr++;
		}


		//real conversa tx out --> to BT tx out
		FltDstPtr=(float *)BtTxOtBuf;
		FltSrcPtr=(float *)p_outputAudioData_Tx_FLT;
		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			*FltDstPtr++=*FltSrcPtr++;
		}

	#else
		//fake conversa down link --- ref in --> spk out
		float *FltDstPtr=(float *)SpkOtBufL;
		float *FltSrcPtr=(float *)BtRxInBuf;
		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			*FltDstPtr++=*FltSrcPtr++;
		}

		//fake conversa up link --- mic0 --> tx out
		FltDstPtr=(float *)BtTxOtBuf;
		FltSrcPtr=(float *)Ptr_Mic0;
		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			*FltDstPtr++=*FltSrcPtr++;
		}

		//raw mic1 signal --> spk out
		FltDstPtr=(float *)SpkOtBufR;
		FltSrcPtr=(float *)Ptr_Mic1;
		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			*FltDstPtr++=*FltSrcPtr++;
		}
	#endif


	#if 1	//folding --- step3: put samples to BT Up cir buffer, and convert tx samples from float to int, and SpkOtBufL to PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL
		//convert SpkOtBufL to int and put to  PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL
		#if 1
			vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL, (const float *)SpkOtBufL,	-31,  AudioFrameSizeInSamplePerCh);
			vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR, (const float *)SpkOtBufR,	-31,  AudioFrameSizeInSamplePerCh);	//mic1 to spk R --- this is for test
		#else
			//conversa BF output
			vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL, (const float *)BtTxOtBuf,	-31,  AudioFrameSizeInSamplePerCh);
			//memcpy(PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR, ConversaTxOut32BitBuf, sizeof(float)*AudioFrameSizeInSamplePerCh);

			//raw mic0
			//memcpy(PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR, RawMic32BitBuf1,      sizeof(float)*AudioFrameSizeInSamplePerCh);
			vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR, (const float *)SpkOtBufL,	-31,  AudioFrameSizeInSamplePerCh);	//mic1 to spk R --- this is for test


			//vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL, (const float *)RawMic32BitBuf0,	-31,  AudioFrameSizeInSamplePerCh);
			//vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR, (const float *)RawMic32BitBuf1,	-31,  AudioFrameSizeInSamplePerCh);

		#endif
		if(PtrVarBlockSharedByDspAndMcu->BtFs==8000)
		{
			vec_float2int((int *)BtTxOtBuf, (const float *)BtTxOtBuf,	-31,  AudioFrameSizeInSamplePerCh);
			#if UsePrimitiveSimpleSrc==1
				for(int i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
				{
					PtrVarBlockSharedByDspAndMcu->BTTxOtAudio[i]=BtTxOtBuf[2*i+0];
				}
			#else
				//int ProcCadenceAsrc(xa_codec_handle_t *xa_process_handle, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum)
				ProcCadenceAsrc( &SRC_process_TxOut_handle,  PtrVarBlockSharedByDspAndMcu->BTTxOtAudio, BtTxOtBuf,  AudioFrameSizeInSamplePerCh,      &OutSampleNum  );
			#endif
		}else if(PtrVarBlockSharedByDspAndMcu->BtFs==16000)
		{
			vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->BTTxOtAudio, (const float *)BtTxOtBuf,	-31,  AudioFrameSizeInSamplePerCh);
		}else
		{
			//should never come here
			PRINTF("DSP: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}
	#endif


	#if 1	//folding --- step4: put interested audio channels to UAC 8 ch: mic0,1,2,3, ConversaTxOut, AecOut[0], BfOut, NlpOut
		//S32 UacUpAudioBuf[AudioFrameSizeInSamplePerCh*8];
		vec_float2int((int *)RawMic32BitBuf0, (const float *)RawMic32BitBuf0,	-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)RawMic32BitBuf1, (const float *)RawMic32BitBuf1,	-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)RawMic32BitBuf2, (const float *)RawMic32BitBuf2,	-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)RawMic32BitBuf3, (const float *)RawMic32BitBuf3,	-31,  AudioFrameSizeInSamplePerCh);

		vec_float2int((int *)BfOut32BitBuf,   (const float *)BfOut32BitBuf,		-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)NlpOut32BitBuf,  (const float *)NlpOut32BitBuf,	-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)AecOut32BitBuf,  (const float *)AecOut32BitBuf,	-31,  AudioFrameSizeInSamplePerCh);
		vec_float2int((int *)ConversaTxOut32BitBuf, (const float *)ConversaTxOut32BitBuf,	-31,  AudioFrameSizeInSamplePerCh);

		for(i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=RawMic32BitBuf0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=RawMic32BitBuf1[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=RawMic32BitBuf2[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=RawMic32BitBuf3[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=ConversaTxOut32BitBuf[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=AecOut32BitBuf[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=BfOut32BitBuf[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=NlpOut32BitBuf[i];
		}
	#endif

	frmcount++;
	#if 1
		if ((frmcount % 128) < 64)
			LedOn_G();
		else
			LedOff_G();
	#endif

	DbgPin7Dn();
}
