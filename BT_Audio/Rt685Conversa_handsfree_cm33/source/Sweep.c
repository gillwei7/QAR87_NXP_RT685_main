/*
 * Copyright 2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "arm_math.h"

#include "Sweep.h"


TSineToneGenerator SineToneGenerator1;
TSineToneGenerator SineToneGenerator2;

void InitSineToneGen1(void)
{
	//                                          Amplitude(),      FreqBeg,   FreqEnd,  Sweeping Duration,  Fs
	//                                        in between +/- 1      Hz          Hz            Sec          Hz
	InitSineToneGenerator(&SineToneGenerator1,     0.99f,          0.01f,     24000.0f,      20.0f,        48000);
}
void InitSineToneGen2(void)
{
	//                                          Amplitude(),      FreqBeg,   FreqEnd,  Sweeping Duration,  Fs
	//                                        in between +/- 1      Hz          Hz            Sec          Hz
	InitSineToneGenerator(&SineToneGenerator2,     0.10f,          1000.0f,    1000.0f,      20.0f,        48000);
}

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

void GenerateSineToneSingleFreq_S32_LRMixed(TSineToneGenerator* PtrSinGen, int* DstPtr, unsigned int SampleNumToGen, int PosNeg)
{
#if 0
	//In LPC55S69 prj, should never open this part
	//this doesn't use phase accumulated --- in double it is OK, in float, it's too noisy
	//if use this one, should only select double.
	//this part can NOT be used in LPC55S69, because with double, it uses toooo many MIPS for LPC55S69
	int i;

	float a;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	for (i = PtrSinGen->CurrentI; i < (PtrSinGen->CurrentI+SampleNumToGen); i++)
	{
		#ifdef SineToneGeneratorIsDouble
			*DstPtr++ = a*sin (2.0*(double)Pi_Value*i*  PtrSinGen->FbegDividedByFs);
			//*DstPtr++ = a*sin (2.0*Pi_Value*i*  PtrSinGen->fBeg/PtrSinGen->Fs);
		#else
			*DstPtr++ = a*sinf (2.0f*(float)Pi_Value*i*  PtrSinGen->FbegDividedByFs);
			//*DstPtr++ = a*sinf (2.0*Pi_Value*i*  PtrSinGen->fBeg/PtrSinGen->Fs);		//this is very noisy, cause the phase value is in float, and sine calculation is in float
		#endif
	}

	PtrSinGen->CurrentI+=SampleNumToGen;
#else
	//this part uses phase accumulation. Both double and float can work. When using double, it has the same noise quality as above,
	//when using float, it is 20dB (in higher freq) more noise than double. (-140dB VS -160dB)
	//this part can also be used in LPC55S69, when using float
	unsigned int i;

	#ifdef EnableLeftSawWave
	unsigned int j=0;
	#endif

	float a,s;
	int floorN;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	for (i = 0; i < SampleNumToGen; i++)
	{
		PtrSinGen->PhaseOfPrevFrameLastSample +=PtrSinGen->FbegDividedByFs;
		#ifdef SineToneGeneratorIsDouble
			s = a*sin (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0 *(double)Pi_Value);
		#else
			s = a*sinf (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);
			//s = a*arm_sin_f32 (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);		//using cmsis function, reduces cycles, but cause higher noise, should not using this here
																										//cause using sinf can already fit in MIPS
		#endif

		#ifdef EnableLeftSawWave
			*DstPtr++= j*0x100000;
			j++;
		#else
			*DstPtr++=(int)(s*_Pow2_31_M1_);
		#endif
		*DstPtr++=(int)(0-s*_Pow2_31_M1_);
	}

	floorN=(int)PtrSinGen->PhaseOfPrevFrameLastSample;
	PtrSinGen->PhaseOfPrevFrameLastSample -= floorN;
	//PtrSinGen->CurrentI+=SampleNumToGen;
#endif
}

void GenerateSineTone_S32_LRMixed(TSineToneGenerator *PtrSinGen, int *DstPtr, unsigned int SampleNumToGen, int PosNeg)
{
#if 0
	//In LPC55S69 prj, should never open this part
	//this doesn't use phase accumulated --- in double it is OK, in float, it's too noisy
	//if use this one, should only select double.
	//this part can NOT be used in LPC55S69, because with double, it uses toooo many MIPS for LPC55S69
	int i;

	float a;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	if(PtrSinGen->CurrentI + SampleNumToGen > PtrSinGen->FinalI)
		PtrSinGen->CurrentI=0;

	for (i = PtrSinGen->CurrentI; i < (PtrSinGen->CurrentI+SampleNumToGen); i++)
	{
		#ifdef SineToneGeneratorIsDouble
			PtrSinGen->tCurrent = (double)i / (double)PtrSinGen->Fs;
			PtrSinGen->fCurrent= pow(PtrSinGen->k,PtrSinGen->tCurrent);
			*DstPtr++ = a*sin (PtrSinGen->p * (PtrSinGen->fCurrent -1.0));
		#else
			PtrSinGen->tCurrent = (float)i / (float)PtrSinGen->Fs;
			PtrSinGen->fCurrent= powf(PtrSinGen->k,PtrSinGen->tCurrent);
			*DstPtr++ = a*sinf        (PtrSinGen->p * (PtrSinGen->fCurrent -1.0f));
		#endif
	}

	PtrSinGen->CurrentI+=SampleNumToGen;
#else
	//this part uses phase accumilated. Both double and float can work. When using double, it has the same noise quality as above,
	//when using float, it is 20dB (in higher freq) more noise than double. (-140dB VS -160dB)
	//this part can also be used in LPC55S69, when using float
	unsigned int i;

	#ifdef EnableLeftSawWave
	unsigned int j=0;
	#endif

	float a,s;
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
			s = a*sin (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0 *(double)Pi_Value);
		#else
			PtrSinGen->tCurrent = (float)i / (float)PtrSinGen->Fs;

			#if 0
				PtrSinGen->fCurrent= powFast(PtrSinGen->k,PtrSinGen->tCurrent);
			#endif
			#if 1
				PtrSinGen->fCurrent= powf(PtrSinGen->k,PtrSinGen->tCurrent);
			#endif

			PtrSinGen->PhaseOfPrevFrameLastSample += PtrSinGen->fCurrent*PtrSinGen->FbegDividedByFs;
			s = a*sinf (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);
			//s = a*arm_sin_f32 (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);		//using cmsis function, reduces cycles, but cause higher noise, should not using this here
																										//cause using sinf can already fit in MIPS
		#endif

		#ifdef EnableLeftSawWave
			*DstPtr++= j*0x100000;
			j++;
		#else
			*DstPtr++=(int)(s*_Pow2_31_M1_);
		#endif

		*DstPtr++=(int)(0-s*_Pow2_31_M1_);
	}

	floorN=(int)PtrSinGen->PhaseOfPrevFrameLastSample;
	PtrSinGen->PhaseOfPrevFrameLastSample -= floorN;

	PtrSinGen->CurrentI+=SampleNumToGen;
#endif
}

void GenerateSineToneSingleFreq_S16_LRMixed(TSineToneGenerator* PtrSinGen, short int* DstPtr, unsigned int SampleNumToGen, int PosNeg)
{
#if 0
	//In LPC55S69 prj, should never open this part
	//this doesn't use phase accumulated --- in double it is OK, in float, it's too noisy
	//if use this one, should only select double.
	//this part can NOT be used in LPC55S69, because with double, it uses toooo many MIPS for LPC55S69
	int i;

	float a;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	for (i = PtrSinGen->CurrentI; i < (PtrSinGen->CurrentI+SampleNumToGen); i++)
	{
		#ifdef SineToneGeneratorIsDouble
			*DstPtr++ = a*sin (2.0*(double)Pi_Value*i*  PtrSinGen->FbegDividedByFs);
			//*DstPtr++ = a*sin (2.0*Pi_Value*i*  PtrSinGen->fBeg/PtrSinGen->Fs);
		#else
			*DstPtr++ = a*sinf (2.0f*(float)Pi_Value*i*  PtrSinGen->FbegDividedByFs);
			//*DstPtr++ = a*sinf (2.0*Pi_Value*i*  PtrSinGen->fBeg/PtrSinGen->Fs);		//this is very noisy, cause the phase value is in float, and sine calculation is in float
		#endif
	}

	PtrSinGen->CurrentI+=SampleNumToGen;
#else
	//this part uses phase accumulation. Both double and float can work. When using double, it has the same noise quality as above,
	//when using float, it is 20dB (in higher freq) more noise than double. (-140dB VS -160dB)
	//this part can also be used in LPC55S69, when using float
	unsigned int i;

	#ifdef EnableLeftSawWave
	unsigned int j=0;
	#endif

	float a,s;
	int floorN;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	for (i = 0; i < SampleNumToGen; i++)
	{
		PtrSinGen->PhaseOfPrevFrameLastSample +=PtrSinGen->FbegDividedByFs;
		#ifdef SineToneGeneratorIsDouble
			s = a*sin (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0 *(double)Pi_Value);
		#else
			s = a*sinf (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);
			//s = a*arm_sin_f32 (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);		//using cmsis function, reduces cycles, but cause higher noise, should not using this here
																										//cause using sinf can already fit in MIPS
		#endif

		#ifdef EnableLeftSawWave
			*DstPtr++= j*0x100000;
			j++;
		#else
			*DstPtr++=(short int)(s*_Pow2_15_M1_);
		#endif
		*DstPtr++=(short int)(0-s*_Pow2_15_M1_);
	}

	floorN=(int)PtrSinGen->PhaseOfPrevFrameLastSample;
	PtrSinGen->PhaseOfPrevFrameLastSample -= floorN;
	//PtrSinGen->CurrentI+=SampleNumToGen;
#endif
}

void GenerateSineTone_S16_LRMixed(TSineToneGenerator *PtrSinGen, short int *DstPtr, unsigned int SampleNumToGen, int PosNeg)
{
#if 0
	//In LPC55S69 prj, should never open this part
	//this doesn't use phase accumulated --- in double it is OK, in float, it's too noisy
	//if use this one, should only select double.
	//this part can NOT be used in LPC55S69, because with double, it uses toooo many MIPS for LPC55S69
	int i;

	float a;

	if(PosNeg)
		a=PtrSinGen->A;
	else
		a=0-PtrSinGen->A;

	if(PtrSinGen->CurrentI + SampleNumToGen > PtrSinGen->FinalI)
		PtrSinGen->CurrentI=0;

	for (i = PtrSinGen->CurrentI; i < (PtrSinGen->CurrentI+SampleNumToGen); i++)
	{
		#ifdef SineToneGeneratorIsDouble
			PtrSinGen->tCurrent = (double)i / (double)PtrSinGen->Fs;
			PtrSinGen->fCurrent= pow(PtrSinGen->k,PtrSinGen->tCurrent);
			*DstPtr++ = a*sin (PtrSinGen->p * (PtrSinGen->fCurrent -1.0));
		#else
			PtrSinGen->tCurrent = (float)i / (float)PtrSinGen->Fs;
			PtrSinGen->fCurrent= powf(PtrSinGen->k,PtrSinGen->tCurrent);
			*DstPtr++ = a*sinf        (PtrSinGen->p * (PtrSinGen->fCurrent -1.0f));
		#endif
	}

	PtrSinGen->CurrentI+=SampleNumToGen;
#else
	//this part uses phase accumilated. Both double and float can work. When using double, it has the same noise quality as above,
	//when using float, it is 20dB (in higher freq) more noise than double. (-140dB VS -160dB)
	//this part can also be used in LPC55S69, when using float
	unsigned int i;

	#ifdef EnableLeftSawWave
	unsigned int j=0;
	#endif

	float a,s;
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
			s = a*sin (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0 *(double)Pi_Value);
		#else
			PtrSinGen->tCurrent = (float)i / (float)PtrSinGen->Fs;

			#if 0
				PtrSinGen->fCurrent= powFast(PtrSinGen->k,PtrSinGen->tCurrent);
			#endif
			#if 1
				PtrSinGen->fCurrent= powf(PtrSinGen->k,PtrSinGen->tCurrent);
			#endif

			PtrSinGen->PhaseOfPrevFrameLastSample += PtrSinGen->fCurrent*PtrSinGen->FbegDividedByFs;
			s = a*sinf (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);
			//s = a*arm_sin_f32 (PtrSinGen->PhaseOfPrevFrameLastSample * 2.0f *(float)Pi_Value);		//using cmsis function, reduces cycles, but cause higher noise, should not using this here
																										//cause using sinf can already fit in MIPS
		#endif

		#ifdef EnableLeftSawWave
			*DstPtr++= j*0x100000;
			j++;
		#else
			*DstPtr++=(short int)(s*_Pow2_15_M1_);
		#endif

		*DstPtr++=(short int)(0-s*_Pow2_15_M1_);
	}

	floorN=(int)PtrSinGen->PhaseOfPrevFrameLastSample;
	PtrSinGen->PhaseOfPrevFrameLastSample -= floorN;

	PtrSinGen->CurrentI+=SampleNumToGen;
#endif
}


