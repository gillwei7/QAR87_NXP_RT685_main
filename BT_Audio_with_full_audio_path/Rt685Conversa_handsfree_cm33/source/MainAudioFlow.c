/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_i2s.h"
#include "fsl_debug_console.h"
//#include "debug_utils.h"
#include "fsl_dmic.h"
#include "fsl_dmic_dma.h"
#include "fsl_i2s_dma.h"
#include "fsl_inputmux.h"
#include "fsl_mu.h"
#include "fsl_sema42.h"


#include <stdio.h>

#include "GlobalDef.h"


#if 1	//folding --- variables

#if EnableUsbComAndAudio==1

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_audio.h"

#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"

#include "UsbApp.h"
#endif

#include "SubFunc.h"
#include "AudioIoCfg_I2s.h"
#include "AudioIoCfg_Pdm.h"
#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "MainAudioFlow.h"
#include "Sweep.h"
#include "WorkStateManager.h"

#if UsingQAR87Board == 1
#include "hal_amp.h"
#include "system_status.h"


#endif

int UartComReportValue_S32_1;
int UartComReportValue_S32_2;
int UartComReportValue_S32_3;
int UartComReportValue_S32_4;

int UsbComReportValue_S32_1;
int UsbComReportValue_S32_2;
int UsbComReportValue_S32_3;
int UsbComReportValue_S32_4;

U32 I2SOutputMuteCntNvt=0;
U32 I2SOutputMuteCntAmp=0;
U32 PdmInputMuteCnt=0;

#if EnableUsbComAndAudio==1
extern usb_device_composite_struct_t *PtrUsbDevComposite;
#endif

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t s_currSendBuf    [1024], 4);

volatile S32 *MicInputCh0Ptr;
volatile S32 *MicInputCh1Ptr;
volatile S32 *MicInputCh2Ptr;
volatile S32 *MicInputCh3Ptr;
volatile S32 *MicInputCh4Ptr;
volatile S32 *MicInputCh5Ptr;
volatile S32 *MicInputCh6Ptr;
volatile S32 *MicInputCh7Ptr;

volatile U8 DmaTxRxIsDone;
volatile U8 DmaTxRxIsExpected;

volatile U8 PdmIsStarted;
volatile U8 I2sIsStarted;

static int CycCntInfoIdx1=0;
static int CycCntInfoIdx2=0;

//---------------------------------functions-------------------------------

U32 TimePoint_PrevAudioCallBack;

extern uint32_t AODOfUacDnBuf;
extern U32 AudioIoFrameCntForMuteMicInputAtStartingUp;
extern volatile unsigned int s_sendSize;

U32 AudioIoFrameCnt=0;

volatile S16 *I2SDmaOtCh01Ptr;		//to AMP
volatile S16 *I2SDmaInCh01Ptr;		//from AMP

volatile S16 *I2SDmaOtCh23Ptr;		//to NVT
volatile S16 *I2SDmaInCh23Ptr;		//from NVT

extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;
extern int UacDnAOD_ForFbAdjust;

extern uint32_t USBAudio_FeedBackEp_feedbackValue;

#endif

