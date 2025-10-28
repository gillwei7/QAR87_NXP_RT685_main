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

#if EnableConversa==1

#if 1	//folding

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


int UartComReportValue_S32_1;
int UartComReportValue_S32_2;
int UartComReportValue_S32_3;
int UartComReportValue_S32_4;

int UsbComReportValue_S32_1;
int UsbComReportValue_S32_2;
int UsbComReportValue_S32_3;
int UsbComReportValue_S32_4;

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

extern uint32_t AODOfUacDnBuf;
extern U32 AudioIoFrameCntForMuteMicInputAtStartingUp;
extern volatile unsigned int s_sendSize;

U32 AudioIoFrameCnt=0;

volatile S32 *I2SDmaOtCh01Ptr;		//to AMP
volatile S32 *I2SDmaInCh01Ptr;		//from AMP

volatile void *I2SDmaOtCh23Ptr;		//to NVT
volatile void *I2SDmaInCh23Ptr;		//from NVT

extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;
extern U32 PdmInputMuteCnt;
extern int UacDnAOD_ForFbAdjust;

extern uint32_t USBAudio_FeedBackEp_feedbackValue;

#endif

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

			SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
			PRINTF(s_currSendBuf);
			SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
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

//main local PDMI2S audio flow
__attribute__((section("CodeQuickAccess")))
 void ProcessAudio_AfterAudioInputBufIsReady_InCall(void)
{
	S32 *TmpPtrS32;
	S16 i,j;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	AudioIoFrameCnt++;
	DbgPin5Up();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data
#if EnableMic01 == 1
		PdmCh0DmaTransferringIsUsingBufA=GetPdmCh0DmaTransferringIsUsingBufAOrB();
#endif
#if EnableMic23 == 1
		PdmCh2DmaTransferringIsUsingBufA=GetPdmCh2DmaTransferringIsUsingBufAOrB();
#endif
#if EnableMic45 == 1
		PdmCh4DmaTransferringIsUsingBufA=GetPdmCh4DmaTransferringIsUsingBufAOrB();
#endif
#if EnableMic67 == 1
		PdmCh6DmaTransferringIsUsingBufA=GetPdmCh6DmaTransferringIsUsingBufAOrB();
#endif

		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();
#if EnableMic01 == 1
		if(PdmCh0DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 1 * AudioFrameSizeInSamplePerCh;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 0 * AudioFrameSizeInSamplePerCh;
		}
#endif
#if EnableMic23 == 1
		if(PdmCh2DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 1 * AudioFrameSizeInSamplePerCh;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 0 * AudioFrameSizeInSamplePerCh;
		}
#endif
#if EnableMic45 == 1
		if(PdmCh4DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 1 * AudioFrameSizeInSamplePerCh;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 0 * AudioFrameSizeInSamplePerCh;
		}
#endif
#if EnableMic67 == 1
		if(PdmCh6DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 1 * AudioFrameSizeInSamplePerCh;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 0 * AudioFrameSizeInSamplePerCh;
		}
#endif
		//left shift 8 bits to have mic signal reach the full scale --- raw data is 24 bit effective located in the lower 24bits
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
#if EnableMic01 == 1
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=(MicInputCh0Ptr[i]<<8);
			//VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=i*0x100000;
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=(MicInputCh1Ptr[i]<<8);
#endif
#if EnableMic23 == 1
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=(MicInputCh2Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=(MicInputCh3Ptr[i]<<8);
#endif
#if EnableMic45 == 1
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=(MicInputCh4Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=(MicInputCh5Ptr[i]<<8);
#endif
#if EnableMic67 == 1
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=(MicInputCh6Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=(MicInputCh7Ptr[i]<<8);
#endif
		}

		if(I2S1DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_A;
		}
		if(I2S3DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_A;
		}
	#endif

	#if 1	//folding --- get I2S1,2 input and put I2S1,2 output
		//copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			VarBlockSharedByDspAndMcu.I2SLineInBufL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SLineInBufR[i]=*I2SDmaInCh01Ptr++;

			//the next 2 lines are moved to MU_Rx event
			//*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufL[i];	//stream out the audio of the previous processed audio block
			//*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufR[i];	//stream out the audio of the previous processed audio block
		}
	#endif

	#if 1	//folding --- get audio data from BT Dn cir buffer
		if(VarBlockSharedByDspAndMcu.BtFs==8000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//take audio samples out from BT Dn buffer --- put to BTRxInAudio, and then DSP side will take in and process
				if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/2))
				{
					//there are enough audio sample from BT down streaming
					CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh/2, TmpAudioS16Buf);
					for(i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
						VarBlockSharedByDspAndMcu.BTRxInAudio[i]=(TmpAudioS16Buf[i]<<16);
				}else
				{
					//not enough audio samples from BT down streaming
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Up CirBuf is F \r\n");
					#endif
					memset((void *)VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio)/2);
				}
				OSA_EXIT_CRITICAL();
			#endif

			#if 0
				OSA_ENTER_CRITICAL();
				//put audio samples into from BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
				if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/2))
				{
					//there are enough free space from BT up streaming
					for(i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
						TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.BTTxOtAudio[i]>>16);
					CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh/2, TmpAudioS16Buf);
				}else
				{
					//not enough free space for BT UP streaming --- abandon the current frame of BTTxOtAudio
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Up CirBuf is F \r\n");
					#endif
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		if(VarBlockSharedByDspAndMcu.BtFs==16000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//take audio samples out from BT Dn buffer --- put to BTRxInAudio, and then DSP side will take in and process
				if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/1))
				{
					//there are enough audio sample from BT down streaming
					CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh/1, TmpAudioS16Buf);
					for(i=0;i<AudioFrameSizeInSamplePerCh/1;i++)
						VarBlockSharedByDspAndMcu.BTRxInAudio[i]=(TmpAudioS16Buf[i]<<16);
				}else
				{
					//not enough audio samples from BT down streaming
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Dn CirBuf is E \r\n");
					#endif
					memset((void *)VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio));
				}
				OSA_EXIT_CRITICAL();
			#endif

			#if 0
				OSA_ENTER_CRITICAL();
				//put audio samples into from BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
				if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/1))
				{
					//there are enough free space from BT up streaming
					for(i=0;i<AudioFrameSizeInSamplePerCh/1;i++)
						TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.BTTxOtAudio[i]>>16);
					CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh/1, TmpAudioS16Buf);
				}else
				{
					//not enough free space for BT UP streaming --- abandon the current frame of BTTxOtAudio
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Up CirBuf is F \r\n");
					#endif
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		{
			//should never come here
			PRINTF("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}
	#endif

#if EnableUsbComAndAudio==1
	#if 1	//folding --- get audio data from UAC Dn cir buffer
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
			if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= AudioFrameSizeInSamplePerCh*3)
			{
				TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, AudioFrameSizeInSamplePerCh*3);
				//PtrUsbDevComposite->audioUnified.audioSendCount += AudioFrameSizeInSamplePerCh * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE;
				//PtrUsbDevComposite->audioUnified.audioSendTimes++;
				#if AUDIO_OUT_FORMAT_CHANNELS==2
					for(int i=0;i<AudioFrameSizeInSamplePerCh*3;i++)
					{
						VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
						VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
					}
				#endif
			}else
			{	//not enough USB down streaming data --- set all to zeros
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
				#if EnableUacCirBufUnderflowOverFlowPrint==1
					PRINTF("UacDn E\r\n");
				#endif
			}
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
				//uac upstreaming request is stopped
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
				//uac dnstreaming request is stopped
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
		GenBtnEvt();
	#endif


	DmaTxRxIsDone=0;
	DbgPin5Dn();
}

