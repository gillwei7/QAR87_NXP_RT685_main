/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#if 0

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
#include "AlgoContainer.h"

#define	AudioContainerFrameLengthInSamplesPerCh	128

//AlgCtn: algorithm container
float *AlgCtnBufPtr_Bus1InL;
float *AlgCtnBufPtr_Bus1InR;
float *AlgCtnBufPtr_Bus2InL;
float *AlgCtnBufPtr_Bus2InR;

float *AlgCtnBufPtr_AudioTx1L;
float *AlgCtnBufPtr_AudioTx1R;
float *AlgCtnBufPtr_AudioTx2L;
float *AlgCtnBufPtr_AudioTx2R;
float *AlgCtnBufPtr_AudioRx1L;
float *AlgCtnBufPtr_AudioRx1R;
float *AlgCtnBufPtr_AudioRx2L;
float *AlgCtnBufPtr_AudioRx2R;

float *AlgCtnBufPtr_Mic0;
float *AlgCtnBufPtr_Mic1;
float *AlgCtnBufPtr_Mic2;
float *AlgCtnBufPtr_Mic3;
float *AlgCtnBufPtr_Mic4;
float *AlgCtnBufPtr_Mic5;

TAlgo CurrentAlgo;

int AlgCtnFrameLengthInSamplesPerCh;

void AudioContainerSrcInputBuf(void)
{
}
void AudioContainerSrcOutputBuf(void)
{
}

int InitAudioContainer(int WorkMode, int FrmLenInSamples, int Fs)
{
	if(FrmLenInSamples>AudioContainerFrameLengthInSamplesPerCh)
		return 1;

	AlgCtnFrameLengthInSamplesPerCh=FrmLenInSamples;

	switch(WorkMode)
	{
	case MuEvtMcuToDsp_AudioFrmIsReady_HfpCall:
		CurrentAlgo=ALGO_Idx_1_ConversaVoiceCall;
		AlgCtnBufPtr_Bus1InL=NULL;
		/*
		AlgCtnBufPtr_Bus1InR;
		AlgCtnBufPtr_Bus2InL;
		AlgCtnBufPtr_Bus2InR;

		AlgCtnBufPtr_AudioTx1L;
		AlgCtnBufPtr_AudioTx1R;
		AlgCtnBufPtr_AudioTx2L;
		AlgCtnBufPtr_AudioTx2R;
		AlgCtnBufPtr_AudioRx1L;
		AlgCtnBufPtr_AudioRx1R;
		AlgCtnBufPtr_AudioRx2L;
		AlgCtnBufPtr_AudioRx2R;

		AlgCtnBufPtr_Mic0;
		AlgCtnBufPtr_Mic1;
		AlgCtnBufPtr_Mic2;
		AlgCtnBufPtr_Mic3;
		AlgCtnBufPtr_Mic4;
		AlgCtnBufPtr_Mic5;
		*/
		memset(AlgCtnBufPtr_Bus1InL,0,sizeof(float)*AlgCtnFrameLengthInSamplesPerCh);

		//init SRC
		//             (ptr to handle.     int InputBlockSizeInSamples,     int inFs,                 int outFs,                   int ChNum,           alloc_memory ptr array,              ptr numbers,       int NeedToDisplay)
		InitCadenceAsrc(&SrcHandle_AlgCtnBus1,DecoderSbc_SrcInSizeInSamples,   8000,     16000,      1,       (void **)&g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus1,   &g_w_malloc_SrcHandle_AlgCtnBus1,         0     );
		InitCadenceAsrc(&SrcHandle_AlgCtnBus2,DecoderSbc_SrcInSizeInSamples,   8000,     16000,      1,       (void **)&g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus2,   &g_w_malloc_SrcHandle_AlgCtnBus2,         0     );
		//                                 DecoderSbc_SrcInSizeInSamples is to reserve enough space for output, later the input block size in samples will be set again in the src processing



		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_VideoRecording:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_MediaPlayer:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_Translation:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_AiConversation:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_VideoAi:
		break;
	default:
		break;
	}






/*
	xa_codec_handle_t SrcHandle_AlgCtnBus1;
	xa_codec_handle_t SrcHandle_AlgCtnBus2;
	xa_codec_handle_t SrcHandle_AlgCtnTx1;
	xa_codec_handle_t SrcHandle_AlgCtnTx2;
	xa_codec_handle_t SrcHandle_AlgCtnRx1;
	xa_codec_handle_t SrcHandle_AlgCtnMic01;
	xa_codec_handle_t SrcHandle_AlgCtnMic23;
	xa_codec_handle_t SrcHandle_AlgCtnMic45;

	WORD  g_w_malloc_SrcHandle_AlgCtnBus1;
	WORD  g_w_malloc_SrcHandle_AlgCtnBus2;
	WORD  g_w_malloc_SrcHandle_AlgCtnTx1;
	WORD  g_w_malloc_SrcHandle_AlgCtnTx2;
	WORD  g_w_malloc_SrcHandle_AlgCtnRx1;
	WORD  g_w_malloc_SrcHandle_AlgCtnMic01;
	WORD  g_w_malloc_SrcHandle_AlgCtnMic23;
	WORD  g_w_malloc_SrcHandle_AlgCtnMic45;

	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus1[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus2[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgTx1[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgTx2[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgRx1[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgMic01[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgMic23[MAX_MEM_ALLOCS];
	pVOID g_pv_arr_alloc_memory_SrcHandle_AlgMic45[MAX_MEM_ALLOCS];
*/
	return 0;
}