#if 1	//folding --- signal flow main processing after audio input is ready
__attribute__((section("CodeQuickAccess")))
int CheckTimePoint_CurrentIntrIsAStartingOne(void)
{
	int r;
	U32 TimePoint;
	GET_CYCLE_COUNTER(TimePoint);
	if((TimePoint - TimePoint_PrevAudioCallBack) > (CLOCK_GetFreq(kCLOCK_CoreSysClk)*0.008f*0.6f))	//current intr time point is already a far distance from last one --- 60% of 8ms away from the previous one
	{
		//enough far away from last intr
		r=1;
	}else
	{
		//still close to last intr
		r=0;
	}
	TimePoint_PrevAudioCallBack=TimePoint;
	return r;
}
__attribute__((section("CodeQuickAccess")))
void CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(U32 MicSelectBitMap, U32 FrmSize, U32 NeedToMuteStarting)
{
	//take real mic audio input as incoming data
	#if EnableMic01 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic01)
			PdmCh0DmaTransferringIsUsingBufA=GetPdmCh0DmaTransferringIsUsingBufAOrB();
	#endif
	#if EnableMic23 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic23)
			PdmCh2DmaTransferringIsUsingBufA=GetPdmCh2DmaTransferringIsUsingBufAOrB();
	#endif
	#if EnableMic45 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic45)
			PdmCh4DmaTransferringIsUsingBufA=GetPdmCh4DmaTransferringIsUsingBufAOrB();
	#endif
	#if EnableMic67 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic67)
			PdmCh6DmaTransferringIsUsingBufA=GetPdmCh6DmaTransferringIsUsingBufAOrB();
	#endif

	#if EnableMic01 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic01)
			if(PdmCh0DmaTransferringIsUsingBufA)
			{
				//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
				MicInputCh0Ptr=MicInputDmaDualBuf_0 + 1 * FrmSize;
				MicInputCh1Ptr=MicInputDmaDualBuf_1 + 1 * FrmSize;
			}else
			{
				//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
				MicInputCh0Ptr=MicInputDmaDualBuf_0 + 0 * FrmSize;
				MicInputCh1Ptr=MicInputDmaDualBuf_1 + 0 * FrmSize;
			}
	#endif
	#if EnableMic23 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic23)
			if(PdmCh2DmaTransferringIsUsingBufA)
			{
				//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
				MicInputCh2Ptr=MicInputDmaDualBuf_2 + 1 * FrmSize;
				MicInputCh3Ptr=MicInputDmaDualBuf_3 + 1 * FrmSize;
			}else
			{
				//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
				MicInputCh2Ptr=MicInputDmaDualBuf_2 + 0 * FrmSize;
				MicInputCh3Ptr=MicInputDmaDualBuf_3 + 0 * FrmSize;
			}
	#endif
	#if EnableMic45 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic45)
			if(PdmCh4DmaTransferringIsUsingBufA)
			{
				//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
				MicInputCh4Ptr=MicInputDmaDualBuf_4 + 1 * FrmSize;
				MicInputCh5Ptr=MicInputDmaDualBuf_5 + 1 * FrmSize;
			}else
			{
				//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
				MicInputCh4Ptr=MicInputDmaDualBuf_4 + 0 * FrmSize;
				MicInputCh5Ptr=MicInputDmaDualBuf_5 + 0 * FrmSize;
			}
	#endif
	#if EnableMic67 == 1
		if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic67)
			if(PdmCh6DmaTransferringIsUsingBufA)
			{
				//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
				MicInputCh6Ptr=MicInputDmaDualBuf_6 + 1 * FrmSize;
				MicInputCh7Ptr=MicInputDmaDualBuf_7 + 1 * FrmSize;
			}else
			{
				//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
				MicInputCh6Ptr=MicInputDmaDualBuf_6 + 0 * FrmSize;
				MicInputCh7Ptr=MicInputDmaDualBuf_7 + 0 * FrmSize;
			}
	#endif
	//left shift 8 bits to have mic signal reach the full scale --- raw data is 24 bit effective located in the lower 24bits


	if((NeedToMuteStarting)&&(PdmInputMuteCnt))
	{
		//need to mute mic at starting up, and still in the muting period
		for(int i=0;i<FrmSize;i++)
		{
			#if EnableMic01 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic01)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=0;
				}
			#endif
			#if EnableMic23 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic23)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=0;
				}
			#endif
			#if EnableMic45 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic45)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=0;
				}
			#endif
			#if EnableMic67 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic67)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=0;
				}
			#endif
		}
		PdmInputMuteCnt--;
	}else
	{
		//no need to mute mic, or mute period is over
		for(int i=0;i<FrmSize;i++)
		{
			//left shift 8 bits to have mic signal reach the full scale --- raw data is 24 bit effective located in the lower 24bits
			#if EnableMic01 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic01)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=(MicInputCh0Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=(MicInputCh1Ptr[i]<<8);
				}
			#endif
			#if EnableMic23 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic23)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=(MicInputCh2Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=(MicInputCh3Ptr[i]<<8);
				}
			#endif
			#if EnableMic45 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic45)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=(MicInputCh4Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=(MicInputCh5Ptr[i]<<8);
				}
			#endif
			#if EnableMic67 == 1
				if(MicSelectBitMap&AudioPdmPortsBitMapFlag_Mic67)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=(MicInputCh6Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=(MicInputCh7Ptr[i]<<8);
				}
			#endif
		}
	}
}
__attribute__((section("CodeQuickAccess")))
void SetI2SBufferABSelect(int ToSetI2SAmp, int ToSetI2SNvt)
{
	if(ToSetI2SAmp)
	{
		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();

		if(I2S1DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh01Ptr=I2SRxFrAmpCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh01Ptr=I2SRxFrAmpCh0And1Mixed_A;
		}
		if(I2S3DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh01Ptr=I2STxToAmpCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh01Ptr=I2STxToAmpCh0And1Mixed_A;
		}
	}
	if(ToSetI2SNvt)
	{
		I2STxToNvtDmaTransferringIsUsingBufA=GetI2STxToNvtDmaTransferringIsUsingBufAOrB();
		I2SRxFrNvtDmaTransferringIsUsingBufA=GetI2SRxFrNvtDmaTransferringIsUsingBufAOrB();

		if(I2STxToNvtDmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh23Ptr=I2STxToNvtCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh23Ptr=I2STxToNvtCh0And1Mixed_A;
		}
		if(I2SRxFrNvtDmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh23Ptr=I2SRxFrNvtCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh23Ptr=I2SRxFrNvtCh0And1Mixed_A;
		}
	}
}
__attribute__((section("CodeQuickAccess")))
void MoveUacAudioToDspSharedUacDnBuf(int FrmSize, int ToDoDeci)
{
	OSA_SR_ALLOC();
	#if 1	//folding --- get audio data from UAC Dn cir buffer
		S32 *TmpPtrS32;
		static uint32_t lastUsbRecvTimes = 0, usbAudioNoInputCounter = 0;
		if (lastUsbRecvTimes != PtrUsbDevComposite->audioUnified.usbRecvTimes)
		{
			lastUsbRecvTimes       = PtrUsbDevComposite->audioUnified.usbRecvTimes;
			usbAudioNoInputCounter = 0;
		}
		else if (PtrUsbDevComposite->audioUnified.usbRecvTimes)
		{
			usbAudioNoInputCounter++;
			if (usbAudioNoInputCounter == 30)
			{
				//audio intrrupt has come here for many times, but no USB audio Rx event occurs ---- this means USB audio Rx is borken
				PtrUsbDevComposite->audioUnified.startPlayHalfFull      = 0;
				PtrUsbDevComposite->audioUnified.speakerDetachOrNoInput = 1;
				lastUsbRecvTimes                                = 0;
				usbAudioNoInputCounter                          = 0;
			}
		}

		if (PtrUsbDevComposite->audioUnified.startPlayHalfFull)
		{	//has enough down-streaming audio
			OSA_ENTER_CRITICAL();
			if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= FrmSize)
			{
				TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, FrmSize);
				//PtrUsbDevComposite->audioUnified.audioSendCount += AudioFrameSizeInSamplePerCh_16KHz * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE;
				//PtrUsbDevComposite->audioUnified.audioSendTimes++;
				#if AUDIO_OUT_FORMAT_CHANNELS==2

					if(ToDoDeci)
					{
						//do 3->1 decimation
						//pick up 1 point every 3 points
						for(int i=0;i<FrmSize/3;i++)
						{
							VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
							VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
							TmpPtrS32+=4;
						}
					}else
					{
						//no decimation
						for(int i=0;i<FrmSize;i++)
						{
							VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
							VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
						}
					}
				#endif
			}else
			{	//not enough USB down streaming data --- set all to zeros
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
				#if EnableUacCirBufUnderflowOverFlowPrint==1
					PRINTF_M("UacDn E\r\n");
				#endif
			}
			OSA_EXIT_CRITICAL();
		}else
		{	//UAC down buffer is not half full yet, --- set all to zeros
			memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
			memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
		}
	#endif

	#if 1	//folding --- check UAC streaming stop and clear UAC cir buffer
		if(UsbUpStreamingIsStarted)
		{
			UsbUpStreamingStopMonitorCnt++;
			if(UsbUpStreamingStopMonitorCnt>200)
			{
				//uac up streaming request is stopped
				OSA_ENTER_CRITICAL();
				CirUacUpAudioBuf_ClearAllSamples_MultiCh(&UacUpAudioBuf_MCh);
				UsbUpStreamingIsStarted=0;
				UsbUpStreamingStopMonitorCnt=0;
				OSA_EXIT_CRITICAL();
			}
		}

		if(UsbDnStreamingIsStarted)
		{
			UsbDnStreamingStopMonitorCnt++;
			if(UsbDnStreamingStopMonitorCnt>200)
			{
				//uac dn streaming request is stopped
				OSA_ENTER_CRITICAL();
				CirUacDnAudioBuf_ClearAllSamples_MultiCh(&UacDnAudioBuf_MCh);
				UsbDnStreamingIsStarted=0;
				UsbDnStreamingStopMonitorCnt=0;
				OSA_EXIT_CRITICAL();
			}
		}
	#endif

	#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1	//folding --- get UAC buf AOD
		OSA_ENTER_CRITICAL();
		UacDnAOD_ForFbAdjust = CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh);
		//VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=CirUacUpAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacUpAudioBuf_MCh);
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[9]=UacDnAOD_ForFbAdjust;
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[10]=USBAudio_FeedBackEp_feedbackValue;
		OSA_EXIT_CRITICAL();
	#endif
}
__attribute__((section("CodeQuickAccess")))
void PrintWatchToUartComAndUsbCom(int Cnt, int PrintFreqAmpScale)
{
	int CycLen=100/PrintFreqAmpScale;
	//report monitor info to Uart com host through uart
	if((Cnt%CycLen)==1)
	{
		//CycCntInfoIdx1=0;	//this is to only display one selected row, the first row of the total 4 rows
		CycCntInfoIdx1=2;	//this is to only display one selected row, the third row of the total 4 rows

		VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]=AOD_BTDnBuf;
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[1]=AOD_BTUpBuf;

		#if EnableAudioPllAdjustingToSyncBetweenBtFsAndLocalFs==1
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[2]=AUDIOPLL0NUM_AdjustingValue;
		#else
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[2]=0;
		#endif

		#if 1
			UartComReportValue_S32_1=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+0];
			UartComReportValue_S32_2=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+1];
			UartComReportValue_S32_3=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+2];
			UartComReportValue_S32_4=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+3];
		#else
			UartComReportValue_S32_1=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx1*4+0];
			UartComReportValue_S32_2=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx1*4+1];
			UartComReportValue_S32_3=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx1*4+2];
			UartComReportValue_S32_4=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx1*4+3];
		#endif

		#if EnableUartComWatchPrint==1
			sprintf((char *)s_currSendBuf,"W#%d,%d,%d,%d,%d\r\n",CycCntInfoIdx1,UartComReportValue_S32_1,UartComReportValue_S32_2,UartComReportValue_S32_3,UartComReportValue_S32_4);
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[3]=0;
			PRINTF_M(s_currSendBuf);
		#endif

		CycCntInfoIdx1++;						//using CycCntInfoIdx1 from 0 to 3, can have 16 cycle cnt values to be printed
		if(CycCntInfoIdx1>3) CycCntInfoIdx1=0;
	}


	#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
		//report cycle count info to USB com host through VCOM
		if((Cnt%CycLen)==2)
		{
			//CycCntInfoIdx2=0;	//this is to only display one selected row, the first row of the total 4 rows
			CycCntInfoIdx2=2;	//this is to only display one selected row, the third row of the total 4 rows

			#if 1
				UsbComReportValue_S32_1=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+0];
				UsbComReportValue_S32_2=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+1];
				UsbComReportValue_S32_3=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+2];
				UsbComReportValue_S32_4=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+3];
			#else
				UsbComReportValue_S32_1=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx2*4+0];
				UsbComReportValue_S32_2=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx2*4+1];
				UsbComReportValue_S32_3=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx2*4+2];
				UsbComReportValue_S32_4=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx2*4+3];
			#endif

			#if EnableUsbComWatchPrint==1
				s_sendSize=CycCntInfoIdx2+1;				//command to do VCOM send in the main loop!
			#endif
			CycCntInfoIdx2++;						//using CycCntInfoIdx2 from 0 to 3, can have 16 cycle cnt values to be printed
			if(CycCntInfoIdx2>3) CycCntInfoIdx2=0;
		}
	#endif
}

