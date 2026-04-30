/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <string.h>



#include "GlobalDef.h"

#include "CircularBufManagement.h"




//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
#if defined(EnableCirBufFunctionsForS8)&&(EnableCirBufFunctionsForS8==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
void InitCirAudioBuf_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr,S8 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S8
void CirAudioBuf_WriteSamples_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBePut, S8 *PtrAudioDataSrc)
{
	if(!SampleNumbersToBePut)
		return;
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBePut)
	{   //no need to cut
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, SampleNumbersToBePut * sizeof(S8));

		if (CirBufPtr->PtrWr + SampleNumbersToBePut > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrWr = CirBufPtr->PtrWr + SampleNumbersToBePut - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrWr += SampleNumbersToBePut;
	} else
	{   //need to cut into 2 parts
		U32 l1;
		U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBePut - l1;
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, l1 * sizeof(S8));

		PtrAudioDataSrc += l1;
		memcpy(CirBufPtr->PtrBufHead, PtrAudioDataSrc, l2 * sizeof(S8));
		CirBufPtr->PtrWr = CirBufPtr->PtrBufHead + l2;
	}
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S8
S8* CirAudioBuf_ReadSamples_GetRdPtr_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBeGot)
{
	S8 *ptrR;
	if(!SampleNumbersToBeGot)
		return NULL;

	//make it clear that: before calling this function, it must have been confirmed that there are enough data (>SampleNumbersToBeGot) available after the read pointer
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
		ptrR = CirBufPtr->PtrRd;
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return ptrR;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;
		memcpy(CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples + 1, CirBufPtr->PtrBufHead, l2 * sizeof(S8));
		ptrR = CirBufPtr->PtrRd;
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return ptrR;
	}
}

U32 CirAudioBuf_SpaceOccupiedInSamples_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr)
{
	//buf is completely empty
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (0);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->PtrWr - CirBufPtr->PtrRd);
	} else {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrWr) + 1);
	}
}
U32 CirAudioBuf_SpaceAvailableInSamples_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr)
{
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (CirBufPtr->LengthInSamples);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrRd));
	} else
	{
		return ((CirBufPtr->PtrRd - CirBufPtr->PtrWr) - 1);
	}
}
void CirAudioBuf_ClearAllSamples_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

U8 CirAudioBuf_GetUsagePercentage_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr)
{
	U32 a, b;
	a = CirAudioBuf_SpaceOccupiedInSamples_S8(CirBufPtr);
	b = CirBufPtr->LengthInSamples;
	return (a * 100 / b);
}
#endif      //EnableCirBufFunctionsForS8
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----






//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- beg----
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- beg----
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- beg----
#if defined(EnableCirBufFunctionsForS16)&&(EnableCirBufFunctionsForS16==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
__attribute__((section("CodeQuickAccess")))
void InitCirAudioBuf_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr,S16 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S16
__attribute__((section("CodeQuickAccess")))
void CirAudioBuf_WriteSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBePut, S16 *PtrAudioDataSrc)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBePut)
	{   //no need to cut
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, SampleNumbersToBePut * sizeof(S16));

		if (CirBufPtr->PtrWr + SampleNumbersToBePut > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrWr = CirBufPtr->PtrWr + SampleNumbersToBePut - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrWr += SampleNumbersToBePut;
	} else
	{   //need to cut into 2 parts
		U32 l1;
		U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBePut - l1;
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, l1 * sizeof(S16));

		PtrAudioDataSrc += l1;
		memcpy(CirBufPtr->PtrBufHead, PtrAudioDataSrc, l2 * sizeof(S16));
		CirBufPtr->PtrWr = CirBufPtr->PtrBufHead + l2;
	}
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S16
__attribute__((section("CodeQuickAccess")))
S16* CirAudioBuf_ReadSamples_GetRdPtr_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot)
{
	S16 *ptrR;
	//make it clear that: before calling this function, it must have been confirmed that there are enough data (>SampleNumbersToBeGot) available after the read pointer
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
		ptrR = CirBufPtr->PtrRd;
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return ptrR;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;
		memcpy(CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples + 1, CirBufPtr->PtrBufHead, l2 * sizeof(S16));
		ptrR = CirBufPtr->PtrRd;
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return ptrR;
	}
}

