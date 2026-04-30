/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifdef XOS_PRESENT
	#include <xtensa/xos.h>
#endif

#include <string.h>

#include "fsl_debug_console.h"

#include "CircularBufManagement.h"



//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
//--------------------------circular buffer management functions for s8 1ch --------------------- beg----
#if defined(EnableCirBufFunctionsForS8)&&(EnableCirBufFunctionsForS8==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
void InitCirAudioBuf_S8(T_CircularAudioBuf_S8 *CirBufPtr,S8 *DataAreaHead,U32 BufLen)
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

void CirAudioBuf_ReadSamples_S8(T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBeGot, S8 *PtrAudioDataDst)
{
	if(!SampleNumbersToBeGot)
		return;
	if ((CirBufPtr->LengthInSamples - (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,SampleNumbersToBeGot*sizeof(S8));
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

        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,     l1*sizeof(S8));
        PtrAudioDataDst+=l1;
        memcpy(PtrAudioDataDst,CirBufPtr->PtrBufHead,l2*sizeof(S8));
		CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + l2;
		return;
	}
}

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S8
S8* CirAudioBuf_ReadSamples_GetRdPtr_S8(T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBeGot)
{
	S8 *ptrR;
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

U32 CirAudioBuf_SpaceOccupiedInSamples_S8(T_CircularAudioBuf_S8 *CirBufPtr)
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
void CirAudioBuf_ClearAllSamples_S8(T_CircularAudioBuf_S8 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
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
void InitCirAudioBuf_S16(T_CircularAudioBuf_S16 *CirBufPtr,S16 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S16
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
S16* CirAudioBuf_ReadSamples_GetRdPtr_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot)
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

U32 CirAudioBuf_SpaceOccupiedInSamples_S16(T_CircularAudioBuf_S16 *CirBufPtr)
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
void CirAudioBuf_ClearAllSamples_S16(T_CircularAudioBuf_S16 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

#if defined(EnableLessUsedCirBufFunctions)&&(EnableLessUsedCirBufFunctions==1)
//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S16
void CirAudioBuf_ReadSamples_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot, S16 *PtrAudioDataDst)
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

//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S16
void CirAudioBuf_ReadSamplesBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot, S16 *PtrAudioDataDst)
{
	//update read pointer to the newest sample written
	CirBufPtr->PtrRd = CirBufPtr->PtrWr;

	if (((CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1) >= SampleNumbersToBeGot)
	{   //no need to cut
		CirBufPtr->PtrRd -=SampleNumbersToBeGot;
		memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,SampleNumbersToBeGot*sizeof(S16));
		/*if (CirBufPtr->PtrRd + SampleNumbersToBeGot > CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
			CirBufPtr->PtrRd = CirBufPtr->PtrRd + SampleNumbersToBeGot - CirBufPtr->LengthInSamples - 1;
		else
			CirBufPtr->PtrRd += SampleNumbersToBeGot;*/
		return;
	} else
	{   //need to merge 2 parts together
	    U32 l1;
	    U32 l2;

		l1 = (CirBufPtr->PtrRd - CirBufPtr->PtrBufHead) + 1;
		l2 = SampleNumbersToBeGot - l1;

        memcpy(PtrAudioDataDst,CirBufPtr->PtrBufHead,     l1*sizeof(S16));
        PtrAudioDataDst+=l1;

        CirBufPtr->PtrRd = CirBufPtr->PtrBufHead + (CirBufPtr->LengthInSamples) - l2;
        memcpy(PtrAudioDataDst,CirBufPtr->PtrRd,l2*sizeof(S16));
		CirBufPtr->PtrRd += l2;
		return;
	}
}

U8 CirAudioBuf_GetUsagePercentage_S16(T_CircularAudioBuf_S16 *CirBufPtr)
{
	U32 a, b;
	a = CirAudioBuf_SpaceOccupiedInSamples_S16(CirBufPtr);
	b = CirBufPtr->LengthInSamples;
	return (a * 100 / b);
}

void CirAudioBuf_MoveRdPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		CirBufPtr->PtrRd += NumOfSamplesToGoForward;
		CirBufPtr->PtrRd -= CirBufPtr->LengthInSamples;
	} else
	{
		CirBufPtr->PtrRd += NumOfSamplesToGoForward;
	}
}
S16* CirAudioBuf_GetDdPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	S16 *p;
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		p = CirBufPtr->PtrRd + NumOfSamplesToGoForward;
		p -= CirBufPtr->LengthInSamples;
	} else
	{
		p = CirBufPtr->PtrRd + NumOfSamplesToGoForward;
	}
	return(p);
}
void CirAudioBuf_MoveRdPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		CirBufPtr->PtrRd -= NumOfSamplesToGoBack;
	} else
	{
		CirBufPtr->PtrRd -= NumOfSamplesToGoBack;
		CirBufPtr->PtrRd += (CirBufPtr->LengthInSamples + 1);
	}
}
S16* CirAudioBuf_GetRdPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	S16 *p;
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		p=CirBufPtr->PtrRd - NumOfSamplesToGoBack;
	} else
	{
		p=CirBufPtr->PtrRd - NumOfSamplesToGoBack;
		p += (CirBufPtr->LengthInSamples + 1);
	}
	return(p);
}

