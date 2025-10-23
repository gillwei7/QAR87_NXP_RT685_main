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


TSoftMixer2To1 SoftMixer2To1_1;
TSoftMixer2To1 SoftMixer2To1_2;
TSoftMixer2To1 SoftMixer2To1_3;
TSoftMixer2To1 SoftMixer2To1_4;
TSoftMixer2To1 SoftMixer2To1_5;
TSoftMixer2To1 SoftMixer2To1_6;
TSoftMixer2To1 SoftMixer2To1_7;
TSoftMixer2To1 SoftMixer2To1_8;

TSoftGain SoftGainControl1;
TSoftGain SoftGainControl2;
TSoftGain SoftGainControl3;
TSoftGain SoftGainControl4;
TSoftGain SoftGainControl5;
TSoftGain SoftGainControl6;
TSoftGain SoftGainControl7;
TSoftGain SoftGainControl8;


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
void AudioProcOneFrame_Mix2To1(TSoftMixer2To1 *Mixer, float *Dst, float *Src1, float *Src2, int l)
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

void Init_SoftGain(TSoftGain *p, float TgtGain)
{
    memset(p,0,sizeof(TSoftGain));
    p->EnableGainFading=1;
    p->SmoothingCoefAlfa=0.999f;
    p->SmoothingCoefBeta=1-p->SmoothingCoefAlfa;
    p->TargetGain=TgtGain;
}
void Init_SoftMixer2To1(TSoftMixer2To1 *p, float TgtGain_In1,  float TgtGain_In2)
{
    memset(p,0,sizeof(TSoftMixer2To1));
    p->EnableGainFading=1;
    p->SmoothingCoefAlfa=0.999f;
    p->SmoothingCoefBeta=1-p->SmoothingCoefAlfa;
    p->TargetGainSrc1=TgtGain_In1;
    p->TargetGainSrc2=TgtGain_In2;
}

void InitGainAndMixing(void)
{
	Init_SoftGain(&SoftGainControl1, 0.9999f);
	Init_SoftGain(&SoftGainControl2, 0.9999f);
	Init_SoftGain(&SoftGainControl3, 0.9999f);
	Init_SoftGain(&SoftGainControl4, 0.9999f);
	Init_SoftGain(&SoftGainControl5, 0.9999f);
	Init_SoftGain(&SoftGainControl6, 0.9999f);
	Init_SoftGain(&SoftGainControl7, 0.9999f);
	Init_SoftGain(&SoftGainControl8, 0.9999f);

	Init_SoftMixer2To1(&SoftMixer2To1_1,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_2,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_3,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_4,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_5,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_6,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_7,0.9999f,0.9999f);
	Init_SoftMixer2To1(&SoftMixer2To1_8,0.9999f,0.9999f);
};


