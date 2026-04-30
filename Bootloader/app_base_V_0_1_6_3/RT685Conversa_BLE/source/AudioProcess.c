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
//#include "Sweep.h"
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

//int I2S1InputBufAod;
//int volatile I2S1InputBufIsHalfFul;
int I2S1InputBufAodHist[8];

int AUDIOPLL0NUM_StartingUpValue;
int AUDIOPLL0NUM_AdjustingValue;
int AUDIOPLL0NUM_AdjustingValuePre;
/*
void GraduallySetAudioPllBackToDefault(void)
{
	CLKCTL1->AUDIOPLL0NUM = AUDIOPLL0NUM_StartingUpValue + AUDIOPLL0NUM_AdjustingValue;
	//gradually make AUDIOPLL0NUM_AdjustingValue back to 0
	AUDIOPLL0NUM_AdjustingValue=0.0f*0.1f+0.9f * AUDIOPLL0NUM_AdjustingValuePre;
	AUDIOPLL0NUM_AdjustingValuePre=AUDIOPLL0NUM_AdjustingValue;
}
*/
void CheckI2SInputBufAodAndAdjustAudioPll(int AodValue)
{
	int Aod=0;
	I2S1InputBufAodHist[7]=I2S1InputBufAodHist[6];
	I2S1InputBufAodHist[6]=I2S1InputBufAodHist[5];
	I2S1InputBufAodHist[5]=I2S1InputBufAodHist[4];
	I2S1InputBufAodHist[4]=I2S1InputBufAodHist[3];
	I2S1InputBufAodHist[3]=I2S1InputBufAodHist[2];
	I2S1InputBufAodHist[2]=I2S1InputBufAodHist[1];
	I2S1InputBufAodHist[1]=I2S1InputBufAodHist[0];
	I2S1InputBufAodHist[0]=AodValue;

	for(int i=0;i<8;i++)
		Aod+=I2S1InputBufAodHist[i];

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
		AUDIOPLL0NUM_AdjustingValue=0.0f*0.1f+0.9f * AUDIOPLL0NUM_AdjustingValuePre;
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
const S16 Table_GenerateSinWav_S16[48+128]=    //cycle count number is 50 --- this is 16000/50=320Hz
{
		0	,
		4106.794064	,
		8148.821533	,
		12062.33722	,
		15785.62264	,
		19259.95936	,
		22430.55502	,
		25247.40743	,
		27666.09313	,
		29648.46803	,
		31163.26887	,
		32186.60634	,
		32702.34181	,
		32702.34181	,
		32186.60634	,
		31163.26887	,
		29648.46803	,
		27666.09314	,
		25247.40743	,
		22430.55502	,
		19259.95936	,
		15785.62264	,
		12062.33722	,
		8148.821535	,
		4106.794067	,
		2.94225E-06	,
		-4106.794061	,
		-8148.82153	,
		-12062.33721	,
		-15785.62264	,
		-19259.95936	,
		-22430.55502	,
		-25247.40742	,
		-27666.09313	,
		-29648.46803	,
		-31163.26887	,
		-32186.60634	,
		-32702.34181	,
		-32702.34181	,
		-32186.60635	,
		-31163.26887	,
		-29648.46803	,
		-27666.09314	,
		-25247.40743	,
		-22430.55502	,
		-19259.95937	,
		-15785.62264	,
		-12062.33722	,
		-8148.821538	,
		-4106.79407	,
		-5.88451E-06	,
		4106.794058	,
		8148.821527	,
		12062.33721	,
		15785.62263	,
		19259.95936	,
		22430.55502	,
		25247.40742	,
		27666.09313	,
		29648.46803	,
		31163.26887	,
		32186.60634	,
		32702.34181	,
		32702.34181	,
		32186.60635	,
		31163.26887	,
		29648.46803	,
		27666.09314	,
		25247.40743	,
		22430.55503	,
		19259.95937	,
		15785.62265	,
		12062.33723	,
		8148.821541	,
		4106.794073	,
		8.82676E-06	,
		-4106.794055	,
		-8148.821524	,
		-12062.33721	,
		-15785.62263	,
		-19259.95935	,
		-22430.55501	,
		-25247.40742	,
		-27666.09313	,
		-29648.46802	,
		-31163.26887	,
		-32186.60634	,
		-32702.34181	,
		-32702.34181	,
		-32186.60635	,
		-31163.26887	,
		-29648.46803	,
		-27666.09314	,
		-25247.40743	,
		-22430.55503	,
		-19259.95937	,
		-15785.62265	,
		-12062.33723	,
		-8148.821544	,
		-4106.794076	,
		-1.1769E-05	,
		4106.794052	,
		8148.821521	,
		12062.33721	,
		15785.62263	,
		19259.95935	,
		22430.55501	,
		25247.40742	,
		27666.09313	,
		29648.46802	,
		31163.26887	,
		32186.60634	,
		32702.34181	,
		32702.34181	,
		32186.60635	,
		31163.26887	,
		29648.46803	,
		27666.09314	,
		25247.40743	,
		22430.55503	,
		19259.95937	,
		15785.62265	,
		12062.33723	,
		8148.821547	,
		4106.794079	,
		1.47113E-05	,
		-4106.794049	,
		-8148.821518	,
		-12062.3372	,
		-15785.62263	,
		-19259.95935	,
		-22430.55501	,
		-25247.40742	,
		-27666.09313	,
		-29648.46802	,
		-31163.26886	,
		-32186.60634	,
		-32702.34181	,
		-32702.34181	,
		-32186.60635	,
		-31163.26887	,
		-29648.46804	,
		-27666.09314	,
		-25247.40744	,
		-22430.55503	,
		-19259.95938	,
		-15785.62265	,
		-12062.33723	,
		-8148.82155	,
		-4106.794082	,
		-1.76535E-05	,
		4106.794047	,
		8148.821515	,
		12062.3372	,
		15785.62262	,
		19259.95935	,
		22430.55501	,
		25247.40741	,
		27666.09312	,
		29648.46802	,
		31163.26886	,
		32186.60634	,
		32702.34181	,
		32702.34181	,
		32186.60635	,
		31163.26888	,
		29648.46804	,
		27666.09315	,
		25247.40744	,
		22430.55503	,
		19259.95938	,
		15785.62266	,
		12062.33724	,
		8148.821552	,
		4106.794085	,
		2.05957E-05	,
};
S16 *GenerateSinWav_Ptr_S16=(S16 *)(Table_GenerateSinWav_S16);
float Gain_GenerateSinWav_L=0.81f;
float Gain_GenerateSinWav_R=0.82f;

void GenerateSinWavFromTable_S32(S32 *DstPtrL,S32 *DstPtrR, int L)	//L should be <=128
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtrL++=(Gain_GenerateSinWav_L)*((*GenerateSinWav_Ptr_S16)<<16);
        *DstPtrR++=(Gain_GenerateSinWav_R)*((*GenerateSinWav_Ptr_S16++)<<16);//15);   //right side is half of left side
    }
    while(GenerateSinWav_Ptr_S16>=(Table_GenerateSinWav_S16+SinToneTable_CycLenthInSamples))
        GenerateSinWav_Ptr_S16-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S32_SingleCh(S32 *DstPtr, int L)	//L should be <=128
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtr++=(S32)(*GenerateSinWav_Ptr_S16++<<16);
    }
    while(GenerateSinWav_Ptr_S16>=(Table_GenerateSinWav_S16+SinToneTable_CycLenthInSamples))
        GenerateSinWav_Ptr_S16-=SinToneTable_CycLenthInSamples;
}

