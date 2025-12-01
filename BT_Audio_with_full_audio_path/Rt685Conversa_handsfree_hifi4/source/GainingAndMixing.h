/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __GainAndMix_H_INCLUDED
#define __GainAndMix_H_INCLUDED

#include "GlobalDef.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
	float GainArraySrc1[AudioFrameSizeInSamplePerChMaxForDMABuf];
	float GainArraySrc2[AudioFrameSizeInSamplePerChMaxForDMABuf];
	float SmoothingCoefAlfa;
	float SmoothingCoefBeta;
	float PrevGainSrc1;
	float PrevGainSrc2;
	float TargetGainSrc1;
	float TargetGainSrc2;
	int   EnableGainFading;
}TSoftMixer2To1;

typedef struct
{
	float GainArray[AudioFrameSizeInSamplePerChMaxForDMABuf*2];		//suppose can also work for LRMixed buffer
	float SmoothingCoefAlfa;
	float SmoothingCoefBeta;
	float PrevGain;
	float TargetGain;
	int   EnableGainFading;
}TSoftGain;


extern TSoftMixer2To1 SoftMixer2To1_1;
extern TSoftMixer2To1 SoftMixer2To1_2;
extern TSoftMixer2To1 SoftMixer2To1_3;
extern TSoftMixer2To1 SoftMixer2To1_4;
extern TSoftMixer2To1 SoftMixer2To1_5;
extern TSoftMixer2To1 SoftMixer2To1_6;
extern TSoftMixer2To1 SoftMixer2To1_7;
extern TSoftMixer2To1 SoftMixer2To1_8;

extern TSoftGain SoftGainControl_MasterLAndR;
/*
extern TSoftGain SoftGainControl2;
extern TSoftGain SoftGainControl3;
extern TSoftGain SoftGainControl4;
extern TSoftGain SoftGainControl5;
extern TSoftGain SoftGainControl6;
extern TSoftGain SoftGainControl7;
extern TSoftGain SoftGainControl8;
*/

extern void AudioProcOneFrameS32_Gain(TSoftGain *Gain, S32 *Dst, S32 *Src, int l);
extern void AudioProcOneFrame_Gain(TSoftGain *Gain, float *Dst, float *Src, int l);
extern void AudioProcOneFrame_Mix2To1(TSoftMixer2To1 *Mixer, float *Dst, float *Src1, float *Src2, int l);
extern void InitGainAndMixing(void);


#ifdef __cplusplus
}
#endif
#endif  // SUBFUNC01_H_INCLUDED
