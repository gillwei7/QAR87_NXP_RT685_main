/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



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
#include "GlobalDef.h"
#include "SubFunc.h"
#include "AudioProc_Conversa.h"

#include "GainingAndMixing.h"

#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"

TMixer2To1WithFadingGain Mixer1;
TMixer2To1WithFadingGain Mixer2;

TSoftGain   GainControl1;
TSoftGain   GainControl2;


__attribute__((__section__(".iram.text")))
void AudioProcOneFrame_Gain(TSoftGain *Gain, float *Dst, float *Src, int l)
{
    if(Gain->EnableGainFading)
    {
        //soft gain
        //step 1: create gain array
        for(int i=0;i<l;i++)
        {
            Gain->GainArray[i]=Gain->TargetGain*Gain->SmoothingCoefBeta + Gain->PrevGain*Gain->SmoothingCoefAlfa;
            Gain->PrevGain=Gain->GainArray[i];
        }
        
        //step 2: use gain array to gain and mix
		//vec_addf(0,0,0,10);
		for(int i=0;i<l;i++)
			*Dst++= *Src++ * Gain->GainArray[i];

    }else
    {
        //hard gain
        for(int i=0;i<l;i++)
            *Dst++= *Src++ * Gain->TargetGain;
    }
}

__attribute__((__section__(".iram.text")))
void AudioProcOneFrame_Mix2To1(TMixer2To1WithFadingGain *Mixer, float *Dst, float *Src1, float *Src2, int l)
{
    if(Mixer->EnableGainFading)
    {
        //soft gain
        //step 1: create gain array
        for(int i=0;i<l;i++)
        {
            Mixer->GainArraySrc1[i]=Mixer->TargetGainSrc1*Mixer->SmoothingCoefBeta + Mixer->PrevGainSrc1*Mixer->SmoothingCoefAlfa;
            Mixer->GainArraySrc2[i]=Mixer->TargetGainSrc2*Mixer->SmoothingCoefBeta + Mixer->PrevGainSrc2*Mixer->SmoothingCoefAlfa;
            Mixer->PrevGainSrc1=Mixer->GainArraySrc1[i];
            Mixer->PrevGainSrc2=Mixer->GainArraySrc2[i];
        }
        
        //step 2: use gain array to gain and mix
        for(int i=0;i<l;i++)
            *Dst++= *Src1++ * Mixer->GainArraySrc1[i] + *Src2++ * Mixer->GainArraySrc2[i];

    }else
    {
        //hard gain
        for(int i=0;i<l;i++)
            *Dst++= *Src1++ * Mixer->TargetGainSrc1 + *Src2++ * Mixer->TargetGainSrc2;
    }
}

void InitGainAndMixing(void)
{
    memset(&Mixer1,0,sizeof(Mixer1));
    memset(&Mixer2,0,sizeof(Mixer2));
    memset(&GainControl1,0,sizeof(GainControl1));
    memset(&GainControl2,0,sizeof(GainControl2));

    Mixer1.EnableGainFading=1;
    Mixer1.SmoothingCoefAlfa=0.999f;
    Mixer1.SmoothingCoefBeta=1-Mixer1.SmoothingCoefAlfa;
    Mixer1.TargetGainSrc1=1.0;
    Mixer1.TargetGainSrc2=0.0;

    Mixer2.EnableGainFading=1;
    Mixer2.SmoothingCoefAlfa=0.999f;
    Mixer2.SmoothingCoefBeta=1-Mixer1.SmoothingCoefAlfa;
    Mixer2.TargetGainSrc1=1.0;
    Mixer2.TargetGainSrc2=0.0;

    GainControl1.EnableGainFading=1;
	GainControl1.SmoothingCoefAlfa=0.9999f;
	GainControl1.SmoothingCoefBeta=1-GainControl1.SmoothingCoefAlfa;
	GainControl1.TargetGain=1.0f;

    GainControl2.EnableGainFading=1;
	GainControl2.SmoothingCoefAlfa=0.9999f;
	GainControl2.SmoothingCoefBeta=1-GainControl1.SmoothingCoefAlfa;
	GainControl2.TargetGain=1.0f;
};


