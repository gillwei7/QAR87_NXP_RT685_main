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
#include "AudioDecoder.h"


#endif

#if 1	//--- folding, variables
extern XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.

//for SRC
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 SrcIn_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 SrcOut_2S32Mixed[48*20*2];		//to hold 20ms at 48KHz, stereo


//for decoding OPUS and SBC
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_OpusDecodedL[AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_OpusDecodedR[AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_SbcDecodedL [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_SbcDecodedR [AudioFrameSizeInSamplePerCh_48KHz];


//for holding 6 ch mic input --- converted to float
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_16KHz_01 [AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_16KHz_02 [AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_16KHz_03 [AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_16KHz_04 [AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_16KHz_05 [AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_16KHz_06 [AudioFrameSizeInSamplePerCh_16KHz];


//for holding 48KHz input --- converted to float
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_01 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_02 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_03 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_04 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_05 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_06 [AudioFrameSizeInSamplePerCh_48KHz];

//for temp1,2 48KHz and SignalGenerator
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_07 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_08 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_09 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_10 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_11 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S32 AudioOneFrameBuf_48KHz_12 [AudioFrameSizeInSamplePerCh_48KHz];

//for temp1,2 16KHz
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_48KHz_13 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_48KHz_14 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_48KHz_15 [AudioFrameSizeInSamplePerCh_48KHz];
__attribute__((__section__(".dram.data"))) __attribute__((aligned(32)))
S16 AudioOneFrameBuf_48KHz_16 [AudioFrameSizeInSamplePerCh_48KHz];


S32 *SrcPtrS32_Mic0;
S32 *SrcPtrS32_Mic1;
S32 *SrcPtrS32_Mic2;
S32 *SrcPtrS32_Mic3;
//S32 *SrcPtrS32_Mic4;
//S32 *SrcPtrS32_Mic5;
//S32 *SrcPtrS32_Mic6;
//S32 *SrcPtrS32_Mic7;
S16 *SrcPtrS16_I2SAmpL;
S16 *SrcPtrS16_I2SAmpR;
S16 *SrcPtrS16_I2SNvtL;
S16 *SrcPtrS16_I2SNvtR;
S32 *SrcPtrS32_UacL;
S32 *SrcPtrS32_UacR;

S16 *DstPtrS16_I2SAmpL;
S16 *DstPtrS16_I2SAmpR;
S16 *DstPtrS16_I2SNvtL;
S16 *DstPtrS16_I2SNvtR;

float *SrcPtrFlt_DmicIn0;
float *SrcPtrFlt_DmicIn1;
float *SrcPtrFlt_DmicIn2;
float *SrcPtrFlt_DmicIn3;
float *SrcPtrFlt_I2SInAmpL;
float *SrcPtrFlt_I2SInAmpR;
float *SrcPtrFlt_I2SInNvtL;
float *SrcPtrFlt_I2SInNvtR;
float *SrcPtrFlt_UacDnL;
float *SrcPtrFlt_UacDnR;

float *FltPtr_GeneratedToneL;
float *FltPtr_GeneratedToneR;
float *FltPtr_Tmp1L;
float *FltPtr_Tmp1R;
float *FltPtr_Tmp2L;
float *FltPtr_Tmp2R;
S32	*S32Ptr_Tmp1L;
S32 *S32Ptr_Tmp1R;
S32 *S32Ptr_Tmp2L;
S32 *S32Ptr_Tmp2R;

S16 *S16Ptr_Tmp1L;
S16 *S16Ptr_Tmp1R;
S16 *S16Ptr_Tmp2L;
S16 *S16Ptr_Tmp2R;


#endif

#if 1	//--- folding, converting functions
__attribute__((__section__(".iram.text")))
void SimpleSrc3xUp(S32 *DstPtr, S32 *SrcPtr, int LenOfInput)
{
	for(int i=0;i<LenOfInput;i++)
	{
		*DstPtr++=*SrcPtr;
		*DstPtr++=*SrcPtr;
		*DstPtr++=*SrcPtr++;
	}
}
__attribute__((__section__(".iram.text")))
void SimpleSrc3xDn(S32 *DstPtr, S32 *SrcPtr, int LenOfOutput)
{
	for(int i=0;i<LenOfOutput;i++)
	{
		*DstPtr++=*SrcPtr;
		SrcPtr+=3;
	}
}
__attribute__((__section__(".iram.text")))
void ConvertS16ToFloat(float *DstPtr, const S16 *SrcPtr, int l)
{
	S32 *p;
	if(!l)
		return;

	p=malloc(sizeof(S32)*l);
	if(p==NULL)
		return;

	for(int i=0;i<l;i++)
		p[i]=((*SrcPtr++)<<16);

	vec_int2float((float *)DstPtr, (const int *)p,	-31,  l);
	free(p);
}
__attribute__((__section__(".iram.text")))
void ConvertFloatToS16(S16 *DstPtr, const float *SrcPtr, int l)
{
	S32 *p;
	if(!l)
		return;

	p=malloc(sizeof(S32)*l);
	if(p==NULL)
		return;

	vec_float2int(p, (const float *)SrcPtr,	-31,  l);

	for(int i=0;i<l;i++)
		*DstPtr++=(p[i]>>16);

	free(p);
}

void PrepareMainAudioFlowPointersAndInputFloatData(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
{
	//input raw mic and I2S audio
	SrcPtrS32_Mic0=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[0];
	SrcPtrS32_Mic1=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[1];
	SrcPtrS32_Mic2=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[2];
	SrcPtrS32_Mic3=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[3];
	//SrcPtrS32_Mic4=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[4];
	//SrcPtrS32_Mic5=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[5];
	//SrcPtrS32_Mic6=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[6];
	//SrcPtrS32_Mic7=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[7];
		SrcPtrS16_I2SAmpL=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SLineInBufL;
		SrcPtrS16_I2SAmpR=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SLineInBufR;
			SrcPtrS16_I2SNvtL=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SInNvtBufL;
			SrcPtrS16_I2SNvtR=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SInNvtBufR;
				SrcPtrS32_UacL=(S32 *)PtrVarBlockSharedByDspAndMcu->UacDnAudioBufL;
				SrcPtrS32_UacR=(S32 *)PtrVarBlockSharedByDspAndMcu->UacDnAudioBufR;

	//output 16bit I2S audio
	DstPtrS16_I2SAmpL=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL;
	DstPtrS16_I2SAmpR=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR;
		DstPtrS16_I2SNvtL=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SOtNvtBufL;
		DstPtrS16_I2SNvtR=(S16 *)PtrVarBlockSharedByDspAndMcu->I2SOtNvtBufR;

	//converting destination for all the inputs
	SrcPtrFlt_DmicIn0=			(float *)AudioOneFrameBuf_16KHz_01;
	SrcPtrFlt_DmicIn1=			(float *)AudioOneFrameBuf_16KHz_02;
	SrcPtrFlt_DmicIn2=			(float *)AudioOneFrameBuf_16KHz_03;
	SrcPtrFlt_DmicIn3=			(float *)AudioOneFrameBuf_16KHz_04;
		SrcPtrFlt_I2SInAmpL=		(float *)AudioOneFrameBuf_48KHz_01;
		SrcPtrFlt_I2SInAmpR=		(float *)AudioOneFrameBuf_48KHz_02;
			SrcPtrFlt_I2SInNvtL=	(float *)AudioOneFrameBuf_48KHz_03;
			SrcPtrFlt_I2SInNvtR=	(float *)AudioOneFrameBuf_48KHz_04;
				SrcPtrFlt_UacDnL=(float *)AudioOneFrameBuf_48KHz_05;
				SrcPtrFlt_UacDnR=(float *)AudioOneFrameBuf_48KHz_06;

	//temp buffers --- float and S32
	FltPtr_Tmp1L=        		(float *)AudioOneFrameBuf_48KHz_07;
	FltPtr_Tmp1R=        		(float *)AudioOneFrameBuf_48KHz_08;
	FltPtr_Tmp2L=        		(float *)AudioOneFrameBuf_48KHz_09;
	FltPtr_Tmp2R=        		(float *)AudioOneFrameBuf_48KHz_10;
	FltPtr_GeneratedToneL=	(float *)AudioOneFrameBuf_48KHz_11;
	FltPtr_GeneratedToneR=	(float *)AudioOneFrameBuf_48KHz_12;
		S32Ptr_Tmp1L=        		AudioOneFrameBuf_48KHz_07;			//S32Ptr_Tmp1L/R,2,3,4 point to the same place as FltPtr_Tmp1L/R,2,3,4
		S32Ptr_Tmp1R=        		AudioOneFrameBuf_48KHz_08;
		S32Ptr_Tmp2L=        		AudioOneFrameBuf_48KHz_09;
		S32Ptr_Tmp2R=        		AudioOneFrameBuf_48KHz_10;
	//temp buffers --- S16
	S16Ptr_Tmp1L=        		AudioOneFrameBuf_48KHz_13;
	S16Ptr_Tmp1R=        		AudioOneFrameBuf_48KHz_14;
	S16Ptr_Tmp2L=        		AudioOneFrameBuf_48KHz_15;
	S16Ptr_Tmp2R=        		AudioOneFrameBuf_48KHz_16;

	//convert needed audio source data to float
	if(NeedToCvtMicToFlt)
	{
		vec_int2float(SrcPtrFlt_DmicIn0, (const int *)SrcPtrS32_Mic0,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_int2float(SrcPtrFlt_DmicIn1, (const int *)SrcPtrS32_Mic1,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_int2float(SrcPtrFlt_DmicIn2, (const int *)SrcPtrS32_Mic2,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		vec_int2float(SrcPtrFlt_DmicIn3, (const int *)SrcPtrS32_Mic3,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float(SrcPtrFlt_DmicIn4, (const int *)SrcPtrS32_Mic4,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float(SrcPtrFlt_DmicIn5, (const int *)SrcPtrS32_Mic5,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float(SrcPtrFlt_DmicIn6, (const int *)SrcPtrS32_Mic6,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
		//vec_int2float(SrcPtrFlt_DmicIn7, (const int *)SrcPtrS32_Mic7,	-31,  AudioFrameSizeInSamplePerCh_16KHz);
	}
	if(NeedToCvtUacDnToFlt)
	{
		vec_int2float(SrcPtrFlt_UacDnL, (const int *)SrcPtrS32_UacL,	-31,  AudioFrameSizeInSamplePerCh_48KHz);
		vec_int2float(SrcPtrFlt_UacDnR, (const int *)SrcPtrS32_UacR,	-31,  AudioFrameSizeInSamplePerCh_48KHz);
	}
	if(NeedToCvtAmpI2SInToFlt)
	{
		ConvertS16ToFloat(SrcPtrFlt_I2SInAmpL, (const S16 *)SrcPtrS16_I2SAmpL, AudioFrameSizeInSamplePerCh_48KHz);
		ConvertS16ToFloat(SrcPtrFlt_I2SInAmpR, (const S16 *)SrcPtrS16_I2SAmpR, AudioFrameSizeInSamplePerCh_48KHz);
	}
	if(NeedToCvtNvtI2SInToFlt)
	{
		ConvertS16ToFloat(SrcPtrFlt_I2SInNvtL, (const S16 *)SrcPtrS16_I2SNvtL, AudioFrameSizeInSamplePerCh_48KHz);
		ConvertS16ToFloat(SrcPtrFlt_I2SInNvtR, (const S16 *)SrcPtrS16_I2SNvtR, AudioFrameSizeInSamplePerCh_48KHz);
	}
}
#endif

#if 1	//--- folding, main audio flow processing functions
__attribute__((__section__(".iram.text")))
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

	#if 1	//step 1: convert input samples from S32 or S16 to float
		PrepareMainAudioFlowPointersAndInputFloatData(1,1,1,1);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
	#endif

	#if 1	//step 2: generate tone
		//sweeping signal overwrites USB down streaming L and R
		GenerateSineTone          (&SineToneGenerator1, FltPtr_GeneratedToneL, AudioFrameSizeInSamplePerCh_48KHz,1);
		//GenerateSineToneSingleFreq(&SineToneGenerator2, FltPtr_GeneratedToneL, AudioFrameSizeInSamplePerCh_48KHz,1);

		for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
			FltPtr_GeneratedToneR[i]=0.0f-FltPtr_GeneratedToneL[i];
	#endif


	#if 1	//step 3: Uac + Nvt to Amp --- or Tone to Amp
		#if 1
			//Uac + Nvt to Amp: note, this is simple add, no satuation
			for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
			{
				FltPtr_Tmp1L[i]=SrcPtrFlt_UacDnL[i]+SrcPtrFlt_I2SInNvtL[i];
				FltPtr_Tmp1R[i]=SrcPtrFlt_UacDnR[i]+SrcPtrFlt_I2SInNvtR[i];
			}
			ConvertFloatToS16(DstPtrS16_I2SAmpL, (const float *)FltPtr_Tmp1L, AudioFrameSizeInSamplePerCh_48KHz);
			ConvertFloatToS16(DstPtrS16_I2SAmpR, (const float *)FltPtr_Tmp1R, AudioFrameSizeInSamplePerCh_48KHz);
		#else
			//sweeping tone to AMP
			ConvertFloatToS16(DstPtrS16_I2SAmpL, (const float *)FltPtr_GeneratedToneL, AudioFrameSizeInSamplePerCh_48KHz);
			ConvertFloatToS16(DstPtrS16_I2SAmpR, (const float *)FltPtr_GeneratedToneR, AudioFrameSizeInSamplePerCh_48KHz);
		#endif
	#endif

	#if 1	//step 4: mic01, or mic23, or Tone to Amp (mic is simple 3x up SRCed)
		#if 1
			//Uac + Nvt to Amp: note, this is simple add, no satuation
			for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				#if 1
					FltPtr_Tmp1L[3*i+0]=SrcPtrFlt_DmicIn0[i];
					FltPtr_Tmp1R[3*i+0]=SrcPtrFlt_DmicIn1[i];
					FltPtr_Tmp1L[3*i+1]=SrcPtrFlt_DmicIn0[i];
					FltPtr_Tmp1R[3*i+1]=SrcPtrFlt_DmicIn1[i];
					FltPtr_Tmp1L[3*i+2]=SrcPtrFlt_DmicIn0[i];
					FltPtr_Tmp1R[3*i+2]=SrcPtrFlt_DmicIn1[i];
				#else
					FltPtr_Tmp1L[3*i+0]=SrcPtrFlt_DmicIn2[i];
					FltPtr_Tmp1R[3*i+0]=SrcPtrFlt_DmicIn3[i];
					FltPtr_Tmp1L[3*i+1]=SrcPtrFlt_DmicIn2[i];
					FltPtr_Tmp1R[3*i+1]=SrcPtrFlt_DmicIn3[i];
					FltPtr_Tmp1L[3*i+2]=SrcPtrFlt_DmicIn2[i];
					FltPtr_Tmp1R[3*i+2]=SrcPtrFlt_DmicIn3[i];
				#endif
			}
			ConvertFloatToS16(DstPtrS16_I2SNvtL, (const float *)FltPtr_Tmp1L, AudioFrameSizeInSamplePerCh_48KHz);
			ConvertFloatToS16(DstPtrS16_I2SNvtR, (const float *)FltPtr_Tmp1R, AudioFrameSizeInSamplePerCh_48KHz);
		#else
			//sweeping tone to AMP
			ConvertFloatToS16(DstPtrS16_I2SNvtL, (const float *)FltPtr_GeneratedToneL, AudioFrameSizeInSamplePerCh_48KHz);
			ConvertFloatToS16(DstPtrS16_I2SNvtR, (const float *)FltPtr_GeneratedToneR, AudioFrameSizeInSamplePerCh_48KHz);
		#endif
	#endif

	#if 1	//step 5: mic0,1,2,3, NvtIn, AmpIn or Tone to UacUp 8ch
		S32 *TmpPtrS32L=(int *)FltPtr_Tmp1L;
		S32 *TmpPtrS32R=(int *)FltPtr_Tmp1R;
		vec_float2int(TmpPtrS32L, (const float *)FltPtr_GeneratedToneL, -31,  AudioFrameSizeInSamplePerCh_48KHz);
		vec_float2int(TmpPtrS32R, (const float *)FltPtr_GeneratedToneR, -31,  AudioFrameSizeInSamplePerCh_48KHz);

		//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
		{
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=SrcPtrS32_Mic1[i];
			#if 1
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=SrcPtrS32_Mic2[i];
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=SrcPtrS32_Mic3[i];
			#else
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=TmpPtrS32L[3*i];		//with simple 1/3 decimation
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=TmpPtrS32R[3*i];		//with simple 1/3 decimation
			#endif

			//all the following 4ch are with simple 1/3 decimation
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=(SrcPtrS16_I2SAmpL[3*i]<<16);
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=(SrcPtrS16_I2SAmpR[3*i]<<16);
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=(SrcPtrS16_I2SNvtL[3*i]<<16);
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=(SrcPtrS16_I2SNvtR[3*i]<<16);
		}
	#endif

	DbgPin8Dn();
}

__attribute__((__section__(".iram.text")))
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

	#if 1	//step 1: convert input samples from S32 or S16 to float
		PrepareMainAudioFlowPointersAndInputFloatData(1,1,1,1);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
	#endif
}

__attribute__((__section__(".iram.text")))
void DspMainAudioFlowProcOneFrame_MediaPlayer(int OptionWord)
{
	int i;
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
	DbgPin7Up();

	#if 1	//step 1: convert input samples from S32 or S16 to float
		S16 RawMicSignal16BitForVitRef[AudioFrameSizeInSamplePerCh_16KHz];

		//B36932 PrepareMainAudioFlowPointersAndInputFloatData(1,0,0,0);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
		PrepareMainAudioFlowPointersAndInputFloatData(1,0,0,1);//B36932 (int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
		int FrmSizeInSamples=PtrVarBlockSharedByDspAndMcu->I2SFrmSizeInSamples_Amp;

		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			RawMicSignal16BitForVitRef[i]=(SrcPtrS32_Mic0[i]>>16);

	#endif

	#if 1	//step 2: get audio from sbc decoder and opus decoder, or clear the decoder buffer if the decoder is NOT running
		//in this mode, OPUS and SBC stream are converted to 48KHz Fs
		#if EnableOpusDec==1
			unsigned short *OtPtrS16_Opus;
			//take out OPUS output audio, and mix with UAC, with satuation, no gaining
			xos_mutex_lock(&g_audio_OpusDecoderMutex);
				if(CirAudioBuf_SpaceOccupiedInSamples_S32(&OpusOutputCirBuf_LRMixed) >= FrmSizeInSamples)
				{
					OtPtrS16_Opus=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&OpusOutputCirBuf_LRMixed, FrmSizeInSamples);
					for(i=0;i<FrmSizeInSamples;i++)
					{
						AudioOneFrameBuf_OpusDecodedL[i]=*OtPtrS16_Opus++;
						AudioOneFrameBuf_OpusDecodedR[i]=*OtPtrS16_Opus++;
					}
				}else
				{
					memset(AudioOneFrameBuf_OpusDecodedL,0,sizeof(S16)*FrmSizeInSamples);
					memset(AudioOneFrameBuf_OpusDecodedR,0,sizeof(S16)*FrmSizeInSamples);
				}
			xos_mutex_unlock(&g_audio_OpusDecoderMutex);
		#endif
		#if EnableSbcDec==1
			unsigned short *OtPtrS16_Sbc;
			//take out Sbc output audio, and mix with UAC, with satuation, no gaining
			xos_mutex_lock(&g_audio_SbcDecoderMutex);
				if(CirAudioBuf_SpaceOccupiedInSamples_S32(&SbcOutputCirBuf_LRMixed) >= FrmSizeInSamples)
				{
					OtPtrS16_Sbc=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&SbcOutputCirBuf_LRMixed, FrmSizeInSamples);
					for(i=0;i<FrmSizeInSamples;i++)
					{
						AudioOneFrameBuf_SbcDecodedL[i]=*OtPtrS16_Sbc++;
						AudioOneFrameBuf_SbcDecodedR[i]=*OtPtrS16_Sbc++;
					}
				}else
				{
					//PRINTF("SBC cir buffer is empty\n");
					memset(AudioOneFrameBuf_SbcDecodedL,0,sizeof(S16)*FrmSizeInSamples);
					memset(AudioOneFrameBuf_SbcDecodedR,0,sizeof(S16)*FrmSizeInSamples);
				}
			xos_mutex_unlock(&g_audio_SbcDecoderMutex);
		#endif
	#endif

	#if 1	//step 3: prepare Rx data for Conversa, need to mix Sbc, Opus and do SRC
			int OutSampleNum;
			S32 TmpS32Buf_LRMixed_SrcInput                    [AudioFrameSizeInSamplePerCh_48KHz*2];
			S32 TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[AudioFrameSizeInSamplePerCh_16KHz*2];
			float *ConversaRefIn;

			//add OPUS and SBC to DstPtrS16_I2SAmpL and R --- will be streamed to AMP
			vec_add16x16(DstPtrS16_I2SAmpL, (const short int*)AudioOneFrameBuf_OpusDecodedL, (const short int*)SrcPtrS16_I2SNvtL, AudioFrameSizeInSamplePerCh_48KHz);
			vec_add16x16(DstPtrS16_I2SAmpR, (const short int*)AudioOneFrameBuf_OpusDecodedL, (const short int*)SrcPtrS16_I2SNvtR, AudioFrameSizeInSamplePerCh_48KHz);

			//convert to S32 and do SRC, then convert back to float --- to make the 16KHz float input ref in for Conversa
			for(i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
			{
				TmpS32Buf_LRMixed_SrcInput[2*i+0]=(DstPtrS16_I2SAmpL[i]<<16);
				TmpS32Buf_LRMixed_SrcInput[2*i+1]=(DstPtrS16_I2SAmpR[i]<<16);
			}

			ProcCadenceAsrc(&SRC_ConversaRx2, TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput, TmpS32Buf_LRMixed_SrcInput,  AudioFrameSizeInSamplePerCh_48KHz,    &OutSampleNum);

			//borrow TmpS32Buf_LRMixed_SrcInput for Conversa ref in buffer, 16KHz float
			ConversaRefIn=(float *)TmpS32Buf_LRMixed_SrcInput;
			for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				float a,b;
				a=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+0]*_Value_Pow_2_Neg31_;
				b=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+i]*_Value_Pow_2_Neg31_;
				ConversaRefIn[i]=(a+b)*0.5f;
			}
	#endif

	#if 1	//step 4: Conversa and VIT process
		PL_FLOAT*  pp_inputAudioData_Tx_FLT[4];
		PL_FLOAT*  pp_OutputAudioSignals   [5];	//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut

		#if 1
			pp_inputAudioData_Tx_FLT[0]=SrcPtrFlt_DmicIn0; //A3, glasses mic location
			pp_inputAudioData_Tx_FLT[1]=SrcPtrFlt_DmicIn1; //C7, glasses mic location
			pp_inputAudioData_Tx_FLT[2]=SrcPtrFlt_DmicIn2; //C8, glasses mic location
		#else
		#endif

		//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut
		ConversaProcessAndFeedToVit(pp_inputAudioData_Tx_FLT, ConversaRefIn, pp_OutputAudioSignals, RawMicSignal16BitForVitRef);
	#endif



	#if 1
		//fill AMP I2S out buffer with FltPtr_Tmp1L (SBC + OPUS mixed audio)
		//this is aready done when mixing opus and sbc

		ConvertFloatToS16(S16Ptr_Tmp1L, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut], AudioFrameSizeInSamplePerCh_16KHz);
		ConvertFloatToS16(S16Ptr_Tmp1R, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut], AudioFrameSizeInSamplePerCh_16KHz);
		//ConvertFloatToS16(S16Ptr_Tmp1L, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut], AudioFrameSizeInSamplePerCh_16KHz);

		for(int i=0; i < AudioFrameSizeInSamplePerCh_16KHz; i++ ) //16KHz to 48KHz
		{
			*DstPtrS16_I2SNvtL++ = *S16Ptr_Tmp1L;
			*DstPtrS16_I2SNvtL++ = *S16Ptr_Tmp1L;
			*DstPtrS16_I2SNvtL++ = *S16Ptr_Tmp1L++;

			*DstPtrS16_I2SNvtR++ = *S16Ptr_Tmp1R;
			*DstPtrS16_I2SNvtR++ = *S16Ptr_Tmp1R;
			*DstPtrS16_I2SNvtR++ = *S16Ptr_Tmp1R++;
		}

	#endif

	#if 1	//folding --- step5: put interested audio channels to UAC 8 ch
		//convert conversa output to S32
		vec_float2int(S32Ptr_Tmp1L, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut], -31,  AudioFrameSizeInSamplePerCh_16KHz);	//conversa TxOut
		vec_float2int(S32Ptr_Tmp1R, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut], -31,  AudioFrameSizeInSamplePerCh_16KHz);	//conversa BfOut

		if(ASR_WavPulse==ASR_WavPulse_WakeWordDetected)
		{
			ConversaTxOut32BitBuf[0]=0x7fff0000;
			ASR_WavPulse=ASR_WavPulse_NothingDetected;
		}else if(ASR_WavPulse==ASR_WavPulse_VoiceCmdDetected)
		{
			AecOut32BitBuf[0]=0x7fff0000;
			ASR_WavPulse=ASR_WavPulse_NothingDetected;
		}

		//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
		for(i=0;i<FrmSizeInSamples/3;i++)
		{
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+0];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+1];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=DstPtrS16_I2SNvtL[i*3]<<16; //to Nvt L
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=DstPtrS16_I2SNvtR[i*3]<<16; //to Nvt R

			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=S32Ptr_Tmp1L[i];	//conversa Tx out
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=S32Ptr_Tmp1R[i];	//conversa Bf out

			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=SrcPtrS32_Mic0[i];
		}
	#endif

	DbgPin7Dn();
}

__attribute__((__section__(".iram.text")))
void DspMainAudioFlowProcOneFrame_MusicPlayer(int OptionWord)
{
	S16 i;
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

	DbgPin7Up();

	#if 1	//step 1: convert input samples from S32 or S16 to float
		S16 RawMicSignal16BitForVitRef[AudioFrameSizeInSamplePerCh_16KHz];

		PrepareMainAudioFlowPointersAndInputFloatData(1,0,0,0);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
		int FrmSizeInSamples=PtrVarBlockSharedByDspAndMcu->I2SFrmSizeInSamples_Amp;

		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			RawMicSignal16BitForVitRef[i]=(SrcPtrS32_Mic0[i]>>16);

	#endif

	#if 1	//step 2: get audio from sbc decoder and opus decoder, or clear the decoder buffer if the decoder is NOT running
		//in this mode, OPUS and SBC stream are converted to 48KHz Fs
		#if EnableOpusDec==1
			unsigned short *OtPtrS16_Opus;
			//take out OPUS output audio, and mix with UAC, with satuation, no gaining
			xos_mutex_lock(&g_audio_OpusDecoderMutex);
				if(CirAudioBuf_SpaceOccupiedInSamples_S32(&OpusOutputCirBuf_LRMixed) >= FrmSizeInSamples)
				{
					OtPtrS16_Opus=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&OpusOutputCirBuf_LRMixed, FrmSizeInSamples);
					for(i=0;i<FrmSizeInSamples;i++)
					{
						AudioOneFrameBuf_OpusDecodedL[i]=*OtPtrS16_Opus++;
						AudioOneFrameBuf_OpusDecodedR[i]=*OtPtrS16_Opus++;
					}
				}else
				{
					memset(AudioOneFrameBuf_OpusDecodedL,0,sizeof(S16)*FrmSizeInSamples);
					memset(AudioOneFrameBuf_OpusDecodedR,0,sizeof(S16)*FrmSizeInSamples);
				}
			xos_mutex_unlock(&g_audio_OpusDecoderMutex);
		#endif
		#if EnableSbcDec==1
			unsigned short *OtPtrS16_Sbc;
			//take out Sbc output audio, and mix with UAC, with satuation, no gaining
			xos_mutex_lock(&g_audio_SbcDecoderMutex);
				if(CirAudioBuf_SpaceOccupiedInSamples_S32(&SbcOutputCirBuf_LRMixed) >= FrmSizeInSamples)
				{
					OtPtrS16_Sbc=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&SbcOutputCirBuf_LRMixed, FrmSizeInSamples);
					for(i=0;i<FrmSizeInSamples;i++)
					{
						AudioOneFrameBuf_SbcDecodedL[i]=*OtPtrS16_Sbc++;
						AudioOneFrameBuf_SbcDecodedR[i]=*OtPtrS16_Sbc++;
					}
				}else
				{
					//PRINTF("SBC cir buffer is empty\n");
					memset(AudioOneFrameBuf_SbcDecodedL,0,sizeof(S16)*FrmSizeInSamples);
					memset(AudioOneFrameBuf_SbcDecodedR,0,sizeof(S16)*FrmSizeInSamples);
				}
			xos_mutex_unlock(&g_audio_SbcDecoderMutex);
		#endif
	#endif

	#if 1	//step 3: prepare Rx data for Conversa, need to mix Sbc, Opus and do SRC
			int OutSampleNum;
			S32 TmpS32Buf_LRMixed_SrcInput                    [AudioFrameSizeInSamplePerCh_48KHz*2];
			S32 TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[AudioFrameSizeInSamplePerCh_16KHz*2];
			float *ConversaRefIn;

			//add OPUS and SBC to DstPtrS16_I2SAmpL and R --- will be streamed to AMP
			vec_add16x16(DstPtrS16_I2SAmpL, (const short int*)AudioOneFrameBuf_OpusDecodedL, (const short int*)AudioOneFrameBuf_SbcDecodedL, AudioFrameSizeInSamplePerCh_48KHz);
			vec_add16x16(DstPtrS16_I2SAmpR, (const short int*)AudioOneFrameBuf_OpusDecodedL, (const short int*)AudioOneFrameBuf_SbcDecodedR, AudioFrameSizeInSamplePerCh_48KHz);

			//convert to S32 and do SRC, then convert back to float --- to make the 16KHz float input ref in for Conversa
			for(i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
			{
				TmpS32Buf_LRMixed_SrcInput[2*i+0]=(DstPtrS16_I2SAmpL[i]<<16);
				TmpS32Buf_LRMixed_SrcInput[2*i+1]=(DstPtrS16_I2SAmpR[i]<<16);
			}

			ProcCadenceAsrc(&SRC_ConversaRx2, TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput, TmpS32Buf_LRMixed_SrcInput,  AudioFrameSizeInSamplePerCh_48KHz,    &OutSampleNum);

			//borrow TmpS32Buf_LRMixed_SrcInput for Conversa ref in buffer, 16KHz float
			ConversaRefIn=(float *)TmpS32Buf_LRMixed_SrcInput;
			for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				float a,b;
				a=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+0]*_Value_Pow_2_Neg31_;
				b=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+i]*_Value_Pow_2_Neg31_;
				ConversaRefIn[i]=(a+b)*0.5f;
			}
	#endif

	#if 1	//step 4: Conversa and VIT process
		PL_FLOAT*  pp_inputAudioData_Tx_FLT[4];
		PL_FLOAT*  pp_OutputAudioSignals   [5];	//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut

		#if 1
			pp_inputAudioData_Tx_FLT[0]=SrcPtrFlt_DmicIn0; //A3, glasses mic location
			pp_inputAudioData_Tx_FLT[1]=SrcPtrFlt_DmicIn1; //C7, glasses mic location
			pp_inputAudioData_Tx_FLT[2]=SrcPtrFlt_DmicIn2; //C8, glasses mic location
		#else
		#endif

		//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut
		ConversaProcessAndFeedToVit(pp_inputAudioData_Tx_FLT, ConversaRefIn, pp_OutputAudioSignals, RawMicSignal16BitForVitRef);
	#endif



	#if 1
		//fill AMP I2S out buffer with FltPtr_Tmp1L (SBC + OPUS mixed audio)
		//this is aready done when mixing opus and sbc
	#endif

	#if 1	//folding --- step5: put interested audio channels to UAC 8 ch
		//convert conversa output to S32
		vec_float2int(S32Ptr_Tmp1L, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut], -31,  AudioFrameSizeInSamplePerCh_16KHz);	//conversa TxOut
		vec_float2int(S32Ptr_Tmp1R, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut], -31,  AudioFrameSizeInSamplePerCh_16KHz);	//conversa BfOut

		//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
		for(i=0;i<FrmSizeInSamples/3;i++)
		{
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+0];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+1];

			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=S32Ptr_Tmp1L[i];	//conversa Tx out
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=S32Ptr_Tmp1R[i];	//conversa Bf out

			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=SrcPtrS32_Mic0[i];
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=SrcPtrS32_Mic0[i];
		}
	#endif

	DbgPin7Dn();
}