__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_HfpCall(void)
{
	S32 *TmpPtrS32;
	S16 i,j;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh_16KHz];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	DbgPin5Up();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data to DSP shared buffer, get I2S in samples to DSP shared buffer
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
		SetI2SBufferABSelect(1,0);		//(int ToSetI2SAmp, int ToSetI2SNvt)

		//copy I2S input (AMP feedback) to shared memory from DMA buffer, for DSP later use
		//actually, now we are not using the signal from AMP loopback
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
		{
			VarBlockSharedByDspAndMcu.I2SBufInFrAmpL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SBufInFrAmpR[i]=*I2SDmaInCh01Ptr++;
		}
	#endif

	#if 1	//folding --- get audio data from BT Dn cir buffer
		if(VarBlockSharedByDspAndMcu.BtHfpFs==8000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//take audio samples out from BT Dn buffer --- put to AudioBufInFrBt, and then DSP side will take in and process
				if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh_16KHz/2))
				{
					//there are enough audio sample from BT down streaming
					CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh_16KHz/2, TmpAudioS16Buf);
					for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz/2;i++)
						VarBlockSharedByDspAndMcu.AudioBufInFrBt[i]=(TmpAudioS16Buf[i]<<16);
				}else
				{
					//not enough audio samples from BT down streaming
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF_M("BT Up CirBuf is F \r\n");
					#endif
					memset((void *)VarBlockSharedByDspAndMcu.AudioBufInFrBt,0,sizeof(VarBlockSharedByDspAndMcu.AudioBufInFrBt)/2);
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		if(VarBlockSharedByDspAndMcu.BtHfpFs==16000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//take audio samples out from BT Dn buffer --- put to AudioBufInFrBt, and then DSP side will take in and process
				if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh_16KHz/1))
				{
					//there are enough audio sample from BT down streaming
					CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh_16KHz/1, TmpAudioS16Buf);
					for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz/1;i++)
						VarBlockSharedByDspAndMcu.AudioBufInFrBt[i]=(TmpAudioS16Buf[i]<<16);
				}else
				{
					//not enough audio samples from BT down streaming
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF_M("BT Dn CirBuf is E \r\n");
					#endif
					memset((void *)VarBlockSharedByDspAndMcu.AudioBufInFrBt,0,sizeof(VarBlockSharedByDspAndMcu.AudioBufInFrBt));
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		{
			//should never come here
			PRINTF_M("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}
	#endif

	#if EnableUsbComAndAudio==1
		MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);		//later can be removed
	#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_HfpCall);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_HfpCall);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_HomeVitStandBy(void)
{
	S32 *TmpPtrS32;
	S16 i,j;
	//S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh_16KHz];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	DbgPin5Up();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
		SetI2SBufferABSelect(1,0);		//(int ToSetI2SAmp, int ToSetI2SNvt)
		//in this mode, no ref signal to conversa is needed, clear it
		memset((void *)VarBlockSharedByDspAndMcu.AudioBufInFrBt,0,sizeof(VarBlockSharedByDspAndMcu.AudioBufInFrBt));
	#endif

	#if EnableUsbComAndAudio==1
		MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);		//later can be removed
	#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_AudioIoDbg(void)
{
	DbgPin5Up();

	OSA_SR_ALLOC();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
		SetI2SBufferABSelect(1,1);//(int ToSetI2SAmp, int ToSetI2SNvt)
	#endif

	#if 1	//folding --- get I2S input from AMP and from Nvt, and put to shared var buffer
		//copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
		{
			VarBlockSharedByDspAndMcu.I2SBufInFrAmpL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SBufInFrAmpR[i]=*I2SDmaInCh01Ptr++;
		}

		for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
		{
			#if 1
				VarBlockSharedByDspAndMcu.I2SBufInFrNvtL[i]=((*(S16 *)I2SDmaInCh23Ptr++)<<16);
				VarBlockSharedByDspAndMcu.I2SBufInFrNvtR[i]=((*(S16 *)I2SDmaInCh23Ptr++)<<16);
				#if NvtI2SFs_48KHz==48000
					I2SDmaInCh23Ptr+=2;
				#endif
			#else
					VarBlockSharedByDspAndMcu.I2SBufInFrNvtL[i]=0x20000*i;
					VarBlockSharedByDspAndMcu.I2SBufInFrNvtR[i]=-0x20000*i;
			#endif
		}
	#endif

	#if EnableUsbComAndAudio==1
		MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);
	#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}

__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_VideoRecording(void)
{
    DbgPin5Up();

    OSA_SR_ALLOC();

    #if 1   //folding --- prepare mic input pointers, and get int type of mic signal data
        //take real mic audio input as incoming data
        CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
        SetI2SBufferABSelect(1,1);//(int ToSetI2SAmp, int ToSetI2SNvt)
    #endif

    #if 1   //folding --- get I2S input from AMP and from Nvt, and put to shared var buffer
        //copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
        for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
        {
            VarBlockSharedByDspAndMcu.I2SBufInFrAmpL[i]=*I2SDmaInCh01Ptr++;
            VarBlockSharedByDspAndMcu.I2SBufInFrAmpR[i]=*I2SDmaInCh01Ptr++;
        }

        for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
        {
            VarBlockSharedByDspAndMcu.I2SBufInFrNvtL[i]=*I2SDmaInCh23Ptr++;
            VarBlockSharedByDspAndMcu.I2SBufInFrNvtR[i]=*I2SDmaInCh23Ptr++;
        }
    #endif

    #if EnableUsbComAndAudio==1
        MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);
    #endif

    #if 1   //folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
            //no need to skip the first several frame
            MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_VideoRecording);       //send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

        PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
    #endif

    DmaTxRxIsDone=0;
    DbgPin5Dn();
}

__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_MediaPlayer(void)
{
//	ProcessAudio_AfterAudioInputBufIsReady_AudioIoDbg();	//they are the same

	DbgPin5Up();

	OSA_SR_ALLOC();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
		SetI2SBufferABSelect(1,1);//(int ToSetI2SAmp, int ToSetI2SNvt)
	#endif

	#if 1	//folding --- get I2S input from AMP and from Nvt, and put to shared var buffer
		//copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
		for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
		{
			VarBlockSharedByDspAndMcu.I2SBufInFrAmpL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SBufInFrAmpR[i]=*I2SDmaInCh01Ptr++;
		}

		for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
		{
			VarBlockSharedByDspAndMcu.I2SBufInFrNvtL[i]=*I2SDmaInCh23Ptr++;
			VarBlockSharedByDspAndMcu.I2SBufInFrNvtR[i]=*I2SDmaInCh23Ptr++;
		}
	#endif

	#if EnableUsbComAndAudio==1
		MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);
	#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_MediaPlayer);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_MusicPlayer(void)
{
	DbgPin5Up();

	OSA_SR_ALLOC();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
		SetI2SBufferABSelect(1,0);//(int ToSetI2SAmp, int ToSetI2SNvt)
	#endif

	#if EnableUsbComAndAudio==1
		MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);
	#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_Translation(void)
{
    DbgPin5Up();

    OSA_SR_ALLOC();

    #if 1   //folding --- prepare mic input pointers, and get int type of mic signal data
        //take real mic audio input as incoming data
        CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
        SetI2SBufferABSelect(1,1);//(int ToSetI2SAmp, int ToSetI2SNvt)
    #endif

    #if 1   //folding --- get I2S input from AMP and from Nvt, and put to shared var buffer
        //copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
        for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
        {
            VarBlockSharedByDspAndMcu.I2SBufInFrAmpL[i]=*I2SDmaInCh01Ptr++;
            VarBlockSharedByDspAndMcu.I2SBufInFrAmpR[i]=*I2SDmaInCh01Ptr++;
        }

        for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
        {
            VarBlockSharedByDspAndMcu.I2SBufInFrNvtL[i]=*I2SDmaInCh23Ptr++;
            VarBlockSharedByDspAndMcu.I2SBufInFrNvtR[i]=*I2SDmaInCh23Ptr++;
        }
    #endif

    #if EnableUsbComAndAudio==1
        MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);
    #endif

    #if 1   //folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
            //no need to skip the first several frame
            MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_Translation);       //send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

        PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
    #endif

    DmaTxRxIsDone=0;
    DbgPin5Dn();
}

__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_AiConversation(void)
{
}

