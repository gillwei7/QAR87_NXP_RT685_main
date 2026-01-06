/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __GlobalDef_h__
#define __GlobalDef_h__


//-----------------------------------APP cfg--------------------------------------
//---beg---

//conversa tuning is in homeVit standby
//set UseUacDnAudioForConversaTuning_VoiceCall16KHz to 1 to do voice call tuning, set both to 1 to do farBF tuning
#define UseUacDnAudioForConversaTuning_VoiceCall16KHz	1
#define UseUacDnAudioForConversaTuning_FarBf16KHz		0
#if UseUacDnAudioForConversaTuning_FarBf16KHz==1
	#undef UseUacDnAudioForConversaTuning_VoiceCall16KHz
	#define UseUacDnAudioForConversaTuning_VoiceCall16KHz	1
#endif

#define EAP_ENABLE					1



#define EnableOpusDecodingPrint		0
#define EnableSbcDecodingPrint		0

#define EnableLvlMeter				1

#define OpusOutputCirBuf_LRMixed_LengthInMs			60		//each time decoding generated 20ms*2, DMA buffer length 8ms max, so 60 should be enough
#define SbcOutputCirBuf_LRMixed_LengthInMs			60		//each time decoding generated no more than 30~400ms, DMA buffer length 8ms max, so 60 should be enough


#define EnableOpusDec		1
#define EnableSbcDec		1

#define DecoderSbc_SrcInSizeInSamples	256
#define DecoderOpus_SrcInSizeInSamples	256

#define TestAlgoInitAndDeInit				0	//in normal running, must set this to 0
#define EnableDebugPrint					1

//---end---
//-----------------------------------APP cfg--------------------------------------

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long long S64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;


#define _Value_Pow_2_31_M1	2147483647
#define _Value_Pow_2_Neg31_	0.0000000004656612873077392578125f

#define _Value_Pow_2_15_M1	32767
#define _Value_Pow_2_Neg15_	0.000030517578125

#define Pi_Value            3.14159265353846



//If UsingQAR87Board == 1, the program can run on the QAR87 development board.
#define UsingQAR87Board		1
#define UsingDbgPin			0			//set to 1, may hurt NVT ???

#define     LedGrnPinPort               0
#define     LedBluPinPort               0
#define     LedRedPinPort               0

#define     LedGrnPin                   14
#define     LedBluPin                   26
#define     LedRedPin                   31


#if UsingQAR87Board==1
	#define     DbgPin5Port                 0
	#define     DbgPin6Port                 2
	#define     DbgPin7Port                 1
	#define     DbgPin8Port                 0

	#define     DbgPin5                     25
	#define     DbgPin6                     15
	#define     DbgPin7                     2
	#define     DbgPin8                     21
#else
	#define     DbgPin5Port                 0
	#define     DbgPin6Port                 0
	#define     DbgPin7Port                 0
	#define     DbgPin8Port                 0

	#define     DbgPin5                     3
	#define     DbgPin6                     4
	#define     DbgPin7                     19
	#define     DbgPin8                     20
#endif


#define APP_MU MUB
#define APP_MU_IRQn 6

#include "..\..\Rt685Conversa_handsfree_cm33\source\DefForBothMcuAndDsp.h"


//should set only 1 of the following 2 to 1, or both to 0
#define Using_UART5ToPrint		0 // =0 will use FC2 (PIO0_15)
#define Using_UART2ToPrint		0 // =1 will use FC2 (PIO0_15)

#if(Using_UART5ToPrint)&&(Using_UART2ToPrint)
	#error can not set Using_UART5ToPrint and Using_UART2ToPrint to 1 at the same time!
#endif

#if UsingQAR87Board == 1	//running on QAR87 board
#if UsingDbgPin == 1
	#define     DbgPin5Up()                 GPIO_PinWrite(GPIO, DbgPin5Port, DbgPin5, 1)
	#define     DbgPin5Dn()                 GPIO_PinWrite(GPIO, DbgPin5Port, DbgPin5, 0)
	#define     DbgPin6Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin6, 1)
	#define     DbgPin6Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin6, 0)

	#define     DbgPin7Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin7, 1)
	#define     DbgPin7Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin7, 0)
	#define     DbgPin8Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin8, 1)
	#define     DbgPin8Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin8, 0)
#else //UsingDbgPin = 0
	#define     DbgPin5Up()
	#define     DbgPin5Dn()
	#define     DbgPin6Up()
	#define     DbgPin6Dn()
	#define     DbgPin7Up()
	#define     DbgPin7Dn()
	#define     DbgPin8Up()
	#define     DbgPin8Dn()

	#define     LedOn_G()
	#define     LedOff_G()
	#define     LedOn_R()
	#define     LedOff_R()
	#define     LedOn_B()
	#define     LedOff_B()
#endif
#else	//running on RT685-EVK
	#define     DbgPin5Up()                 GPIO_PinWrite(GPIO, DbgPin5Port, DbgPin5, 1)
	#define     DbgPin5Dn()                 GPIO_PinWrite(GPIO, DbgPin5Port, DbgPin5, 0)
	#define     DbgPin6Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin6, 1)
	#define     DbgPin6Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin6, 0)
	#define     DbgPin7Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin7, 1)
	#define     DbgPin7Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin7, 0)
	#define     DbgPin8Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin8, 1)
	#define     DbgPin8Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin8, 0)

	#define     LedOn_G()                   GPIO_PinWrite(GPIO, LedGrnPinPort, LedGrnPin, 1)
	#define     LedOff_G()                  GPIO_PinWrite(GPIO, LedGrnPinPort, LedGrnPin, 0)
	#define     LedOn_R()                   GPIO_PinWrite(GPIO, LedRedPinPort, LedRedPin, 1)
	#define     LedOff_R()                  GPIO_PinWrite(GPIO, LedRedPinPort, LedRedPin, 0)
	#define     LedOn_B()                   GPIO_PinWrite(GPIO, LedBluPinPort, LedBluPin, 1)
	#define     LedOff_B()                  GPIO_PinWrite(GPIO, LedBluPinPort, LedBluPin, 0)
#endif



extern volatile T_CommonVarSharedByDspAndMcu *PtrVarBlockSharedByDspAndMcu;

extern U32 read_ccount(void);




typedef enum _ReturnResultType
{
	RtnValue_Success=0,
	RtnValue_Fail,
} ReturnResultType;


#define PRINTF_M(x, ...)		do {			\
SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);	\
	PRINTF(x, ##__VA_ARGS__);					\
SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);			\
} while(0)



#endif
