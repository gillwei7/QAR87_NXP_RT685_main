/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __CirbufManagement___
#define __CirbufManagement___

#if EnableConversa==1

#include "DefForBothMcuAndDsp.h"



#define EnableCirBufFunctionsForS16         			1
#define EnableCirBufFunctionsForS32         			0
#define EnableCirBufFunctionsForS64         			0

#define EnableCirBufFunctionsForUacUpStreamMultiCh		EnableUsbComAndAudio
#define EnableCirBufFunctionsForUacDnStreamMultiCh		EnableUsbComAndAudio



#if defined(EnableCirBufFunctionsForS16)&&(EnableCirBufFunctionsForS16==1)
typedef struct
{
	U32 LengthInSamples;
	S16 *PtrBufHead;
	S16 *PtrRd;
	S16 *PtrWr;
} T_CircularAudioBuf_S16;

    void InitCirAudioBuf_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr,S16 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBePut, S16 *PtrAudioDataSrc);
    S16* CirAudioBuf_ReadSamples_GetRdPtr_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr);
    void CirAudioBuf_ReadSamples_S16(volatile T_CircularAudioBuf_S16 *CirBufPtr, U32 SampleNumbersToBeGot, S16 *PtrAudioDataDst);
#endif      //EnableCirBufFunctionsForS16



#if defined(EnableCirBufFunctionsForS32)&&(EnableCirBufFunctionsForS32==1)
typedef struct
{
	U32 LengthInSamples;
	S32 *PtrBufHead;
	S32 *PtrRd;
	S32 *PtrWr;
} T_CircularAudioBuf_S32;

    void InitCirAudioBuf_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr,S32 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBePut, S32 *PtrAudioDataSrc);
    S32* CirAudioBuf_ReadSamples_GetRdPtr_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr);
    void CirAudioBuf_ReadSamples_S32(volatile T_CircularAudioBuf_S32 *CirBufPtr, U32 SampleNumbersToBeGot, S32 *PtrAudioDataDst);
#endif      //EnableCirBufFunctionsForS32



#if defined(EnableCirBufFunctionsForS64)&&(EnableCirBufFunctionsForS64==1)
typedef struct
{
	U32 LengthInSamples;
	S64 *PtrBufHead;
	S64 *PtrRd;
	S64 *PtrWr;
} T_CircularAudioBuf_S64;

    void InitCirAudioBuf_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr,S64 *DataAreaHead,U32 BufLen);
    void CirAudioBuf_WriteSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBePut, S64 *PtrAudioDataSrc);
    S64* CirAudioBuf_ReadSamples_GetRdPtr_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot);
    void CirAudioBuf_ReadSamples_S64(T_CircularAudioBuf_S64 *CirBufPtr, U32 SampleNumbersToBeGot, S64 *PtrAudioDataDst);
    U32 CirAudioBuf_SpaceOccupiedInSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr);
    U32 CirAudioBuf_SpaceAvailableInSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr);
    void CirAudioBuf_ClearAllSamples_S64(volatile T_CircularAudioBuf_S64 *CirBufPtr);
#endif      //EnableCirBufFunctionsForS64



#if defined(EnableCirBufFunctionsForUacUpStreamMultiCh)&&(EnableCirBufFunctionsForUacUpStreamMultiCh==1)
	typedef struct
	{
		#ifdef UsbAudioFormat_UpStreamChNumIsOne
			S32 s[1];
		#endif
		#ifdef UsbAudioFormat_UpStreamChNumIsTwo
			S32 s[2];
		#endif
		#ifdef UsbAudioFormat_UpStreamChNumIsFour
			S32 s[4];
		#endif
		#ifdef UsbAudioFormat_UpStreamChNumIsSix
			S32 s[6];
		#endif
		#ifdef UsbAudioFormat_UpStreamChNumIsEight
			S32 s[8];
		#endif
	} T_MCh32BitUacUpAudioSample;

	typedef struct
	{
		U32 LengthInSamples;
		T_MCh32BitUacUpAudioSample *PtrBufHead;
		T_MCh32BitUacUpAudioSample *PtrRd;
		T_MCh32BitUacUpAudioSample *PtrWr;
	} T_CirUacUpAudioBuf_MCh;

    void InitCirUacUpAudioBuf_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr,T_MCh32BitUacUpAudioSample *DataAreaHead,U32 BufLen);
    void CirUacUpAudioBuf_WriteSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBePut, T_MCh32BitUacUpAudioSample *PtrAudioDataSrc);
    T_MCh32BitUacUpAudioSample* CirUacUpAudioBuf_ReadSamples_GetRdPtr_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirUacUpAudioBuf_SpaceOccupiedInSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr);
    U32 CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr);
    void CirUacUpAudioBuf_ClearAllSamples_MultiCh(T_CirUacUpAudioBuf_MCh *CirBufPtr);
#endif



#if defined(EnableCirBufFunctionsForUacDnStreamMultiCh)&&(EnableCirBufFunctionsForUacDnStreamMultiCh==1)
	typedef struct
	{
		#ifdef UsbAudioFormat_DnStreamChNumIsOne
			S32 s[1];
		#endif
		#ifdef UsbAudioFormat_DnStreamChNumIsTwo
			S32 s[2];
		#endif
		#ifdef UsbAudioFormat_DnStreamChNumIsFour
			S32 s[4];
		#endif
		#ifdef UsbAudioFormat_DnStreamChNumIsSix
			S32 s[6];
		#endif
		#ifdef UsbAudioFormat_DnStreamChNumIsEight
			S32 s[8];
		#endif
	} T_MCh32BitUacDnAudioSample;

	typedef struct
	{
		U32 LengthInSamples;
		T_MCh32BitUacDnAudioSample *PtrBufHead;
		T_MCh32BitUacDnAudioSample *PtrRd;
		T_MCh32BitUacDnAudioSample *PtrWr;
	} T_CirUacDnAudioBuf_MCh;

    void InitCirUacDnAudioBuf_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr,T_MCh32BitUacDnAudioSample *DataAreaHead,U32 BufLen);
    void CirUacDnAudioBuf_WriteSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBePut, T_MCh32BitUacDnAudioSample *PtrAudioDataSrc);
    T_MCh32BitUacDnAudioSample* CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr, U32 SampleNumbersToBeGot);
    U32 CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr);
    U32 CirUacDnAudioBuf_SpaceAvailableInSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr);
    void CirUacDnAudioBuf_ClearAllSamples_MultiCh(T_CirUacDnAudioBuf_MCh *CirBufPtr);
#endif

#define S32LRMixedSampleSizeInBytes 		8			//this is S32 L and R samples mixed


#endif


#endif