void CirAudioBuf_MoveWrPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		CirBufPtr->PtrWr += NumOfSamplesToGoForward;
		CirBufPtr->PtrWr -= CirBufPtr->LengthInSamples;
	} else
	{
		CirBufPtr->PtrWr += NumOfSamplesToGoForward;
	}
}
S16* CirAudioBuf_GetWrPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	S16 *p;
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		p = CirBufPtr->PtrWr + NumOfSamplesToGoForward;
		p -= CirBufPtr->LengthInSamples;
	} else
	{
		p = CirBufPtr->PtrWr + NumOfSamplesToGoForward;
	}
	return(p);
}
void CirAudioBuf_MoveWrPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		CirBufPtr->PtrWr -= NumOfSamplesToGoBack;
	} else
	{
		CirBufPtr->PtrWr -= NumOfSamplesToGoBack;
		CirBufPtr->PtrWr += (CirBufPtr->LengthInSamples + 1);
	}
}
S16* CirAudioBuf_GetWrPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	S16 *p;
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		p=CirBufPtr->PtrWr - NumOfSamplesToGoBack;
	} else
	{
		p=CirBufPtr->PtrWr - NumOfSamplesToGoBack;
		p += (CirBufPtr->LengthInSamples + 1);
	}
	return(p);
}
#endif      //EnableLessUsedCirBufFunctions
#endif      //EnableCirBufFunctionsForS16
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- end----
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- end----
//--------------------------circular buffer management functions for s16 1ch, or S8 2ch --------------------- end----








//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- beg----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- beg----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- beg----
#if defined(EnableCirBufFunctionsForS32)&&(EnableCirBufFunctionsForS32==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
void InitCirAudioBuf_S32(T_CircularAudioBuf_S32 *CirBufPtr,S32 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S32
void CirAudioBuf_WriteSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBePut, S32 *PtrAudioDataSrc)
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
S32* CirAudioBuf_ReadSamples_GetRdPtr_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot)
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

U32 CirAudioBuf_SpaceOccupiedInSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr)
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
U32 CirAudioBuf_SpaceAvailableInSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr)
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
void CirAudioBuf_ClearAllSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

U8 CirAudioBuf_GetUsagePercentage_S32(T_CircularAudioBuf_S32 *CirBufPtr)
{
	U32 a, b;
	a = CirAudioBuf_SpaceOccupiedInSamples_S32(CirBufPtr);
	b = CirBufPtr->LengthInSamples;
	return (a * 100 / b);
}