__attribute__((__section__(".iram.text")))
void DspMainAudioFlowProcOneFrame_Translation(int OptionWord)
{
	int i;
#if 0
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

	#if 1	//step 1: convert input samples from S32 or S16 to float
		PrepareMainAudioFlowPointersAndInputFloatData(1,1,1,1);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
	#endif
#else
		DbgPin7Up();

		#if 1	//step 1: convert input samples from S32 or S16 to float
			S16 RawMicSignal16BitForVitRef[AudioFrameSizeInSamplePerCh_16KHz];

			PrepareMainAudioFlowPointersAndInputFloatData(1,0,0,1);//B36932 (int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
			int FrmSizeInSamples=PtrVarBlockSharedByDspAndMcu->I2SFrmSizeInSamples_Amp;

			for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
				RawMicSignal16BitForVitRef[i]=(SrcPtrS32_Mic0[i]>>16);

		#endif

		#if 1	//step 2: get audio from sbc decoder and opus decoder, or clear the decoder buffer if the decoder is NOT running
			//in this mode, OPUS and SBC stream are converted to 48KHz Fs
			#if EnableOpusDec==1
				unsigned short *OtPtrS16_Opus;
				//take out OPUS output audio, and mix with UAC, with satuation, no gaining
				xos_mutex_lock(&g_audio_OpusDecoderMutex);
					if(CirAudioBuf_SpaceOccupiedInSamples_S32(&OpusOutputCirBuf_LRMixed) >= FrmSizeInSamples)
					{
						OtPtrS16_Opus=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&OpusOutputCirBuf_LRMixed, FrmSizeInSamples);
						for(i=0;i<FrmSizeInSamples;i++)
						{
							AudioOneFrameBuf_OpusDecodedL[i]=*OtPtrS16_Opus++;
							AudioOneFrameBuf_OpusDecodedR[i]=*OtPtrS16_Opus++;
						}
					}else
					{
						memset(AudioOneFrameBuf_OpusDecodedL,0,sizeof(S16)*FrmSizeInSamples);
						memset(AudioOneFrameBuf_OpusDecodedR,0,sizeof(S16)*FrmSizeInSamples);
					}
				xos_mutex_unlock(&g_audio_OpusDecoderMutex);
			#endif
			#if EnableSbcDec==1
				unsigned short *OtPtrS16_Sbc;
				//take out Sbc output audio, and mix with UAC, with satuation, no gaining
				xos_mutex_lock(&g_audio_SbcDecoderMutex);
					if(CirAudioBuf_SpaceOccupiedInSamples_S32(&SbcOutputCirBuf_LRMixed) >= FrmSizeInSamples)
					{
						OtPtrS16_Sbc=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&SbcOutputCirBuf_LRMixed, FrmSizeInSamples);
						for(i=0;i<FrmSizeInSamples;i++)
						{
							AudioOneFrameBuf_SbcDecodedL[i]=*OtPtrS16_Sbc++;
							AudioOneFrameBuf_SbcDecodedR[i]=*OtPtrS16_Sbc++;
						}
					}else
					{
						//PRINTF("SBC cir buffer is empty\n");
						memset(AudioOneFrameBuf_SbcDecodedL,0,sizeof(S16)*FrmSizeInSamples);
						memset(AudioOneFrameBuf_SbcDecodedR,0,sizeof(S16)*FrmSizeInSamples);
					}
				xos_mutex_unlock(&g_audio_SbcDecoderMutex);
			#endif
		#endif

		#if 1	//step 3: prepare Rx data for Conversa, need to mix Sbc, Opus and do SRC
				int OutSampleNum;
				S32 TmpS32Buf_LRMixed_SrcInput                    [AudioFrameSizeInSamplePerCh_48KHz*2];
				S32 TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[AudioFrameSizeInSamplePerCh_16KHz*2];
				float *ConversaRefIn;

				vec_add16x16(DstPtrS16_I2SAmpL, (const short int*)AudioOneFrameBuf_OpusDecodedL, (const short int*)AudioOneFrameBuf_SbcDecodedL, AudioFrameSizeInSamplePerCh_48KHz);
				vec_add16x16(DstPtrS16_I2SAmpR, (const short int*)AudioOneFrameBuf_OpusDecodedL, (const short int*)AudioOneFrameBuf_SbcDecodedR, AudioFrameSizeInSamplePerCh_48KHz);

				//convert to S32 and do SRC, then convert back to float --- to make the 16KHz float input ref in for Conversa
				for(i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
				{
					TmpS32Buf_LRMixed_SrcInput[2*i+0]=(DstPtrS16_I2SAmpL[i]<<16);
					TmpS32Buf_LRMixed_SrcInput[2*i+1]=(DstPtrS16_I2SAmpR[i]<<16);
				}

				ProcCadenceAsrc(&SRC_ConversaRx2, TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput, TmpS32Buf_LRMixed_SrcInput,  AudioFrameSizeInSamplePerCh_48KHz,    &OutSampleNum);

				//borrow TmpS32Buf_LRMixed_SrcInput for Conversa ref in buffer, 16KHz float
				ConversaRefIn=(float *)TmpS32Buf_LRMixed_SrcInput;
				for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
				{
					float a,b;
					a=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+0]*_Value_Pow_2_Neg31_;
					b=TmpS32Buf_LRMixed_SbcOpusMixed_16KHz_SrcOutput[2*i+i]*_Value_Pow_2_Neg31_;
					ConversaRefIn[i]=(a+b)*0.5f;
				}
		#endif

		#if 1	//step 4: Conversa and VIT process
			PL_FLOAT*  pp_inputAudioData_Tx_FLT[4];
			PL_FLOAT*  pp_OutputAudioSignals   [5];	//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut

			#if 1
				pp_inputAudioData_Tx_FLT[0]=SrcPtrFlt_DmicIn0; //A3, glasses mic location
				pp_inputAudioData_Tx_FLT[1]=SrcPtrFlt_DmicIn1; //C7, glasses mic location
				pp_inputAudioData_Tx_FLT[2]=SrcPtrFlt_DmicIn2; //C8, glasses mic location
			#else
			#endif

			//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut
			ConversaProcessAndFeedToVit(pp_inputAudioData_Tx_FLT, ConversaRefIn, pp_OutputAudioSignals, RawMicSignal16BitForVitRef);
		#endif

		#if 1 //fill AMP I2S out buffer with FltPtr_Tmp1L (SBC + OPUS mixed audio)
			//fill AMP I2S out buffer with FltPtr_Tmp1L (SBC + OPUS mixed audio)
			//this is aready done when mixing opus and sbc

			ConvertFloatToS16(S16Ptr_Tmp1L, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut], AudioFrameSizeInSamplePerCh_16KHz);
			ConvertFloatToS16(S16Ptr_Tmp1R, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut], AudioFrameSizeInSamplePerCh_16KHz);

			for(int i=0; i < AudioFrameSizeInSamplePerCh_16KHz; i++ ) //16KHz to 48KHz
			{
				*DstPtrS16_I2SNvtL++ = *S16Ptr_Tmp1L;
				*DstPtrS16_I2SNvtL++ = *S16Ptr_Tmp1L;
				*DstPtrS16_I2SNvtL++ = *S16Ptr_Tmp1L++;

				*DstPtrS16_I2SNvtR++ = *S16Ptr_Tmp1R;
				*DstPtrS16_I2SNvtR++ = *S16Ptr_Tmp1R;
				*DstPtrS16_I2SNvtR++ = *S16Ptr_Tmp1R++;
			}

		#endif

		#if 1	//folding --- step5: put interested audio channels to UAC 8 ch
			//convert conversa output to S32
			vec_float2int(S32Ptr_Tmp1L, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_TxOut], -31,  AudioFrameSizeInSamplePerCh_16KHz);	//conversa TxOut
			vec_float2int(S32Ptr_Tmp1R, (const float *)pp_OutputAudioSignals[CONVERSA_OutSignalIdx_BfOut], -31,  AudioFrameSizeInSamplePerCh_16KHz);	//conversa BfOut

			if(ASR_WavPulse==ASR_WavPulse_WakeWordDetected)
			{
				ConversaTxOut32BitBuf[0]=0x7fff0000;
				ASR_WavPulse=ASR_WavPulse_NothingDetected;
			}else if(ASR_WavPulse==ASR_WavPulse_VoiceCmdDetected)
			{
				AecOut32BitBuf[0]=0x7fff0000;
				ASR_WavPulse=ASR_WavPulse_NothingDetected;
			}

			//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
			for(i=0;i<FrmSizeInSamples/3;i++)
			{
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=DstPtrS16_I2SNvtL[i*3]<<16; //to Nvt L
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=DstPtrS16_I2SNvtR[i*3]<<16; //to Nvt R

				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=S32Ptr_Tmp1L[i];	//conversa Tx out
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=S32Ptr_Tmp1R[i];	//conversa Bf out

				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=SrcPtrS32_Mic0[i];
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=SrcPtrS32_Mic0[i];
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=SrcPtrS32_Mic0[i];
				PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=SrcPtrS32_Mic0[i];
			}
		#endif

		DbgPin7Dn();