__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady_VideoAi(void)
{
    DbgPin5Up();

    OSA_SR_ALLOC();

    #if 1   //folding --- prepare mic input pointers, and get int type of mic signal data
        //take real mic audio input as incoming data
        CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh_16KHz,1);
        SetI2SBufferABSelect(1,1);//(int ToSetI2SAmp, int ToSetI2SNvt)
    #endif

    #if 1   //folding --- get I2S input from AMP and from Nvt, and put to shared var buffer
        //copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
        for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
        {
            VarBlockSharedByDspAndMcu.I2SBufInFrAmpL[i]=*I2SDmaInCh01Ptr++;
            VarBlockSharedByDspAndMcu.I2SBufInFrAmpR[i]=*I2SDmaInCh01Ptr++;
        }

        for(int i=0;i<AudioFrameSizeInSamplePerCh_48KHz;i++)
        {
            VarBlockSharedByDspAndMcu.I2SBufInFrNvtL[i]=*I2SDmaInCh23Ptr++;
            VarBlockSharedByDspAndMcu.I2SBufInFrNvtR[i]=*I2SDmaInCh23Ptr++;
        }
    #endif

    #if EnableUsbComAndAudio==1
        MoveUacAudioToDspSharedUacDnBuf(AudioFrameSizeInSamplePerCh_48KHz, 0);
    #endif

    #if 1   //folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
            //no need to skip the first several frame
            MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_VideoAi);       //send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

        PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
    #endif

    DmaTxRxIsDone=0;
    DbgPin5Dn();
}
#endif

#if 1	//folding --- audioflow finalization
extern volatile uint8_t NowInIncomingCallRingTone;

int LocalToneGainControlCnt=0;
float LocalToneGainCurrent=0.0f;
float LocalToneGainTarget=0.0f;
__attribute__((section("CodeQuickAccess")))
void PutAudioDataFromDspToUacUpCirBuffer(int FrmSize)	//audio in this fucntion is always 16KHz, cause the UAC up inteface is 16KHz
{
	int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
	AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(&UacUpAudioBuf_MCh);
	AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
	if(UsbUpStreamingIsStarted)
	{
		if (AOFSOfUacUpBuf >= (FrmSize))
		{
			CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, FrmSize, (T_MCh32BitUacUpAudioSample *)VarBlockSharedByDspAndMcu.UacUpAudioBuf);
		}
		else
		{
			#if EnableUacCirBufUnderflowOverFlowPrint==1
				PRINTF_M("UacUp F\r\n");
			#endif
		}
	}

	//adjust mic upstreaming tx length
	//call adjust mic upstreaming length with AOFS just after writing the cir buffer
	if(UsbUpStreamingIsStarted)
	{
		USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+FrmSize);	//calling adjust just before writing the circular buffer
	}

	#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
		if(UsbUpStreamingIsStarted)
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=AODOfUacUpBuf+FrmSize;
	#endif
}
__attribute__((section("CodeQuickAccess")))
void MoveAudioDataFromDspToUacUpCirBuf(int FrmSize)
{
	OSA_SR_ALLOC();
	OSA_ENTER_CRITICAL();
		//put audio data from DSP side to UacUp cir buffer
		int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
		AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(&UacUpAudioBuf_MCh);
		AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
		if(UsbUpStreamingIsStarted)
		{
			if (AOFSOfUacUpBuf >= (FrmSize))
			{
				CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, FrmSize, (T_MCh32BitUacUpAudioSample *)VarBlockSharedByDspAndMcu.UacUpAudioBuf);
			}
			else
			{
				#if EnableUacCirBufUnderflowOverFlowPrint==1
					PRINTF_M("UacUp F\r\n");
				#endif
			}
		}

		//adjust mic upstreaming tx length
		//call adjust mic upstreaming length with AOFS just after writing the cir buffer
		if(UsbUpStreamingIsStarted)
		{
			USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+FrmSize);	//calling adjust just before writing the circular buffer
		}
	OSA_EXIT_CRITICAL();

	#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
		if(UsbUpStreamingIsStarted)
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=AODOfUacUpBuf+FrmSize;
	#endif
}
__attribute__((section("CodeQuickAccess")))
void MoveAudioDataFromDspToI2SDmaBufAmp(int FrmSize, int ToUseStartingMute)
{
	if(!ToUseStartingMute)
		I2SOutputMuteCntAmp=0;

	if(I2SOutputMuteCntAmp)
	{
		I2SOutputMuteCntAmp--;
		#if 1
			memset((void *)I2SDmaOtCh01Ptr,0,sizeof(S16)*2*FrmSize);
		#else
			//for debug watching audio wave form
			for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				*I2SDmaOtCh01Ptr++=0;
				*I2SDmaOtCh01Ptr++=0x10*i;
			}
		#endif
	}else
	{
		for(int i=0;i<FrmSize;i++)
		{
			*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SBufOtToAmpL[i];	//stream out the audio of conversa Rx output --- L
			//*I2SDmaOtCh01Ptr=0x10*i;
			*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SBufOtToAmpR[i];	//stream out the audio of conversa output
			//*I2SDmaOtCh01Ptr=0-0x10*i;
		}
	}
}
__attribute__((section("CodeQuickAccess")))
void MoveAudioDataFromDspToI2SDmaBufNvt(int FrmSize, int ToUseStartingMute)
{
	if(!ToUseStartingMute)
		I2SOutputMuteCntNvt=0;

	if(I2SOutputMuteCntNvt)
	{
		I2SOutputMuteCntNvt--;
		#if 1
			memset((void *)I2SDmaOtCh23Ptr,0,sizeof(S16)*2*FrmSize);
		#else
			//for debug watching audio wave form
			for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
			{
				*I2SDmaOtCh23Ptr++=0;
				*I2SDmaOtCh23Ptr++=0x10*i;
			}
		#endif
	}else
	{
		for(int i=0;i<FrmSize;i++)
		{
			*I2SDmaOtCh23Ptr++=VarBlockSharedByDspAndMcu.I2SBufOtToNvtL[i];	//stream out the audio of conversa Rx output --- L
			//*I2SDmaOtCh23Ptr=0x10*i;
			*I2SDmaOtCh23Ptr++=VarBlockSharedByDspAndMcu.I2SBufOtToNvtL[i];	//stream out the audio of conversa output
			//*I2SDmaOtCh23Ptr=0-0x10*i;
		}
	}
}

__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_HfpCall(void)
{
	int i;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh_16KHz];
	S16 TmpRingToneS32_SingleCh[AudioFrameSizeInSamplePerCh_16KHz];

	#if 1	//folding --- write conversa Tx output audio to BT up streaming cir buffer
		OSA_SR_ALLOC();
		if(VarBlockSharedByDspAndMcu.BtHfpFs==8000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//put audio samples into BT Up buffer --- AudioBufOtToBt has the processed audio of the previous frame
				if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh_16KHz/2))
				{
					//there are enough free space from BT up streaming
					for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz/2;i++)
						TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.AudioBufOtToBt[i]>>16);
					CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh_16KHz/2, TmpAudioS16Buf);
				}else
				{
					//not enough free space for BT UP streaming --- abandon the current frame of AudioBufOtToBt
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF_M("BT Up CirBuf is F \r\n");
					#endif
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		if(VarBlockSharedByDspAndMcu.BtHfpFs==16000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//put audio samples into BT Up buffer --- AudioBufOtToBt has the processed audio of the previous frame
				if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh_16KHz/1))
				{
					//there are enough free space from BT up streaming
					for(i=0;i<AudioFrameSizeInSamplePerCh_16KHz/1;i++)
						TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.AudioBufOtToBt[i]>>16);
					CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh_16KHz/1, TmpAudioS16Buf);
				}else
				{
					//not enough free space for BT UP streaming --- abandon the current frame of AudioBufOtToBt
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF_M("BT Up CirBuf is F \r\n");
					#endif
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		{
			//should never come here
			PRINTF_M("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}
	#endif

	#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
		MoveAudioDataFromDspToUacUpCirBuf(AudioFrameSizeInSamplePerCh_16KHz);
	#endif

	#if 1	//folding --- write conversa Rx output audio to I2S
		if(NowInIncomingCallRingTone)
		{
			#if 0
				//generate tone from sin calculation --- much more mips are needed, but can be flexible setting different f
				GenerateSineToneSingleFreq_S16_LRMixed(&SineToneGenerator1, TmpRingToneS32_SingleCh, AudioFrameSizeInSamplePerCh_16KHz , 1);
			#else
				//generate tone from sin wav table --- much smaller mips, but freq is fixed at 320Hz
				GenerateSinWavFromTable_S16_SingleCh(TmpRingToneS32_SingleCh, AudioFrameSizeInSamplePerCh_16KHz);
			#endif

			LocalToneGainControlCnt++;

			//16KHz Fs
			//0.8s tone sound, 1.2s silence
			if((LocalToneGainControlCnt%400)<180)
			{
				LocalToneGainTarget=0.7f;
			}else
			{
				LocalToneGainTarget=0.0f;
			}

			for(int j=0;j<16;j++)
			{
				//loop 16 times, each time processes 8 samples --- this is to avoid volume change click noise
				LocalToneGainCurrent=0.02f*LocalToneGainTarget + 0.98f*LocalToneGainCurrent;
				//copy I2S output buffer (line out to DAC) from shared memory to DMA buffer, I2S output buffer are just processed and written by DSP
				for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz/16;i++)
				{
					*I2SDmaOtCh01Ptr++=TmpRingToneS32_SingleCh[j*8+i]*LocalToneGainCurrent;
					*I2SDmaOtCh01Ptr++=TmpRingToneS32_SingleCh[j*8+i]*LocalToneGainCurrent;
				}
			}
		}else
		{
			LocalToneGainCurrent=0.0f;
			LocalToneGainTarget=0.0f;
			MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_16KHz, 1);
		}

	#endif

	//close the following part --- to let button be used for connecting HFP/A2DP
	#if 0	//folding --- check button long press to answer or reject an incoming call
		if((NowInHfpTelCall)||(NowInHfpAppCall)||(NowInIncomingCallRingTone))	//Note!!! when calling out, not able to identify if it is HfpApp call or HfpTel call. So, no matter what call, check the button long press (answer, reject/stop)
																//check the beyerdynamic speaker, it is the same!!!
																//so, we keep the logic of checking button in App call or Hfp call as it is.
		{
			//if(codec_inited)  //this means: in ring tone or in the call
			//if(RingToneIsInitialized)
			{
				//check if button 1 is pressed --- answer the call
				if(BtnEvtVarGroup[0].BtnEvt1==BTN_EVT_LONG_PRESS_2)
				{
					if(!NowInHfpTelCall)
						NeedToCall_hfp_AnswerCall=1;
					//hfp_AnswerCall();		//should be called outside intr
					BtnEvtVarGroup[0].BtnEvt1=0;
				}
				//check if button 2 is pressed --- reject the call
				if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_LONG_PRESS_2)
				{
					NeedToCall_hfp_RejectCall=1;
					//hfp_RejectCall();		//should be called outside intr
					BtnEvtVarGroup[1].BtnEvt1=0;
				}
			}
		}
	#endif
}
__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_HomeVitStandBy(void)
{
	#if EnableUsbComAndAudio==1	//folding --- write conversa Tx output audio to UAC up streaming buffer
		OSA_SR_ALLOC();
		OSA_ENTER_CRITICAL();
			//put audio data from DSP side to UacUp cir buffer
			PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
		OSA_EXIT_CRITICAL();
	#endif
	MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_16KHz, 1);
}
__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_AudioIoDbg(void)
{
	#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
		OSA_SR_ALLOC();
		OSA_ENTER_CRITICAL();
			//put audio data from DSP side to UacUp cir buffer
			PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
		OSA_EXIT_CRITICAL();
	#endif

	#if 1	//folding --- write Dsp out audio to I2S to AMP and I2S to NVT
		MoveAudioDataFromDspToI2SDmaBufNvt(AudioFrameSizeInSamplePerCh_48KHz,1);
		MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_48KHz,1);
	#endif
}

