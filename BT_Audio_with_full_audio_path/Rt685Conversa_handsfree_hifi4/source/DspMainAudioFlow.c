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

#include "DspMainAudioFlow.h"

#ifndef CORE_DSP
#define CORE_DSP
#endif

#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"


#endif

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_01 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_02 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_03 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_04 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_05 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_06 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_07 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_08 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_09 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_10 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_11 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_12 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_13 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_14 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_15 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_16 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_17 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_18 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_19 [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 AudioOneFrameBuf_20 [AudioFrameSizeInSamplePerCh];

void SimpleSrc3xUp(S32 *DstPtr, S32 *SrcPtr, int LenOfInput)
{
	for(int i=0;i<LenOfInput;i++)
	{
		*DstPtr++=*SrcPtr;
		*DstPtr++=*SrcPtr;
		*DstPtr++=*SrcPtr++;
	}
}
void SimpleSrc3xDn(S32 *DstPtr, S32 *SrcPtr, int LenOfOutput)
{
	for(int i=0;i<LenOfOutput;i++)
	{
		*DstPtr++=*SrcPtr;
		SrcPtr+=3;
	}
}

void DspMainAudioFlowProcOneFrame_AudioIoDbg(int OptionWord)
{
	//Note: Fs here is 16KHz
	DbgPin8Up();
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}

	//convert all audio sources from S32 to float

	S32 *Ptr_Mic0=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[0];
	S32 *Ptr_Mic1=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[1];
	S32 *Ptr_Mic2=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[2];
	S32 *Ptr_Mic3=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[3];
	//S32 *Ptr_Mic4=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[4];
	//S32 *Ptr_Mic5=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[5];
	//S32 *Ptr_Mic6=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[6];
	//S32 *Ptr_Mic7=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[7];

	S32 *SrcPtr_I2SAmpL;
	S32 *SrcPtr_I2SAmpR;
	S32 *SrcPtr_I2SNvtL;
	S32 *SrcPtr_I2SNvtR;
	S32 *SrcPtr_UacL;
	S32 *SrcPtr_UacR;

	S32 *DstPtr_I2SAmpL;
	S32 *DstPtr_I2SAmpR;
	S32 *DstPtr_I2SNvtL;
	S32 *DstPtr_I2SNvtR;



	float *FltPtr_I2SInAmpL=(float *)AudioOneFrameBuf_01;
	float *FltPtr_I2SInAmpR=(float *)AudioOneFrameBuf_02;
	float *FltPtr_I2SInNvtL=(float *)AudioOneFrameBuf_03;
	float *FltPtr_I2SInNvtR=(float *)AudioOneFrameBuf_04;
	float *FltPtr_UacDnL=(float *)AudioOneFrameBuf_05;
	float *FltPtr_UacDnR=(float *)AudioOneFrameBuf_06;
	float *FltPtr_TmpL=(float *)AudioOneFrameBuf_07;
	float *FltPtr_TmpR=(float *)AudioOneFrameBuf_08;

	S32 *S32Ptr_DmicIn0=AudioOneFrameBuf_09;
	S32 *S32Ptr_DmicIn1=AudioOneFrameBuf_10;
	S32 *S32Ptr_DmicIn2=AudioOneFrameBuf_11;
	S32 *S32Ptr_DmicIn3=AudioOneFrameBuf_12;

	float *FltPtr_GeneratedToneL=(float *)AudioOneFrameBuf_13;
	float *FltPtr_GeneratedToneR=(float *)AudioOneFrameBuf_14;

	S32 *S32Ptr_UacUpCh5=AudioOneFrameBuf_15;
	S32 *S32Ptr_UacUpCh6=AudioOneFrameBuf_16;
	S32 *S32Ptr_UacUpCh7=AudioOneFrameBuf_17;
	S32 *S32Ptr_UacUpCh8=AudioOneFrameBuf_18;


	SrcPtr_I2SAmpL=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SLineInBufL;
	SrcPtr_I2SAmpR=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SLineInBufR;
	SrcPtr_I2SNvtL=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SInNvtBufL;
	SrcPtr_I2SNvtR=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SInNvtBufR;
	SrcPtr_UacL=(S32 *)PtrVarBlockSharedByDspAndMcu->UacDnAudioBufL;
	SrcPtr_UacR=(S32 *)PtrVarBlockSharedByDspAndMcu->UacDnAudioBufR;

	DstPtr_I2SAmpL=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL;
	DstPtr_I2SAmpR=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR;
	DstPtr_I2SNvtL=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SOtNvtBufL;
	DstPtr_I2SNvtR=(S32 *)PtrVarBlockSharedByDspAndMcu->I2SOtNvtBufR;

	//all audio source data to float
	vec_int2float(FltPtr_I2SInAmpL, (const int *)SrcPtr_I2SAmpL,	-31,  AudioFrameSizeInSamplePerCh);
	vec_int2float(FltPtr_I2SInAmpR, (const int *)SrcPtr_I2SAmpR,	-31,  AudioFrameSizeInSamplePerCh);
	vec_int2float(FltPtr_I2SInNvtL, (const int *)SrcPtr_I2SNvtL,	-31,  AudioFrameSizeInSamplePerCh);
	vec_int2float(FltPtr_I2SInNvtR, (const int *)SrcPtr_I2SNvtR,	-31,  AudioFrameSizeInSamplePerCh);

	SimpleSrc3xDn(SrcPtr_UacL, SrcPtr_UacL, AudioFrameSizeInSamplePerCh);
	SimpleSrc3xDn(SrcPtr_UacR, SrcPtr_UacR, AudioFrameSizeInSamplePerCh);
	vec_int2float(FltPtr_UacDnL,  (const int *)SrcPtr_UacL,	    -31,  AudioFrameSizeInSamplePerCh);
	vec_int2float(FltPtr_UacDnR,  (const int *)SrcPtr_UacR,	    -31,  AudioFrameSizeInSamplePerCh);
	
	vec_int2float((float *)S32Ptr_DmicIn0, (const int *)Ptr_Mic0,	-31,  AudioFrameSizeInSamplePerCh);
	vec_int2float((float *)S32Ptr_DmicIn1, (const int *)Ptr_Mic1,	-31,  AudioFrameSizeInSamplePerCh);
	vec_int2float((float *)S32Ptr_DmicIn2, (const int *)Ptr_Mic2,	-31,  AudioFrameSizeInSamplePerCh);
	vec_int2float((float *)S32Ptr_DmicIn3, (const int *)Ptr_Mic3,	-31,  AudioFrameSizeInSamplePerCh);

	//I2S Nvt in + UacDn (using soft mixer 1,2) --> Compressor1,2 --> I2S Amp out
	//Pdm0,1                                                      --> I2S Nvt out
	//UacUp:
	//	ch0: dmic0
	//	ch1: dmic1
	//	ch2: dmic2 / or NVT I2S in L
	//	ch3: dmic3 / or NVT I2S in R
	//	ch4: Pdm0 + UacDn L 	(using soft mixer 3) --> Compressor3
	//	ch5: Pdm1 + UacDn R 	(using soft mixer 4) --> Compressor4
	//	ch6: Pdm2 + SweepTone L	(using soft mixer 5) --> Compressor5
	//	ch7: Pdm3 + SweepTone R	(using soft mixer 6) --> Compressor6

	//I2S Nvt in + UacDn (using soft mixer 1,2) --> Compressor1,2 --> I2S Amp out
	AudioProcOneFrame_Mix2To1(&SoftMixer2To1_1,FltPtr_TmpL, FltPtr_UacDnL, FltPtr_I2SInNvtL, AudioFrameSizeInSamplePerCh);
	AudioProcOneFrame_Mix2To1(&SoftMixer2To1_2,FltPtr_TmpR, FltPtr_UacDnR, FltPtr_I2SInNvtR, AudioFrameSizeInSamplePerCh);
	AudioCompressorProcOneFrame(AudioCompressor1, FltPtr_TmpL,  FltPtr_TmpL);
	AudioCompressorProcOneFrame(AudioCompressor2, FltPtr_TmpR,  FltPtr_TmpR);
	vec_float2int(DstPtr_I2SAmpL, (const float *)FltPtr_TmpL, -31,  AudioFrameSizeInSamplePerCh);
	vec_float2int(DstPtr_I2SAmpR, (const float *)FltPtr_TmpR, -31,  AudioFrameSizeInSamplePerCh);
	#if 0
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			DstPtr_I2SAmpL[i]=0x10000*i;
			DstPtr_I2SAmpR[i]=0x20000*i;
		}
	#endif


	//Pdm0,1                                                      --> I2S Nvt out
	vec_float2int(DstPtr_I2SNvtL, (const float *)S32Ptr_DmicIn0, -31,  AudioFrameSizeInSamplePerCh);
	vec_float2int(DstPtr_I2SNvtR, (const float *)S32Ptr_DmicIn1, -31,  AudioFrameSizeInSamplePerCh);
	#if 0
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			DstPtr_I2SNvtL[i]=0x10000*i;
			DstPtr_I2SNvtR[i]=-0x20000*i;
		}
	#endif


	//fill Uac 8 channels upstreaming buffer
	#if 1
		//sweeping signal overwrites USB down streaming L and R
		GenerateSineTone          (&SineToneGenerator1, FltPtr_GeneratedToneL, AudioFrameSizeInSamplePerCh,1);
		//GenerateSineToneSingleFreq(&SineToneGenerator2, FltPtr_GeneratedToneL, AudioFrameSizeInSamplePerCh,1);

		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
			FltPtr_GeneratedToneR[i]=0.0f-FltPtr_GeneratedToneL[i];
	#endif

	//	ch0: dmic0
	//	ch1: dmic1
	//	ch2: dmic2 / or NVT I2S in L
	//	ch3: dmic3 / or NVT I2S in R
	
	//	ch4: Pdm0 + UacDn L 	(using soft mixer 3) --> Compressor3
	//	ch5: Pdm1 + UacDn R 	(using soft mixer 4) --> Compressor4
	AudioProcOneFrame_Mix2To1(&SoftMixer2To1_3,FltPtr_TmpL, FltPtr_UacDnL, (float *)S32Ptr_DmicIn0, AudioFrameSizeInSamplePerCh);
	AudioProcOneFrame_Mix2To1(&SoftMixer2To1_4,FltPtr_TmpR, FltPtr_UacDnR, (float *)S32Ptr_DmicIn1, AudioFrameSizeInSamplePerCh);
	AudioCompressorProcOneFrame(AudioCompressor3, FltPtr_TmpL,  FltPtr_TmpL);
	AudioCompressorProcOneFrame(AudioCompressor4, FltPtr_TmpR,  FltPtr_TmpR);
	vec_float2int(S32Ptr_UacUpCh5, (const float *)FltPtr_TmpL, -31,  AudioFrameSizeInSamplePerCh);
	vec_float2int(S32Ptr_UacUpCh6, (const float *)FltPtr_TmpR, -31,  AudioFrameSizeInSamplePerCh);
	
	//	ch6: Pdm2 + SweepTone L	(using soft mixer 5) --> Compressor5
	//	ch7: Pdm3 + SweepTone R	(using soft mixer 6) --> Compressor6
	AudioProcOneFrame_Mix2To1(&SoftMixer2To1_5,FltPtr_TmpL, FltPtr_GeneratedToneL, (float *)S32Ptr_DmicIn2, AudioFrameSizeInSamplePerCh);
	AudioProcOneFrame_Mix2To1(&SoftMixer2To1_6,FltPtr_TmpR, FltPtr_GeneratedToneR, (float *)S32Ptr_DmicIn3, AudioFrameSizeInSamplePerCh);
	AudioCompressorProcOneFrame(AudioCompressor5, FltPtr_TmpL,  FltPtr_TmpL);
	AudioCompressorProcOneFrame(AudioCompressor6, FltPtr_TmpR,  FltPtr_TmpR);
	vec_float2int(S32Ptr_UacUpCh7, (const float *)FltPtr_TmpL, -31,  AudioFrameSizeInSamplePerCh);
	vec_float2int(S32Ptr_UacUpCh8, (const float *)FltPtr_TmpR, -31,  AudioFrameSizeInSamplePerCh);
	
	vec_float2int((S32 *)S32Ptr_DmicIn0, (const float *)S32Ptr_DmicIn0,	-31,  AudioFrameSizeInSamplePerCh);
	vec_float2int((S32 *)S32Ptr_DmicIn1, (const float *)S32Ptr_DmicIn1,	-31,  AudioFrameSizeInSamplePerCh);
	//vec_float2int((S32 *)S32Ptr_DmicIn2, (const float *)S32Ptr_DmicIn2,	-31,  AudioFrameSizeInSamplePerCh);
	//vec_float2int((S32 *)S32Ptr_DmicIn3, (const float *)S32Ptr_DmicIn3,	-31,  AudioFrameSizeInSamplePerCh);
	

	for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
	{
		PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=S32Ptr_DmicIn0[i];
		PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=S32Ptr_DmicIn1[i];
		//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=S32Ptr_DmicIn2[i];
		//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=S32Ptr_DmicIn3[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=SrcPtr_I2SNvtL[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=SrcPtr_I2SNvtR[i];
		PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=S32Ptr_UacUpCh5[i];
		PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=S32Ptr_UacUpCh6[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=0x10000*i;
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=0-0x10000*i;
		PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=S32Ptr_UacUpCh7[i];
		PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=S32Ptr_UacUpCh8[i];
	}
	DbgPin8Dn();
}

void DspMainAudioFlowProcOneFrame_VideoRecording(int OptionWord)
{
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
}

void DspMainAudioFlowProcOneFrame_MediaPlayer(int OptionWord)
{
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
}


void DspMainAudioFlowProcOneFrame_MusicPlayer(int OptionWord)
{
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
}

void DspMainAudioFlowProcOneFrame_Translation(int OptionWord)
{
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
}

void DspMainAudioFlowProcOneFrame_AiConversation(int OptionWord)
{
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
}

void DspMainAudioFlowProcOneFrame_VideoAi(int OptionWord)
{
	switch(OptionWord)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
		default:
			break;
	}
}
