/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#if 1	//folding
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
#include "MeterAndCompressor.h"

#ifndef CORE_DSP
#define CORE_DSP
#endif

#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"

#include "DspMainAudioFlow.h"
#include "AudioDecoder.h"


//tuning parameter file
#include "conversaParam7_3_1_q1bcMic034_Fb128_16kHz.h"
#include "conversaParam7_5_0_q2bcMic034_Fb128_fbfFront_16kHz.h"

#include "conversaParam_virtual2Dmic32k_smh.h"	//this one is included only as an example that shows more tuning cfg files can be added --- later to be removed
#include "conversaParam_virtual4Dmic32k_smh.h"	//this one is included only as an example that shows more tuning cfg files can be added --- later to be removed



// Se model small for 16k
#include "ml_i16xi8_GDFT128-128-768_16kHz.h"			// Conversa Speech enhancement Model small at 16Khz

#define UsePrimitiveSimpleSRC		0
#define SkipConversa				0
#define EnableLvlMeter				1

extern XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.

nxp_conversa_plugin_t conversaPluginParams;
__attribute__((__section__(".control_reg_addr")))
PL_UINT32 s_conversaControlBlockAddress;

__attribute__((__section__("NonCacheable")))
uint8_t s_conversaCriticalmemory[76800];

S32 *BtRxInBuf;
S32 *BtTxOtBuf;

S32 *SpkOtBufL;
S32 *SpkOtBufR;

S32 *RawMic32BitBuf0;
S32 *RawMic32BitBuf1;
S32 *RawMic32BitBuf2;
S32 *RawMic32BitBuf3;

S32 *ConversaTxOut32BitBuf;
S32 *AecOut32BitBuf;
S32 *BfOut32BitBuf;
S32 *NlpOut32BitBuf;

#if EnableLvlMeter==1
	float MicInMeterLvl1_InDb;
	float MicInMeterLvl2_InDb;
	float MicInMeterLvl3_InDb;
	float MicInMeterLvl4_InDb;
	float ConversaOutLvl1_InDb;
	float ConversaOutLvl2_InDb;
	float ConversaOutLvl3_InDb;
	float ConversaOutLvl4_InDb;
#endif

#endif

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
	p_conversaPluginConfig->framesize 	= AudioFrameSizeInSamplePerCh_16KHz;

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
	//p_conversaPluginConfig->create_bf 			= CONVERSA_BF_MODE_ADAPTIVE_OR_FIX;		// Conversa beam former mode (select adaptive + fix steering available)
	p_conversaPluginConfig->create_bf           = CONVERSA_BF_MODE_ADAPTIVE;

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

void InitConversaBufPtr(void)
{
	//some of the 48KHz buffers are used, this is OK for 16KHz processing. Extra more space doesn't hurt anything
	BtRxInBuf=AudioOneFrameBuf_48KHz_01;
	BtTxOtBuf=AudioOneFrameBuf_48KHz_02;

	SpkOtBufL=AudioOneFrameBuf_48KHz_03;
	SpkOtBufR=AudioOneFrameBuf_48KHz_04;

	RawMic32BitBuf0=AudioOneFrameBuf_16KHz_01;
	RawMic32BitBuf1=AudioOneFrameBuf_16KHz_02;
	RawMic32BitBuf2=AudioOneFrameBuf_16KHz_03;
	RawMic32BitBuf3=AudioOneFrameBuf_16KHz_04;

	ConversaTxOut32BitBuf=AudioOneFrameBuf_48KHz_05;
	AecOut32BitBuf=		  AudioOneFrameBuf_48KHz_06;
	BfOut32BitBuf=		  AudioOneFrameBuf_48KHz_07;
	NlpOut32BitBuf=		  AudioOneFrameBuf_48KHz_08;
}