__attribute__((section("CodeQuickAccess")))
U32 CirAudioBuf_SpaceOccupiedInSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr)
{
	//buf is completely empty
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (0);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->PtrWr - CirBufPtr->PtrRd);
	} else {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrWr) + 1);
	}
}
__attribute__((section("CodeQuickAccess")))
U32 CirAudioBuf_SpaceAvailableInSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr)
{
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (CirBufPtr->LengthInSamples);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrRd));
	} else
	{
		return ((CirBufPtr->PtrRd - CirBufPtr->PtrWr) - 1);
	}
}
__attribute__((section("CodeQuickAccess")))
void CirAudioBuf_ClearAllSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S16
__attribute__((section("CodeQuickAccess")))
void CirAudioBuf_ReadSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot, S16 *PtrAudioDataDst)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,SampleNumbersToBeGot*sizeof(S16));
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;

        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,     l1*sizeof(S16));
        PtrAudioDataDst+=l1;
        memcpy(PtrAudioDataDst,CirBufPtr->PtrBufHead,l2*sizeof(S16));
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return;
	}
}
#endif      //EnableCirBufFunctionsForS16
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- end----
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- end----
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- end----








//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- beg----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- beg----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- beg----
#if defined(EnableCirBufFunctionsForS32)&&(EnableCirBufFunctionsForS32==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
void InitCirAudioBuf_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr,S32 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S32
void CirAudioBuf_WriteSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBePut, S32 *PtrAudioDataSrc)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBePut)
	{   //no need to cut
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, SampleNumbersToBePut * sizeof(S32));

		if (CirBufPtr->PtrWr + SampleNumbersToBePut > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrWr = CirBufPtr->PtrWr + SampleNumbersToBePut - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrWr += SampleNumbersToBePut;
	} else
	{   //need to cut into 2 parts
		U32 l1;
		U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBePut - l1;
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, l1 * sizeof(S32));

		PtrAudioDataSrc += l1;
		memcpy(CirBufPtr->PtrBufHead, PtrAudioDataSrc, l2 * sizeof(S32));
		CirBufPtr->PtrWr = CirBufPtr->PtrBufHead + l2;
	}
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S32
S32* CirAudioBuf_ReadSamples_GetRdPtr_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot)
{
	S32 *ptrR;
	//make it clear that: before calling this function, it must have been confirmed that there are enough data (>SampleNumbersToBeGot) available after the read pointer
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
		ptrR = CirBufPtr->PtrRd;
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return ptrR;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;
		memcpy(CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples + 1, CirBufPtr->PtrBufHead, l2 * sizeof(S32));
		ptrR = CirBufPtr->PtrRd;
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return ptrR;
	}
}

U32 CirAudioBuf_SpaceOccupiedInSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr)
{
	//buf is completely empty
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (0);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->PtrWr - CirBufPtr->PtrRd);
	} else {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrWr) + 1);
	}
}
U32 CirAudioBuf_SpaceAvailableInSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr)
{
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (CirBufPtr->LengthInSamples);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrRd));
	} else
	{
		return ((CirBufPtr->PtrRd - CirBufPtr->PtrWr) - 1);
	}
}
void CirAudioBuf_ClearAllSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S32
void CirAudioBuf_ReadSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot, S32 *PtrAudioDataDst)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,SampleNumbersToBeGot*sizeof(S32));
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;

        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,     l1*sizeof(S32));
        PtrAudioDataDst+=l1;
        memcpy(PtrAudioDataDst,CirBufPtr->PtrBufHead,l2*sizeof(S32));
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return;
	}
}

#endif      //EnableCirBufFunctionsForS32
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- end----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- end----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- end----








