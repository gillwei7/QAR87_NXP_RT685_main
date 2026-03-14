/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "stdio.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i2s_dma.h"
//#include "debug_utils.h"

#include "GlobalDef.h"


#include "fsl_mu.h"
#include "fsl_sema42.h"

#include "AudioIoCfg_I2s.h"
#include "AudioIoCfg_Pdm.h"
#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"

#include "MainAudioFlow.h"


__attribute__((aligned(32))) S32 I2SInAudioBufL[AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((aligned(32))) S32 I2SInAudioBufR[AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((aligned(32))) S32 I2SOtAudioBufL[AudioFrameSizeInSamplePerCh_16KHz];
__attribute__((aligned(32))) S32 I2SOtAudioBufR[AudioFrameSizeInSamplePerCh_16KHz];

volatile S32 *I2SInputCh01Ptr;
volatile S32 *I2SOtputCh01Ptr;
volatile S32 *I2SOtputCh0123Ptr;


extern unsigned int VComReportValue_U32_1;
extern unsigned int VComReportValue_U32_2;
extern unsigned int VComReportValue_U32_3;
extern unsigned int VComReportValue_U32_4;

unsigned char WatchInfoSendBuf[600];
int CycCntInfoIdx;


#if EnableAudioPllAdjustingToSyncBetweenBtFsAndLocalFs==1

int PcmRxFrBtBufAodHistory[8];

int AUDIOPLL0NUM_StartingUpValue;
int AUDIOPLL0NUM_AdjustingValue;
int AUDIOPLL0NUM_AdjustingValuePre;

__attribute__((section("CodeQuickAccess")))
void GraduallySetAudioPllBackToDefault(void)
{

	CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue + AUDIOPLL0NUM_AdjustingValue;

	if(AUDIOPLL0NUM_AdjustingValue>0)
	{
		AUDIOPLL0NUM_AdjustingValue--;
	}else
	if(AUDIOPLL0NUM_AdjustingValue<0)
	{
		AUDIOPLL0NUM_AdjustingValue++;
	}

	AUDIOPLL0NUM_AdjustingValuePre=AUDIOPLL0NUM_AdjustingValue;
}

__attribute__((section("CodeQuickAccess")))
void CheckPcmRxFrBtBufAodAndAdjustAudioPll(int AodValue)
{
	int Aod=0;
	PcmRxFrBtBufAodHistory[7]=PcmRxFrBtBufAodHistory[6];
	PcmRxFrBtBufAodHistory[6]=PcmRxFrBtBufAodHistory[5];
	PcmRxFrBtBufAodHistory[5]=PcmRxFrBtBufAodHistory[4];
	PcmRxFrBtBufAodHistory[4]=PcmRxFrBtBufAodHistory[3];
	PcmRxFrBtBufAodHistory[3]=PcmRxFrBtBufAodHistory[2];
	PcmRxFrBtBufAodHistory[2]=PcmRxFrBtBufAodHistory[1];
	PcmRxFrBtBufAodHistory[1]=PcmRxFrBtBufAodHistory[0];
	PcmRxFrBtBufAodHistory[0]=AodValue;

	for(int i=0;i<8;i++)
		Aod+=PcmRxFrBtBufAodHistory[i];

	Aod>>=3;

	Aod=AodValue;		//this is to skip average, use AodValue directly

	//simple logic

	/*

					PLL out
22	528					1000	22.528	540.672				442.368			24.576
22	280(min Num value)	1000	22.28	534.72				437.4981818		24.30545455 this is 1.1% slower
22	776(max Num value)	1000	22.776	546.624				447.2378182		24.84654545 this is 1.1% faster

								0.988991477	0.011008523
								1.011008523	0.011008523
	*/

	if(Aod>AodLevel_6_8_LenInSamples)
	{
		//AOD = (9~10)/10 of full
		CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue+AUDIOPLL0NUM_AdjustingValue;
		if(AUDIOPLL0NUM_AdjustingValue<248)		//max --> +/- 1.1%
			AUDIOPLL0NUM_AdjustingValue++;
		//else
		//	AUDIOPLL0NUM_AdjustingValue=500;
//			PRINTF("1 %d %d\r\n",Aod,AUDIOPLL0NUM_AdjustingValue);
	}else
	if(Aod>AodLevel_5_8_LenInSamples)
	{
		//AOD = 8/10 of full
		CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue+AUDIOPLL0NUM_AdjustingValue;
		if(AUDIOPLL0NUM_AdjustingValue<124)
			AUDIOPLL0NUM_AdjustingValue++;
		//else
		//	AUDIOPLL0NUM_AdjustingValue=250;
//			PRINTF("2 %d %d\r\n",Aod,AUDIOPLL0NUM_AdjustingValue);
	}else
	if(Aod>AodLevel_3_8_LenInSamples)
	{
		//AOD = (4~7)/10 of full
		//no need to inc or dec CLKCTL1->AUDIOPLL0NUM
		CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue + AUDIOPLL0NUM_AdjustingValue;
		//gradually make AUDIOPLL0NUM_AdjustingValue back to 0
		//AUDIOPLL0NUM_AdjustingValue=0.0f*0.1f+0.9f * AUDIOPLL0NUM_AdjustingValuePre;
		if(AUDIOPLL0NUM_AdjustingValue>0)
		{
			AUDIOPLL0NUM_AdjustingValue--;
		}else
		if(AUDIOPLL0NUM_AdjustingValue<0)
		{
			AUDIOPLL0NUM_AdjustingValue++;
		}
//			PRINTF("0 %d %d\r\n",Aod,AUDIOPLL0NUM_AdjustingValue);
	}else
	if(Aod>AodLevel_2_8_LenInSamples)
	{
		//AOD = 3/10 of full
		CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue+AUDIOPLL0NUM_AdjustingValue;
		if(AUDIOPLL0NUM_AdjustingValue>-124)
			AUDIOPLL0NUM_AdjustingValue--;
		//else
		//	AUDIOPLL0NUM_AdjustingValue=250;
//			PRINTF("3 %d %d\r\n",Aod,AUDIOPLL0NUM_AdjustingValue);
	}else
	{
		//AOD = (0~2)/10 of full
		CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue+AUDIOPLL0NUM_AdjustingValue;
		if(AUDIOPLL0NUM_AdjustingValue>-248)		//max --> +/- 1.1%
			AUDIOPLL0NUM_AdjustingValue--;
		//else
		//	AUDIOPLL0NUM_AdjustingValue=500;
//			PRINTF("4 %d %d\r\n",Aod,AUDIOPLL0NUM_AdjustingValue);
	}
	AUDIOPLL0NUM_AdjustingValuePre=AUDIOPLL0NUM_AdjustingValue;
}
#endif

#if 1	//folding
#define SinToneTable_CycLenthInSamples		50
const float Table_GenerateSinWav_Flt[SinToneTable_CycLenthInSamples+AudioFrameSizeInSamplePerCh_48KHz]=    //cycle count number is 50 --- this is 16000/50=320Hz at 16KHz, 48000/50=960Hz at 48KHz
{
#if 1	//folding
		0	,
		0.125333234	,
		0.248689887	,
		0.368124553	,
		0.481753674	,
		0.587785252	,
		0.684547106	,
		0.770513243	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056516	,
		0.904827052	,
		0.844327926	,
		0.770513243	,
		0.684547106	,
		0.587785252	,
		0.481753674	,
		0.368124553	,
		0.248689887	,
		0.125333234	,
		8.97932E-11	,
		-0.125333233	,
		-0.248689887	,
		-0.368124553	,
		-0.481753674	,
		-0.587785252	,
		-0.684547106	,
		-0.770513243	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026728	,
		-0.982287251	,
		-0.951056516	,
		-0.904827053	,
		-0.844327926	,
		-0.770513243	,
		-0.684547106	,
		-0.587785252	,
		-0.481753674	,
		-0.368124553	,
		-0.248689887	,
		-0.125333234	,
		-1.79586E-10	,
		0.125333233	,
		0.248689887	,
		0.368124553	,
		0.481753674	,
		0.587785252	,
		0.684547106	,
		0.770513243	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056516	,
		0.904827053	,
		0.844327926	,
		0.770513243	,
		0.684547106	,
		0.587785252	,
		0.481753674	,
		0.368124553	,
		0.248689887	,
		0.125333234	,
		2.6938E-10	,
		-0.125333233	,
		-0.248689887	,
		-0.368124552	,
		-0.481753674	,
		-0.587785252	,
		-0.684547106	,
		-0.770513243	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026728	,
		-0.982287251	,
		-0.951056516	,
		-0.904827053	,
		-0.844327926	,
		-0.770513243	,
		-0.684547106	,
		-0.587785253	,
		-0.481753674	,
		-0.368124553	,
		-0.248689888	,
		-0.125333234	,
		-3.59173E-10	,
		0.125333233	,
		0.248689887	,
		0.368124552	,
		0.481753674	,
		0.587785252	,
		0.684547106	,
		0.770513243	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056516	,
		0.904827053	,
		0.844327926	,
		0.770513243	,
		0.684547106	,
		0.587785253	,
		0.481753674	,
		0.368124553	,
		0.248689888	,
		0.125333234	,
		4.48966E-10	,
		-0.125333233	,
		-0.248689887	,
		-0.368124552	,
		-0.481753674	,
		-0.587785252	,
		-0.684547106	,
		-0.770513242	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026728	,
		-0.982287251	,
		-0.951056516	,
		-0.904827053	,
		-0.844327926	,
		-0.770513243	,
		-0.684547106	,
		-0.587785253	,
		-0.481753675	,
		-0.368124553	,
		-0.248689888	,
		-0.125333234	,
		-5.38759E-10	,
		0.125333233	,
		0.248689887	,
		0.368124552	,
		0.481753674	,
		0.587785252	,
		0.684547106	,
		0.770513242	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056516	,
		0.904827053	,
		0.844327926	,
		0.770513243	,
		0.684547106	,
		0.587785253	,
		0.481753675	,
		0.368124553	,
		0.248689888	,
		0.125333234	,
		6.28551E-10	,
		-0.125333233	,
		-0.248689887	,
		-0.368124552	,
		-0.481753674	,
		-0.587785252	,
		-0.684547105	,
		-0.770513242	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026728	,
		-0.982287251	,
		-0.951056517	,
		-0.904827053	,
		-0.844327926	,
		-0.770513243	,
		-0.684547106	,
		-0.587785253	,
		-0.481753675	,
		-0.368124553	,
		-0.248689888	,
		-0.125333234	,
		-7.18345E-10	,
		0.125333233	,
		0.248689886	,
		0.368124552	,
		0.481753673	,
		0.587785252	,
		0.684547105	,
		0.770513242	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056517	,
		0.904827053	,
		0.844327926	,
		0.770513243	,
		0.684547107	,
		0.587785253	,
		0.481753675	,
		0.368124553	,
		0.248689888	,
		0.125333234	,
		8.0814E-10	,
		-0.125333233	,
		-0.248689886	,
		-0.368124552	,
		-0.481753673	,
		-0.587785252	,
		-0.684547105	,
		-0.770513242	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026728	,
		-0.982287251	,
		-0.951056517	,
		-0.904827053	,
		-0.844327926	,
		-0.770513243	,
		-0.684547107	,
		-0.587785253	,
		-0.481753675	,
		-0.368124554	,
		-0.248689888	,
		-0.125333234	,
		-8.97932E-10	,
		0.125333233	,
		0.248689886	,
		0.368124552	,
		0.481753673	,
		0.587785252	,
		0.684547105	,
		0.770513242	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056517	,
		0.904827053	,
		0.844327926	,
		0.770513243	,
		0.684547107	,
		0.587785253	,
		0.481753675	,
		0.368124554	,
		0.248689888	,
		0.125333235	,
		9.87723E-10	,
		-0.125333233	,
		-0.248689886	,
		-0.368124552	,
		-0.481753673	,
		-0.587785251	,
		-0.684547105	,
		-0.770513242	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026728	,
		-0.982287251	,
		-0.951056517	,
		-0.904827053	,
		-0.844327926	,
		-0.770513243	,
		-0.684547107	,
		-0.587785253	,
		-0.481753675	,
		-0.368124554	,
		-0.248689888	,
		-0.125333235	,
		-1.07752E-09	,
		0.125333232	,
		0.248689886	,
		0.368124552	,
		0.481753673	,
		0.587785251	,
		0.684547105	,
		0.770513242	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.982287251	,
		0.998026728	,
		0.998026728	,
		0.982287251	,
		0.951056517	,
		0.904827053	,
		0.844327926	,
		0.770513244	,
		0.684547107	,
		0.587785253	,
		0.481753675	,
		0.368124554	,
		0.248689888	,
		0.125333235	,
		1.16731E-09	,
		-0.125333232	,
		-0.248689886	,
		-0.368124552	,
		-0.481753673	,
		-0.587785251	,
		-0.684547105	,
		-0.770513242	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.982287251	,
		-0.998026728	,
		-0.998026729	,
		-0.982287251	,
		-0.951056517	,
		-0.904827053	,
		-0.844327926	,
		-0.770513244	,
		-0.684547107	,
		-0.587785253	,
		-0.481753675	,
		-0.368124554	,
		-0.248689888	,
		-0.125333235	,
		-1.2571E-09	,
		0.125333232	,
		0.248689886	,
		0.368124552	,
		0.481753673	,
		0.587785251	,
		0.684547105	,
		0.770513242	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.98228725	,
		0.998026728	,
		0.998026729	,
		0.982287251	,
		0.951056517	,
		0.904827053	,
		0.844327926	,
		0.770513244	,
		0.684547107	,
		0.587785253	,
		0.481753675	,
		0.368124554	,
		0.248689888	,
		0.125333235	,
		1.3469E-09	,
		-0.125333232	,
		-0.248689886	,
		-0.368124551	,
		-0.481753673	,
		-0.587785251	,
		-0.684547105	,
		-0.770513242	,
		-0.844327925	,
		-0.904827052	,
		-0.951056516	,
		-0.98228725	,
		-0.998026728	,
		-0.998026729	,
		-0.982287251	,
		-0.951056517	,
		-0.904827053	,
		-0.844327926	,
		-0.770513244	,
		-0.684547107	,
		-0.587785253	,
		-0.481753675	,
		-0.368124554	,
		-0.248689889	,
		-0.125333235	,
		-1.43669E-09	,
		0.125333232	,
		0.248689886	,
		0.368124551	,
		0.481753673	,
		0.587785251	,
		0.684547105	,
		0.770513242	,
		0.844327925	,
		0.904827052	,
		0.951056516	,
		0.98228725	,
		0.998026728	,
		0.998026729	,
		0.982287251	,
		0.951056517	,
		0.904827053	,
		0.844327926	,
		0.770513244	,
		0.684547107	,
		0.587785254	,
		0.481753675	,
		0.368124554	,
		0.248689889	,
		0.125333235	,
		1.52649E-09	,
		-0.125333232	,
		-0.248689886	,
		-0.368124551	,
		-0.481753673	,
		-0.587785251	,
		-0.684547105	,
		-0.770513242	,
		-0.844327925	,
#endif
};
const float *GenerateSinWav_Ptr_Flt[10]=
{
		Table_GenerateSinWav_Flt+0,
		Table_GenerateSinWav_Flt+4,
		Table_GenerateSinWav_Flt+8,
		Table_GenerateSinWav_Flt+12,
		Table_GenerateSinWav_Flt+16,
		Table_GenerateSinWav_Flt+20,
		Table_GenerateSinWav_Flt+24,
		Table_GenerateSinWav_Flt+28,
		Table_GenerateSinWav_Flt+32,
		Table_GenerateSinWav_Flt+36,
};
float Gain_GenerateSinWav  =0.80f;
float Gain_GenerateSinWav_L=0.80f;
float Gain_GenerateSinWav_R=0.80f;

//Note: all the following functions uses GenerateSinWav_Ptr_Flt[10], so they are actually 10 stand alone sin tone generators
void GenerateSinWavFromTable_S32_DualCh(int WhichOne, S32 *DstPtrL, S32 *DstPtrR, int L)	//L should be <=AudioFrameSizeInSamplePerCh_48KHz
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtrL++=  (Gain_GenerateSinWav_L)*((*GenerateSinWav_Ptr_Flt[WhichOne]  )*_Value_Pow_2_31_M1);
        *DstPtrR++=0-(Gain_GenerateSinWav_R)*((*GenerateSinWav_Ptr_Flt[WhichOne]++)*_Value_Pow_2_31_M1);
    }
    while(GenerateSinWav_Ptr_Flt[WhichOne]>=(Table_GenerateSinWav_Flt+SinToneTable_CycLenthInSamples))
    	GenerateSinWav_Ptr_Flt[WhichOne]-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S32_DualCh_LRMixed(int WhichOne, S32 *DstPtr, int L)	//L should be <=AudioFrameSizeInSamplePerCh_48KHz
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtr++=  Gain_GenerateSinWav_L*((*GenerateSinWav_Ptr_Flt[WhichOne]  )*_Value_Pow_2_31_M1);
        *DstPtr++=0-Gain_GenerateSinWav_R*((*GenerateSinWav_Ptr_Flt[WhichOne]++)*_Value_Pow_2_31_M1);
    }
    while(GenerateSinWav_Ptr_Flt[WhichOne]>=(Table_GenerateSinWav_Flt+SinToneTable_CycLenthInSamples))
    	GenerateSinWav_Ptr_Flt[WhichOne]-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S32_SingleCh(int WhichOne, S32 *DstPtr, int L, int PosNeg)	//L should be <=AudioFrameSizeInSamplePerCh_48KHz
{
    U16 i;
    if(PosNeg>0)
    	PosNeg=1;
    else
    	PosNeg=-1;

    for(i=0;i<L;i++)
    {
        *DstPtr++=PosNeg*Gain_GenerateSinWav*(S32)(*GenerateSinWav_Ptr_Flt[WhichOne]++)*_Value_Pow_2_31_M1;
    }
    while(GenerateSinWav_Ptr_Flt[WhichOne]>=(Table_GenerateSinWav_Flt+SinToneTable_CycLenthInSamples))
    	GenerateSinWav_Ptr_Flt[WhichOne]-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S16_DualCh  (int WhichOne, S16 *DstPtrL, S16 *DstPtrR, int L)
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtrL++=  Gain_GenerateSinWav_L*(*GenerateSinWav_Ptr_Flt[WhichOne]  )*_Value_Pow_2_15_M1;
        *DstPtrR++=0-Gain_GenerateSinWav_R*(*GenerateSinWav_Ptr_Flt[WhichOne]++)*_Value_Pow_2_15_M1;
    }
    while(GenerateSinWav_Ptr_Flt[WhichOne]>=(Table_GenerateSinWav_Flt+SinToneTable_CycLenthInSamples))
    	GenerateSinWav_Ptr_Flt[WhichOne]-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S16_DualCh_LRMixed(int WhichOne, S16 *DstPtr, int L)			//L should be <=AudioFrameSizeInSamplePerCh_48KHz
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtr++=  Gain_GenerateSinWav_L*(*GenerateSinWav_Ptr_Flt[WhichOne]  )*_Value_Pow_2_15_M1;
        *DstPtr++=0-Gain_GenerateSinWav_R*(*GenerateSinWav_Ptr_Flt[WhichOne]++)*_Value_Pow_2_15_M1;
    }
    while(GenerateSinWav_Ptr_Flt[WhichOne]>=(Table_GenerateSinWav_Flt+SinToneTable_CycLenthInSamples))
    	GenerateSinWav_Ptr_Flt[WhichOne]-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S16_SingleCh(int WhichOne, S16 *DstPtr, int L, int PosNeg)		//L should be <=AudioFrameSizeInSamplePerCh_48KHz
{
    U16 i;
    if(PosNeg>0)
    	PosNeg=1;
    else
    	PosNeg=-1;

    for(i=0;i<L;i++)
    {
        *DstPtr++=PosNeg*Gain_GenerateSinWav*(Gain_GenerateSinWav_L)*(*GenerateSinWav_Ptr_Flt[WhichOne]++)*_Value_Pow_2_15_M1;
    }
    while(GenerateSinWav_Ptr_Flt[WhichOne]>=(Table_GenerateSinWav_Flt+SinToneTable_CycLenthInSamples))
    	GenerateSinWav_Ptr_Flt[WhichOne]-=SinToneTable_CycLenthInSamples;
}
#endif