__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_VideoRecording(void)
{
#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
    OSA_SR_ALLOC();
    OSA_ENTER_CRITICAL();
        //put audio data from DSP side to UacUp cir buffer
        PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
    OSA_EXIT_CRITICAL();
#endif

#if 1   //folding --- write Dsp out audio to I2S to AMP and I2S to NVT
    MoveAudioDataFromDspToI2SDmaBufNvt(AudioFrameSizeInSamplePerCh_48KHz,1);
    //MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_48KHz,1);
#endif
}

__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_MediaPlayer(void)
{
	//McuMainAudioFlowFinalize_AudioIoDbg();	//they are the same
#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
	OSA_SR_ALLOC();
	OSA_ENTER_CRITICAL();
		//put audio data from DSP side to UacUp cir buffer
		PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
	OSA_EXIT_CRITICAL();
#endif

#if 1	//folding --- write Dsp out audio to I2S to AMP and I2S to NVT
	MoveAudioDataFromDspToI2SDmaBufNvt(AudioFrameSizeInSamplePerCh_48KHz,1);
	MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_48KHz,1);
#endif
}
__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_MusicPlayer(void)
{
	#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
		OSA_SR_ALLOC();
		OSA_ENTER_CRITICAL();
			//put audio data from DSP side to UacUp cir buffer
			PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
		OSA_EXIT_CRITICAL();
	#endif

	MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_48KHz,1);
}
__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_Translation(void)
{
#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
    OSA_SR_ALLOC();
    OSA_ENTER_CRITICAL();
        //put audio data from DSP side to UacUp cir buffer
        PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
    OSA_EXIT_CRITICAL();
#endif

#if 1   //folding --- write Dsp out audio to I2S to AMP and I2S to NVT
    MoveAudioDataFromDspToI2SDmaBufNvt(AudioFrameSizeInSamplePerCh_48KHz,1);
    MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_48KHz,1);
#endif
}

__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_AiConversation(void)
{
}

__attribute__((section("CodeQuickAccess")))
void McuMainAudioFlowFinalize_VideoAi(void)
{
#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
    OSA_SR_ALLOC();
    OSA_ENTER_CRITICAL();
        //put audio data from DSP side to UacUp cir buffer
        PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh_16KHz);
    OSA_EXIT_CRITICAL();
#endif

#if 1   //folding --- write Dsp out audio to I2S to AMP and I2S to NVT
    MoveAudioDataFromDspToI2SDmaBufNvt(AudioFrameSizeInSamplePerCh_48KHz,1);
    MoveAudioDataFromDspToI2SDmaBufAmp(AudioFrameSizeInSamplePerCh_48KHz,1);
#endif
}
#endif

#if 1	//folding --- audio interface init
extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx);

