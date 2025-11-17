/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include <math.h>

#include <stdlib.h>
#include <string.h>

#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"

#include "fsl_debug_console.h"

#include "GlobalDef.h"
#include "MeterAndCompressor.h"
#include "SubFunc.h"


int DeInitRmsMeter(TRmsMeter *p)
{
    if(p==NULL) return RtnValue_Fail;
    void *pTmp;
    pTmp=p->HeapAddrForCheckingAtDeInit;
    free(p);

    if(GetCurrentHeapTail(sizeof(TRmsMeter))!=pTmp)
        return RtnValue_Fail;
    else
        return RtnValue_Success;
}
TRmsMeter *InitRmsMeter(int FrameSZ, int CfgIdx)
{
    void *HeapBeforeInit;
    TRmsMeter *p;
    TRmsMeter_Internal *pInternal;

    HeapBeforeInit=GetCurrentHeapTail(sizeof(TRmsMeter));

    p=malloc(sizeof(TRmsMeter));
    if(p==NULL)
        return NULL;

    pInternal=malloc(sizeof(TAudioCompressor_Internal));
    if(pInternal==NULL)
    {
        free(p);
        return NULL;
    }
    p->InstancePrivate=(void *)pInternal;

    p->HeapAddrForCheckingAtDeInit=HeapBeforeInit;        //backup the heap address before allocate the space for TDafx4y_Dummy2_Internal
    pInternal->FrmSize=FrameSZ;
    switch(CfgIdx)
    {
        case 1:
            pInternal->MeterFrmCnt=0;
            pInternal->MeterRmsCalcTotalFrmNum=3;
            pInternal->MeterRmsAccValue=0;
            pInternal->MeterRmsValue=0;
            #ifdef EnableAudioDbgInfo
                pInternal->SaveDbgInfoPtr_InputSignalRmsValue=NULL;     //set to somewhere, when really needed
            #endif
            break;
        default:
            pInternal->MeterFrmCnt=0;
            pInternal->MeterRmsCalcTotalFrmNum=40;
            pInternal->MeterRmsAccValue=0;
            pInternal->MeterRmsValue=0;
            #ifdef EnableAudioDbgInfo
                p->SaveDbgInfoPtr_InputSignalRmsValue=NULL;     //set to somewhere, when really needed
            #endif
            break;
    }
    return p;
}
int DeInitAudioCompressor(TAudioCompressor *p)
{
    if(p==NULL) return RtnValue_Fail;
    TAudioCompressor_Internal *pInternal;
    pInternal=(TAudioCompressor_Internal *)p->InstancePrivate;

    void *pTmp;
    pTmp=p->HeapAddrForCheckingAtDeInit;
    free(pInternal->CompressorGainArray);
    free(pInternal);
    free(p);

    if(GetCurrentHeapTail(sizeof(TAudioCompressor))!=pTmp)
        return RtnValue_Fail;
    else
        return RtnValue_Success;
}
TAudioCompressor *InitAudioCompressor(int FrameSZ, int CfgIdx)
{
    //int CfgIdx is not used yet

    void *HeapBeforeInit;
    TAudioCompressor *p;
    TAudioCompressor_Internal *pInternal;

    HeapBeforeInit=GetCurrentHeapTail(sizeof(TAudioCompressor));

    p=malloc(sizeof(TAudioCompressor));
    if(p==NULL)
        return NULL;

    pInternal=malloc(sizeof(TAudioCompressor_Internal));
    if(pInternal==NULL)
    {
        free(p);
        return NULL;
    }
    p->InstancePrivate=(void *)pInternal;


    pInternal->CompressorGainArray=malloc(sizeof(float)*FrameSZ);
    if(pInternal->CompressorGainArray==NULL)
    {
        free(p);
        free(pInternal);
        return NULL;
    }
    pInternal=(TAudioCompressor_Internal *)p->InstancePrivate;

    p->HeapAddrForCheckingAtDeInit=HeapBeforeInit;        //backup the heap address before allocate the space for TDafx4y_Dummy2_Internal
    pInternal->FrmSize=FrameSZ;
    memset(pInternal->CompressorGainArray,0,sizeof(float)*FrameSZ);

    switch(CfgIdx)
    {
        case 1:
            //full scale --- level up to 0.999, 0dB
            pInternal->HasBeenCalledInOneFrame=0;
            pInternal->CompressorThrRms=0.707f;//0.707/2;       //0.707f is 0dBFs sin wave's RMS
            pInternal->CompressorRatio =12.0f;   //14.0f;         //14 makes the compressor almost like a limiter
            pInternal->CompressorGainPre=0.0f;
            pInternal->CompressorGainTarget=1.0f;
            pInternal->GenerateSmoothGain_CurrentGain=0.0f;
            pInternal->CompressorRms=0.0f;
            pInternal->CompressorFrmCnt=0;
            pInternal->CompressorRmsCalcLengthInFrm=20;         //acc x*x for 20 frames, and then update the RMS
            //pInternal->SmoothAlfa_Value=0.994f;
            #ifdef EnableAudioDbgInfo
                pInternal->SaveDbgInfoPtr_InputSignalRmsValue=NULL;     //set to somewhere, when really needed
                pInternal->SaveDbgInfoPtr_TargetGain=NULL;              //set to somewhere, when really needed
            #endif
            break;
        case 2:
            //0.8 full scale --- level up to 0.8, 0dB
            pInternal->HasBeenCalledInOneFrame=0;
            pInternal->CompressorThrRms=0.707f*0.8f*0.8f;//0.707/2;       //0.707f is 0dBFs sin wave's RMS
            pInternal->CompressorRatio =12.0f;   //14.0f;         //14 makes the compressor almost like a limiter
            pInternal->CompressorGainPre=0.0f;
            pInternal->CompressorGainTarget=1.0f;
            pInternal->GenerateSmoothGain_CurrentGain=0.0f;
            pInternal->CompressorRms=0.0f;
            pInternal->CompressorFrmCnt=0;
            pInternal->CompressorRmsCalcLengthInFrm=20;         //acc x*x for 20 frames, and then update the RMS
            //pInternal->SmoothAlfa_Value=0.994f;
            #ifdef EnableAudioDbgInfo
                pInternal->SaveDbgInfoPtr_InputSignalRmsValue=NULL;     //set to somewhere, when really needed
                pInternal->SaveDbgInfoPtr_TargetGain=NULL;              //set to somewhere, when really needed
            #endif
            break;
        case 3:
            //half scale --- level up to 0.5, 0dB
            pInternal->HasBeenCalledInOneFrame=0;
            pInternal->CompressorThrRms=0.707f*0.5f*0.5f;//0.707/2;       //0.707f is 0dBFs sin wave's RMS
            pInternal->CompressorRatio =12.0f;   //14.0f;         //14 makes the compressor almost like a limiter
            pInternal->CompressorGainPre=0.0f;
            pInternal->CompressorGainTarget=1.0f;
            pInternal->GenerateSmoothGain_CurrentGain=0.0f;
            pInternal->CompressorRms=0.0f;
            pInternal->CompressorFrmCnt=0;
            pInternal->CompressorRmsCalcLengthInFrm=20;         //acc x*x for 20 frames, and then update the RMS
            //pInternal->SmoothAlfa_Value=0.994f;
            #ifdef EnableAudioDbgInfo
                pInternal->SaveDbgInfoPtr_InputSignalRmsValue=NULL;     //set to somewhere, when really needed
                pInternal->SaveDbgInfoPtr_TargetGain=NULL;              //set to somewhere, when really needed
            #endif
            break;
        default:
            pInternal->HasBeenCalledInOneFrame=0;
            pInternal->CompressorThrRms=0.707f/2;//0.707;       //0.707f is 0dBFs sin wave's RMS
            pInternal->CompressorRatio =12.0f;   //14.0f;         //14 makes the compressor almost like a limiter
            pInternal->CompressorGainPre=0.0f;
            pInternal->CompressorGainTarget=1.0f;
            pInternal->GenerateSmoothGain_CurrentGain=0.0f;
            pInternal->CompressorRms=0.0f;
            pInternal->CompressorFrmCnt=0;
            pInternal->CompressorRmsCalcLengthInFrm=30;
            //pInternal->SmoothAlfa_Value=0.900f;
            #ifdef EnableAudioDbgInfo
                pInternal->SaveDbgInfoPtr_InputSignalRmsValue=NULL;     //set to somewhere, when really needed
                pInternal->SaveDbgInfoPtr_TargetGain=NULL;              //set to somewhere, when really needed
            #endif
            break;
    }
    return p;
}