#if EnableVitBeforeTheCall==1
void ProcessAudio_AfterAudioInputBufIsReady_HomeVitStandBy(void)
{
	S32 *TmpPtrS32;
	S16 i,j;
	//S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	AudioIoFrameCnt++;
	DbgPin5Up();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data

		PdmCh0DmaTransferringIsUsingBufA=GetPdmCh0DmaTransferringIsUsingBufAOrB();
		PdmCh2DmaTransferringIsUsingBufA=GetPdmCh2DmaTransferringIsUsingBufAOrB();
		PdmCh4DmaTransferringIsUsingBufA=GetPdmCh4DmaTransferringIsUsingBufAOrB();
		PdmCh6DmaTransferringIsUsingBufA=GetPdmCh6DmaTransferringIsUsingBufAOrB();

		//I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		//I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();

		if(PdmCh0DmaTransferringIsUsingBufA)
		{
		#if EnableMic01==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic01==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}
		if(PdmCh2DmaTransferringIsUsingBufA)
		{
		#if EnableMic23==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic23==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}
		if(PdmCh4DmaTransferringIsUsingBufA)
		{
		#if EnableMic45==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic45==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}
		if(PdmCh6DmaTransferringIsUsingBufA)
		{
		#if EnableMic67==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic67==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}

		if(PdmInputMuteCnt)
		{
			//mute all mic input signals --- cause at PDM starting up, there might be bad audio data incoming
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[0],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[0]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[1],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[1]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[2],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[2]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[3],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[3]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[4],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[4]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[5],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[5]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[6],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[6]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[7],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[7]));
			PdmInputMuteCnt--;
		}else
		{
			//left shift 8 bits to have mic signal reach the full scale --- raw data is 24 bit effective located in the lower 24bits
			for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
			{
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=(MicInputCh0Ptr[i]<<8);
				//VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=i*0x100000;
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=(MicInputCh1Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=(MicInputCh2Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=(MicInputCh3Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=(MicInputCh4Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=(MicInputCh5Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=(MicInputCh6Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=(MicInputCh7Ptr[i]<<8);
			}
		}

		/*
		if(I2S1DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_A;
		}
		if(I2S3DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_A;
		}
		*/
	#endif

	#if 0	//folding --- get I2S1,2 input and put I2S1,2 output
		//copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			VarBlockSharedByDspAndMcu.I2SLineInBufL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SLineInBufR[i]=*I2SDmaInCh01Ptr++;

			//the next 2 lines are moved to MU_Rx event
			//*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufL[i];	//stream out the audio of the previous processed audio block
			//*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufR[i];	//stream out the audio of the previous processed audio block
		}
	#endif

	#if 0	//folding --- read and write BT cir buffer
		if(VarBlockSharedByDspAndMcu.BtFs==8000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//take audio samples out from BT Dn buffer --- put to BTRxInAudio, and then DSP side will take in and process
				if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/2))
				{
					//there are enough audio sample from BT down streaming
					CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh/2, TmpAudioS16Buf);
					for(i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
						VarBlockSharedByDspAndMcu.BTRxInAudio[i]=(TmpAudioS16Buf[i]<<16);
				}else
				{
					//not enough audio samples from BT down streaming
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Up CirBuf is F \r\n");
					#endif
					memset(VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio)/2);
				}
				OSA_EXIT_CRITICAL();
			#endif

			#if 0
				OSA_ENTER_CRITICAL();
				//put audio samples into from BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
				if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/2))
				{
					//there are enough free space from BT up streaming
					for(i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
						TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.BTTxOtAudio[i]>>16);
					CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh/2, TmpAudioS16Buf);
				}else
				{
					//not enough free space for BT UP streaming --- abandon the current frame of BTTxOtAudio
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Up CirBuf is F \r\n");
					#endif
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		if(VarBlockSharedByDspAndMcu.BtFs==16000)
		{
			#if 1
				OSA_ENTER_CRITICAL();
				//take audio samples out from BT Dn buffer --- put to BTRxInAudio, and then DSP side will take in and process
				if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/1))
				{
					//there are enough audio sample from BT down streaming
					CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh/1, TmpAudioS16Buf);
					for(i=0;i<AudioFrameSizeInSamplePerCh/1;i++)
						VarBlockSharedByDspAndMcu.BTRxInAudio[i]=(TmpAudioS16Buf[i]<<16);
				}else
				{
					//not enough audio samples from BT down streaming
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Dn CirBuf is E \r\n");
					#endif
					memset(VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio));
				}
				OSA_EXIT_CRITICAL();
			#endif

			#if 0
				OSA_ENTER_CRITICAL();
				//put audio samples into from BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
				if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/1))
				{
					//there are enough free space from BT up streaming
					for(i=0;i<AudioFrameSizeInSamplePerCh/1;i++)
						TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.BTTxOtAudio[i]>>16);
					CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh/1, TmpAudioS16Buf);
				}else
				{
					//not enough free space for BT UP streaming --- abandon the current frame of BTTxOtAudio
					//this should not happen when audio PLL sync is doing well
					#if EnableBtCirBufUnderflowOverFlowPrint==1
						PRINTF("BT Up CirBuf is F \r\n");
					#endif
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		{
			//should never come here
			PRINTF("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}
	#else
		memset((void *)VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio));
	#endif


#if EnableUsbComAndAudio==1
	#if 1	//folding --- get audio data from UAC Dn cir buffer
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
			if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= AudioFrameSizeInSamplePerCh*3)
			{
				TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, AudioFrameSizeInSamplePerCh*3);
				//PtrUsbDevComposite->audioUnified.audioSendCount += AudioFrameSizeInSamplePerCh * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE;
				//PtrUsbDevComposite->audioUnified.audioSendTimes++;
				#if AUDIO_OUT_FORMAT_CHANNELS==2
					for(int i=0;i<AudioFrameSizeInSamplePerCh*3;i++)
					{
						VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
						VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
					}
				#endif
			}else
			{	//not enough USB down streaming data --- set all to zeros
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
				#if EnableUacCirBufUnderflowOverFlowPrint==1
					PRINTF("UacDn E\r\n");
				#endif
			}
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
				//uac upstreaming request is stopped
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
				//uac dnstreaming request is stopped
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
#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, EvtFlag_AudioFrameIsReadyToProcessVit);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
		GenBtnEvt();
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
#endif