void InitPdm(U32 MicActiveBitMap, int FrmSize, int Fs)
{
	if(!AudioPortIsActive_Pdm)
	{
		/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
		CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
		//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
		CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

		Init_MicDmaCfgCh    (MicActiveBitMap,FrmSize,32);	//mic0,1,2,3,4,5
		BOARD_Init_DMA_PDM  (MicActiveBitMap);
		BOARD_Init_DMIC     (MicActiveBitMap,0,Fs); //0: no skip general Dmic init. If not the first mic init, then should skip.
		ConfigDmicChainedDma(MicActiveBitMap);
		AudioPortIsActive_Pdm=1;
		PdmInputMuteCnt=12;	//96ms
	}
}
void InitAmpI2S(int FrmSize, int Fs, int bits)
{
	if(!AudioPortIsActive_I2SToAmp)
	{
		BOARD_Init_DMA_I2S_FcTxToAmp();
		BOARD_Init_DMA_I2S_FcRxFrAmp();
			BOARD_Init_I2S_FcTxToAmp(Fs,bits);
			BOARD_Init_I2S_FcRxFrAmp(Fs,bits);
				ClearDmaBuf_I2SToAmp();
				ClearDmaBuf_I2SFrAmp();
					ConfigI2STxToAmpChainedDma(FrmSize,bits);
					ConfigI2SRxFrAmpChainedDma(FrmSize,bits);
						EnableI2SToAmpDmaChannel();
						EnableI2SFrAmpDmaChannel();
		AudioPortIsActive_I2SToAmp=1;
		I2SOutputMuteCntAmp=25;	//200ms
	}
}
void InitNvtI2S(int FrmSize, int Fs, int bits)
{
	if(!AudioPortIsActive_I2SToNvt)
	{
		BOARD_Init_DMA_I2S_FcTxToNvt();
		BOARD_Init_DMA_I2S_FcRxFrNvt();
			BOARD_Init_I2S_FcTxToNvt(Fs,bits);
			BOARD_Init_I2S_FcRxFrNvt(Fs,bits);
				ClearDmaBuf_I2STxToNvt();
				ClearDmaBuf_I2SRxFrNvt();
					ConfigI2STxToNvtChainedDma(FrmSize,bits);
					ConfigI2SRxFrNvtChainedDma(FrmSize,bits);
						EnableI2STxToNvtDmaChannel();
						EnableI2SRxFrNvtDmaChannel();
		AudioPortIsActive_I2SToNvt=1;
		I2SOutputMuteCntNvt=25;	//200ms
	}
}
void InitAudioInterface_AudioIoDebug(int Opt)
{
	//if everything is configured, no need to do anything, return
	if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_I2SToNvt)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
		return;

	int MclkFreq;
	/* Enable clock */
	//if any audio port is NOT configured, set the clk.
	//no matter BT side is 16KHz or 8KHz, no matter A2dp source is 44.1KHz or whatever, CODEC is always 16KHz or 48KHz
	if((!AudioPortIsActive_I2SToAmp)||(!AudioPortIsActive_I2SToNvt)||!(AudioPortIsActive_Pdm))
	{
		#if UsingQAR87Board == 1
			MclkFreq=BOARD_SwitchAudioFreq(16000,NvtFc5Fc6_AmpFc1Fc3);
		#else
			MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
		#endif
	}

	InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
	InitAmpI2S(AudioFrameSizeInSamplePerCh_48KHz, 48000, 16);
	#if Rt685I2SToNvtIsI2SMaster==1
		InitNvtI2S(AudioFrameSizeInSamplePerCh_48KHz, NvtI2SFs_48KHz, 16);
	#else
		InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, NvtI2SFs_48KHz, 16);
	#endif

	//configure AMP
	InitAndStartCodec(48000, 16, MclkFreq);

	InitAudioCircularBuf(0,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

	DmaTxRxIsExpected=(
						AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
						AudioI2sPortsBitMapFlag_FcTxToNvt|AudioI2sPortsBitMapFlag_FcRxFrNvt|
						//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
						#if EnableMic01==1
							AudioPdmPortsBitMapFlag_Mic01
						#endif
						#if EnableMic23==1
							|AudioPdmPortsBitMapFlag_Mic23
						#endif
						#if EnableMic45==1
							|AudioPdmPortsBitMapFlag_Mic45
						#endif
						#if EnableMic67==1
							|AudioPdmPortsBitMapFlag_Mic67
						#endif

					  );

	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

	VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
	VarBlockSharedByDspAndMcu.I2SFs_Amp=48000;
	VarBlockSharedByDspAndMcu.PdmFs=16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples=AudioFrameSizeInSamplePerCh_16KHz;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_48KHz;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Nvt=AudioFrameSizeInSamplePerCh_48KHz;
	VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;
	return;
}
void InitAudioInterface_HfpCall(int Opt)
{
	int MclkFreq,src_clk_hz;

	PRINTF_M("InitAudioInterface_HfpCall BT Side Fs: %d  BT Side BitWidth: %d \r\n", BtHfpAudioFs, BtHfpAudioBitWidth);


	if((BtHfpAudioFs!=8000)&&(BtHfpAudioFs!=16000))
	{
		PRINTF_M("InitAudioInterface_HfpCall failed, Hfp audio can not be started --- BT Side bit Fs is NOT 8kHz or 16kHz \r\n");
		return;
	}

	VarBlockSharedByDspAndMcu.BtHfpFs=BtHfpAudioFs;

	/* Enable clock */
	//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
	#if UsingQAR87Board == 1
		src_clk_hz = BOARD_SwitchAudioFreq(16000,BtPcmFc2Fc4_AmpFc1Fc3);
	#else
		src_clk_hz = BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
	#endif

	InitHfpAudioIntfToBT(MclkFreq, BtHfpAudioFs);


	InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
	InitAmpI2S(AudioFrameSizeInSamplePerCh_16KHz,    16000, 16);			//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
	InitAndStartCodec(16000, 16, MclkFreq);
	InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir


	DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
			#if EnableMic01==1
				AudioPdmPortsBitMapFlag_Mic01
			#endif
			#if EnableMic23==1
				|AudioPdmPortsBitMapFlag_Mic23
			#endif
			#if EnableMic45==1
				|AudioPdmPortsBitMapFlag_Mic45
			#endif
			#if EnableMic67==1
				|AudioPdmPortsBitMapFlag_Mic67
			#endif
					  );

	VarBlockSharedByDspAndMcu.I2SFs_Amp=16000;
	VarBlockSharedByDspAndMcu.PdmFs    =16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_16KHz;
	VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples    =AudioFrameSizeInSamplePerCh_16KHz;
	VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;
	return;
}
void InitAudioInterface_HomeVitStandby(int Opt)
{
	int MclkFreq;


	//if everything is configured, no need to do anything, return
	if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
		return;

	if((!AudioPortIsActive_I2SToAmp)||!(AudioPortIsActive_Pdm))
	{
		#if UsingQAR87Board == 1
			MclkFreq=BOARD_SwitchAudioFreq(16000,AmpFc1Fc3); //in case we need Amp and PDM only in standby mode
		#else
			MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
		#endif
	}

	InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
	VarBlockSharedByDspAndMcu.BtHfpFs=8000;	//dsp may check this value, need to set it to either 8000 or 16000
	InitAmpI2S(AudioFrameSizeInSamplePerCh_16KHz,    16000, 16);			//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
	InitAndStartCodec(16000, 16, MclkFreq);
	InitAudioCircularBuf(0,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

	DmaTxRxIsExpected=(
					AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp
					#if EnableMic01==1
						|AudioPdmPortsBitMapFlag_Mic01
					#endif
					#if EnableMic23==1
						|AudioPdmPortsBitMapFlag_Mic23
					#endif
					#if EnableMic45==1
						|AudioPdmPortsBitMapFlag_Mic45
					#endif
					#if EnableMic67==1
						|AudioPdmPortsBitMapFlag_Mic67
					#endif
			);

	//start dmic immediately --- but no need to start fc1 fc3 in the PDM callback
	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

	VarBlockSharedByDspAndMcu.PdmFs=16000;
	VarBlockSharedByDspAndMcu.I2SFs_Amp=16000;
	VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples=AudioFrameSizeInSamplePerCh_16KHz;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_16KHz;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;
	return;
}

void InitAudioInterface_VideoRecording(int Opt)
{
    //if everything is configured, no need to do anything, return
    if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_I2SToNvt)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
        return;

    int MclkFreq;
    /* Enable clock */
    //if any audio port is NOT configured, set the clk.
    //no matter BT side is 16KHz or 8KHz, no matter A2dp source is 44.1KHz or whatever, CODEC is always 16KHz or 48KHz
    if((!AudioPortIsActive_I2SToAmp)||(!AudioPortIsActive_I2SToNvt)||!(AudioPortIsActive_Pdm))
    {
        #if UsingQAR87Board == 1
            MclkFreq=BOARD_SwitchAudioFreq(48000,NvtFc5Fc6_AmpFc1Fc3);
        #else
            MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
        #endif
    }

    InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
    InitAmpI2S(AudioFrameSizeInSamplePerCh_48KHz, 48000, 16);
    #if Rt685I2SToNvtIsI2SMaster==1
        InitNvtI2S(AudioFrameSizeInSamplePerCh_48KHz, NvtI2SFs_48KHz, 16);
    #else
        InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, NvtI2SFs_48KHz, 16);
    #endif

    //configure AMP
    InitAndStartCodec(48000, 16, MclkFreq);

    InitAudioCircularBuf(0,1,0);    //int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

    DmaTxRxIsExpected=(
                        AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
                        AudioI2sPortsBitMapFlag_FcTxToNvt|AudioI2sPortsBitMapFlag_FcRxFrNvt|
                        //AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
                        #if EnableMic01==1
                            AudioPdmPortsBitMapFlag_Mic01
                        #endif
                        #if EnableMic23==1
                            |AudioPdmPortsBitMapFlag_Mic23
                        #endif
                        #if EnableMic45==1
                            |AudioPdmPortsBitMapFlag_Mic45
                        #endif
                        #if EnableMic67==1
                            |AudioPdmPortsBitMapFlag_Mic67
                        #endif

                      );

    ImmediatelyStartDmicDmaChannels(0xff);  //mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

    VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
    VarBlockSharedByDspAndMcu.I2SFs_Amp=48000;
    VarBlockSharedByDspAndMcu.PdmFs=16000;
    VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
    VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

    VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples=AudioFrameSizeInSamplePerCh_16KHz;
    VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_48KHz;
    VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Nvt=AudioFrameSizeInSamplePerCh_48KHz;
    VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;

    return;
}