int ProcAudioContainer(int WorkMode, int FrmLenInSamples, int Fs)
{
	if(FrmLenInSamples>AudioContainerFrameLengthInSamplesPerCh)
		return 1;

	//AlgCtnFrameLengthInSamplesPerCh=FrmLenInSamples;

	switch(WorkMode)
	{
	case MuEvtMcuToDsp_AudioFrmIsReady_HfpCall:
		AlgCtnBufPtr_Bus1InL=NULL;
		/*
		AlgCtnBufPtr_Bus1InR;
		AlgCtnBufPtr_Bus2InL;
		AlgCtnBufPtr_Bus2InR;

		AlgCtnBufPtr_AudioTx1L;
		AlgCtnBufPtr_AudioTx1R;
		AlgCtnBufPtr_AudioTx2L;
		AlgCtnBufPtr_AudioTx2R;
		AlgCtnBufPtr_AudioRx1L;
		AlgCtnBufPtr_AudioRx1R;
		AlgCtnBufPtr_AudioRx2L;
		AlgCtnBufPtr_AudioRx2R;

		AlgCtnBufPtr_Mic0;
		AlgCtnBufPtr_Mic1;
		AlgCtnBufPtr_Mic2;
		AlgCtnBufPtr_Mic3;
		AlgCtnBufPtr_Mic4;
		AlgCtnBufPtr_Mic5;
		*/
		memset(AlgCtnBufPtr_Bus1InL,0,sizeof(float)*AlgCtnFrameLengthInSamplesPerCh);

		//init SRC
		//             (ptr to handle.     int InputBlockSizeInSamples,     int inFs,                 int outFs,                   int ChNum,           alloc_memory ptr array,              ptr numbers,       int NeedToDisplay)
		InitCadenceAsrc(&SrcHandle_AlgCtnBus1,DecoderSbc_SrcInSizeInSamples,   8000,     16000,      1,       (void **)&g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus1,   &g_w_malloc_SrcHandle_AlgCtnBus1,         0     );
		InitCadenceAsrc(&SrcHandle_AlgCtnBus2,DecoderSbc_SrcInSizeInSamples,   8000,     16000,      1,       (void **)&g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus2,   &g_w_malloc_SrcHandle_AlgCtnBus2,         0     );
		//                                 DecoderSbc_SrcInSizeInSamples is to reserve enough space for output, later the input block size in samples will be set again in the src processing



		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_VideoRecording:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_MediaPlayer:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_Translation:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_AiConversation:
		break;
	case MuEvtMcuToDsp_AudioFrmIsReady_VideoAi:
		break;
	default:
		break;
	}

	return 0;
}

#endif