#define SmoothAlfa_Value	(0.99f)
__attribute__((__section__(".iram.text")))
void GenerateGainArray(float TargetG, float *DstPtr, float *GenerateSmoothGain_CurrentGain, int l)
{
	int i;
	float CurGain;
	for (i = 0; i < l; i++)
	{
		CurGain = SmoothAlfa_Value* *GenerateSmoothGain_CurrentGain + (1.0f - SmoothAlfa_Value)*TargetG;
		*GenerateSmoothGain_CurrentGain = CurGain;
		*DstPtr++ = *GenerateSmoothGain_CurrentGain;
	}
}

__attribute__((__section__(".iram.text")))
void AudioCompressorProcOneFrame(TAudioCompressor *CmpPtr, float *InPtr,float *OtPtr)
{
    TAudioCompressor_Internal *pInternal;
    if(!CmpPtr)
        return;
    pInternal=(TAudioCompressor_Internal *)CmpPtr->InstancePrivate;

    //pInternal->CompressorRms+=Asm_VecASqrAccToOneValue_4FloatSamplesBlocked(InPtr,(U32)pInternal->FrmSize/4);
    pInternal->CompressorRms+=vec_powerf(InPtr,(U32)pInternal->FrmSize);
    //for(i=0;i<AudioCompressorAudioFrameSize;i++)
    //    Compressor1Rms+=(*(InPtr+i))*(*(InPtr+i));

    if(!(pInternal->CompressorFrmCnt%pInternal->CompressorRmsCalcLengthInFrm))
    {
        //now AccuSqr is ready
        pInternal->CompressorRms/=(float)(pInternal->CompressorRmsCalcLengthInFrm*(U32)pInternal->FrmSize);
        //pInternal->CompressorRms=sqrt(pInternal->CompressorRms);
        //arm_sqrt_f32(pInternal->CompressorRms,&pInternal->CompressorRms);
        pInternal->CompressorRms=sqrtf(pInternal->CompressorRms);

        #ifdef EnableAudioDbgInfo
            *pInternal->SaveDbgInfoPtr_InputSignalRmsValue++=pInternal->CompressorRms;
        #endif

        if(pInternal->CompressorRms>pInternal->CompressorThrRms)
        {
            float x=pInternal->CompressorThrRms/pInternal->CompressorRms;
            float a=1-1/pInternal->CompressorRatio;

            //pInternal->CompressorGainTarget=pow(x,a);        //this is 10810 cycles
            //pInternal->CompressorGainTarget=powFast(x,a);      //this is   300 cycles
            pInternal->CompressorGainTarget=powf(x,a);      //this is   300 cycles
        }else
            pInternal->CompressorGainTarget=1.0;

        #ifdef EnableAudioDbgInfo
            *pInternal->SaveDbgInfoPtr_TargetGain++=pInternal->CompressorGainTarget;
        #endif


        pInternal->CompressorRmsKeep = pInternal->CompressorRms;
        pInternal->CompressorRms = 0;
    }

    //GenerateGainArray(Compressor1GainTarget, Compressor1GainArray,AudioCompressorAudioFrameSize);
    //GenerateGainArray_AsmFast_4FloatSamplesBlocked(pInternal->CompressorGainTarget, pInternal->CompressorGainArray, &pInternal->GenerateSmoothGain_CurrentGain, AudioFrameSizeInSamplePerCh/4);
    //Asm_GenerateSoftChangeCoefArray_4ValuesBlocked(1.0f-pInternal->SmoothAlfa_Value,pInternal->SmoothAlfa_Value,pInternal->CompressorGainTarget,&pInternal->GenerateSmoothGain_CurrentGain,pInternal->CompressorGainArray,pInternal->FrmSize/4);
    GenerateGainArray(pInternal->CompressorGainTarget, pInternal->CompressorGainArray, &pInternal->GenerateSmoothGain_CurrentGain, pInternal->FrmSize);

    #ifndef DisalbeAudioCompressor
        for(int i=0;i<pInternal->FrmSize;i++)
            *OtPtr++ = (*InPtr++)*pInternal->CompressorGainArray[i];
        //OneFrameAudioGainProc_4FloatSamplesBlocked(InPtr,pInternal->CompressorGainArray,OtPtr,pInternal->FrmSize/4);
    #endif

    pInternal->CompressorFrmCnt++;
    pInternal->HasBeenCalledInOneFrame=1;
}