#if defined(EnableLessUsedCirBufFunctions)&&(EnableLessUsedCirBufFunctions==1)
//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S32
void CirAudioBuf_ReadSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot, S32 *PtrAudioDataDst)
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

U8 CirAudioBuf_GetUsagePercentage_S32(T_CircularAudioBuf_S32 *CirBufPtr)
{
	U32 a, b;
	a = CirAudioBuf_SpaceOccupiedInSamples_S32(CirBufPtr);
	b = CirBufPtr->LengthInSamples;
	return (a * 100 / b);
}

void CirAudioBuf_MoveRdPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		CirBufPtr->PtrRd += NumOfSamplesToGoForward;
		CirBufPtr->PtrRd -= CirBufPtr->LengthInSamples;
	} else
	{
		CirBufPtr->PtrRd += NumOfSamplesToGoForward;
	}
}
S32* CirAudioBuf_GetDdPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	S32 *p;
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		p = CirBufPtr->PtrRd + NumOfSamplesToGoForward;
		p -= CirBufPtr->LengthInSamples;
	} else
	{
		p = CirBufPtr->PtrRd + NumOfSamplesToGoForward;
	}
	return(p);
}
void CirAudioBuf_MoveRdPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		CirBufPtr->PtrRd -= NumOfSamplesToGoBack;
	} else
	{
		CirBufPtr->PtrRd -= NumOfSamplesToGoBack;
		CirBufPtr->PtrRd += (CirBufPtr->LengthInSamples + 1);
	}
}
S32* CirAudioBuf_GetRdPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	S32 *p;
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		p=CirBufPtr->PtrRd - NumOfSamplesToGoBack;
	} else
	{
		p=CirBufPtr->PtrRd - NumOfSamplesToGoBack;
		p += (CirBufPtr->LengthInSamples + 1);
	}
	return(p);
}

void CirAudioBuf_MoveWrPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		CirBufPtr->PtrWr += NumOfSamplesToGoForward;
		CirBufPtr->PtrWr -= CirBufPtr->LengthInSamples;
	} else
	{
		CirBufPtr->PtrWr += NumOfSamplesToGoForward;
	}
}
S32* CirAudioBuf_GetWrPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	S32 *p;
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		p = CirBufPtr->PtrWr + NumOfSamplesToGoForward;
		p -= CirBufPtr->LengthInSamples;
	} else
	{
		p = CirBufPtr->PtrWr + NumOfSamplesToGoForward;
	}
	return(p);
}
void CirAudioBuf_MoveWrPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		CirBufPtr->PtrWr -= NumOfSamplesToGoBack;
	} else
	{
		CirBufPtr->PtrWr -= NumOfSamplesToGoBack;
		CirBufPtr->PtrWr += (CirBufPtr->LengthInSamples + 1);
	}
}
S32* CirAudioBuf_GetWrPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	S32 *p;
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		p=CirBufPtr->PtrWr - NumOfSamplesToGoBack;
	} else
	{
		p=CirBufPtr->PtrWr - NumOfSamplesToGoBack;
		p += (CirBufPtr->LengthInSamples + 1);
	}
	return(p);
}
#endif      //EnableLessUsedCirBufFunctions
#endif      //EnableCirBufFunctionsForS32
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- end----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- end----
//-------------------circular buffer management functions for S32 type, could be s16 2ch, or S32 1ch -------- end----








//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- beg----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- beg----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- beg----
#if defined(EnableCirBufFunctionsForS64)&&(EnableCirBufFunctionsForS64==1)
//it is critical to understand that, DataAreaHead must points to a place with space >= (BufLen + MaxReadLen)
void InitCirAudioBuf_S64(T_CircularAudioBuf_S64 *CirBufPtr,S64 *DataAreaHead,U32 BufLen)
{
    CirBufPtr->PtrBufHead=DataAreaHead;
    CirBufPtr->LengthInSamples=BufLen;
    CirBufPtr->PtrRd=DataAreaHead;
    CirBufPtr->PtrWr=DataAreaHead;
}

