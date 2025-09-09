/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include <math.h>

#include "GlobalDef.h"
#include "Sweep.h"

//-------------------------------------sweep------------------------------------------
//---beg---
#if 1	//---folding
__attribute__((__section__(".dram0.data")))
TSineToneGenerator SineToneGenerator1;
__attribute__((__section__(".dram0.data")))
TSineToneGenerator SineToneGenerator2;

void InitSineToneGenerator(TSineToneGenerator *PtrSinGen, float A, float fBeg, float fEnd, float TimeInS, unsigned int Fs)
{
	PtrSinGen->CycleLengthInS=TimeInS;
	PtrSinGen->fBeg=fBeg;
	PtrSinGen->fEnd=fEnd;
	PtrSinGen->A=A;
	PtrSinGen->Fs=Fs;
	PtrSinGen->PhaseOfPrevFrameLastSample=0;
	PtrSinGen->FbegDividedByFs=fBeg/Fs;

	#ifdef SineToneGeneratorIsDouble
		PtrSinGen->k = exp(
			(log(PtrSinGen->fEnd / PtrSinGen->fBeg))
			/
			PtrSinGen->CycleLengthInS
		);
		PtrSinGen->p = 2.0 * (double)Pi_Value * PtrSinGen->fBeg / log(PtrSinGen->k);
	#else
		PtrSinGen->k = expf(
			(logf(PtrSinGen->fEnd / PtrSinGen->fBeg))
			/
			PtrSinGen->CycleLengthInS
		);
		PtrSinGen->p = 2.0f * (float)Pi_Value * PtrSinGen->fBeg / logf(PtrSinGen->k);
	#endif

	PtrSinGen->CurrentI=0;
	PtrSinGen->FinalI=TimeInS*Fs;
}

__attribute__((__section__(".iram.text")))
void GenerateSineToneSingleFreq(TSineToneGenerator* PtrSinGen, float* DstPtr, unsigned int SampleNumToGen, int PosNeg)
{
	int i;

	float a;
	int floorN;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	for (i = 0; i < SampleNumToGen; i++)
	{
		PtrSinGen->PhaseOfPrevFrameLastSample +=PtrSinGen->FbegDividedByFs;
		#ifdef SineToneGeneratorIsDouble
			*DstPtr++ = a*sin (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0 *(double)Pi_Value);
		#else
			*DstPtr++ = a*sinf (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);
		#endif
	}

	floorN=(int)PtrSinGen->PhaseOfPrevFrameLastSample;
	PtrSinGen->PhaseOfPrevFrameLastSample -= floorN;
}

__attribute__((__section__(".iram.text")))
void GenerateSineTone(TSineToneGenerator *PtrSinGen, float *DstPtr, unsigned int SampleNumToGen, int PosNeg)
{
	unsigned int i;

	float a;
	int floorN;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	if(PtrSinGen->CurrentI + SampleNumToGen > PtrSinGen->FinalI)
	{
		PtrSinGen->PhaseOfPrevFrameLastSample=0;
		PtrSinGen->CurrentI=0;
	}

	for (i = PtrSinGen->CurrentI; i < (PtrSinGen->CurrentI+SampleNumToGen); i++)
	{
		#ifdef SineToneGeneratorIsDouble
			PtrSinGen->tCurrent = (double)i / (double)PtrSinGen->Fs;
			PtrSinGen->fCurrent= pow(PtrSinGen->k,PtrSinGen->tCurrent);
			PtrSinGen->PhaseOfPrevFrameLastSample += PtrSinGen->fCurrent*PtrSinGen->FbegDividedByFs;
			*DstPtr++ = a*sin (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0 *(double)Pi_Value);
		#else
			PtrSinGen->tCurrent = (float)i / (float)PtrSinGen->Fs;
			PtrSinGen->fCurrent= powf(PtrSinGen->k,PtrSinGen->tCurrent);
			PtrSinGen->PhaseOfPrevFrameLastSample += PtrSinGen->fCurrent*PtrSinGen->FbegDividedByFs;
			*DstPtr++ = a*sinf (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);
		#endif
	}

	floorN=(int)PtrSinGen->PhaseOfPrevFrameLastSample;
	PtrSinGen->PhaseOfPrevFrameLastSample -= floorN;

	PtrSinGen->CurrentI+=SampleNumToGen;
}
#endif
//---end---
//-------------------------------------sweep------------------------------------------


//-------------------------------------SRC, 48KHz<-->16KHz------------------------------------------
//---beg---
TSrc48To16SingleCh Src48KHzTo16KHz_Mic0;
TSrc48To16SingleCh Src48KHzTo16KHz_Mic1;
TSrc48To16SingleCh Src48KHzTo16KHz_Mic2;
TSrc48To16SingleCh Src48KHzTo16KHz_Mic3;
TSrc48To16SingleCh Src48KHzTo16KHz_Mic4;
TSrc48To16SingleCh Src48KHzTo16KHz_Mic5;

TSrc48To16SingleCh Src48KHzTo16KHz_Spk;
TSrc48To16SingleCh Src48KHzTo16KHz_Ref;

TSrc48To16SingleCh Src16KHzTo48KHz_TxOut;
TSrc48To16SingleCh Src16KHzTo48KHz_RxOut;

void Src_48KHzTo16KHz(TSrc48To16SingleCh *Src48To16Ptr, float *DstPtr, float *SrcPtr, int L)
{

}
void Src_16KHzTo48KHz(TSrc16To48SingleCh *Src16To48Ptr, float *DstPtr, float *SrcPtr, int L)
{

}

void InitSrc48KHzTo16KHzAnd16KHzTo48KHz(void)
{

}

//---end---
//-------------------------------------SRC, 48KHz<-->16KHz------------------------------------------
