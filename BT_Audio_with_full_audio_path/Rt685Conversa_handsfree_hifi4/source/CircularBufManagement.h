/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */




#ifndef __CirbufManagement___
#define __CirbufManagement___


#include "GlobalDef.h"

#define EnableLessUsedCirBufFunctions       0
#define EnableCirBufFunctionsForS8          1
#define EnableCirBufFunctionsForS16         1
#define EnableCirBufFunctionsForS32         1
#define EnableCirBufFunctionsForS64         0


#if defined(EnableCirBufFunctionsForS8)&&(EnableCirBufFunctionsForS8==1)
typedef struct
{
	U32 LengthInSamples;
	S8 *PtrBufHead;
	S8 *PtrRd;
	S8 *PtrWr;
} T_CircularAudioBuf_S8;

    void InitCirAudioBuf_S8(T_CircularAudioBuf_S8 *CirBufPtr,S8 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBePut, S8 *PtrAudioDataSrc);
    extern void CirAudioBuf_ReadSamples_S8(T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBeGot, S8 *PtrAudioDataDst);
    S8* CirAudioBuf_ReadSamples_GetRdPtr_S8(T_CircularAudioBuf_S8 *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S8(T_CircularAudioBuf_S8 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S8(volatile T_CircularAudioBuf_S8 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S8(T_CircularAudioBuf_S8 *CirBufPtr);
#endif      //EnableCirBufFunctionsForS8


#if defined(EnableCirBufFunctionsForS16)&&(EnableCirBufFunctionsForS16==1)
typedef struct
{
	U32 LengthInSamples;
	S16 *PtrBufHead;
	S16 *PtrRd;
	S16 *PtrWr;
} T_CircularAudioBuf_S16;

    void InitCirAudioBuf_S16(T_CircularAudioBuf_S16 *CirBufPtr,S16 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBePut, S16 *PtrAudioDataSrc);
    S16* CirAudioBuf_ReadSamples_GetRdPtr_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S16(T_CircularAudioBuf_S16 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S16(T_CircularAudioBuf_S16 *CirBufPtr);
    #if defined(EnableLessUsedCirBufFunctions)&&(EnableLessUsedCirBufFunctions==1)
        void CirAudioBuf_ReadSamples_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot, S16 *PtrAudioDataDst);
        void CirAudioBuf_ReadSamplesBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot, S16 *PtrAudioDataDst);
        U8 CirAudioBuf_GetUsagePercentage_S16(T_CircularAudioBuf_S16 *CirBufPtr);
        void CirAudioBuf_MoveRdPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward);
        S16* CirAudioBuf_GetDdPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward);
        void CirAudioBuf_MoveRdPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack);
        S16* CirAudioBuf_GetRdPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack);
        void CirAudioBuf_MoveWrPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward);
        S16* CirAudioBuf_GetWrPtrForwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoForward);
        void CirAudioBuf_MoveWrPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack);
        S16* CirAudioBuf_GetWrPtrBackwards_S16(T_CircularAudioBuf_S16 *CirBufPtr, U32 NumOfSamplesToGoBack);
    #endif      //EnableLessUsedCirBufFunctions
#endif      //EnableCirBufFunctionsForS16





#if defined(EnableCirBufFunctionsForS32)&&(EnableCirBufFunctionsForS32==1)
typedef struct
{
	U32 LengthInSamples;
	S32 *PtrBufHead;
	S32 *PtrRd;
	S32 *PtrWr;
} T_CircularAudioBuf_S32;

    void InitCirAudioBuf_S32(T_CircularAudioBuf_S32 *CirBufPtr,S32 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBePut, S32 *PtrAudioDataSrc);
    S32* CirAudioBuf_ReadSamples_GetRdPtr_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr);
    extern U8 CirAudioBuf_GetUsagePercentage_S32(T_CircularAudioBuf_S32 *CirBufPtr);
    #if defined(EnableLessUsedCirBufFunctions)&&(EnableLessUsedCirBufFunctions==1)
        void CirAudioBuf_ReadSamples_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot, S32 *PtrAudioDataDst);
        U8 CirAudioBuf_GetUsagePercentage_S32(T_CircularAudioBuf_S32 *CirBufPtr);
        void CirAudioBuf_MoveRdPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward);
        S32* CirAudioBuf_GetDdPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward);
        void CirAudioBuf_MoveRdPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack);
        S32* CirAudioBuf_GetRdPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack);
        void CirAudioBuf_MoveWrPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward);
        S32* CirAudioBuf_GetWrPtrForwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoForward);
        void CirAudioBuf_MoveWrPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack);
        S32* CirAudioBuf_GetWrPtrBackwards_S32(T_CircularAudioBuf_S32 *CirBufPtr, U32 NumOfSamplesToGoBack);
    #endif      //EnableLessUsedCirBufFunctions
#endif      //EnableCirBufFunctionsForS32



#if defined(EnableCirBufFunctionsForS64)&&(EnableCirBufFunctionsForS64==1)
typedef struct
{
	U32 LengthInSamples;
	S64 *PtrBufHead;
	S64 *PtrRd;
	S64 *PtrWr;
} T_CircularAudioBuf_S64;

    void InitCirAudioBuf_S64(T_CircularAudioBuf_S64 *CirBufPtr,S64 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBePut, S64 *PtrAudioDataSrc);
    S64* CirAudioBuf_ReadSamples_GetRdPtr_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr);
    #if defined(EnableLessUsedCirBufFunctions)&&(EnableLessUsedCirBufFunctions==1)
        void CirAudioBuf_ReadSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot, S64 *PtrAudioDataDst);
        U8 CirAudioBuf_GetUsagePercentage_S64(T_CircularAudioBuf_S64 *CirBufPtr);
        void CirAudioBuf_MoveRdPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward);
        S64* CirAudioBuf_GetDdPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward);
        void CirAudioBuf_MoveRdPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack);
        S64* CirAudioBuf_GetRdPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack);
        void CirAudioBuf_MoveWrPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward);
        S64* CirAudioBuf_GetWrPtrForwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoForward);
        void CirAudioBuf_MoveWrPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack);
        S64* CirAudioBuf_GetWrPtrBackwards_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 NumOfSamplesToGoBack);
    #endif      //EnableLessUsedCirBufFunctions
#endif      //EnableCirBufFunctionsForS64



#endif