__attribute__((__section__(".iram.text")))
float GetSignaLvlIndB_ByFrameRms(TRmsMeter *MeterPtr, float *InPtr)
{
    TRmsMeter_Internal *pInternal;
    if(!MeterPtr)
        return -10000.0f;
    pInternal=(TRmsMeter_Internal *)MeterPtr->InstancePrivate;

    //pInternal->MeterRmsAccValue+=Asm_VecASqrAccToOneValue_4FloatSamplesBlocked(InPtr,pInternal->FrmSize/4);
    pInternal->MeterRmsAccValue+=vec_powerf(InPtr,(U32)pInternal->FrmSize);


    if(!(pInternal->MeterFrmCnt%pInternal->MeterRmsCalcTotalFrmNum))
    {
        //now AccuSqr is ready
        pInternal->MeterRmsValue=pInternal->MeterRmsAccValue/(pInternal->MeterRmsCalcTotalFrmNum*pInternal->FrmSize);
        //arm_sqrt_f32(pInternal->MeterRmsValue,&pInternal->MeterRmsValue);

        #ifdef EnableAudioDbgInfo
            *pInternal->SaveDbgInfoPtr_InputSignalRmsValue++=pInternal->MeterRmsValue;
        #endif

        pInternal->MeterRmsAccValue = 0;
    }

    pInternal->MeterFrmCnt++;

    float dBValue;
    if(pInternal->MeterRmsValue < 1e-19)
        return -190.0f;

    //pInternal->MeterRmsValue = 1e-19;     //this is a test to check if fastlog10 works well when input value is very small --- it is good
    //dBValue=10.0f*fastlog10(pInternal->MeterRmsValue)+3.0f;
    dBValue=10.0f*log10f(pInternal->MeterRmsValue)+3.0f;

    return dBValue;
}

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//Note: audio meter is from other referencing