//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- beg----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- beg----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- beg----
#if defined(EnableCirBufFunctionsForS64)&&(EnableCirBufFunctionsForS64==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
void InitCirAudioBuf_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr,S64 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S64
void CirAudioBuf_WriteSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBePut, S64 *PtrAudioDataSrc)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBePut)
	{   //no need to cut
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, SampleNumbersToBePut * sizeof(S64));

		if (CirBufPtr->PtrWr + SampleNumbersToBePut > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrWr = CirBufPtr->PtrWr + SampleNumbersToBePut - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrWr += SampleNumbersToBePut;
	} else
	{   //need to cut into 2 parts
		U32 l1;
		U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBePut - l1;
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, l1 * sizeof(S64));

		PtrAudioDataSrc += l1;
		memcpy(CirBufPtr->PtrBufHead, PtrAudioDataSrc, l2 * sizeof(S64));
		CirBufPtr->PtrWr = CirBufPtr->PtrBufHead + l2;
	}
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S64
S64* CirAudioBuf_ReadSamples_GetRdPtr_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot)
{
	S64 *ptrR;
	//make it clear that: before calling this function, it must have been confirmed that there are enough data (>SampleNumbersToBeGot) available after the read pointer
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
		ptrR = CirBufPtr->PtrRd;
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return ptrR;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;
		memcpy(CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples + 1, CirBufPtr->PtrBufHead, l2 * sizeof(S64));
		ptrR = CirBufPtr->PtrRd;
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return ptrR;
	}
}
void CirAudioBuf_ReadSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot, S64 *PtrAudioDataDst)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,SampleNumbersToBeGot*sizeof(S64));
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;

        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,     l1*sizeof(S64));
        PtrAudioDataDst+=l1;
        memcpy(PtrAudioDataDst,CirBufPtr->PtrBufHead,l2*sizeof(S64));
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return;
	}
}

U32 CirAudioBuf_SpaceOccupiedInSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr)
{
	//buf is completely empty
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (0);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->PtrWr - CirBufPtr->PtrRd);
	} else {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrWr) + 1);
	}
}
U32 CirAudioBuf_SpaceAvailableInSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr)
{
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (CirBufPtr->LengthInSamples);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd)
	{
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrRd));
	} else
	{
		return ((CirBufPtr->PtrRd - CirBufPtr->PtrWr) - 1);
	}
}
void CirAudioBuf_ClearAllSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

#endif      //EnableCirBufFunctionsForS64
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- end----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- end----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- end----





//----------------------circular buffer management functions for UAC Up multiple ch type ------------------- beg----
//----------------------circular buffer management functions for UAC Up multiple ch type ------------------- beg----
//----------------------circular buffer management functions for UAC Up multiple ch type ------------------- beg----
#if defined(EnableCirBufFunctionsForUacUpStreamMultiCh)&&(EnableCirBufFunctionsForUacUpStreamMultiCh==1)
__attribute__((section("CodeQuickAccess")))
void InitCirUacUpAudioBuf_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr,T_MCh32BitUacUpAudioSample *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}
__attribute__((section("CodeQuickAccess")))
void CirUacUpAudioBuf_WriteSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBePut, T_MCh32BitUacUpAudioSample *PtrAudioDataSrc)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBePut) {   //no need to cut
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, SampleNumbersToBePut * sizeof(T_MCh32BitUacUpAudioSample));

		if (CirBufPtr->PtrWr + SampleNumbersToBePut > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrWr = CirBufPtr->PtrWr + SampleNumbersToBePut - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrWr += SampleNumbersToBePut;
	} else {   //need to cut into 2 parts
	    S32 l1;
	    S32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBePut - l1;
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, l1 * sizeof(T_MCh32BitUacUpAudioSample));

		PtrAudioDataSrc += l1;
		memcpy(CirBufPtr->PtrBufHead, PtrAudioDataSrc, l2 * sizeof(T_MCh32BitUacUpAudioSample));
		CirBufPtr->PtrWr = CirBufPtr->PtrBufHead + l2;
	}
}
__attribute__((section("CodeQuickAccess")))
T_MCh32BitUacUpAudioSample* CirUacUpAudioBuf_ReadSamples_GetRdPtr_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBeGot)
{
    T_MCh32BitUacUpAudioSample *ptrR;
	//make it clear that: before calling this function, it must have been confirmed that there are enough data (>SampleNumbersToBeGot) available after the read pointer
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot) {   //no need to cut
		ptrR = CirBufPtr->PtrRd;
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return ptrR;
	} else {   //need to merge 2 parts together
	    S32 l1;
	    S32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;
		memcpy(CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples + 1, CirBufPtr->PtrBufHead, l2 * sizeof(T_MCh32BitUacUpAudioSample));
		ptrR = CirBufPtr->PtrRd;
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return ptrR;
	}
}
__attribute__((section("CodeQuickAccess")))
U32 CirUacUpAudioBuf_SpaceOccupiedInSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr)
{
	//buf is completely empty
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (0);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd) {
		return (CirBufPtr->PtrWr - CirBufPtr->PtrRd);
	} else {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrWr) + 1);
	}
}
__attribute__((section("CodeQuickAccess")))
U32 CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr)
{
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (CirBufPtr->LengthInSamples);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd) {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrRd));
	} else {
		return ((CirBufPtr->PtrRd - CirBufPtr->PtrWr) - 1);
	}
}
__attribute__((section("CodeQuickAccess")))
void CirUacUpAudioBuf_ClearAllSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr)
    {
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}
#endif
//----------------------circular buffer management functions for UAC Up multiple ch type ------------------- end----
//----------------------circular buffer management functions for UAC Up multiple ch type ------------------- end----
//----------------------circular buffer management functions for UAC Up multiple ch type ------------------- end----