#if 0
//don't open this part, this part is only for reference
//example code of using CMSIS filter


#define FirCoefLength	64
#define FirBlockSize	48
//this is 1/3 FIR LPF
const float B[FirCoefLength] = {
		  0.0003175469796565,0.0006955264527167,0.0005521719831693,-0.001097925697437,
		  -0.004747652678265,  -0.0094239017937, -0.01253477819377, -0.01125116297883,
		  -0.004878921872828, 0.003624288733652, 0.008820569254099, 0.006604939508406,
		  -0.001951919070258,  -0.0102657680642, -0.01074858630556,-0.001354638233184,
		    0.01133430783888,  0.01620443297627, 0.006860978190084, -0.01128773506176,
		   -0.02320430362328, -0.01588336604844, 0.008939285416451,  0.03242022281066,
		    0.03130769275422,-0.001852903841553, -0.04676570260408, -0.06370481877455,
		   -0.01964239937978,   0.0850765718126,   0.2098032994481,    0.294157546337,
		      0.294157546337,   0.2098032994481,   0.0850765718126, -0.01964239937978,
		   -0.06370481877455, -0.04676570260408,-0.001852903841553,  0.03130769275422,
		    0.03242022281066, 0.008939285416451, -0.01588336604844, -0.02320430362328,
		   -0.01128773506176, 0.006860978190084,  0.01620443297627,  0.01133430783888,
		  -0.001354638233184, -0.01074858630556,  -0.0102657680642,-0.001951919070258,
		   0.006604939508406, 0.008820569254099, 0.003624288733652,-0.004878921872828,
		   -0.01125116297883, -0.01253477819377,  -0.0094239017937,-0.004747652678265,
		  -0.001097925697437,0.0005521719831693,0.0006955264527167,0.0003175469796565
};