int DeInitAudioMeter(TAudioMeter *p)
{
    if(p==NULL) return RtnValue_Fail;
    void *pTmp;
    pTmp=p->HeapAddrForCheckingAtDeInit;
    free(p);

    if(GetCurrentHeapTail(sizeof(TAudioMeter))!=pTmp)
        return RtnValue_Fail;
    else
        return RtnValue_Success;
}
TAudioMeter *InitAudioMeter(int FrameSZ, int CfgIdx)
{
    void *HeapBeforeInit;
    TAudioMeter *p;
    TAudioMeter_Internal *pInternal;

    HeapBeforeInit=GetCurrentHeapTail(sizeof(TAudioMeter));

    p=malloc(sizeof(TAudioMeter));
    if(p==NULL)
        return NULL;

    pInternal=malloc(sizeof(TAudioMeter_Internal));
    if(pInternal==NULL)
    {
        free(p);
        return NULL;
    }
    memset(pInternal,0,sizeof(TAudioMeter_Internal));
    p->InstancePrivate=(void *)pInternal;

    p->HeapAddrForCheckingAtDeInit=HeapBeforeInit;        //backup the heap address before allocate the space for TDafx4y_Dummy2_Internal
    pInternal->FrmSize=FrameSZ;
    switch(CfgIdx)
    {
        case 1:
            pInternal->tav=0.1f;
            pInternal->at=0.3f;
            pInternal->rt=0.3f;
            pInternal->CalcLengthInFrm=3;
            break;
        default:
            pInternal->tav=0.01f;
            pInternal->at=0.03f;
            pInternal->rt=0.03f;
            pInternal->CalcLengthInFrm=20;
            break;
    }
    return p;
}