#if 1
void ProcessAudio_AfterAudioInputBufIsReady_AudioIoDbg(void)
{
	S32 *TmpPtrS32;
	S16 i,j;
	//S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	AudioIoFrameCnt++;
	DbgPin5Up();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data

		PdmCh0DmaTransferringIsUsingBufA=GetPdmCh0DmaTransferringIsUsingBufAOrB();
		PdmCh2DmaTransferringIsUsingBufA=GetPdmCh2DmaTransferringIsUsingBufAOrB();
		PdmCh4DmaTransferringIsUsingBufA=GetPdmCh4DmaTransferringIsUsingBufAOrB();
		PdmCh6DmaTransferringIsUsingBufA=GetPdmCh6DmaTransferringIsUsingBufAOrB();

		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();

		I2STxToNtDmaTransferringIsUsingBufA=GetI2STxToNtDmaTransferringIsUsingBufAOrB();
		I2SRxFrNtDmaTransferringIsUsingBufA=GetI2SRxFrNtDmaTransferringIsUsingBufAOrB();

		if(PdmCh0DmaTransferringIsUsingBufA)
		{
		#if EnableMic01==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic01==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}
		if(PdmCh2DmaTransferringIsUsingBufA)
		{
		#if EnableMic23==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic23==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}
		if(PdmCh4DmaTransferringIsUsingBufA)
		{
		#if EnableMic45==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic45==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}
		if(PdmCh6DmaTransferringIsUsingBufA)
		{
		#if EnableMic67==1
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 1 * AudioFrameSizeInSamplePerCh;
		#endif
		}else
		{
		#if EnableMic67==1
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 0 * AudioFrameSizeInSamplePerCh;
		#endif
		}

		if(PdmInputMuteCnt)
		{
			//mute all mic input signals --- cause at PDM starting up, there might be bad audio data incoming
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[0],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[0]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[1],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[1]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[2],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[2]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[3],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[3]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[4],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[4]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[5],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[5]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[6],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[6]));
			memset((void *)VarBlockSharedByDspAndMcu.PdmInAudioBuf[7],0,sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf[7]));
			PdmInputMuteCnt--;
		}else
		{
			//left shift 8 bits to have mic signal reach the full scale --- raw data is 24 bit effective located in the lower 24bits
			for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
			{
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=(MicInputCh0Ptr[i]<<8);
				//VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=i*0x100000;
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=(MicInputCh1Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=(MicInputCh2Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=(MicInputCh3Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=(MicInputCh4Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=(MicInputCh5Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=(MicInputCh6Ptr[i]<<8);
				VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=(MicInputCh7Ptr[i]<<8);
			}
		}

		if(I2S1DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_A;
		}
		if(I2S3DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_A;
		}
		if(I2STxToNtDmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh23Ptr=(void *)I2STxToNtCh0And1Mixed_A;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh23Ptr=(void *)I2STxToNtCh0And1Mixed_B;
		}
		if(I2SRxFrNtDmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh23Ptr=(void *)I2SRxFrNtCh0And1Mixed_A;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh23Ptr=(void *)I2SRxFrNtCh0And1Mixed_B;
		}
	#endif

	#if 1	//folding --- get I2S input from AMP and from Nvt, and put to shared var buffer
		//copy I2S input (line in ADC) to shared memory from DMA buffer, for DSP later use
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			VarBlockSharedByDspAndMcu.I2SLineInBufL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SLineInBufR[i]=*I2SDmaInCh01Ptr++;
		}

		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			#if Rt685I2SToNvtBitWidth==16
				#if 1
					VarBlockSharedByDspAndMcu.I2SInNvtBufL[i]=((*(S16 *)I2SDmaInCh23Ptr++)<<16);
					VarBlockSharedByDspAndMcu.I2SInNvtBufR[i]=((*(S16 *)I2SDmaInCh23Ptr++)<<16);
					#if Fs_I2SToNvt_MicSpkTest==48000
						I2SDmaInCh23Ptr+=2;
					#endif
				#else
						VarBlockSharedByDspAndMcu.I2SInNvtBufL[i]=0x20000*i;
						VarBlockSharedByDspAndMcu.I2SInNvtBufR[i]=-0x20000*i;
				#endif
			#endif
			#if Rt685I2SToNvtBitWidth==32
				VarBlockSharedByDspAndMcu.I2SInNvtBufL[i]=*(S32 *)I2SDmaInCh23Ptr++;
				VarBlockSharedByDspAndMcu.I2SInNvtBufR[i]=*(S32 *)I2SDmaInCh23Ptr++;
				#if Fs_I2SToNvt_MicSpkTest==48000
					I2SDmaInCh23Ptr+=4;
				#endif
			#endif
		}
	#endif

#if EnableUsbComAndAudio==1
	#if 1	//folding --- get audio data from UAC Dn cir buffer
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
			if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= AudioFrameSizeInSamplePerCh*3)
			{
				TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, AudioFrameSizeInSamplePerCh*3);
				//PtrUsbDevComposite->audioUnified.audioSendCount += AudioFrameSizeInSamplePerCh * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE;
				//PtrUsbDevComposite->audioUnified.audioSendTimes++;
				#if AUDIO_OUT_FORMAT_CHANNELS==2
					for(int i=0;i<AudioFrameSizeInSamplePerCh*3;i++)
					{
						VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
						VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
					}
				#endif
			}else
			{	//not enough USB down streaming data --- set all to zeros
				#if 0
					memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
					memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
				#else
					//weird: if use memset 0 to clear UAC dn buffer --- it may half the task that runs here
					for(int i=0;i<AudioFrameSizeInSamplePerCh*3;i++)
					{
						VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=0;
						VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=0;
					}
				#endif
				#if EnableUacCirBufUnderflowOverFlowPrint==1
					PRINTF("UacDn E\r\n");
				#endif
			}
		}else
		{	//UAC down buffer is not half full yet, --- set all to zeros
			#if 0
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
				memset((void *)VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
			#else
				//weird: if use memset 0 to clear UAC dn buffer --- it may half the task that runs here
				for(int i=0;i<AudioFrameSizeInSamplePerCh*3;i++)
				{
					VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=0;
					VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=0;
				}
			#endif
		}
	#endif

	#if 1	//folding --- check UAC streaming stop and clear UAC cir buffer
		if(UsbUpStreamingIsStarted)
		{
			UsbUpStreamingStopMonitorCnt++;
			if(UsbUpStreamingStopMonitorCnt>200)
			{
				//uac upstreaming request is stopped
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
				//uac dnstreaming request is stopped
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
		GenBtnEvt();
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
void ProcessAudio_AfterAudioInputBufIsReady_VideoRecording(void)
{
}
void ProcessAudio_AfterAudioInputBufIsReady_MediaPlayer(void)
{
}
void ProcessAudio_AfterAudioInputBufIsReady_MusicPlayer(void)
{
}
void ProcessAudio_AfterAudioInputBufIsReady_Translation(void)
{
}
void ProcessAudio_AfterAudioInputBufIsReady_AiConversation(void)
{
}
void ProcessAudio_AfterAudioInputBufIsReady_VideoAi(void)
{
}
#endif



#if EnableOnlyMicSpk_NoBT==1
extern void USB_MicUpStreamDataRateControl_AdjustPacketLength(int AodInCirBuf);

#if EnableUsbComAndAudio==1
	#if Fs_I2SToAmp_MicSpkTest==16000
		T_MCh32BitUacUpAudioSample OneUacUpStreamFrame[AudioFrameSizeInSamplePerCh];
	#endif
	#if Fs_I2SToAmp_MicSpkTest==48000
		T_MCh32BitUacUpAudioSample OneUacUpStreamFrame[AudioFrameSizeInSamplePerCh/3];
	#endif
#endif

__attribute__((section("CodeQuickAccess")))
 void ProcessAudio_AfterAudioInputBufIsReady_TestMicSpk(void)
{
	S32 *TmpPtrS32;
	S16 i,j;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];
	S32 TmpAudioS32BufL[AudioFrameSizeInSamplePerCh];
	S32 TmpAudioS32BufR[AudioFrameSizeInSamplePerCh];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	AudioIoFrameCnt++;
	DbgPin5Up();

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data

		PdmCh0DmaTransferringIsUsingBufA=GetPdmCh0DmaTransferringIsUsingBufAOrB();
		PdmCh2DmaTransferringIsUsingBufA=GetPdmCh2DmaTransferringIsUsingBufAOrB();
		PdmCh4DmaTransferringIsUsingBufA=GetPdmCh4DmaTransferringIsUsingBufAOrB();
		PdmCh6DmaTransferringIsUsingBufA=GetPdmCh6DmaTransferringIsUsingBufAOrB();

		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();

		if(PdmCh0DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 1 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 1 * AudioFrameSizeInSamplePerCh_PDM;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 0 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 0 * AudioFrameSizeInSamplePerCh_PDM;
		}
		if(PdmCh2DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 1 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 1 * AudioFrameSizeInSamplePerCh_PDM;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh2Ptr=MicInputDmaDualBuf_2 + 0 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh3Ptr=MicInputDmaDualBuf_3 + 0 * AudioFrameSizeInSamplePerCh_PDM;
		}
		if(PdmCh4DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 1 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 1 * AudioFrameSizeInSamplePerCh_PDM;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh4Ptr=MicInputDmaDualBuf_4 + 0 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh5Ptr=MicInputDmaDualBuf_5 + 0 * AudioFrameSizeInSamplePerCh_PDM;
		}
		if(PdmCh6DmaTransferringIsUsingBufA)
		{
			//now DMA is using PDM BufA, the MCU code should use PDM DMA buffer B (the later half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 1 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 1 * AudioFrameSizeInSamplePerCh_PDM;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh6Ptr=MicInputDmaDualBuf_6 + 0 * AudioFrameSizeInSamplePerCh_PDM;
			MicInputCh7Ptr=MicInputDmaDualBuf_7 + 0 * AudioFrameSizeInSamplePerCh_PDM;
		}

		//left shift 8 bits to have mic signal reach the full scale --- raw data is 24 bit effective located in the lower 24bits
		for(int i=0;i<AudioFrameSizeInSamplePerCh_PDM;i++)
		{
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=(MicInputCh0Ptr[i]<<8);
			//VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=i*0x100000;
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=(MicInputCh1Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=(MicInputCh2Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=(MicInputCh3Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=(MicInputCh4Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=(MicInputCh5Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=(MicInputCh6Ptr[i]<<8);
			VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=(MicInputCh7Ptr[i]<<8);
		}

		if(I2S1DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_A;
		}
		if(I2S3DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh0And1Mixed_A;
		}

		//generate tone from sin calculation --- much more mips are needed, but can be flexible setting different f
		GenerateSineTone_S32_SingleCh(&SineToneGenerator2, TmpAudioS32BufL, AudioFrameSizeInSamplePerCh , 1);
		//generate tone from sin wav table --- much smaller mips, but freq is fixed at 320Hz
		GenerateSinWavFromTable_S32_SingleCh(TmpAudioS32BufR, AudioFrameSizeInSamplePerCh);
	#endif

#if EnableUsbComAndAudio==1
	#if 1	//folding --- get audio data from UAC Dn cir buffer
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
			#if Fs_I2SToAmp_MicSpkTest==16000
				if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= AudioFrameSizeInSamplePerCh*3)
				{
					TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, AudioFrameSizeInSamplePerCh*3);
					//PtrUsbDevComposite->audioUnified.audioSendCount += AudioFrameSizeInSamplePerCh * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE;
					//PtrUsbDevComposite->audioUnified.audioSendTimes++;
					#if AUDIO_OUT_FORMAT_CHANNELS==2
						for(int i=0;i<AudioFrameSizeInSamplePerCh*3;i++)
						{
							VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
							VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
						}
					#endif
				}else
			#endif
			#if Fs_I2SToAmp_MicSpkTest==48000
				if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= AudioFrameSizeInSamplePerCh)
				{
					TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, AudioFrameSizeInSamplePerCh);
					//PtrUsbDevComposite->audioUnified.audioSendCount += AudioFrameSizeInSamplePerCh * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE;
					//PtrUsbDevComposite->audioUnified.audioSendTimes++;
					#if AUDIO_OUT_FORMAT_CHANNELS==2
						for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
						{
							VarBlockSharedByDspAndMcu.UacDnAudioBufL[i]=*TmpPtrS32++;
							VarBlockSharedByDspAndMcu.UacDnAudioBufR[i]=*TmpPtrS32++;
						}
					#endif
				}else
			#endif
			{	//not enough USB down streaming data --- set all to zeros
				memset(VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
				memset(VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
				#if EnableUacCirBufUnderflowOverFlowPrint==1
					PRINTF("UacDn E\r\n");
				#endif
			}
		}else
		{	//UAC down buffer is not half full yet, --- set all to zeros
			memset(VarBlockSharedByDspAndMcu.UacDnAudioBufL,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufL));
			memset(VarBlockSharedByDspAndMcu.UacDnAudioBufR,0,sizeof(VarBlockSharedByDspAndMcu.UacDnAudioBufR));
		}
	#endif

	#if 1	//folding --- prepare UAC up streaming buffer and write to UAC up streaming cir buffer
		//put raw mic signal to UacUp cir buffer
		#if Fs_I2SToAmp_MicSpkTest==16000
			//both I2S and PDM are 16KHzm --- copy samples 1 to 1, no need to decimate
			for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
			{
				OneUacUpStreamFrame[i].s[0]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i];
				OneUacUpStreamFrame[i].s[1]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i];
				OneUacUpStreamFrame[i].s[2]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i];
				OneUacUpStreamFrame[i].s[3]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i];
				//OneUacUpStreamFrame[i].s[4]=VarBlockSharedByDspAndMcu.I2SLineInBufL[i];
				//OneUacUpStreamFrame[i].s[5]=VarBlockSharedByDspAndMcu.I2SLineInBufR[i];
					OneUacUpStreamFrame[i].s[4]=VarBlockSharedByDspAndMcu.UacDnAudioBufL[i*3];
					OneUacUpStreamFrame[i].s[5]=VarBlockSharedByDspAndMcu.UacDnAudioBufR[i*3];
				OneUacUpStreamFrame[i].s[6]=TmpAudioS32BufL[i];
				OneUacUpStreamFrame[i].s[7]=TmpAudioS32BufR[i];
			}
		#endif
		#if Fs_I2SToAmp_MicSpkTest==48000
			//I2S is 48KHz and PDM is 16KHz --- need to decimate I2S samples
			for(int i=0;i<AudioFrameSizeInSamplePerCh/3;i++)
			{
				OneUacUpStreamFrame[i].s[0]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i];
				OneUacUpStreamFrame[i].s[1]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i];
				OneUacUpStreamFrame[i].s[2]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i];
				OneUacUpStreamFrame[i].s[3]=VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i];
				//OneUacUpStreamFrame[i].s[4]=VarBlockSharedByDspAndMcu.I2SLineInBufL[i];
				//OneUacUpStreamFrame[i].s[5]=VarBlockSharedByDspAndMcu.I2SLineInBufR[i];
					OneUacUpStreamFrame[i].s[4]=VarBlockSharedByDspAndMcu.UacDnAudioBufL[i*3];
					OneUacUpStreamFrame[i].s[5]=VarBlockSharedByDspAndMcu.UacDnAudioBufR[i*3];
				OneUacUpStreamFrame[i].s[6]=TmpAudioS32BufL[i*3];
				OneUacUpStreamFrame[i].s[7]=TmpAudioS32BufR[i*3];
			}
		#endif

		int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
		OSA_ENTER_CRITICAL();
			AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh);
			AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
			if(UsbUpStreamingIsStarted)
			{
				#if Fs_I2SToAmp_MicSpkTest==16000
					if (AOFSOfUacUpBuf >= (AudioFrameSizeInSamplePerCh))
					{
						CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, AudioFrameSizeInSamplePerCh, OneUacUpStreamFrame);
					}
					else
					{
						//PRINTF("F\r\n");
					}
				#endif
				#if Fs_I2SToAmp_MicSpkTest==48000
					if (AOFSOfUacUpBuf >= (AudioFrameSizeInSamplePerCh/3))
					{
						CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, AudioFrameSizeInSamplePerCh/3, OneUacUpStreamFrame);
					}
					else
					{
						//PRINTF("F\r\n");
					}
				#endif
			}
		OSA_EXIT_CRITICAL();

		//adjust mic upstreaming tx length
		//call adjust mic upstreaming length with AOFS just after writing the cir buffer
		if(UsbUpStreamingIsStarted)
		{
			USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+AudioFrameSizeInSamplePerCh);	//calling adjust just before writing the circular buffer
		}
	#endif

	#if 1	//folding --- check UAC streaming stop and clear UAC cir buffer
		if(UsbUpStreamingIsStarted)
		{
			UsbUpStreamingStopMonitorCnt++;
			if(UsbUpStreamingStopMonitorCnt>200)
			{
				//uac upstreaming request is stopped
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
				//uac dnstreaming request is stopped
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
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=CirUacUpAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacUpAudioBuf_MCh);
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[9]=UacDnAOD_ForFbAdjust;
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[10]=USBAudio_FeedBackEp_feedbackValue;
		OSA_EXIT_CRITICAL();
	#endif
#endif

	#if 1	//folding --- get I2S1,2 input and put I2S1,2 output
		//copy I2S input (line in ADC of CODEC) to shared memory from DMA buffer, for DSP later use
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			VarBlockSharedByDspAndMcu.I2SLineInBufL[i]=*I2SDmaInCh01Ptr++;
			VarBlockSharedByDspAndMcu.I2SLineInBufR[i]=*I2SDmaInCh01Ptr++;
		}

		//stream audio to fc3, to CODEC/AMP, fill I2S DMA Tx output buffer (line out to DAC of CODEC / AMP)
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			#if 0
				//I2S plays generated tone
				*I2SDmaOtCh01Ptr++=TmpAudioS32BufL[i];
				*I2SDmaOtCh01Ptr++=TmpAudioS32BufR[i];
			#else
				//I2S plays UAC down streaming
				#if Fs_I2SToAmp_MicSpkTest==16000
					*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.UacDnAudioBufL[i*3];
					*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.UacDnAudioBufR[i*3];
				#endif
				#if Fs_I2SToAmp_MicSpkTest==48000
					*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.UacDnAudioBufL[i];
					*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.UacDnAudioBufR[i];
				#endif
			#endif
		}
	#endif

	#if Fs_I2SToAmp_MicSpkTest==16000
		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,3);
	#endif
	#if Fs_I2SToAmp_MicSpkTest==48000
		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,1);
	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
}
#endif