void InitAudioInterface_MediaPlayer(int Opt)
{
	//InitAudioInterface_AudioIoDebug(0);		//they are the same
	//if everything is configured, no need to do anything, return
	if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_I2SToNvt)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
		return;

	int MclkFreq;
	/* Enable clock */
	//if any audio port is NOT configured, set the clk.
	//no matter BT side is 16KHz or 8KHz, no matter A2dp source is 44.1KHz or whatever, CODEC is always 16KHz or 48KHz
	if((!AudioPortIsActive_I2SToAmp)||(!AudioPortIsActive_I2SToNvt)||!(AudioPortIsActive_Pdm))
	{
		#if UsingQAR87Board == 1
			MclkFreq=BOARD_SwitchAudioFreq(48000,NvtFc5Fc6_AmpFc1Fc3);
		#else
			MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
		#endif
	}

	InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
	InitAmpI2S(AudioFrameSizeInSamplePerCh_48KHz, 48000, 16);
	#if Rt685I2SToNvtIsI2SMaster==1
		InitNvtI2S(AudioFrameSizeInSamplePerCh_48KHz, NvtI2SFs_48KHz, 16);
	#else
		InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, NvtI2SFs_48KHz, 16);
	#endif

	//configure AMP
	InitAndStartCodec(48000, 16, MclkFreq);

	InitAudioCircularBuf(0,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

	DmaTxRxIsExpected=(
						AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
						AudioI2sPortsBitMapFlag_FcTxToNvt|AudioI2sPortsBitMapFlag_FcRxFrNvt|
						//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
						#if EnableMic01==1
							AudioPdmPortsBitMapFlag_Mic01
						#endif
						#if EnableMic23==1
							|AudioPdmPortsBitMapFlag_Mic23
						#endif
						#if EnableMic45==1
							|AudioPdmPortsBitMapFlag_Mic45
						#endif
						#if EnableMic67==1
							|AudioPdmPortsBitMapFlag_Mic67
						#endif

					  );

	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

	VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
	VarBlockSharedByDspAndMcu.I2SFs_Amp=48000;
	VarBlockSharedByDspAndMcu.PdmFs=16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples=AudioFrameSizeInSamplePerCh_16KHz;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_48KHz;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Nvt=AudioFrameSizeInSamplePerCh_48KHz;
	VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;
	return;
}
void InitAudioInterface_MusicPlayer(int Opt)
{
	//return;

	int fs_I2S;
	int bits_I2S;
	int frmsizeInSamples_I2S;
	int MclkFreq;

	//if everything is configured, no need to do anything, return
	if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
		return;

	#define FrmSizeInMs		8
	//if any audio port is NOT configured, set the clk.
	if((!AudioPortIsActive_I2SToAmp)||!(AudioPortIsActive_Pdm))
	{
		switch(BtA2dpFs_ProvidedFromBtStack)
		{
			case 16000:
					#if UsingQAR87Board == 1
						MclkFreq=BOARD_SwitchAudioFreq(16000,AmpFc1Fc3);
					#else
						MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
					#endif
					fs_I2S=16000;
					frmsizeInSamples_I2S=16*FrmSizeInMs;
				break;
			case 32000:
					#if UsingQAR87Board == 1
						MclkFreq=BOARD_SwitchAudioFreq(32000,AmpFc1Fc3);
					#else
						MclkFreq=BOARD_SwitchAudioFreq(32000,BtPcmFc5Fc2_CodecFc1Fc3);
					#endif
					fs_I2S=32000;
					frmsizeInSamples_I2S=32*FrmSizeInMs;
				break;
			case 48000:
			case 44100:
					#if UsingQAR87Board == 1
						MclkFreq=BOARD_SwitchAudioFreq(48000,AmpFc1Fc3);
					#else
						MclkFreq=BOARD_SwitchAudioFreq(48000,BtPcmFc5Fc2_CodecFc1Fc3);
					#endif
					fs_I2S=48000;
					frmsizeInSamples_I2S=48*FrmSizeInMs;
			break;
		}
	}

	bits_I2S=16;

	InitPdm(0xff, FrmSizeInMs*16, 16000);
	InitAmpI2S(frmsizeInSamples_I2S, fs_I2S, 16);
	InitAndStartCodec(fs_I2S, bits_I2S, MclkFreq); //smart amp MUST start after I2S clock ready

	InitAudioCircularBuf(0,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir


	if(AmpState==AmpState_UnConfigured)
	{
		//configure AMP
		//...
		//...
		//...
		//start smart amp
		//hal_amp_aw88166_left_start("Music");
		//hal_amp_aw88166_right_start("Music");
		AmpState==AmpState_ConfiguredAndSleep;
	}

	DmaTxRxIsExpected=(
						AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
	//B36932 TxToNT maybe need to enabled //AudioI2sPortsBitMapFlag_FcTxToNvt|AudioI2sPortsBitMapFlag_FcRxFrNvt|
						#if EnableMic01==1
							AudioPdmPortsBitMapFlag_Mic01
						#endif
						#if EnableMic23==1
							|AudioPdmPortsBitMapFlag_Mic23
						#endif
						#if EnableMic45==1
							|AudioPdmPortsBitMapFlag_Mic45
						#endif
						#if EnableMic67==1
							|AudioPdmPortsBitMapFlag_Mic67
						#endif
					  );
	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

	VarBlockSharedByDspAndMcu.I2SFs_Amp=fs_I2S;
	VarBlockSharedByDspAndMcu.PdmFs=16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=frmsizeInSamples_I2S;

	VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
	VarBlockSharedByDspAndMcu.PlaySbcFileIdx=0xffff;		//0xffff stands for a2dp sbc stream
	VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;
	return;
}
void InitAudioInterface_Translation(int Opt)
{
    //if everything is configured, no need to do anything, return
    if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_I2SToNvt)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
        return;

    int MclkFreq;
    /* Enable clock */
    //if any audio port is NOT configured, set the clk.
    //no matter BT side is 16KHz or 8KHz, no matter A2dp source is 44.1KHz or whatever, CODEC is always 16KHz or 48KHz
    if((!AudioPortIsActive_I2SToAmp)||(!AudioPortIsActive_I2SToNvt)||!(AudioPortIsActive_Pdm))
    {
        #if UsingQAR87Board == 1
            MclkFreq=BOARD_SwitchAudioFreq(48000,NvtFc5Fc6_AmpFc1Fc3);
        #else
            MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
        #endif
    }

    InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
    InitAmpI2S(AudioFrameSizeInSamplePerCh_48KHz, 48000, 16);
    #if Rt685I2SToNvtIsI2SMaster==1
        InitNvtI2S(AudioFrameSizeInSamplePerCh_48KHz, NvtI2SFs_48KHz, 16);
    #else
        InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, NvtI2SFs_48KHz, 16);
    #endif

    //configure AMP
    InitAndStartCodec(48000, 16, MclkFreq);

    InitAudioCircularBuf(0,1,0);    //int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

    // May not needed
    if(AmpState==AmpState_UnConfigured)
    {
        AmpState==AmpState_ConfiguredAndSleep;
    }

    DmaTxRxIsExpected=(
                        AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
                        AudioI2sPortsBitMapFlag_FcTxToNvt|AudioI2sPortsBitMapFlag_FcRxFrNvt|
                        //AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
                        #if EnableMic01==1
                            AudioPdmPortsBitMapFlag_Mic01
                        #endif
                        #if EnableMic23==1
                            |AudioPdmPortsBitMapFlag_Mic23
                        #endif
                        #if EnableMic45==1
                            |AudioPdmPortsBitMapFlag_Mic45
                        #endif
                        #if EnableMic67==1
                            |AudioPdmPortsBitMapFlag_Mic67
                        #endif

                      );

    ImmediatelyStartDmicDmaChannels(0xff);  //mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

    VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
    VarBlockSharedByDspAndMcu.I2SFs_Amp=48000;
    VarBlockSharedByDspAndMcu.PdmFs=16000;
    VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
    VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

    VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples=AudioFrameSizeInSamplePerCh_16KHz;
    VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_48KHz;
    VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Nvt=AudioFrameSizeInSamplePerCh_48KHz;
    VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;

    // Add for SBC
    VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
    VarBlockSharedByDspAndMcu.PlaySbcFileIdx=0xffff;        //0xffff stands for a2dp sbc stream

    return;
}

void InitAudioInterface_AiConversation(int Opt)
{
	VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_NearEnd;
}