__attribute__((__section__(".iram.text")))
float GetPeakMeterValue(TAudioMeter *p, float *SrcPtr)
{
    if(p==NULL) return 0;
    TAudioMeter_Internal *pInternal;
    pInternal=(TAudioMeter_Internal *)p->InstancePrivate;

    float MeterPeakAccValue=0;

    //calculater sum of abs(x) in one frame
    //MeterPeakAccValue=Asm_VecAAccToOneValue_4FloatSamplesBlocked(SrcPtr,pInternal->FrmSize/4);
    for(int i=0;i<pInternal->FrmSize;i++)
    	MeterPeakAccValue+=*SrcPtr++;

    pInternal->MeterAccValue+=fabsf(MeterPeakAccValue);

    if(!(pInternal->FrmCnt%pInternal->CalcLengthInFrm))
    {
        float PeakValue;
        float coeff;

        //now Acc of all needed frames is ready
        pInternal->MeterAccValue=pInternal->MeterAccValue/(pInternal->CalcLengthInFrm);

        if (pInternal->MeterAccValue>pInternal->GetPeakMeterLogValue_Internal)
            coeff = pInternal->at;
        else
            coeff = pInternal->rt;

        PeakValue = (1 - coeff) * pInternal->GetPeakMeterLogValue_Internal + coeff * pInternal->MeterAccValue;
        pInternal->GetPeakMeterLogValue_Internal = PeakValue;

        pInternal->MeterAccValue = 0;
    }
    pInternal->FrmCnt++;
    if(pInternal->GetPeakMeterLogValue_Internal < 1e-9)
    {
        return -180.0f;
    }else
    {
		//return pInternal->GetPeakMeterLogValue_Internal;
		return (-12.0f + 20.0f * log10f(pInternal->GetPeakMeterLogValue_Internal));	//return value 0dB when input is a 0dBFs sine wave
    }
}

__attribute__((__section__(".iram.text")))
float GetRmsMeterLogValue(TAudioMeter *p, float *SrcPtr)
{
    if(p==NULL) return 0;
    TAudioMeter_Internal *pInternal;
    pInternal=(TAudioMeter_Internal *)p->InstancePrivate;

    float MeterRmsAccValue;

    //calculater sum of x*x in one frame
    //MeterRmsAccValue=Asm_VecASqrAccToOneValue_4FloatSamplesBlocked(SrcPtr,pInternal->FrmSize/4);
    MeterRmsAccValue=vec_powerf(SrcPtr,(U32)pInternal->FrmSize);

    pInternal->MeterAccValue+=MeterRmsAccValue;

    if(!(pInternal->FrmCnt%pInternal->CalcLengthInFrm))
    {
        float RmsValue;

        //now SquareAcc of all needed frames is ready
        pInternal->MeterAccValue=pInternal->MeterAccValue/(pInternal->CalcLengthInFrm * pInternal->FrmSize);

        #if 1
            RmsValue = (pInternal->tav*(pInternal->MeterAccValue - pInternal->GetRmsMeterLogValue_Internal)) + pInternal->GetRmsMeterLogValue_Internal;
            pInternal->GetRmsMeterLogValue_Internal = RmsValue;
        #else
            RmsValue=pInternal->MeterAccValue;
        #endif

        pInternal->MeterAccValue = 0;
    }
    pInternal->FrmCnt++;

    if(pInternal->GetRmsMeterLogValue_Internal < 1e-19)
    {
        return -190.0f;
    }else
    {
		//return 10.0f * fastlog10(pInternal->GetRmsMeterLogValue_Internal);	//return value -3dB when input is a 0dBFs sine wave
		return (10.0f * log10f(pInternal->GetRmsMeterLogValue_Internal));	//return value 0dB when input is a 0dBFs sine wave
    }
}

