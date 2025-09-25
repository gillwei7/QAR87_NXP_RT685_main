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
	float GainArraySrc1[AudioFrameSizeInSamplePerCh];
	float GainArraySrc2[AudioFrameSizeInSamplePerCh];
	float SmoothingCoefAlfa;
	float SmoothingCoefBeta;
	float PrevGainSrc1;
	float PrevGainSrc2;
	float TargetGainSrc1;
	float TargetGainSrc2;
	int   EnableGainFading;
}TMixer2To1WithFadingGain;

typedef struct
{
	float GainArray[AudioFrameSizeInSamplePerCh];
	float SmoothingCoefAlfa;
	float SmoothingCoefBeta;
	float PrevGain;
	float TargetGain;
	int   EnableGainFading;
}TSoftGain;


extern TMixer2To1WithFadingGain Mixer1;
extern TMixer2To1WithFadingGain Mixer2;

extern TSoftGain GainControl1;
extern TSoftGain GainControl2;

extern void AudioProcOneFrame_Gain(TSoftGain *Gain, float *Dst, float *Src, int l);
extern void AudioProcOneFrame_Mix2To1(TMixer2To1WithFadingGain *Mixer, float *Dst, float *Src1, float *Src2, int l);
extern void InitGainAndMixing(void);


#ifdef __cplusplus
}
#endif
#endif  // SUBFUNC01_H_INCLUDED
