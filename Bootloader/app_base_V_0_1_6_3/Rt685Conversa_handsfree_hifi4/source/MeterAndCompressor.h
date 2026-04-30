/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __MeterAndCompressor_h_INCLUDED
#define __MeterAndCompressor_h_INCLUDED

#include "GlobalDef.h"


typedef struct
{
    void *HeapAddrForCheckingAtDeInit;
    void *InstancePrivate;
}TAudioMeter;

typedef struct
{
    void *HeapAddrForCheckingAtDeInit;
    void *InstancePrivate;
}TAudioCompressor;

typedef struct
{
    void *HeapAddrForCheckingAtDeInit;
    void *InstancePrivate;
}TRmsMeter;


//#define EnableAudioDbgInfo
typedef struct
{
    U32 FrmSize;
    U32 MeterFrmCnt;
    U32 MeterRmsCalcTotalFrmNum;
    float MeterRmsAccValue;     //temp
    float MeterRmsValue;        //final, square rooted
#ifdef EnableAudioDbgInfo
    float *SaveDbgInfoPtr_InputSignalRmsValue;
#endif
}TRmsMeter_Internal;

typedef struct
{
    U32 FrmSize;
    U32 HasBeenCalledInOneFrame;
    U32 CompressorFrmCnt;
    U32 CompressorRmsCalcLengthInFrm;
    float *CompressorGainArray;
    float CompressorGainPre;
    float CompressorGainTarget;
    float CompressorRms;
    float CompressorRmsKeep;
    float CompressorThrRms;
    float CompressorRatio;
    float GenerateSmoothGain_CurrentGain;
    //float SmoothAlfa_Value;
#ifdef EnableAudioDbgInfo
    float *SaveDbgInfoPtr_InputSignalRmsValue;
    float *SaveDbgInfoPtr_TargetGain;
#endif
}TAudioCompressor_Internal;

typedef struct
{
    U32 FrmCnt;
    U32 FrmSize;
    U32 CalcLengthInFrm;
    float MeterAccValue;
    float GetPeakMeterLogValue_Internal;
    float GetRmsMeterLogValue_Internal;
    float tav;
    float at;
    float rt;
}TAudioMeter_Internal;



#include <math.h>

#include <stdlib.h>
#include <string.h>

#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"

#include "GlobalDef.h"
#include "MeterAndCompressor.h"
#include "SubFunc.h"


extern TRmsMeter *AudioRmsMeter1;
extern TRmsMeter *AudioRmsMeter2;
extern TRmsMeter *AudioRmsMeter3;
extern TRmsMeter *AudioRmsMeter4;
extern TRmsMeter *AudioRmsMeter5;
extern TRmsMeter *AudioRmsMeter6;
extern TRmsMeter *AudioRmsMeter7;
extern TRmsMeter *AudioRmsMeter8;

extern TAudioMeter *AudioMeter1;
extern TAudioMeter *AudioMeter2;
extern TAudioMeter *AudioMeter3;
extern TAudioMeter *AudioMeter4;

extern TAudioCompressor *AudioCompressor1;
extern TAudioCompressor *AudioCompressor2;
extern TAudioCompressor *AudioCompressor3;
extern TAudioCompressor *AudioCompressor4;
extern TAudioCompressor *AudioCompressor5;
extern TAudioCompressor *AudioCompressor6;
extern TAudioCompressor *AudioCompressor7;
extern TAudioCompressor *AudioCompressor8;


extern void InitMeterAndCompressor(void);

extern int DeInitRmsMeter(TRmsMeter *p);
extern TRmsMeter *InitRmsMeter(int FrameSZ, int CfgIdx);
extern int DeInitAudioCompressor(TAudioCompressor *p);
extern TAudioCompressor *InitAudioCompressor(int FrameSZ, int CfgIdx);
extern void AudioCompressorProcOneFrame(TAudioCompressor *CmpPtr, float *InPtr,float *OtPtr);
extern float GetSignaLvlIndB_ByFrameRms(TRmsMeter *MeterPtr, float *InPtr);

extern int DeInitAudioMeter(TAudioMeter *p);
extern TAudioMeter *InitAudioMeter(int FrameSZ, int CfgIdx);
extern float GetPeakMeterValue(TAudioMeter *p, float *SrcPtr);
extern float GetRmsMeterLogValue(TAudioMeter *p, float *SrcPtr);


#endif