arm_fir_decimate_instance_f32 FirFilter_48KTo16K_PostLPF_Ch1;
arm_fir_decimate_instance_f32 FirFilter_48KTo16K_PostLPF_Ch2;

float HistState_FirFilter_48KTo16K_PostLPF_Ch1[FirCoefLength+FirBlockSize-1];		//pState is of length numTaps+blockSize-1
float HistState_FirFilter_48KTo16K_PostLPF_Ch2[FirCoefLength+FirBlockSize-1];		//pState is of length numTaps+blockSize-1

void InitFirFilter(void)
{
	arm_fir_decimate_init_f32	(
			&FirFilter_48KTo16K_PostLPF_Ch1,
			FirCoefLength,
			3,
			B,
			HistState_FirFilter_48KTo16K_PostLPF_Ch1,
			FirBlockSize
	);
	arm_fir_decimate_init_f32	(
			&FirFilter_48KTo16K_PostLPF_Ch2,
			FirCoefLength,
			3,
			B,
			HistState_FirFilter_48KTo16K_PostLPF_Ch2,
			FirBlockSize
	);

}

void Convert48KTo16K(arm_fir_decimate_instance_f32 *FirFlt, float *DstPtr, float *SrcPtr, int LengthOfOutput)
{
    arm_fir_decimate_f32	(
            FirFlt,
            SrcPtr,
            DstPtr,
            FirBlockSize
    );
}