void InitAudioInterface_VideoAi(int Opt)
{
    //if everything is configured, no need to do anything, return
    if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_I2SToNvt)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
        return;

    int MclkFreq;
    /* Enable clock */
    //if any audio port is NOT configured, set the clk.
    //no matter BT side is 16KHz or 8KHz, no matter A2dp source is 44.1KHz or whatever, CODEC is always 16KHz or 48KHz
    if((!AudioPortIsActive_I2SToAmp)||(!AudioPortIsActive_I2SToNvt)||!(AudioPortIsActive_Pdm))
    {
        #if UsingQAR87Board == 1
            MclkFreq=BOARD_SwitchAudioFreq(48000,NvtFc5Fc6_AmpFc1Fc3);
        #else
            MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
        #endif
    }

    InitPdm(0xff, AudioFrameSizeInSamplePerCh_16KHz, 16000);
    InitAmpI2S(AudioFrameSizeInSamplePerCh_48KHz, 48000, 16);
    #if Rt685I2SToNvtIsI2SMaster==1
        InitNvtI2S(AudioFrameSizeInSamplePerCh_48KHz, NvtI2SFs_48KHz, 16);
    #else
        InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, NvtI2SFs_48KHz, 16);
    #endif

    //configure AMP
    InitAndStartCodec(48000, 16, MclkFreq);

    InitAudioCircularBuf(0,1,0);    //int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

    // May not needed
    if(AmpState==AmpState_UnConfigured)
    {
        AmpState==AmpState_ConfiguredAndSleep;
    }

    DmaTxRxIsExpected=(
                        AudioI2sPortsBitMapFlag_FcTxToAmp|AudioI2sPortsBitMapFlag_FcRxFrAmp|
                        AudioI2sPortsBitMapFlag_FcTxToNvt|AudioI2sPortsBitMapFlag_FcRxFrNvt|
                        //AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
                        #if EnableMic01==1
                            AudioPdmPortsBitMapFlag_Mic01
                        #endif
                        #if EnableMic23==1
                            |AudioPdmPortsBitMapFlag_Mic23
                        #endif
                        #if EnableMic45==1
                            |AudioPdmPortsBitMapFlag_Mic45
                        #endif
                        #if EnableMic67==1
                            |AudioPdmPortsBitMapFlag_Mic67
                        #endif

                      );

    ImmediatelyStartDmicDmaChannels(0xff);  //mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

    VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
    VarBlockSharedByDspAndMcu.I2SFs_Amp=48000;
    VarBlockSharedByDspAndMcu.PdmFs=16000;
    VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
    VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

    VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples=AudioFrameSizeInSamplePerCh_16KHz;
    VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Amp=AudioFrameSizeInSamplePerCh_48KHz;
    VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Nvt=AudioFrameSizeInSamplePerCh_48KHz;
    VarBlockSharedByDspAndMcu.NeedToSwitchConversaTuningCfg=ConversaTuningCfg_HfpVoiceCall;

    // Add for SBC
    VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
    VarBlockSharedByDspAndMcu.PlaySbcFileIdx=0xffff;        //0xffff stands for a2dp sbc stream

    return;
}
#endif

#if 1	//folding --- audio interface Deinit
extern void DeinitHfpVariables(void);
void Deinit_GeneralAudio(int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec)
{
	#if UsingQAR87Board == 1
		//deinit code (amplifier)
		#if 0//B36932 should be 0 and do it in DeInitCodec, 1 
			hal_amp_aw88166_left_stop();
			hal_amp_aw88166_right_stop();
		#endif
		if(ToDeinitCodec)
		{
			DeInitCodec();
		}
	#else
		//CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		if(ToDeinitCodec)
		{
			//CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);			//maybe can just set mute instead of Deinit
			DeInitCodec();
		}
	#endif

	//close all I2S and related DMA --- if need to just close the wanted, just call one of the 3 grouped functions
	if(ToDeinitAmpI2S)
		if(AudioPortIsActive_I2SToAmp)
		{
			CloseI2sDma((I2S_Type *)DEMO_I2SRxFrAmp);
			CloseI2sDma((I2S_Type *)DEMO_I2STxToAmp);
				CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2SRxFrAmp);
				CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2STxToAmp);
					ClearDmaBuf_I2SToAmp();
					ClearDmaBuf_I2SFrAmp();
			AudioPortIsActive_I2SToAmp=0;
		}

	if(ToDeinitNvtI2S)
		if(AudioPortIsActive_I2SToNvt)
		{
			CloseI2sDma((I2S_Type *)I2STxToNvtInstance);
			CloseI2sDma((I2S_Type *)I2SRxFrNvtInstance);
				CloseI2sAndI2sIntr((I2S_Type *)I2STxToNvtInstance);
				CloseI2sAndI2sIntr((I2S_Type *)I2SRxFrNvtInstance);
					ClearDmaBuf_I2STxToNvt();
					ClearDmaBuf_I2SRxFrNvt();
			AudioPortIsActive_I2SToNvt=0;
		}

	//close PDM all channels
	if(ToDeinitPdm)
		if(AudioPortIsActive_Pdm)
		{
			BOARD_DeInit_DMA_PDM(0xff);
			AudioPortIsActive_Pdm=0;
		}

	(void)BOARD_SwitchAudioFreq(0U,0);

	// During the deinit process, the SPI IRQ is disabled and needs to be re-enable
	EnableIRQ(SOC_SPI_SLAVE_IRQ);
}

void DeInitAudioInterface_AudioIoDebug(int Opt)
{

	Deinit_GeneralAudio(1,1,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(0,1,0);	//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir

	return;

	if(Opt&0b0001)
	{
		//sleep AMP ...
		AmpState=AmpState_ConfiguredAndSleep;
	}

	DeInitCodec();

	if(AudioPortIsActive_I2SToAmp)
	{
		CloseI2sDma((I2S_Type *)DEMO_I2SRxFrAmp);
		CloseI2sDma((I2S_Type *)DEMO_I2STxToAmp);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2SRxFrAmp);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2STxToAmp);
				ClearDmaBuf_I2SToAmp();
				ClearDmaBuf_I2SFrAmp();
		AudioPortIsActive_I2SToAmp=0;
	}

	if(AudioPortIsActive_I2SToNvt)
	{
		CloseI2sDma((I2S_Type *)I2STxToNvtInstance);
		CloseI2sDma((I2S_Type *)I2SRxFrNvtInstance);
			CloseI2sAndI2sIntr((I2S_Type *)I2STxToNvtInstance);
			CloseI2sAndI2sIntr((I2S_Type *)I2SRxFrNvtInstance);
				ClearDmaBuf_I2STxToNvt();
				ClearDmaBuf_I2SRxFrNvt();
		AudioPortIsActive_I2SToNvt=0;
	}

	if(AudioPortIsActive_Pdm)
	{
		BOARD_DeInit_DMA_PDM(0xff);
		AudioPortIsActive_Pdm=0;
	}

	(void)BOARD_SwitchAudioFreq(0U,0);

    CirUacUpAudioBuf_ClearAllSamples_MultiCh(&UacUpAudioBuf_MCh);
    CirUacDnAudioBuf_ClearAllSamples_MultiCh(&UacDnAudioBuf_MCh);
}
void DeInitAudioInterface_HfpCall(int Opt)
{
	Deinit_GeneralAudio(1,0,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(1,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
	DeinitHfpVariables();
}
void DeInitAudioInterface_HomeVitStandby(int Opt)
{
	Deinit_GeneralAudio(1,0,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(0,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
}

void DeInitAudioInterface_VideoRecording(int Opt)
{
    //VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=1;
    Deinit_GeneralAudio(1,1,1,1);   //int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
    ClearAudioCirBuf(0,1,0);        //int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
    return;
}

void DeInitAudioInterface_MediaPlayer(int Opt)
{
    //May be need it //VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=1;
	Deinit_GeneralAudio(1,1,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(0,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
	return;

}
void DeInitAudioInterface_MusicPlayer(int Opt)
{
	//return;
    VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=1;
	Deinit_GeneralAudio(1,0,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(0,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
	return;

	if(Opt&0b0001)
	{
		//sleep AMP ...
		AmpState=AmpState_ConfiguredAndSleep;
	}

	if(AudioPortIsActive_I2SToAmp)
	{
		CloseI2sDma((I2S_Type *)DEMO_I2SRxFrAmp);
		CloseI2sDma((I2S_Type *)DEMO_I2STxToAmp);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2SRxFrAmp);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2STxToAmp);
				ClearDmaBuf_I2SToAmp();
				ClearDmaBuf_I2SFrAmp();
		AudioPortIsActive_I2SToAmp=0;
	}

	if(AudioPortIsActive_Pdm)
	{
		BOARD_DeInit_DMA_PDM(0xff);
		AudioPortIsActive_Pdm=0;
	}

	(void)BOARD_SwitchAudioFreq(0U,0);

    CirUacUpAudioBuf_ClearAllSamples_MultiCh(&UacUpAudioBuf_MCh);
    CirUacDnAudioBuf_ClearAllSamples_MultiCh(&UacDnAudioBuf_MCh);
}
void DeInitAudioInterface_Translation(int Opt)
{
    VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=1;
    Deinit_GeneralAudio(1,1,1,1);   //int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
    ClearAudioCirBuf(0,1,0);        //int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
    return;
}
void DeInitAudioInterface_AiConversation(int Opt)
{
}
void DeInitAudioInterface_VideoAi(int Opt)
{
    VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=1;
    Deinit_GeneralAudio(1,1,1,1);   //int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
    ClearAudioCirBuf(0,1,0);        //int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
    return;
}
#endif