void GenerateSinWavFromTable_S16_LRMixed(S16 *DstPtr, int L)			//L should be <=128
{
    U16 i;
    for(i=0;i<L;i++)
    {
        //*DstPtr++=(Gain_GenerateSinWav_L)*(*GenerateSinWav_Ptr_S16);
        //*DstPtr++=(Gain_GenerateSinWav_R)*(*GenerateSinWav_Ptr_S16++);
        *DstPtr++=(*GenerateSinWav_Ptr_S16);
        *DstPtr++=(*GenerateSinWav_Ptr_S16++);
    }
    while(GenerateSinWav_Ptr_S16>=(Table_GenerateSinWav_S16+SinToneTable_CycLenthInSamples))
        GenerateSinWav_Ptr_S16-=SinToneTable_CycLenthInSamples;
}
void GenerateSinWavFromTable_S16_SingleCh(S16 *DstPtr, int L)		//L should be <=128
{
    U16 i;
    for(i=0;i<L;i++)
    {
        *DstPtr++=(Gain_GenerateSinWav_L)*(*GenerateSinWav_Ptr_S16++);
    }
    while(GenerateSinWav_Ptr_S16>=(Table_GenerateSinWav_S16+SinToneTable_CycLenthInSamples))
        GenerateSinWav_Ptr_S16-=SinToneTable_CycLenthInSamples;
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