short int InputBuffer_L[48*10];
short int OutputBuffer_L[16*10];
short int InputBuffer_R[48*10];
short int OutputBuffer_R[16*10];

void TestFirFilter(void)
{
    short int *InputPtrL=InputBuffer_L;
    short int *InputPtrR=InputBuffer_R;
    short int *OutputPtrL=OutputBuffer_L;
    short int *OutputPtrR=OutputBuffer_R;

	float InBuf_Flt[48];
	float OutBuf_Flt[16];

    for(int i=0;i<480;i++)
    {
        InputBuffer_L[i]=i%48*600;
        InputBuffer_R[i]=-i%48*600;
    }

	InitFirFilter();

    for(int i=0;i<10;i++)
    {
        //convert 48k to 16k for ch1 (left)
        arm_q15_to_float(InputPtrL, InBuf_Flt, 48);			//convert source data to float
        Convert48KTo16K(&FirFilter_48KTo16K_PostLPF_Ch1, OutBuf_Flt, InBuf_Flt, 16);	//convert 48 float samples to 16 float samples
        arm_float_to_q15(OutBuf_Flt, OutputPtrL, 16);		//convert result data back to short int
        InputPtrL+=48;
        OutputPtrL+=16;

        //convert 48k to 16k for ch2 (right)
        arm_q15_to_float(InputPtrR, InBuf_Flt, 48);			//convert source data to float
        Convert48KTo16K(&FirFilter_48KTo16K_PostLPF_Ch2, OutBuf_Flt, InBuf_Flt, 16);	//convert 48 float samples to 16 float samples
        arm_float_to_q15(OutBuf_Flt, OutputPtrR, 16);		//convert result data back to short int
        InputPtrR+=48;
        OutputPtrR+=16;
    }
}

#endif


