/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __Sweep_h_INCLUDED
#define __Sweep_h_INCLUDED

#include "GlobalDef.h"

//-------------------------------------sweep------------------------------------------
//---beg---

#define SineToneGeneratorIsDouble

typedef struct
{
	float	A,fBeg,fEnd;
	float	CycleLengthInS;
	unsigned int		Fs;
	unsigned int		CurrentI;
	unsigned int		FinalI;
	#ifdef SineToneGeneratorIsDouble
		double	k, p, fCurrent;
		double	tCurrent;
		double  PhaseOfPrevFrameLastSample;
	#else
		float	k, p, fCurrent;
		float	tCurrent;
		float   PhaseOfPrevFrameLastSample;
	#endif
	float	FbegDividedByFs;
}TSineToneGenerator;

extern TSineToneGenerator SineToneGenerator1,SineToneGenerator2;
extern void InitSineToneGenerator(TSineToneGenerator *PtrSinGen, float A, float fBeg, float fEnd, float TimeInS, unsigned int Fs);
extern void GenerateSineTone          (TSineToneGenerator *PtrSinGen, float *DstPtr, unsigned int SampleNumToGen, int PosNeg);
extern void GenerateSineToneSingleFreq(TSineToneGenerator* PtrSinGen, float* DstPtr, unsigned int SampleNumToGen, int PosNeg);
//---end---
//-------------------------------------sweep------------------------------------------


//-------------------------------------SRC, 48KHz<-->16KHz------------------------------------------
//---beg---

typedef struct
{
	U32 LengthInSamples;
	S32 *PtrBufHead;
	S32 *PtrRd;
	S32 *PtrWr;
} TSrc16To48SingleCh;

typedef struct
{
	U32 LengthInSamples;
	S32 *PtrBufHead;
	S32 *PtrRd;
	S32 *PtrWr;
} TSrc48To16SingleCh;


extern TSrc48To16SingleCh Src48KHzTo16KHz_Mic0;
extern TSrc48To16SingleCh Src48KHzTo16KHz_Mic1;
extern TSrc48To16SingleCh Src48KHzTo16KHz_Mic2;
extern TSrc48To16SingleCh Src48KHzTo16KHz_Mic3;
extern TSrc48To16SingleCh Src48KHzTo16KHz_Mic4;
extern TSrc48To16SingleCh Src48KHzTo16KHz_Mic5;

extern TSrc48To16SingleCh Src48KHzTo16KHz_Spk;
extern TSrc48To16SingleCh Src48KHzTo16KHz_Ref;

extern TSrc48To16SingleCh Src16KHzTo48KHz_TxOut;
extern TSrc48To16SingleCh Src16KHzTo48KHz_RxOut;

extern void InitSrc48KHzTo16KHzAnd16KHzTo48KHz(void);
extern void Src_48KHzTo16KHz(TSrc48To16SingleCh *Src48To16Ptr, float *DstPtr, float *SrcPtr, int L);
extern void Src_16KHzTo48KHz(TSrc16To48SingleCh *Src16To48Ptr, float *DstPtr, float *SrcPtr, int L);
//---end---
//-------------------------------------SRC, 48KHz<-->16KHz------------------------------------------



#ifdef __cplusplus
}
#endif
#endif
