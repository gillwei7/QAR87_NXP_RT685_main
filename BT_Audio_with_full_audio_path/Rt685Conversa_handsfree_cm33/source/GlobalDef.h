/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __GlobalDef_h__
#define __GlobalDef_h__


//------------------------settings can be changed ----------------------
//sub working modes (work state) enable or disable --- go to WorkStateManager.h to setup

#define LetComConnectTuningTool								0		//0 or 1 can be selected
#if LetComConnectTuningTool==1
	#define PRINTF_GoesToUsbCom								0		//must be 0
#else
	#define PRINTF_GoesToUsbCom								1		//if defined to 1, PRINTF doesn't use UART but goes to USB com
#endif

#define EnableAudioPllAdjustingToSyncBetweenBtFsAndLocalFs	1		//should always enable this to keep I2S data sync with BT (on buffer under flow or over flow)

#define EnableMonitorUsbAudioUpStreamLengthAdjusting		1		//should be 1, if 0 then UAC up streaming may get buffer over/under flow

#define EnableBtCirBufUnderflowOverFlowPrint				0
#define EnableUacCirBufUnderflowOverFlowPrint				0
#define EnableUartComWatchPrint								0
#define EnableUsbComWatchPrint								0		//to use this, must set #define LetComConnectTuningTool	0, and must set #define PRINTF_GoesToUsbCom 0,  (otherwise there is no USB COM print --- USB COM is for Conversa tuning)

#define	Rt685I2SToNvtIsI2SMaster							1
#define Rt685I2SToAmpIsI2SMaster							1		//could be 1 or 0, both works

//------------------------settings can be changed ----------------------







//NOTE: all defines below, should should not be changed, unless you clearly know what you want

//-------------------------------------------------------------------------------------
#if 1	//folding --- general defines --- should not be changed

#define EnableUsbComAndAudio						1		//must be 1
#define NvtI2SFs_16KHz								16000
#define NvtI2SFs_48KHz								48000

#define I2SToNvtIntrRecoverWaitTime					100


#if EnableUsbComAndAudio==0
	#undef EnableMonitorUsbAudioUpStreamLengthAdjusting
	#undef EnableUsbComWatchPrint
	#define EnableMonitorUsbAudioUpStreamLengthAdjusting	0
	#define EnableUsbComWatchPrint							0
#endif

#if EnableUsbComWatchPrint==1
	#undef LetComConnectTuningTool
	#define LetComConnectTuningTool							0
#endif

#if ((NvtI2SFs_48KHz!=16000)&&(NvtI2SFs_48KHz!=48000))
	#error NvtI2SFs_48KHz must be either 16000 or 48000
#endif


typedef unsigned char  		U8;
typedef unsigned short 		U16;
typedef unsigned int 		U32;
typedef unsigned long long	U64;

typedef signed char  		S8;
typedef signed short 		S16;
typedef signed int 			S32;
typedef long long			S64;

typedef unsigned char  		u8;
typedef unsigned short 		u16;
typedef unsigned int 		u32;
typedef unsigned long long	u64;

typedef signed char  		s8;
typedef signed short 		s16;
typedef signed int 			s32;
typedef long long			s64;

#define _Value_Pow_2_31_M1	2147483647
#define _Value_Pow_2_Neg31_	0.0000000004656612873077392578125f

#define _Value_Pow_2_15_M1	32767
#define _Value_Pow_2_Neg15_	0.000030517578125

#define Pi_Value            3.14159265353846


#define DEMO_DMA			(DMA0)

#define APP_MU MUA
#define APP_MU_IRQHandler MU_A_IRQHandler
/* Channel transmit and receive register */
#endif
//-------------------------------------------------------------------------------------


#include "DefForBothMcuAndDsp.h"

extern volatile T_CommonVarSharedByDspAndMcu VarBlockSharedByDspAndMcu;

//--------------- USB PID definition according to MCU type and USB audio format ----------------------
#if EnableUsbComAndAudio==1
#define USB_DEVICE_VID 		(0x2FD9U)			//suppose this is for Quanta, and Quanta decides to change to their own PID VID

#define USB_DEVICE_PID_Bit8To15		(0x68U)		//this stands for RT685, also we have 59 for RT595, 11,12,14,15,16,17 --> RT101x,102x,104x,105x,106x,117x
#define USB_DEVICE_PID_Bit4To7		AUDIO_IN_FORMAT_CHANNELS
#define USB_DEVICE_PID_Bit0To3		AUDIO_OUT_FORMAT_CHANNELS
#endif
//--------------- USB PID definition according to MCU type and USB audio format ----------------------


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
#if 1	//folding --- reading cycle counter --- should never change this part
//#define GET_CYCLE_COUNTER(x)   x=DWT->CYCCNT;			//this only works when jlink is connected

/* DWT (Data Watchpoint and Trace) registers, only exists on ARM Cortex with a DWT unit */
#define KIN1_DWT_CONTROL             (*((volatile uint32_t*)0xE0001000))
/*!< DWT Control register */
#define KIN1_DWT_CYCCNTENA_BIT       (1UL<<0)
/*!< CYCCNTENA bit in DWT_CONTROL register */
#define KIN1_DWT_CYCCNT              (*((volatile uint32_t*)0xE0001004))
/*!< DWT Cycle Counter register */
#define KIN1_DEMCR                   (*((volatile uint32_t*)0xE000EDFC))
/*!< DEMCR: Debug Exception and Monitor Control Register */
#define KIN1_TRCENA_BIT              (1UL<<24)
/*!< Trace enable bit in DEMCR register */


#define KIN1_InitCycleCounter() \
  KIN1_DEMCR |= KIN1_TRCENA_BIT
  /*!< TRCENA: Enable trace and debug block DEMCR (Debug Exception and Monitor Control Register */

#define KIN1_ResetCycleCounter() \
  KIN1_DWT_CYCCNT = 0
  /*!< Reset cycle counter */

#define KIN1_EnableCycleCounter() \
  KIN1_DWT_CONTROL |= KIN1_DWT_CYCCNTENA_BIT
  /*!< Enable cycle counter */

#define KIN1_DisableCycleCounter() \
  KIN1_DWT_CONTROL &= ~KIN1_DWT_CYCCNTENA_BIT
  /*!< Disable cycle counter */

#define KIN1_GetCycleCounter() \
  KIN1_DWT_CYCCNT
  /*!< Read cycle counter register */

#define GET_CYCLE_COUNTER_JLink(x)  x=DWT->CYCCNT;
#define GET_CYCLE_COUNTER(x)   		x=KIN1_GetCycleCounter();	//this works even when jlink is NOT connected

#endif
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------



#define PRINTF_M(x, ...)		do {			\
SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);	\
	PRINTF(x, ##__VA_ARGS__);					\
SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);			\
} while(0)

//gill
#define AUTO_CONNECT_ENABLE     1
#define BT_CONNECTION_LOG       0
#define ENABLE_POWER_DOWN       1


#endif