#if 1	//folding --- audioflow finalization
void McuMainAudioFlowFinalize_AudioIoDbg(void)
{
	OSA_SR_ALLOC();

	#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
		OSA_ENTER_CRITICAL();
			//put audio data from DSP side to UacUp cir buffer
			int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
			AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(&UacUpAudioBuf_MCh);
			AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
			if(UsbUpStreamingIsStarted)
			{
				if (AOFSOfUacUpBuf >= (AudioFrameSizeInSamplePerCh))
				{
					CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, AudioFrameSizeInSamplePerCh, (T_MCh32BitUacUpAudioSample *)VarBlockSharedByDspAndMcu.UacUpAudioBuf);
				}
				else
				{
					#if EnableUacCirBufUnderflowOverFlowPrint==1
						PRINTF("UacUp F\r\n");
					#endif
				}
			}

			//adjust mic upstreaming tx length
			//call adjust mic upstreaming length with AOFS just after writing the cir buffer
			if(UsbUpStreamingIsStarted)
			{
				USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+AudioFrameSizeInSamplePerCh);	//calling adjust just before writing the circular buffer
			}
		OSA_EXIT_CRITICAL();

		#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
			if(UsbUpStreamingIsStarted)
				VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=AODOfUacUpBuf+AudioFrameSizeInSamplePerCh;
		#endif
	#endif

	#if 1	//folding --- write Dsp out audio to I2S to AMP and I2S to NVT
		for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
		{
			//I2S tx to AMP
			*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufL[i];
			*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufR[i];
		}
		#if Rt685I2SToNvtBitWidth==16
			S16 *SrcPtr=(S16 *)I2SDmaOtCh23Ptr;
			for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
			{
				//I2S tx to NVT
				*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufL[i]>>16;
				*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufR[i]>>16;
				#if Fs_I2SToNvt_MicSpkTest==48000
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufL[i]>>16;
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufR[i]>>16;
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufL[i]>>16;
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufR[i]>>16;
				#endif
			}
		#endif
		#if Rt685I2SToNvtBitWidth==32
			S32 *SrcPtr=(S32 *)I2SDmaOtCh23Ptr;
			for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
			{
				//I2S tx to NVT
				*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufL[i];
				*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufR[i];
				#if Fs_I2SToNvt_MicSpkTest==48000
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufL[i];
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufR[i];
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufL[i];
					*SrcPtr++=VarBlockSharedByDspAndMcu.I2SOtNvtBufR[i];
				#endif
			}
		#endif
	#endif
}
void McuMainAudioFlowFinalize_VideoRecording(void)
{
}
void McuMainAudioFlowFinalize_MediaPlayer(void)
{
}
void McuMainAudioFlowFinalize_MusicPlayer(void)
{
}
void McuMainAudioFlowFinalize_Translation(void)
{
}
void McuMainAudioFlowFinalize_AiConversation(void)
{
}
void McuMainAudioFlowFinalize_VideoAi(void)
{
}
#endif