#endif // #if 0

}

__attribute__((__section__(".iram.text")))
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

	#if 1	//step 1: convert input samples from S32 or S16 to float
		PrepareMainAudioFlowPointersAndInputFloatData(1,1,1,1);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
	#endif

}

__attribute__((__section__(".iram.text")))
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

	#if 1	//step 1: convert input samples from S32 or S16 to float
		PrepareMainAudioFlowPointersAndInputFloatData(1,1,1,1);//(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt)
	#endif

}
#endif



//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut
void ConversaProcessAndFeedToVit(float **PtrArray_MicIn, float *PtrRxIn, float **PtrArray_OutSignals, S16 *RawMicSigForVitRef)
{
	int i;

	NXP_STATUS retStatusConversa = OK;
	status_t   retStatus 		 = kStatus_Success;

	S32 VitInTmpBuf32Bit [AudioFrameSizeInSamplePerCh_16KHz];
	S16 VitInTmpBuf16Bit [AudioFrameSizeInSamplePerCh_16KHz];

	PL_FLOAT*  pp_inputAudioData_Rx_FLT [2];
	PL_FLOAT** p_currentSensingInput 	= NULL;		// current sensing input
	PL_FLOAT*  p_outputAudioData_Tx_FLT = NULL; 	// conversa out pointer
	PL_FLOAT*  ConversaTmpOutputPtr		= NULL;
	PL_FLOAT** pp_outputAudioData_Rx_FLT= NULL;

	pp_inputAudioData_Rx_FLT[0]=PtrRxIn;

	retStatusConversa = NxpConversa_Plugin_Process( &conversaPluginParams,
													PtrArray_MicIn,
													pp_inputAudioData_Rx_FLT,
													p_currentSensingInput );

	if (retStatusConversa != OK) 						// if return status not OK
	{
		if (retStatusConversa == LICENSE_EXPIRED)       // if license expired occurs
		{
			PRINTF_M("FAIL: conversa LICENSE_EXPIRED error\r\n");
			retStatus = kStatus_LicenseError;
		}
		else
		{
			PRINTF_M("FAIL: conversa process error %d\r\n",retStatusConversa);
			retStatus = kStatus_Fail;
		}
	}

	pp_outputAudioData_Rx_FLT = NxpConversa_Plugin_GetRxOut(&conversaPluginParams);
	if (pp_outputAudioData_Rx_FLT[0] != PL_NULL)
	{
		PtrArray_OutSignals[CONVERSA_OutSignalIdx_RxOut]=pp_outputAudioData_Rx_FLT[0];
	}
	else
	{
		PRINTF_M("FAIL: conversaProcess return NULL RxOut pointer\r\n");
		retStatus = kStatus_OutOfRange;
	}

	p_outputAudioData_Tx_FLT = NxpConversa_Plugin_GetTxOut(&conversaPluginParams);
	if (p_outputAudioData_Tx_FLT != PL_NULL)
	{
		PtrArray_OutSignals[CONVERSA_OutSignalIdx_TxOut]=p_outputAudioData_Tx_FLT;
	}
	else
	{
		PRINTF_M("FAIL: conversaProcess return NULL TxOut pointer\r\n");
		retStatus = kStatus_NullPointer;
	}



	//------------------take out internal output streaming from conversa and put to VIT input cir buffer, with converting float --> int --> short in-------
	//---beg---
	#if 1	//folding
		#if 1
			ConversaTmpOutputPtr = NxpConversa_Plugin_GetTxAecOut(&conversaPluginParams, 0);
			if(ConversaTmpOutputPtr!=PL_NULL)
			{
				PtrArray_OutSignals[CONVERSA_OutSignalIdx_AecOut]=ConversaTmpOutputPtr;
			}else
			{
				PRINTF("FAIL: conversaProcess return NULL AecOut pointer\r\n");
				retStatus = kStatus_NullPointer;
			}
		#endif

		#if 1
			ConversaTmpOutputPtr = NxpConversa_Plugin_GetTxBfOut(&conversaPluginParams);
			if(ConversaTmpOutputPtr!=PL_NULL)
			{
				PtrArray_OutSignals[CONVERSA_OutSignalIdx_BfOut]=ConversaTmpOutputPtr;
			}else
			{
				PRINTF("FAIL: conversaProcess return NULL BfOut pointer\r\n");
				retStatus = kStatus_NullPointer;
			}
		#endif

		#if 1
			ConversaTmpOutputPtr = NxpConversa_Plugin_GetTxNlpOut(&conversaPluginParams);
			if(ConversaTmpOutputPtr!=PL_NULL)
			{
				PtrArray_OutSignals[CONVERSA_OutSignalIdx_NlpOut]=ConversaTmpOutputPtr;
			}else
			{
				PRINTF("FAIL: conversaProcess return NULL NlpOut pointer\r\n");
				retStatus = kStatus_NullPointer;
			}
		#endif

		//select one of the following 5
		vec_float2int((int *)VitInTmpBuf32Bit, (const float *)PtrArray_OutSignals[CONVERSA_OutSignalIdx_AecOut],	-31,  AudioFrameSizeInSamplePerCh_16KHz);		//use conversa bf out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)PtrArray_OutSignals[CONVERSA_OutSignalIdx_BfOut],	-31,  AudioFrameSizeInSamplePerCh_16KHz);		//use conversa aec out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)PtrArray_OutSignals[CONVERSA_OutSignalIdx_NlpOut],	-31,  AudioFrameSizeInSamplePerCh_16KHz);		//use conversa nlp out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)PtrArray_OutSignals[CONVERSA_OutSignalIdx_TxOut],	-31,  AudioFrameSizeInSamplePerCh_16KHz);		//use conversa final tx out as VIT input
		//vec_float2int((int *)VitInTmpBuf32Bit, (const float *)PtrArray_MicIn[0],		-31,  AudioFrameSizeInSamplePerCh_16KHz);		//use raw mic 0 bf out as VIT input

		for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			VitInTmpBuf16Bit[i]=(VitInTmpBuf32Bit[i]>>16);

		xos_mutex_lock(&g_audio_vitBufferMutex);
			if(CirAudioBuf_SpaceAvailableInSamples_S16(&VitCircBuff) >= AudioFrameSizeInSamplePerCh_16KHz)
			{
				//PRINTF(".\r\n");
				CirAudioBuf_WriteSamples_S16(&VitCircBuff, AudioFrameSizeInSamplePerCh_16KHz, VitInTmpBuf16Bit);
				CirAudioBuf_WriteSamples_S16(&VitCircBuff_RawMic, AudioFrameSizeInSamplePerCh_16KHz, RawMicSigForVitRef);
			}
			//trigger VIT task to step on
			if(CirAudioBuf_SpaceOccupiedInSamples_S16(&VitCircBuff) >= VIT_SAMPLES_PER_30MS_FRAME)
			{
				ToTempSkipVitPorcess=0;
				xos_sem_put( &g_audioTask_audioVitProcessSemaphore );  	// Audio process semaphore put
				//PRINTF("1\r\n");
			}
		xos_mutex_unlock(&g_audio_vitBufferMutex);
	#endif
	//---end---
	//------------------take out internal output streaming from conversa and put to VIT input cir buffer, with converting float --> int --> short in-------
}