//this function doesn't check if there are enough space available. Before calling this function, should call CirAudioBuf_SpaceAvailableInSamples_S64
void CirAudioBuf_WriteSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBePut, S64 *PtrAudioDataSrc)
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
S64* CirAudioBuf_ReadSamples_GetRdPtr_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot)
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

U32 CirAudioBuf_SpaceOccupiedInSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr)
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
U32 CirAudioBuf_SpaceAvailableInSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr)
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
void CirAudioBuf_ClearAllSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr)
{
	CirBufPtr->PtrRd = CirBufPtr->PtrBufHead;
	CirBufPtr->PtrWr = CirBufPtr->PtrBufHead;
}

#if defined(EnableLessUsedCirBufFunctions)&&(EnableLessUsedCirBufFunctions==1)
//this function doesn't check if there are enough samples available. Before calling this function, should call CirAudioBuf_SpaceOccupiedInSamples_S64
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

U8 CirAudioBuf_GetUsagePercentage_S64(T_CircularAudioBuf_S64 *CirBufPtr)
{
	U32 a, b;
	a = CirAudioBuf_SpaceOccupiedInSamples_S64(CirBufPtr);
	b = CirBufPtr->LengthInSamples;
	return (a * 100 / b);
}

void CirAudioBuf_MoveRdPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		CirBufPtr->PtrRd += NumOfSamplesToGoForward;
		CirBufPtr->PtrRd -= CirBufPtr->LengthInSamples;
	} else
	{
		CirBufPtr->PtrRd += NumOfSamplesToGoForward;
	}
}
S64* CirAudioBuf_GetDdPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	S64 *p;
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		p = CirBufPtr->PtrRd + NumOfSamplesToGoForward;
		p -= CirBufPtr->LengthInSamples;
	} else
	{
		p = CirBufPtr->PtrRd + NumOfSamplesToGoForward;
	}
	return(p);
}
void CirAudioBuf_MoveRdPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		CirBufPtr->PtrRd -= NumOfSamplesToGoBack;
	} else
	{
		CirBufPtr->PtrRd -= NumOfSamplesToGoBack;
		CirBufPtr->PtrRd += (CirBufPtr->LengthInSamples + 1);
	}
}
S64* CirAudioBuf_GetRdPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	S64 *p;
	if ((CirBufPtr->PtrRd - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		p=CirBufPtr->PtrRd - NumOfSamplesToGoBack;
	} else
	{
		p=CirBufPtr->PtrRd - NumOfSamplesToGoBack;
		p += (CirBufPtr->LengthInSamples + 1);
	}
	return(p);
}

void CirAudioBuf_MoveWrPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		CirBufPtr->PtrWr += NumOfSamplesToGoForward;
		CirBufPtr->PtrWr -= CirBufPtr->LengthInSamples;
	} else
	{
		CirBufPtr->PtrWr += NumOfSamplesToGoForward;
	}
}
S64* CirAudioBuf_GetWrPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward)
{
	S64 *p;
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoForward) >= CirBufPtr->PtrBufHead + CirBufPtr->LengthInSamples)
	{
		p = CirBufPtr->PtrWr + NumOfSamplesToGoForward;
		p -= CirBufPtr->LengthInSamples;
	} else
	{
		p = CirBufPtr->PtrWr + NumOfSamplesToGoForward;
	}
	return(p);
}
void CirAudioBuf_MoveWrPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		CirBufPtr->PtrWr -= NumOfSamplesToGoBack;
	} else
	{
		CirBufPtr->PtrWr -= NumOfSamplesToGoBack;
		CirBufPtr->PtrWr += (CirBufPtr->LengthInSamples + 1);
	}
}
S64* CirAudioBuf_GetWrPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack)
{
	S64 *p;
	if ((CirBufPtr->PtrWr - NumOfSamplesToGoBack) >= CirBufPtr->PtrBufHead)
	{
		p=CirBufPtr->PtrWr - NumOfSamplesToGoBack;
	} else
	{
		p=CirBufPtr->PtrWr - NumOfSamplesToGoBack;
		p += (CirBufPtr->LengthInSamples + 1);
	}
	return(p);
}
#endif      //EnableLessUsedCirBufFunctions
#endif      //EnableCirBufFunctionsForS64
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- end----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- end----
//------------------------circular buffer management functions for s64 1ch, or S32 2ch --------------------- end----