void DeInitConversa(void)
{
	if (conversaPluginParams.memory_base_address != PL_NULL)
	{
		free(conversaPluginParams.memory_base_address);
	}
}
//status_t initCreateConversa( AUDIO_conversa_st* p_definitionConversa )
void InitConversa(ConversaTuningCfg_t TuningCfg, int NeedToPrintInfo)
{

	NXP_STATUS retStatusConversa = OK;

	initSetConversaInstParam(&conversaPluginParams);

	// conversa version
	PL_UINT32 conversaVersionMajor;
	PL_UINT32 conversaVersionMinor;
	PL_UINT32 conversaVersionPatch;

	conversa_memory_req_t conversaMemReq; // Memory size (Cache and non cached) required by conversa in bytes

	if(NeedToPrintInfo)
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

	if(NeedToPrintInfo)
	{
	PRINTF("\t-Conversa memory size required: %i bytes\r\n", conversaMemReq.NonCritical_mem);
	PRINTF("\t-Conversa critical memory size required: %i bytes\r\n", conversaMemReq.Critical_mem);
	}

	/* Reserved required memory for Conversa instance */
	conversaPluginParams.memory_base_address = malloc(conversaPluginParams.memory_size_bytes);   // Get Conversa memory base address from malloc function

	/* Check Convera instance memory allocation pass */
	if (conversaPluginParams.memory_base_address != PL_NULL)				// if Conversa memory allocation pass
	{
		if(NeedToPrintInfo)
		PRINTF("\t-Conversa memory allocation: PASS, structure at adr 0x%8.8X\r\n", conversaPluginParams.memory_base_address);
	}
	else																						// else if Conversa memory allocation fail
	{
		PRINTF("\tFAIL - DSP process init: Conversa memory allocation failed \r\n");
		printConversaPluginParam (&conversaPluginParams);  // print conversaPluginParams parameters
		return;
	}

	if(NeedToPrintInfo)
	PRINTF("\t-Conversa critical memory size required: %i bytes\r\n", conversaPluginParams.critical_memory_size_bytes);

	/* Reserved required memory for Conversa instance */
	conversaPluginParams.critical_memory_base_address = (void *) s_conversaCriticalmemory;   // Get Conversa memory base address from malloc function

	/* Check Convera instance memory allocation pass */
	if (conversaPluginParams.critical_memory_base_address != PL_NULL)				// if Conversa memory allocation pass
	{
		if(NeedToPrintInfo)
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
		if(NeedToPrintInfo)
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

	if(NeedToPrintInfo)
	PRINTF("\t-Conversa control data address ( to be used for the tuning tool 'Control Addr. (hex)' ) = 0x%x\r\n", &s_conversaControlBlockAddress);
	if (s_conversaControlBlockAddress < RT600_CACHE_MEM_START_ADR)
	{
		PRINTF("\t-FAIL - DSP process init: True Conversa control data address = 0x%x is located in a memory cached section\r\n", s_conversaControlBlockAddress); // ensure to provide a section called "NonCacheable" memory section in the linker script. Conversa library is using it.
		return;
	}

	/******************************/
	/* UPDATE CONVERSA PARAMETERS */
	/* 	 Load conversa parameters from .c configuration file
	 */
	const conversa_parameter_config_t* 		p_address;
	switch((int)TuningCfg)
	{
		case (int)ConversaTuningCfg_HfpVoiceCall:
			p_address=&conversaParam7_3_1_q1bcMic034_Fb128_16kHz;
		break;
		case (int)ConversaTuningCfg_NearEnd:
			p_address=&conversaParam7_3_1_q1bcMic034_Fb128_16kHz;
			//just for example //p_address=&conversaParam_virtual2Dmic32k_smh;
		break;
		case (int)ConversaTuningCfg_FarEnd:
			p_address=&conversaParam7_5_0_q2bcMic034_Fb128_fbfFront_16kHz;
		break;
	}

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

	CurrentConversaTuningCfg=TuningCfg;

	/* At this step Conversa instance is created */
	InitConversaBufPtr();
}

extern T_CircularAudioBuf_S16  VitCircBuff;
extern T_CircularAudioBuf_S16  VitCircBuff_RawMic;



__attribute__((__section__(".iram.text")))
void DspMainAudioFlowProcOneFrame_HfpCall(int OptionWord)
{
	//DbgPin7Up();

	int OutSampleNum;

	S32 *Ptr_Mic0=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[0];
	S32 *Ptr_Mic1=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[1];
	S32 *Ptr_Mic2=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[2];
	S32 *Ptr_Mic3=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[3];
	S32 *Ptr_Mic4=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[4];
	S32 *Ptr_Mic5=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[5];
	S32 *Ptr_Mic6=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[6];
	S32 *Ptr_Mic7=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[7];

	S16 RawMicSignal16BitForVitRef[AudioFrameSizeInSamplePerCh_16KHz];
	float TmpBuf_PromptSound[AudioFrameSizeInSamplePerCh_16KHz];

	S16 i;


	#if 0	//folding --- step 0: generate sine tone for debugging purpose
		//sweeping signal overwrites USB down streaming L and R
		CycCnt1=read_ccount();
		GenerateSineTone          (&SineToneGenerator1, (float *)RawMic32BitBuf1, AudioFrameSizeInSamplePerCh_16KHz,1);
		CycCnt2=read_ccount(); PtrVarBlockSharedByDspAndMcu->MonitorInfoArray1[1]= CycCnt2-CycCnt1;
		CycCnt1=read_ccount();
		GenerateSineToneSingleFreq(&SineToneGenerator2, (float *)RawMic32BitBuf2, AudioFrameSizeInSamplePerCh_16KHz,1);
		CycCnt2=read_ccount(); PtrVarBlockSharedByDspAndMcu->MonitorInfoArray1[2]= CycCnt2-CycCnt1;

		vec_float2int((int *)RawMic32BitBuf1, (const float *)RawMic32BitBuf1,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)RawMic32BitBuf2, (const float *)RawMic32BitBuf2,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
	#endif

	#if 1	//folding --- step 2: get audio from sbc decoder and opus decoder, or clear the decoder buffer if the decoder is NOT running
		//in this mode, OPUS and SBC stream are converted to 16KHz Fs
		#if EnableOpusDec==1
			unsigned short *OtPtrS16_Opus;
			//take out OPUS output audio, and mix with UAC, with satuation, no gaining
			xos_mutex_lock(&g_audio_OpusDecoderMutex);
				if(CirAudioBuf_SpaceOccupiedInSamples_S32(&OpusOutputCirBuf_LRMixed) >= AudioFrameSizeInSamplePerCh_16KHz)
				{
					OtPtrS16_Opus=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&OpusOutputCirBuf_LRMixed, AudioFrameSizeInSamplePerCh_16KHz);
					for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
					{
						AudioOneFrameBuf_OpusDecodedL[i]=*OtPtrS16_Opus++;
						AudioOneFrameBuf_OpusDecodedR[i]=*OtPtrS16_Opus++;
					}
				}else
				{
					memset(AudioOneFrameBuf_OpusDecodedL,0,sizeof(AudioOneFrameBuf_OpusDecodedL));
					memset(AudioOneFrameBuf_OpusDecodedR,0,sizeof(AudioOneFrameBuf_OpusDecodedR));
				}
			xos_mutex_unlock(&g_audio_OpusDecoderMutex);
		#endif
		#if EnableSbcDec==1
			unsigned short *OtPtrS16_Sbc;
			//take out Sbc output audio, and mix with UAC, with satuation, no gaining
			xos_mutex_lock(&g_audio_SbcDecoderMutex);
				if(CirAudioBuf_SpaceOccupiedInSamples_S32(&SbcOutputCirBuf_LRMixed) >= AudioFrameSizeInSamplePerCh_16KHz)
				{
					OtPtrS16_Sbc=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&SbcOutputCirBuf_LRMixed, AudioFrameSizeInSamplePerCh_16KHz);
					for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
					{
						AudioOneFrameBuf_SbcDecodedL[i]=*OtPtrS16_Sbc++;
						AudioOneFrameBuf_SbcDecodedR[i]=*OtPtrS16_Sbc++;
					}
				}else
				{
					memset(AudioOneFrameBuf_SbcDecodedL,0,sizeof(AudioOneFrameBuf_SbcDecodedL));
					memset(AudioOneFrameBuf_SbcDecodedR,0,sizeof(AudioOneFrameBuf_SbcDecodedR));
				}
			xos_mutex_unlock(&g_audio_SbcDecoderMutex);
		#endif
	#endif

	#if 1	//folding --- step 2: get samples from BT Dn cir buffer, and convert all mic input buffers to float
		if(OptionWord==MuEvtMcuToDsp_AudioFrmIsReady_HfpCall)
		{
			//in this case, conversa Tx and Rx are both really working, need to get the real Rx audio from BT
			if(PtrVarBlockSharedByDspAndMcu->BtHfpFs==8000)
			{
				#if UsePrimitiveSimpleSRC==1
					for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz/2;i++)
					{
						BtRxInBuf[2*i+0]=PtrVarBlockSharedByDspAndMcu->AudioBufInFrBt[i];
						BtRxInBuf[2*i+1]=PtrVarBlockSharedByDspAndMcu->AudioBufInFrBt[i];
					}
				#else
					//int ProcCadenceAsrc(TCadenceSRC *SRCPtr, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum)
					ProcCadenceAsrc(&SRC_ConversaRx1,    BtRxInBuf, (S32 *)PtrVarBlockSharedByDspAndMcu->AudioBufInFrBt,  AudioFrameSizeInSamplePerCh_16KHz/2,    &OutSampleNum  );
				#endif

				vec_int2float((float *)BtRxInBuf, (const int *)BtRxInBuf,	                            -31,  AudioFrameSizeInSamplePerCh_16KHz);
			}else if(PtrVarBlockSharedByDspAndMcu->BtHfpFs==16000)
			{
				vec_int2float((float *)BtRxInBuf, (const int *)PtrVarBlockSharedByDspAndMcu->AudioBufInFrBt,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
			}else
			{
				//should never come here
				PRINTF("DSP: audio flow error --- BT fs is not 8Khz or 16KHz - call\r\n");
			}
		}else if(OptionWord==MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy)
		{
			//in this case, only conversa Tx for VIT is really working, no need for audio from BT, set Rx audio to 0
			memset((S32 *)BtRxInBuf,0,sizeof(int)*AudioFrameSizeInSamplePerCh_16KHz);
		}else
		{
			//should never come here
			PRINTF("DSP: event from MCU is error \r\n");
		}

		//convert to float
		for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			TmpBuf_PromptSound[i]=AudioOneFrameBuf_OpusDecodedL[i]*_Value_Pow_2_Neg15_;

		//use AudioOneFrameBuf_OpusDecodedR as a temp buffer
		vec_addf((float *)AudioOneFrameBuf_OpusDecodedR, (const float32_t*)BtRxInBuf, (const float32_t*)TmpBuf_PromptSound, AudioFrameSizeInSamplePerCh_16KHz);
		memcpy(BtRxInBuf, AudioOneFrameBuf_OpusDecodedR, sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);		//now, BtRxInBuf is added with wakesound

		//save raw mic0 data in 16 bit format, later VIT will use it
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			RawMicSignal16BitForVitRef[i]=(Ptr_Mic0[i]>>16);


		//here ??? can not explain: if to use Ptr_Mic0,1,2,3 inplace converting, conversa input is wrong.
		//have to use Ptr_Mic4,5,6,7 for holding the converted float samples from Ptr_Mic0,1,2,3
		vec_int2float((float *)Ptr_Mic4, (const int *)Ptr_Mic0,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_int2float((float *)Ptr_Mic5, (const int *)Ptr_Mic1,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_int2float((float *)Ptr_Mic6, (const int *)Ptr_Mic2,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_int2float((float *)Ptr_Mic7, (const int *)Ptr_Mic3,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float((float *)Ptr_Mic4, (const int *)Ptr_Mic4,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float((float *)Ptr_Mic5, (const int *)Ptr_Mic5,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float((float *)Ptr_Mic6, (const int *)Ptr_Mic6,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float((float *)Ptr_Mic7, (const int *)Ptr_Mic7,	-31,  AudioFrameSizeInSamplePerCh_16KHz);

		memcpy(RawMic32BitBuf0,Ptr_Mic4,sizeof(S32)*AudioFrameSizeInSamplePerCh_16KHz);
		memcpy(RawMic32BitBuf1,Ptr_Mic5,sizeof(S32)*AudioFrameSizeInSamplePerCh_16KHz);
		memcpy(RawMic32BitBuf2,Ptr_Mic6,sizeof(S32)*AudioFrameSizeInSamplePerCh_16KHz);
		memcpy(RawMic32BitBuf3,Ptr_Mic7,sizeof(S32)*AudioFrameSizeInSamplePerCh_16KHz);

		#if EnableLvlMeter==1
			MicInMeterLvl1_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter1, (float *)RawMic32BitBuf0);
			MicInMeterLvl2_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter2, (float *)RawMic32BitBuf1);
			MicInMeterLvl3_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter3, (float *)RawMic32BitBuf2);
			MicInMeterLvl4_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter4, (float *)RawMic32BitBuf3);
		#endif

	#endif

	#if 1	//folding --- step 3: Conversa processing
		#if SkipConversa==0	//folding --- step2: Conversa processing
			//real conversa

			PL_FLOAT*  pp_inputAudioData_Tx_FLT [4];
			PL_FLOAT*  pp_OutputAudioSignals[5];	//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut

			#if 1
				pp_inputAudioData_Tx_FLT[0]=(float *)Ptr_Mic4; //A3, glasses mic location
				pp_inputAudioData_Tx_FLT[1]=(float *)Ptr_Mic5; //C7, glasses mic location
				pp_inputAudioData_Tx_FLT[2]=(float *)Ptr_Mic6; //C8, glasses mic location
			#else
				pp_inputAudioData_Tx_FLT[0]=(float *)RawMic32BitBuf0;
				pp_inputAudioData_Tx_FLT[1]=(float *)RawMic32BitBuf1;
				pp_inputAudioData_Tx_FLT[2]=(float *)RawMic32BitBuf2;
			#endif

			//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut
			ConversaProcessAndFeedToVit(pp_inputAudioData_Tx_FLT, (float *)BtRxInBuf, pp_OutputAudioSignals, RawMicSignal16BitForVitRef);


			#if EnableLvlMeter==1
				memcpy(ConversaTxOut32BitBuf, pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut],  sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);	//TxOut
				memcpy(AecOut32BitBuf, 		  pp_OutputAudioSignals[CONVERSA_OutSignalIdx_AecOut], sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);	//AecOut
				memcpy(NlpOut32BitBuf, 		  pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut],  sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);	//BfOut
				memcpy(BfOut32BitBuf, 		  pp_OutputAudioSignals[CONVERSA_OutSignalIdx_NlpOut], sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);	//NlpOut

				#if 0
					ConversaOutLvl1_InDb=GetRmsMeterLogValue(AudioMeter1, (float *)ConversaTxOut32BitBuf);
					ConversaOutLvl2_InDb=GetRmsMeterLogValue(AudioMeter2, (float *)AecOut32BitBuf);
					ConversaOutLvl3_InDb=GetRmsMeterLogValue(AudioMeter3, (float *)BfOut32BitBuf);
					ConversaOutLvl4_InDb=GetRmsMeterLogValue(AudioMeter4, (float *)NlpOut32BitBuf);
				#endif
				#if 0
					ConversaOutLvl1_InDb=GetPeakMeterValue(AudioMeter1, (float *)ConversaTxOut32BitBuf);
					ConversaOutLvl2_InDb=GetPeakMeterValue(AudioMeter2, (float *)AecOut32BitBuf);
					ConversaOutLvl3_InDb=GetPeakMeterValue(AudioMeter3, (float *)BfOut32BitBuf);
					ConversaOutLvl4_InDb=GetPeakMeterValue(AudioMeter4, (float *)NlpOut32BitBuf);
				#endif
				#if 1
					ConversaOutLvl1_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter5, (float *)ConversaTxOut32BitBuf);
					ConversaOutLvl2_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter6, (float *)AecOut32BitBuf);
					ConversaOutLvl3_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter7, (float *)BfOut32BitBuf);
					ConversaOutLvl4_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter8, (float *)NlpOut32BitBuf);
				#endif
			#endif


			//real conversa rx out --> to spk out L
			memcpy(SpkOtBufL,pp_OutputAudioSignals[CONVERSA_OutSignalIdx_RxOut],sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);

			//raw mic1 signal --> to spk out R --- can be changed to other interested audio source
			//memcpy(SpkOtBufR,RawMic32BitBuf0,sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);

			// gill Do not pass Raw Mic audio into Speaker, will generate echo
			memcpy(SpkOtBufR,SpkOtBufL,sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);

			//real conversa tx out --> to BT tx out
			memcpy(BtTxOtBuf,pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut],sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);

		#else
			//fake conversa down link --- ref in --> spk out L
			float *FltDstPtr=(float *)SpkOtBufL;
			float *FltSrcPtr=(float *)BtRxInBuf;
			for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				*FltDstPtr++=*FltSrcPtr++;
			}

			//fake conversa up link --- mic0 --> tx out
			FltDstPtr=(float *)BtTxOtBuf;
			FltSrcPtr=(float *)Ptr_Mic4;
			for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				*FltDstPtr++=*FltSrcPtr++;
			}

			//raw mic1 signal --> spk out R
			FltDstPtr=(float *)SpkOtBufR;
			FltSrcPtr=(float *)Ptr_Mic5;	//could be any one of other raw mics
			for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				*FltDstPtr++=*FltSrcPtr++;
				//*FltDstPtr++=0.001f * i;
			}

			//------------------take out raw mic1 signal and put to VIT input cir buffer-----------------
			//---beg---
			#if 1
				xos_mutex_lock(&g_audio_vitBufferMutex);
					if(CirAudioBuf_SpaceAvailableInSamples_S16(&VitCircBuff) >= AudioFrameSizeInSamplePerCh_16KHz)
					{
						//PRINTF(".\r\n");
						CirAudioBuf_WriteSamples_S16(&VitCircBuff,        AudioFrameSizeInSamplePerCh_16KHz, RawMicSignal16BitForVitRef);
						CirAudioBuf_WriteSamples_S16(&VitCircBuff_RawMic, AudioFrameSizeInSamplePerCh_16KHz, RawMicSignal16BitForVitRef);
					}
					//trigger VIT task to step on
					if(CirAudioBuf_SpaceOccupiedInSamples_S16(&VitCircBuff) >= VIT_SAMPLES_PER_30MS_FRAME)
					{
						xos_sem_put( &g_audioTask_audioVitProcessSemaphore );  	// Audio process semaphore put
						//PRINTF("1\r\n");
					}
				xos_mutex_unlock(&g_audio_vitBufferMutex);
			#endif
			//---end---
			//------------------take out raw mic1 signal and put to VIT input cir buffer-----------------


			#if 1	//put raw mic 1 to
				memcpy(ConversaTxOut32BitBuf, Ptr_Mic4, sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);
				memcpy(AecOut32BitBuf,        Ptr_Mic4, sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);
				memcpy(BfOut32BitBuf,         Ptr_Mic4, sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);
				memcpy(NlpOut32BitBuf,        Ptr_Mic4, sizeof(float)*AudioFrameSizeInSamplePerCh_16KHz);

				#if EnableLvlMeter==1
					#if 0
						ConversaOutLvl1_InDb=GetRmsMeterLogValue(AudioMeter1, (float *)ConversaTxOut32BitBuf);
						ConversaOutLvl2_InDb=GetRmsMeterLogValue(AudioMeter2, (float *)AecOut32BitBuf);
						ConversaOutLvl3_InDb=GetRmsMeterLogValue(AudioMeter3, (float *)BfOut32BitBuf);
						ConversaOutLvl4_InDb=GetRmsMeterLogValue(AudioMeter4, (float *)NlpOut32BitBuf);
					#endif
					#if 0
						ConversaOutLvl1_InDb=GetPeakMeterValue(AudioMeter1, (float *)ConversaTxOut32BitBuf);
						ConversaOutLvl2_InDb=GetPeakMeterValue(AudioMeter2, (float *)AecOut32BitBuf);
						ConversaOutLvl3_InDb=GetPeakMeterValue(AudioMeter3, (float *)BfOut32BitBuf);
						ConversaOutLvl4_InDb=GetPeakMeterValue(AudioMeter4, (float *)NlpOut32BitBuf);
					#endif
					#if 1
						ConversaOutLvl1_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter5, (float *)ConversaTxOut32BitBuf);
						ConversaOutLvl2_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter6, (float *)AecOut32BitBuf);
						ConversaOutLvl3_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter7, (float *)BfOut32BitBuf);
						ConversaOutLvl4_InDb=GetSignaLvlIndB_ByFrameRms(AudioRmsMeter8, (float *)NlpOut32BitBuf);
					#endif
				#endif
			#endif
		#endif
	#endif

	#if 1	//folding --- step 4: put samples to BT Up cir buffer, and convert tx samples from float to int, and SpkOtBufL/R to PtrVarBlockSharedByDspAndMcu->I2SBufOtToAmpL/R

		//stream out AMP I2S audio
		//use BtRxInBuf as a temp --- content in BtRxInBuf is already useless after calling conversa main processing
		vec_float2int((int *)BtRxInBuf, (const float *)SpkOtBufL,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			PtrVarBlockSharedByDspAndMcu->I2SBufOtToAmpL[i]=((BtRxInBuf[i])>>16);

		vec_float2int((int *)BtRxInBuf, (const float *)SpkOtBufR,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			PtrVarBlockSharedByDspAndMcu->I2SBufOtToAmpR[i]=((BtRxInBuf[i])>>16);


		if(OptionWord==MuEvtMcuToDsp_AudioFrmIsReady_HfpCall)
		{
			//in this case, conversa Tx and Rx are both really working, need to get the Tx audio to BT
			if(PtrVarBlockSharedByDspAndMcu->BtHfpFs==8000)
			{
				//use BtRxInBuf as a temp --- content in BtRxInBuf is already useless after calling conversa main processing
				vec_float2int((int *)BtRxInBuf, (const float *)BtTxOtBuf,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
				#if UsePrimitiveSimpleSRC==1
					for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz/2;i++)
					{
						PtrVarBlockSharedByDspAndMcu->AudioBufOtToBt[i]=BtTxOtBuf[2*i+0];
					}
				#else
					//int ProcCadenceAsrc(TCadenceSRC *SRCPtr, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum)
					ProcCadenceAsrc( &SRC_ConversaTx1,  (S32 *)PtrVarBlockSharedByDspAndMcu->AudioBufOtToBt, BtRxInBuf,  AudioFrameSizeInSamplePerCh_16KHz,  &OutSampleNum);
				#endif
			}else if(PtrVarBlockSharedByDspAndMcu->BtHfpFs==16000)
			{
				vec_float2int((int *)PtrVarBlockSharedByDspAndMcu->AudioBufOtToBt, (const float *)BtTxOtBuf,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
			}else
			{
				//should never come here
				PRINTF("DSP: audio flow error --- BT fs is not 8Khz or 16KHz - Vit \r\n");
			}
		}else if(OptionWord==MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy)
		{
			//in this case, only conversa Tx for VIT is really working, no need to put audio to BT, set Tx audio to 0
			memset((S32 *)PtrVarBlockSharedByDspAndMcu->AudioBufOtToBt,0,sizeof(PtrVarBlockSharedByDspAndMcu->AudioBufOtToBt));
		}else
		{
			//should never come here
			PRINTF("DSP: event from MCU is error \r\n");
		}
	#endif

	#if 1	//folding --- step 5: put interested audio channels to UAC 8 ch: mic0,1,2,3, ConversaTxOut, AecOut[0], BfOut, NlpOut
		//S32 UacUpAudioBuf[AudioFrameSizeInSamplePerCh_16KHz*8];
		vec_float2int((int *)RawMic32BitBuf0, (const float *)RawMic32BitBuf0,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)RawMic32BitBuf1, (const float *)RawMic32BitBuf1,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)RawMic32BitBuf2, (const float *)RawMic32BitBuf2,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)RawMic32BitBuf3, (const float *)RawMic32BitBuf3,	-31,  AudioFrameSizeInSamplePerCh_16KHz);

		vec_float2int((int *)ConversaTxOut32BitBuf, (const float *)ConversaTxOut32BitBuf,  -31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)AecOut32BitBuf,        (const float *)AecOut32BitBuf,	       -31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)BfOut32BitBuf,         (const float *)BfOut32BitBuf,		   -31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_float2int((int *)NlpOut32BitBuf,        (const float *)NlpOut32BitBuf,	       -31,  AudioFrameSizeInSamplePerCh_16KHz);

		vec_float2int((int *)TmpBuf_PromptSound,    (const float *)TmpBuf_PromptSound,	   -31,  AudioFrameSizeInSamplePerCh_16KHz);

		#if 1	//later to be closed
		if(ASR_WavPulse==ASR_WavPulse_WakeWordDetected)
		{
			ConversaTxOut32BitBuf[0]=0x7fff0000;
			ASR_WavPulse=ASR_WavPulse_NothingDetected;
		}else if(ASR_WavPulse==ASR_WavPulse_VoiceCmdDetected)
		{
			AecOut32BitBuf[0]=0x7fff0000;
			ASR_WavPulse=ASR_WavPulse_NothingDetected;
		}
		#endif

		//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
		for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
		{
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=RawMic32BitBuf0[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=RawMic32BitBuf1[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=RawMic32BitBuf2[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=RawMic32BitBuf3[i];
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=MicInMeterLvl1_InDb/100.0f*(float)0x7fffffff;
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=MicInMeterLvl2_InDb/100.0f*(float)0x7fffffff;
					//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=(AudioOneFrameBuf_OpusDecodedL[i] <<16);
					PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=(AudioOneFrameBuf_OpusDecodedL[i] <<16);
					//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=(AudioOneFrameBuf_SbcDecodedL[i] <<16);
					//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=(AudioOneFrameBuf_SbcDecodedR[i] <<16);

					PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=((int *)TmpBuf_PromptSound)[i];

			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=ConversaTxOut32BitBuf[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=AecOut32BitBuf[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=BfOut32BitBuf[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=NlpOut32BitBuf[i];
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=ConversaOutLvl1_InDb/100.0f*(float)0x7fffffff;
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=ConversaOutLvl2_InDb/100.0f*(float)0x7fffffff;
					//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=PtrVarBlockSharedByDspAndMcu->UacDnAudioBufL[i*3];	// *3, because Uac Dn is 48KHz
					//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=PtrVarBlockSharedByDspAndMcu->UacDnAudioBufR[i*3];	// *3, because Uac Dn is 48KHz
		}
	#endif

	//DbgPin7Dn();
}
