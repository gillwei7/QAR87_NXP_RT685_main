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

extern XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.

__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 SrcIn_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S32 SrcOut_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo


__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S16 AudioOneFrameBuf_OpusDecodedL [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S16 AudioOneFrameBuf_OpusDecodedR [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S16 AudioOneFrameBuf_SbcDecodedL [AudioFrameSizeInSamplePerCh];
__attribute__((__section__(".dram.data")))
__attribute__((aligned(32)))
S16 AudioOneFrameBuf_SbcDecodedR [AudioFrameSizeInSamplePerCh];


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
	//	ch4: dmic0 + UacDn L 	(using soft mixer 3) --> Compressor3
	//	ch5: dmic1 + UacDn R 	(using soft mixer 4) --> Compressor4
	//	ch6: dmic2 + SweepTone L	(using soft mixer 5) --> Compressor5
	//	ch7: dmic3 + SweepTone R	(using soft mixer 6) --> Compressor6

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
	

	//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
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
}

__attribute__((__section__(".iram.text")))
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


__attribute__((__section__(".iram.text")))
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

	DbgPin7Up();

	/*
	int OutSampleNum;

	S32 *Ptr_Mic0=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[0];
	S32 *Ptr_Mic1=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[1];
	S32 *Ptr_Mic2=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[2];
	S32 *Ptr_Mic3=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[3];
	S32 *Ptr_Mic4=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[4];
	S32 *Ptr_Mic5=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[5];
	S32 *Ptr_Mic6=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[6];
	S32 *Ptr_Mic7=(S32 *)PtrVarBlockSharedByDspAndMcu->PdmInAudioBuf[7];

	S16 RawMicSignal16BitForVitRef[AudioFrameSizeInSamplePerCh];
	*/

	S16 i;

	int FrmSizeInSamples=PtrVarBlockSharedByDspAndMcu->I2SFrmSizeInSamples_Loc;

	//conversa processing and VIT is needed in music play --- to move conversa processing in a thread, rather than in the intr --- later to add


	#if 1	//folding --- step4: get audio from sbc decoder and opus decoder, or clear the decoder buffer if the decoder is NOT running
		#if EnableOpusDec==1
			unsigned short *OtPtrS16_Opus;
			//take out OPUS output audio, and mix with UAC, with satuation, no gaining
			if(OpusDecoderIsRunning)
			{
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
			}else
			{
				memset(AudioOneFrameBuf_OpusDecodedL,0,sizeof(S16)*FrmSizeInSamples);
				memset(AudioOneFrameBuf_OpusDecodedR,0,sizeof(S16)*FrmSizeInSamples);
			}
			//mix with satuation: EapProcOtBuf + OpusTmpBuf --> EapProcInBuf, all buffers should not be overlapped
			//vec_add32x32 ( EapProcInBuf, EapProcOtBuf, OpusTmpBuf, FrmSizeInSamples*2);		//this vect add is with satuation
		#endif
		#if EnableSbcDec==1
			unsigned short *OtPtrS16_Sbc;
			//take out Sbc output audio, and mix with UAC, with satuation, no gaining
			if(SbcOutputCirBuf_LRMixed_IsHalfFull)
			{
				xos_mutex_lock(&g_audio_SbcDecoderMutex);
					if(CirAudioBuf_SpaceOccupiedInSamples_S32(&SbcOutputCirBuf_LRMixed) >= FrmSizeInSamples)
					{
						OtPtrS16_Sbc=(unsigned short *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&SbcOutputCirBuf_LRMixed, FrmSizeInSamples);
						for(i=0;i<FrmSizeInSamples;i++)
						{
							AudioOneFrameBuf_SbcDecodedL[i]=*OtPtrS16_Sbc++;
							AudioOneFrameBuf_SbcDecodedR[i]=*OtPtrS16_Sbc++;
							//AudioOneFrameBuf_SbcDecodedL[i]=  0x10*i;
							//AudioOneFrameBuf_SbcDecodedR[i]=0-0x10*i;
						}
					}else
					{
						//PRINTF("SBC cir buffer is empty\n");
						memset(AudioOneFrameBuf_SbcDecodedL,0,sizeof(S16)*FrmSizeInSamples);
						memset(AudioOneFrameBuf_SbcDecodedR,0,sizeof(S16)*FrmSizeInSamples);
					}
				xos_mutex_unlock(&g_audio_SbcDecoderMutex);
			}else
			{
				memset(AudioOneFrameBuf_SbcDecodedL,0,sizeof(S16)*FrmSizeInSamples);
				memset(AudioOneFrameBuf_SbcDecodedR,0,sizeof(S16)*FrmSizeInSamples);
			}
			//mix with satuation: EapProcOtBuf + SbcTmpBuf --> EapProcInBuf, all buffers should not be overlapped
			//vec_add32x32 ( EapProcInBuf, EapProcOtBuf, SbcTmpBuf, FrmSizeInSamples*2);		//this vect add is with satuation   --- just for reference
		#endif
	#endif

	#if 1
		//fill AMP I2S out buffer with SBC or OPUS audio
		for(i=0;i<FrmSizeInSamples;i++)
		{
			PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL[i]=(AudioOneFrameBuf_SbcDecodedL[i] <<16);
			PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR[i]=(AudioOneFrameBuf_SbcDecodedR[i] <<16);
			//PtrVarBlockSharedByDspAndMcu->I2SLineOtBufL[i]=  i*0x100000;
			//PtrVarBlockSharedByDspAndMcu->I2SLineOtBufR[i]=0-i*0x100000;
		}
	#endif

	#if 1	//folding --- step5: put interested audio channels to UAC 8 ch

		//fill USB up streaming buffer --- 8 channels, all 16KHz, 32bit
		for(i=0;i<FrmSizeInSamples/3;i++)
		{
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=RawMic32BitBuf0[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=RawMic32BitBuf1[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=RawMic32BitBuf2[i];
			//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=RawMic32BitBuf3[i];
				//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=MicInMeterLvl1_InDb/100.0f*(float)0x7fffffff;
				//PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=MicInMeterLvl2_InDb/100.0f*(float)0x7fffffff;
					PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+0]=(AudioOneFrameBuf_OpusDecodedL[3*i] <<16);	//simple 1/3 decimation, Opus signal is 48KHz, UAC Up is 16KHz
					PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+1]=(AudioOneFrameBuf_OpusDecodedR[3*i] <<16);	//simple 1/3 decimation, Opus signal is 48KHz, UAC Up is 16KHz
					PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+2]=(AudioOneFrameBuf_SbcDecodedL[3*i] <<16);	//simple 1/3 decimation, Sbc signal is 48KHz, UAC Up is 16KHz
					PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+3]=(AudioOneFrameBuf_SbcDecodedR[3*i] <<16);	//simple 1/3 decimation, Sbc signal is 48KHz, UAC Up is 16KHz

			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+4]=  i*0x00100000;
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+5]=0-i*0x00100000;
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+6]=  i*0x00200000;
			PtrVarBlockSharedByDspAndMcu->UacUpAudioBuf[i*8+7]=0-i*0x00200000;
		}
	#endif

	#if 0
		if ((AudioFrameCnt % 128) < 64)
			LedOn_G();
		else
			LedOff_G();
	#endif

	//every 30 seconds trigger VIT process
	ToTempSkipVitPorcess=1;
	if ((AudioFrameCnt % 15)==1)
		xos_sem_put( &g_audioTask_audioVitProcessSemaphore );  	// Audio process semaphore put

	DbgPin7Dn();
}

__attribute__((__section__(".iram.text")))
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
}