#if 1	//folding --- audio interface init
extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx);
void InitAudioInterface_AudioIoDebug(int Opt)
{
	//return;

	//if everything is configured, no need to do anything, return
	if((AudioPortIsActive_I2SToAmp)&&(AudioPortIsActive_I2SToNvt)&&(AudioPortIsActive_Pdm)&&(AmpState>AmpState_UnConfigured))
		return;

	/* Enable clock */
	//if any audio port is NOT configured, set the clk.
	//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
	if((!AudioPortIsActive_I2SToAmp)||(!AudioPortIsActive_I2SToNvt)||!(AudioPortIsActive_Pdm))
		BOARD_SwitchAudioFreq(16000,0);

	if(!AudioPortIsActive_Pdm)
	{
		/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
		CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
		//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
		CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

		Init_MicDmaCfgCh(0xff);	//mic0,1,2,3,4,5
		BOARD_Init_DMA_PDM(0xff);
		BOARD_Init_DMIC(0xff,0); //0: no skip general Dmic init. If not the first mic init, then should skip.
		ConfigDmicChainedDma(0xff);
		AudioPortIsActive_Pdm=1;
	}
	if(!AudioPortIsActive_I2SToAmp)
	{
		BOARD_Init_DMA_I2S_Fc1();
		BOARD_Init_DMA_I2S_Fc3();
			BOARD_Init_I2S_Fc1();
			BOARD_Init_I2S_Fc3();
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
					ConfigI2S1ChainedDma();
					ConfigI2S3ChainedDma();
						EnableI2S1Rx0DmaChannel();
						EnableI2S3Tx0DmaChannel();
		AudioPortIsActive_I2SToAmp=1;
	}
	if(!AudioPortIsActive_I2SToNvt)
	{
		BOARD_Init_DMA_I2S_FcTxToNt();
		BOARD_Init_DMA_I2S_FcRxFrNt();
			BOARD_Init_I2S_FcTxToNt();
			BOARD_Init_I2S_FcRxFrNt();
				ClearDmaBuf_I2STxToNt();
				ClearDmaBuf_I2SRxFrNt();
					ConfigI2STxToNtChainedDma();
					ConfigI2SRxFrNtChainedDma();
						EnableI2STxToNtDmaChannel();
						EnableI2SRxFrNtDmaChannel();
		AudioPortIsActive_I2SToNvt=1;
	}

	InitAudioCircularBuf();

	if(AmpState==AmpState_UnConfigured)
	{
		//configure AMP
		//...
		//...
		//...
		AmpState==AmpState_ConfiguredAndSleep;
	}

	DmaTxRxIsExpected=(
						AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|
						AudioI2sPortsBitMapFlag_FcTxToNt|AudioI2sPortsBitMapFlag_FcRxFrNt|
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


}

void DeInitAudioInterface_AudioIoDebug(int Opt)
{
	//return;

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
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
		AudioPortIsActive_I2SToAmp=0;
	}
	if(AudioPortIsActive_I2SToNvt)
	{
		CloseI2sDma((I2S_Type *)I2STxToNtInstance);
		CloseI2sDma((I2S_Type *)I2SRxFrNtInstance);
			CloseI2sAndI2sIntr((I2S_Type *)I2STxToNtInstance);
			CloseI2sAndI2sIntr((I2S_Type *)I2SRxFrNtInstance);
				ClearDmaBuf_I2STxToNt();
				ClearDmaBuf_I2SRxFrNt();
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


void InitAudioInterface_HfpCall(int Opt)
{
}
void DeInitAudioInterface_HfpCall(int Opt)
{
}
void InitAudioInterface_HomeVitStandby(int Opt)
{
}
void DeInitAudioInterface_HomeVitStandby(int Opt)
{
	Deinit_Board_Audio();
}
void InitAudioInterface_VideoRecording(int Opt)
{
}
void DeInitAudioInterface_VideoRecording(int Opt)
{
}
void InitAudioInterface_MediaPlayer(int Opt)
{
}
void DeInitAudioInterface_MediaPlayer(int Opt)
{
}
void InitAudioInterface_MusicPlayer(int Opt)
{
}
void DeInitAudioInterface_MusicPlayer(int Opt)
{
}
void InitAudioInterface_Translation(int Opt)
{
}
void DeInitAudioInterface_Translation(int Opt)
{
}
void InitAudioInterface_AiConversation(int Opt)
{
}
void DeInitAudioInterface_AiConversation(int Opt)
{
}
void InitAudioInterface_VideoAi(int Opt)
{
}
void DeInitAudioInterface_VideoAi(int Opt)
{
}
#endif



#if 0
void CloseAllActivedPDMPorts(void)
{
	//close active audio ports --- PDM ports
	//if(ActivedPdmPorts&AudioPdmPortsBitMapFlag_Mic01)
	{
		BOARD_DeInit_DMA_PDM(0x03);
	}
	//if(ActivedPdmPorts&AudioPdmPortsBitMapFlag_Mic23)
	{
		BOARD_DeInit_DMA_PDM(0x0c);
	}
	#if EnableMic45==1
		if(ActivedPdmPorts&AudioPdmPortsBitMapFlag_Mic45)
		{
			BOARD_DeInit_DMA_PDM(0x30);
		}
	#endif
	#if EnableMic67==1
		if(ActivedPdmPorts&AudioPdmPortsBitMapFlag_Mic67)
		{
			BOARD_DeInit_DMA_PDM(0xc0);
		}
	#endif

}
void CloseAllActivedI2SPorts(void)
{
	//close active audio ports --- I2S in ports
	//if(ActivedI2sPorts&AudioI2sPortsBitMapFlag_Fc3)
	{
    	CloseI2sDma((I2S_Type *)DEMO_I2STxToAmp);
    	CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2STxToAmp);
    	ClearDmaBuf_I2S3Tx0();
	}

	//close active audio ports --- I2S out ports
	//if(ActivedI2sPorts&AudioI2sPortsBitMapFlag_Fc1)
	{
    	CloseI2sDma((I2S_Type *)DEMO_I2SRxFrAmp);
    	CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2SRxFrAmp);
    	ClearDmaBuf_I2S1Rx0();
	}
	/*
	if(ActivedI2sPorts&AudioI2sPortsBitMapFlag_Fc2)
	{
    	CloseI2sDma((I2S_Type *)DEMO_I2S2_RX0);
    	CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2S2_RX0);
    	ClearDmaBuf_I2S2_Rx0();
	}
	if(ActivedI2sPorts&AudioI2sPortsBitMapFlag_Fc4)
	{
    	CloseI2sDma((I2S_Type *)DEMO_I2S4_TX1);
    	CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2S4_TX1);
    	ClearDmaBuf_I2S4_Tx1();
	}
	*/
}
#endif

#endif


