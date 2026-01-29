/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#define _Pow2_31_M1_   	2147483647
#define _Pow2_15_M1_   	32767

//In LPC55S69 prj, should never enable double here, LPC55S69 is not fast enough


//#define EnableLeftSawWave
//#define SineToneGeneratorIsDouble

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



extern TSineToneGenerator SineToneGenerator1;
extern TSineToneGenerator SineToneGenerator2;

extern void InitSineToneGen1(void);
extern void InitSineToneGen2(void);

extern void InitSineToneGenerator(TSineToneGenerator *PtrSinGen, float A, float fBeg, float fEnd, float TimeInS, unsigned int Fs);
extern void GenerateSineTone_S32_LRMixed(TSineToneGenerator *PtrSinGen, int *DstPtr, unsigned int SampleNumToGen, int PosNeg);
extern void GenerateSineToneSingleFreq_S32_LRMixed(TSineToneGenerator* PtrSinGen, int* DstPtr, unsigned int SampleNumToGen, int PosNeg);

extern void GenerateSineTone_S32_SingleCh(TSineToneGenerator *PtrSinGen, int *DstPtr, unsigned int SampleNumToGen, int PosNeg);
extern void GenerateSineToneSingleFreq_S32_SingleCh(TSineToneGenerator* PtrSinGen, int* DstPtr, unsigned int SampleNumToGen, int PosNeg);

extern void GenerateSineTone_S16_LRMixed(TSineToneGenerator *PtrSinGen, short int *DstPtr, unsigned int SampleNumToGen, int PosNeg);
extern void GenerateSineToneSingleFreq_S16_LRMixed(TSineToneGenerator* PtrSinGen, short int* DstPtr, unsigned int SampleNumToGen, int PosNeg);