//----------------------circular buffer management functions for UAC Dn multiple ch type ------------------- beg----
//----------------------circular buffer management functions for UAC Dn multiple ch type ------------------- beg----
//----------------------circular buffer management functions for UAC Dn multiple ch type ------------------- beg----
#if defined(EnableCirBufFunctionsForUacDnStreamMultiCh)&&(EnableCirBufFunctionsForUacDnStreamMultiCh==1)
__attribute__((section("CodeQuickAccess")))
void InitCirUacDnAudioBuf_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr,T_MCh32BitUacDnAudioSample *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}
__attribute__((section("CodeQuickAccess")))
void CirUacDnAudioBuf_WriteSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBePut, T_MCh32BitUacDnAudioSample *PtrAudioDataSrc)
{
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBePut) {   //no need to cut
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, SampleNumbersToBePut * sizeof(T_MCh32BitUacDnAudioSample));

		if (CirBufPtr->PtrWr + SampleNumbersToBePut > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrWr = CirBufPtr->PtrWr + SampleNumbersToBePut - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrWr += SampleNumbersToBePut;
	} else {   //need to cut into 2 parts
	    S32 l1;
	    S32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBePut - l1;
		memcpy(CirBufPtr->PtrWr, PtrAudioDataSrc, l1 * sizeof(T_MCh32BitUacDnAudioSample));

		PtrAudioDataSrc += l1;
		memcpy(CirBufPtr->PtrBufHead, PtrAudioDataSrc, l2 * sizeof(T_MCh32BitUacDnAudioSample));
		CirBufPtr->PtrWr = CirBufPtr->PtrBufHead + l2;
	}
}
__attribute__((section("CodeQuickAccess")))
T_MCh32BitUacDnAudioSample* CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBeGot)
{
    T_MCh32BitUacDnAudioSample *ptrR;
	//make it clear that: before calling this function, it must have been confirmed that there are enough data (>SampleNumbersToBeGot) available after the read pointer
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot) {   //no need to cut
		ptrR = CirBufPtr->PtrRd;
		if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;
		return ptrR;
	} else {   //need to merge 2 parts together
	    S32 l1;
	    S32 l2;

		l1 = CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;
		memcpy(CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples + 1, CirBufPtr->PtrBufHead, l2 * sizeof(T_MCh32BitUacDnAudioSample));
		ptrR = CirBufPtr->PtrRd;
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return ptrR;
	}
}
__attribute__((section("CodeQuickAccess")))
U32 CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr)
{
	//buf is completely empty
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (0);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd) {
		return (CirBufPtr->PtrWr - CirBufPtr->PtrRd);
	} else {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrWr) + 1);
	}
}
__attribute__((section("CodeQuickAccess")))
U32 CirUacDnAudioBuf_SpaceAvailableInSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr)
{
	if (CirBufPtr->PtrRd == CirBufPtr->PtrWr)
		return (CirBufPtr->LengthInSamples);

	if (CirBufPtr->PtrWr > CirBufPtr->PtrRd) {
		return (CirBufPtr->LengthInSamples - (CirBufPtr->PtrWr - CirBufPtr->PtrRd));
	} else {
		return ((CirBufPtr->PtrRd - CirBufPtr->PtrWr) - 1);
	}
}
__attribute__((section("CodeQuickAccess")))
void CirUacDnAudioBuf_ClearAllSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr)
    {
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}
#endif
//----------------------circular buffer management functions for UAC Dn multiple ch type ------------------- end----
//----------------------circular buffer management functions for UAC Dn multiple ch type ------------------- end----
//----------------------circular buffer management functions for UAC Dn multiple ch type ------------------- end----