/*
Note: 	TRmsMeter works well.
		TAudioMeter not working well,  parameter needs to be tuned.
		TAudioCompressor not checked yet.
*/


TRmsMeter *AudioRmsMeter1;
TRmsMeter *AudioRmsMeter2;
TRmsMeter *AudioRmsMeter3;
TRmsMeter *AudioRmsMeter4;
TRmsMeter *AudioRmsMeter5;
TRmsMeter *AudioRmsMeter6;
TRmsMeter *AudioRmsMeter7;
TRmsMeter *AudioRmsMeter8;

TAudioMeter *AudioMeter1;
TAudioMeter *AudioMeter2;
TAudioMeter *AudioMeter3;
TAudioMeter *AudioMeter4;

TAudioCompressor *AudioCompressor1;
TAudioCompressor *AudioCompressor2;
TAudioCompressor *AudioCompressor3;
TAudioCompressor *AudioCompressor4;
TAudioCompressor *AudioCompressor5;
TAudioCompressor *AudioCompressor6;
TAudioCompressor *AudioCompressor7;
TAudioCompressor *AudioCompressor8;

//------------------------------------------------------------------------------------------------------------------
void InitMeterAndCompressor(void)
{
	//for Mic0~1 --- accurate enough
	AudioRmsMeter1=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter1==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 1 \r\n");
	AudioRmsMeter2=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter2==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 2 \r\n");
	AudioRmsMeter3=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter3==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 1 \r\n");
	AudioRmsMeter4=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter4==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 2 \r\n");

	//for conversa output signals --- accurate enough
	AudioRmsMeter5=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter5==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 1 \r\n");
	AudioRmsMeter6=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter6==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 2 \r\n");
	AudioRmsMeter7=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter7==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 1 \r\n");
	AudioRmsMeter8=InitRmsMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioRmsMeter8==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 2 \r\n");

	//for conversa output signals --- not accurate enough, not used --- calculating result still needs to be adjusted
	AudioMeter1=InitAudioMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioMeter1==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 3 \r\n");
	AudioMeter2=InitAudioMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioMeter2==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 4 \r\n");
	AudioMeter3=InitAudioMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioMeter3==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 3 \r\n");
	AudioMeter4=InitAudioMeter(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioMeter4==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 4 \r\n");


	//not used for now
	AudioCompressor1=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor1==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 5 \r\n");
	AudioCompressor2=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor2==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 6 \r\n");

	AudioCompressor3=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor3==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 7 \r\n");
	AudioCompressor4=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor4==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 8 \r\n");

	AudioCompressor5=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor5==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 7 \r\n");
	AudioCompressor6=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor6==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 8 \r\n");

	AudioCompressor7=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor7==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 7 \r\n");
	AudioCompressor8=InitAudioCompressor(AudioFrameSizeInSamplePerCh_16KHz,1);
	if(AudioCompressor8==NULL)
		PRINTF("DSP: InitMeterAndCompressor fail 8 \r\n");

/*
	void AudioCompressorProcOneFrame(TAudioCompressor *CmpPtr, float *InPtr,float *OtPtr);
	float GetSignaLvlIndB_ByFrameRms(TRmsMeter *MeterPtr, float *InPtr);
	float GetPeakMeterValue(TAudioMeter *p, float *SrcPtr);
	float GetRmsMeterLogValue(TAudioMeter *p, float *SrcPtr);
*/

}