#ifdef EnableTestCirBufManagement

S64 TmpBufForTestingCirBuf_S64[100+9];
S32 TmpBufForTestingCirBuf_S32[100+21];
S16 TmpBufForTestingCirBuf_S16[100+18];

T_CircularAudioBuf_S16 CirBuf1_S16;
T_CircularAudioBuf_S32 CirBuf1_S32;
T_CircularAudioBuf_S64 CirBuf1_S64;

S16 CirBufTestDataForWr_S16[50];
S16 CirBufTestDataForRd_S16[50];
S32 CirBufTestDataForWr_S32[50];
S32 CirBufTestDataForRd_S32[50];
S64 CirBufTestDataForWr_S64[50];
S64 CirBufTestDataForRd_S64[50];
void TestCirBufManagement(void)
{
    int i,j,k;
    S32 *TestRdPtr;

    //test S16 circular buffer
    InitCirAudioBuf_S16(&CirBuf1_S16, TmpBufForTestingCirBuf_S16, 20);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%10;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S16[i]=i+10;
        CirAudioBuf_WriteSamples_S16(&CirBuf1_S16,L,CirBufTestDataForWr_S16);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S16(&CirBuf1_S16, L);
        memcpy(CirBufTestDataForRd_S16,TestRdPtr,i*sizeof(S16));
        if(memcmp(CirBufTestDataForWr_S16,CirBufTestDataForRd_S16,i*sizeof(S16))) while(1){};
    }
    
    InitCirAudioBuf_S16(&CirBuf1_S16, TmpBufForTestingCirBuf_S16, 29);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S16[i]=i+10;
        CirAudioBuf_WriteSamples_S16(&CirBuf1_S16,L,CirBufTestDataForWr_S16);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S16(&CirBuf1_S16, L);
        memcpy(CirBufTestDataForRd_S16,TestRdPtr,i*sizeof(S16));
        if(memcmp(CirBufTestDataForWr_S16,CirBufTestDataForRd_S16,i*sizeof(S16))) while(1){};
    }
    
    
    InitCirAudioBuf_S16(&CirBuf1_S16, TmpBufForTestingCirBuf_S16, 49);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S16[i]=i+10;
        CirAudioBuf_WriteSamples_S16(&CirBuf1_S16,L,CirBufTestDataForWr_S16);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S16(&CirBuf1_S16, L);
        memcpy(CirBufTestDataForRd_S16,TestRdPtr,i*sizeof(S16));
        if(memcmp(CirBufTestDataForWr_S16,CirBufTestDataForRd_S16,i*sizeof(S16))) while(1){};
    }
    

    InitCirAudioBuf_S16(&CirBuf1_S16, TmpBufForTestingCirBuf_S16, 20);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%10;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S16[i]=i+10;
        CirAudioBuf_WriteSamples_S16(&CirBuf1_S16,L,CirBufTestDataForWr_S16);
        CirAudioBuf_ReadSamples_S16(&CirBuf1_S16, L,CirBufTestDataForRd_S16);
        if(memcmp(CirBufTestDataForWr_S16,CirBufTestDataForRd_S16,i*sizeof(S16))) while(1){};
    }
    
    InitCirAudioBuf_S16(&CirBuf1_S16, TmpBufForTestingCirBuf_S16, 29);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S16[i]=i+10;
        CirAudioBuf_WriteSamples_S16(&CirBuf1_S16,L,CirBufTestDataForWr_S16);
        CirAudioBuf_ReadSamples_S16(&CirBuf1_S16, L,CirBufTestDataForRd_S16);
        if(memcmp(CirBufTestDataForWr_S16,CirBufTestDataForRd_S16,i*sizeof(S16))) while(1){};
    }
    
    
    InitCirAudioBuf_S16(&CirBuf1_S16, TmpBufForTestingCirBuf_S16, 49);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S16[i]=i+10;
        CirAudioBuf_WriteSamples_S16(&CirBuf1_S16,L,CirBufTestDataForWr_S16);
        CirAudioBuf_ReadSamples_S16(&CirBuf1_S16, L,CirBufTestDataForRd_S16);
        if(memcmp(CirBufTestDataForWr_S16,CirBufTestDataForRd_S16,i*sizeof(S16))) while(1){};
    }
    
    
    //test S32 circular buffer
    InitCirAudioBuf_S32(&CirBuf1_S32, TmpBufForTestingCirBuf_S32, 20);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%10;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S32[i]=i+10;
        CirAudioBuf_WriteSamples_S32(&CirBuf1_S32,L,CirBufTestDataForWr_S32);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S32(&CirBuf1_S32, L);
        memcpy(CirBufTestDataForRd_S32,TestRdPtr,i*sizeof(S32));
        if(memcmp(CirBufTestDataForWr_S32,CirBufTestDataForRd_S32,i*sizeof(S32))) while(1){};
    }
    
    InitCirAudioBuf_S32(&CirBuf1_S32, TmpBufForTestingCirBuf_S32, 29);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S32[i]=i+10;
        CirAudioBuf_WriteSamples_S32(&CirBuf1_S32,L,CirBufTestDataForWr_S32);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S32(&CirBuf1_S32, L);
        memcpy(CirBufTestDataForRd_S32,TestRdPtr,i*sizeof(S32));
        if(memcmp(CirBufTestDataForWr_S32,CirBufTestDataForRd_S32,i*sizeof(S32))) while(1){};
    }
    
    
    InitCirAudioBuf_S32(&CirBuf1_S32, TmpBufForTestingCirBuf_S32, 49);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S32[i]=i+10;
        CirAudioBuf_WriteSamples_S32(&CirBuf1_S32,L,CirBufTestDataForWr_S32);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S32(&CirBuf1_S32, L);
        memcpy(CirBufTestDataForRd_S32,TestRdPtr,i*sizeof(S32));
        if(memcmp(CirBufTestDataForWr_S32,CirBufTestDataForRd_S32,i*sizeof(S32))) while(1){};
    }
    

    InitCirAudioBuf_S32(&CirBuf1_S32, TmpBufForTestingCirBuf_S32, 20);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%10;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S32[i]=i+10;
        CirAudioBuf_WriteSamples_S32(&CirBuf1_S32,L,CirBufTestDataForWr_S32);
        CirAudioBuf_ReadSamples_S32(&CirBuf1_S32, L,CirBufTestDataForRd_S32);
        if(memcmp(CirBufTestDataForWr_S32,CirBufTestDataForRd_S32,i*sizeof(S32))) while(1){};
    }
    
    InitCirAudioBuf_S32(&CirBuf1_S32, TmpBufForTestingCirBuf_S32, 29);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S32[i]=i+10;
        CirAudioBuf_WriteSamples_S32(&CirBuf1_S32,L,CirBufTestDataForWr_S32);
        CirAudioBuf_ReadSamples_S32(&CirBuf1_S32, L,CirBufTestDataForRd_S32);
        if(memcmp(CirBufTestDataForWr_S32,CirBufTestDataForRd_S32,i*sizeof(S32))) while(1){};
    }
    
    
    InitCirAudioBuf_S32(&CirBuf1_S32, TmpBufForTestingCirBuf_S32, 49);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S32[i]=i+10;
        CirAudioBuf_WriteSamples_S32(&CirBuf1_S32,L,CirBufTestDataForWr_S32);
        CirAudioBuf_ReadSamples_S32(&CirBuf1_S32, L,CirBufTestDataForRd_S32);
        if(memcmp(CirBufTestDataForWr_S32,CirBufTestDataForRd_S32,i*sizeof(S32))) while(1){};
    }
    

    //test S64 circular buffer
    InitCirAudioBuf_S64(&CirBuf1_S64, TmpBufForTestingCirBuf_S64, 20);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%10;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S64[i]=i+10;
        CirAudioBuf_WriteSamples_S64(&CirBuf1_S64,L,CirBufTestDataForWr_S64);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S64(&CirBuf1_S64, L);
        memcpy(CirBufTestDataForRd_S64,TestRdPtr,i*sizeof(S64));
        if(memcmp(CirBufTestDataForWr_S64,CirBufTestDataForRd_S64,i*sizeof(S64))) while(1){};
    }
    
    InitCirAudioBuf_S64(&CirBuf1_S64, TmpBufForTestingCirBuf_S64, 29);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S64[i]=i+10;
        CirAudioBuf_WriteSamples_S64(&CirBuf1_S64,L,CirBufTestDataForWr_S64);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S64(&CirBuf1_S64, L);
        memcpy(CirBufTestDataForRd_S64,TestRdPtr,i*sizeof(S64));
        if(memcmp(CirBufTestDataForWr_S64,CirBufTestDataForRd_S64,i*sizeof(S64))) while(1){};
    }
    
    
    InitCirAudioBuf_S64(&CirBuf1_S64, TmpBufForTestingCirBuf_S64, 49);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S64[i]=i+10;
        CirAudioBuf_WriteSamples_S64(&CirBuf1_S64,L,CirBufTestDataForWr_S64);
        TestRdPtr=CirAudioBuf_ReadSamples_GetRdPtr_S64(&CirBuf1_S64, L);
        memcpy(CirBufTestDataForRd_S64,TestRdPtr,i*sizeof(S64));
        if(memcmp(CirBufTestDataForWr_S64,CirBufTestDataForRd_S64,i*sizeof(S64))) while(1){};
    }
    

    InitCirAudioBuf_S64(&CirBuf1_S64, TmpBufForTestingCirBuf_S64, 20);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%10;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S64[i]=i+10;
        CirAudioBuf_WriteSamples_S64(&CirBuf1_S64,L,CirBufTestDataForWr_S64);
        CirAudioBuf_ReadSamples_S64(&CirBuf1_S64, L,CirBufTestDataForRd_S64);
        if(memcmp(CirBufTestDataForWr_S64,CirBufTestDataForRd_S64,i*sizeof(S64))) while(1){};
    }
    
    InitCirAudioBuf_S64(&CirBuf1_S64, TmpBufForTestingCirBuf_S64, 29);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S64[i]=i+10;
        CirAudioBuf_WriteSamples_S64(&CirBuf1_S64,L,CirBufTestDataForWr_S64);
        CirAudioBuf_ReadSamples_S64(&CirBuf1_S64, L,CirBufTestDataForRd_S64);
        if(memcmp(CirBufTestDataForWr_S64,CirBufTestDataForRd_S64,i*sizeof(S64))) while(1){};
    }
    
    
    InitCirAudioBuf_S64(&CirBuf1_S64, TmpBufForTestingCirBuf_S64, 49);
    for(j=0;j<1000;j++)
    {
        int L;
        L=j%30;
        
        if(!L) continue;
        for(i=0;i<L;i++)
            CirBufTestDataForWr_S64[i]=i+10;
        CirAudioBuf_WriteSamples_S64(&CirBuf1_S64,L,CirBufTestDataForWr_S64);
        CirAudioBuf_ReadSamples_S64(&CirBuf1_S64, L,CirBufTestDataForRd_S64);
        if(memcmp(CirBufTestDataForWr_S64,CirBufTestDataForRd_S64,i*sizeof(S64))) while(1){};
    }
}

#endif
