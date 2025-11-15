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

#if UsingQAR87Board == 1
#include "hal_amp.h"

#endif

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
void CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(U32 MicSelectBitMap, U32 FrmSize, U32 NeedToMuteStarting)
{
	//take real mic audio input as incoming data
	#if EnableMic01 == 1
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic01)
			PdmCh0DmaTransferringIsUsingBufA=GetPdmCh0DmaTransferringIsUsingBufAOrB();
	#endif
	#if EnableMic23 == 1
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic23)
			PdmCh2DmaTransferringIsUsingBufA=GetPdmCh2DmaTransferringIsUsingBufAOrB();
	#endif
	#if EnableMic45 == 1
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic45)
			PdmCh4DmaTransferringIsUsingBufA=GetPdmCh4DmaTransferringIsUsingBufAOrB();
	#endif
	#if EnableMic67 == 1
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic67)
			PdmCh6DmaTransferringIsUsingBufA=GetPdmCh6DmaTransferringIsUsingBufAOrB();
	#endif

	#if EnableMic01 == 1
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic01)
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
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic23)
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
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic45)
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
		if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic67)
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
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic01)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=0;
				}
			#endif
			#if EnableMic23 == 1
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic23)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=0;
				}
			#endif
			#if EnableMic45 == 1
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic45)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=0;
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=0;
				}
			#endif
			#if EnableMic67 == 1
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic67)
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
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic01)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[0][i]=(MicInputCh0Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[1][i]=(MicInputCh1Ptr[i]<<8);
				}
			#endif
			#if EnableMic23 == 1
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic23)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[2][i]=(MicInputCh2Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[3][i]=(MicInputCh3Ptr[i]<<8);
				}
			#endif
			#if EnableMic45 == 1
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic45)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[4][i]=(MicInputCh4Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[5][i]=(MicInputCh5Ptr[i]<<8);
				}
			#endif
			#if EnableMic67 == 1
				if(AudioPdmPortsBitMapFlag_Mic01&AudioPdmPortsBitMapFlag_Mic67)
				{
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[6][i]=(MicInputCh6Ptr[i]<<8);
					VarBlockSharedByDspAndMcu.PdmInAudioBuf[7][i]=(MicInputCh7Ptr[i]<<8);
				}
			#endif
		}
	}
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

			SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
			PRINTF(s_currSendBuf);
			SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);
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
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh,1);

		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();

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
		//GenBtnEvt();
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
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh,1);

		/*
		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();
		 *
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
		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
		//GenBtnEvt();

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
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,AudioFrameSizeInSamplePerCh,1);

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
		//GenBtnEvt();
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
	S32 *TmpPtrS32;
	S16 i,j;

	DbgPin5Up();

	int I2SFrmSizeInSamples=VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Loc;
	int PdmFrmSizeInSamples=VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples_Loc;
	//S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];

	OSA_SR_ALLOC();

	//all needed audio src/snk tx/rx are done
	AudioIoFrameCnt++;

/*
	//to test with less things running
	MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer);		//send event to DSP, to trigger DSP MU interrupt --- after 300ms delay
	DmaTxRxIsDone=0;
	DbgPin5Dn();
	return;
*/

	#if 1	//folding --- prepare mic input pointers, and get int type of mic signal data
		//take real mic audio input as incoming data
		CopyMicAudioDataFromDmaBufferToSharedVarMicBuf(AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23,PdmFrmSizeInSamples,1);

		I2S1DmaTransferringIsUsingBufA=GetI2S1DmaTransferringIsUsingBufAOrB();
		I2S3DmaTransferringIsUsingBufA=GetI2S3DmaTransferringIsUsingBufAOrB();

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
			//UAC dn is 48KHz, I2S is 48KHz
			if(CirUacDnAudioBuf_SpaceOccupiedInSamples_MultiCh(&UacDnAudioBuf_MCh) >= I2SFrmSizeInSamples)
						{
				TmpPtrS32=(S32 *)CirUacDnAudioBuf_ReadSamples_GetRdPtr_MultiCh(&UacDnAudioBuf_MCh, I2SFrmSizeInSamples);
					#if AUDIO_OUT_FORMAT_CHANNELS==2
					for(int i=0;i<I2SFrmSizeInSamples;i++)
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
		PrintWatchToUartComAndUsbCom(AudioIoFrameCnt,10);
		//GenBtnEvt();

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

	#endif

	DmaTxRxIsDone=0;
	DbgPin5Dn();
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

#if 1	//folding --- audioflow finalization
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
						PRINTF("UacUp F\r\n");
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
void McuMainAudioFlowFinalize_AudioIoDbg(void)
{
	OSA_SR_ALLOC();
	#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
		OSA_ENTER_CRITICAL();
			//put audio data from DSP side to UacUp cir buffer
			PutAudioDataFromDspToUacUpCirBuffer(AudioFrameSizeInSamplePerCh);
		OSA_EXIT_CRITICAL();
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
	OSA_SR_ALLOC();

	int I2SFrmSizeInSamples=VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Loc;

	#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
		OSA_ENTER_CRITICAL();
			//put audio data from DSP side to UacUp cir buffer
			PutAudioDataFromDspToUacUpCirBuffer(I2SFrmSizeInSamples/3);
		OSA_EXIT_CRITICAL();
	#endif

	#if 1	//folding --- write Dsp out audio to I2S to AMP and I2S to NVT
		for(int i=0;i<I2SFrmSizeInSamples;i++)
		{
			//I2S tx to AMP
			*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufL[i];
			*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufR[i];
			//*I2SDmaOtCh01Ptr++=  0x100000*i;
			//*I2SDmaOtCh01Ptr++=0-0x100000*i;
		}
	#endif
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
		PdmInputMuteCnt=12;	//96ms or so
	}
}
void InitAmpI2S(int FrmSize, int Fs)
{
	if(!AudioPortIsActive_I2SToAmp)
	{
		BOARD_Init_DMA_I2S_Fc1();
		BOARD_Init_DMA_I2S_Fc3();
			BOARD_Init_I2S_Fc1(Fs,32);
			BOARD_Init_I2S_Fc3(Fs,32);
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
					ConfigI2S1ChainedDma(FrmSize,32);
					ConfigI2S3ChainedDma(FrmSize,32);
						EnableI2S1Rx0DmaChannel();
						EnableI2S3Tx0DmaChannel();
		AudioPortIsActive_I2SToAmp=1;
	}
}
void InitNvtI2S(int FrmSize, int Fs, int bits)
{
	if(!AudioPortIsActive_I2SToNvt)
	{
		BOARD_Init_DMA_I2S_FcTxToNt();
		BOARD_Init_DMA_I2S_FcRxFrNt();
			BOARD_Init_I2S_FcTxToNt(Fs,bits);
			BOARD_Init_I2S_FcRxFrNt(Fs,bits);
				ClearDmaBuf_I2STxToNt();
				ClearDmaBuf_I2SRxFrNt();
					ConfigI2STxToNtChainedDma(FrmSize,bits);
					ConfigI2SRxFrNtChainedDma(FrmSize,bits);
						EnableI2STxToNtDmaChannel();
						EnableI2SRxFrNtDmaChannel();
		AudioPortIsActive_I2SToNvt=1;
	}
}
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
		#if UsingQAR87Board == 1
		BOARD_SwitchAudioFreq(16000,BtPcmFc2Fc4_AmpFc1Fc3);
		#else
		BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
		#endif

#if 0
	if(!AudioPortIsActive_Pdm)
	{
		/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
		CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
		//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
		CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

		Init_MicDmaCfgCh(0xff,AudioFrameSizeInSamplePerCh_PDM,32);	//mic0,1,2,3,4,5
		BOARD_Init_DMA_PDM(0xff);
		BOARD_Init_DMIC(0xff,0,16000); //0: no skip general Dmic init. If not the first mic init, then should skip.
		ConfigDmicChainedDma(0xff);
		AudioPortIsActive_Pdm=1;
		PdmInputMuteCnt=12;			//96ms
	}
	if(!AudioPortIsActive_I2SToAmp)
	{
		BOARD_Init_DMA_I2S_Fc1();
		BOARD_Init_DMA_I2S_Fc3();
			BOARD_Init_I2S_Fc1(16000,32);
			BOARD_Init_I2S_Fc3(16000,32);
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
					ConfigI2S1ChainedDma(AudioFrameSizeInSamplePerCh,32);
					ConfigI2S3ChainedDma(AudioFrameSizeInSamplePerCh,32);
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
					#if Rt685I2SToNvtIsI2SMaster==1
						#if Rt685I2SToNvtBitWidth==16
							ConfigI2STxToNtChainedDma(AudioFrameSizeInSamplePerCh_NVT,16);
							ConfigI2SRxFrNtChainedDma(AudioFrameSizeInSamplePerCh_NVT,16);
						#endif
						#if Rt685I2SToNvtBitWidth==32
							#error : CAN not use this one --- 128*3 makes dma packet size bigger than 2048!!!
							ConfigI2STxToNtChainedDma(AudioFrameSizeInSamplePerCh_NVT,32);
							ConfigI2SRxFrNtChainedDma(AudioFrameSizeInSamplePerCh_NVT,32);
						#endif
					#else
						#if Rt685I2SToNvtBitWidth==16
							ConfigI2STxToNtChainedDma(AudioFrameSizeInSamplePerCh_I2SToNvt,16);
							ConfigI2SRxFrNtChainedDma(AudioFrameSizeInSamplePerCh_I2SToNvt,16);
						#endif
						#if Rt685I2SToNvtBitWidth==32
							//No error here, framesize is smaller now, #error : CAN not use this one --- 128*3 makes dma packet size bigger than 2048!!!
							ConfigI2STxToNtChainedDma(AudioFrameSizeInSamplePerCh_I2SToNvt,32);
							ConfigI2SRxFrNtChainedDma(AudioFrameSizeInSamplePerCh_I2SToNvt,32);
						#endif
					#endif
						EnableI2STxToNtDmaChannel();
						EnableI2SRxFrNtDmaChannel();
		AudioPortIsActive_I2SToNvt=1;
	}
#else	
	InitPdm(0xff, AudioFrameSizeInSamplePerCh_PDM, 16000);
	InitAmpI2S(AudioFrameSizeInSamplePerCh, 16000);

	#if Rt685I2SToNvtIsI2SMaster==1
		#if Rt685I2SToNvtBitWidth==16
			InitNvtI2S(AudioFrameSizeInSamplePerCh_NVT, Fs_I2SToNvt_MicSpkTest, 16);
		#endif
		#if Rt685I2SToNvtBitWidth==32
			#error : CAN not use this one --- 128*3 makes dma packet size bigger than 2048!!!
			InitNvtI2S(AudioFrameSizeInSamplePerCh_NVT/2, Fs_I2SToNvt_MicSpkTest, 32);
		#endif
	#else
		#if Rt685I2SToNvtBitWidth==16
			InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, Fs_I2SToNvt_MicSpkTest, 16);
		#endif
		#if Rt685I2SToNvtBitWidth==32
			//No error here, framesize is smaller now, #error : CAN not use this one --- 128*3 makes dma packet size bigger than 2048!!!
			InitNvtI2S(AudioFrameSizeInSamplePerCh_I2SToNvt, Fs_I2SToNvt_MicSpkTest, 32);
		#endif
	#endif
#endif

	InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

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

	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

	VarBlockSharedByDspAndMcu.I2SFs_Nvt=Fs_I2SToNvt_MicSpkTest;
	VarBlockSharedByDspAndMcu.I2SFs_Loc=16000;
	VarBlockSharedByDspAndMcu.PdmFs_Loc=16000;


	VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples_Loc=AudioFrameSizeInSamplePerCh_PDM;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Loc=AudioFrameSizeInSamplePerCh;
	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Loc=AudioFrameSizeInSamplePerCh_NVT;

	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;
}
void InitAudioInterface_HfpCall(int Opt)
{
}
void InitAudioInterface_HomeVitStandby(int Opt)
{
	//if PDM is configured, no need to do anything, return
	if(AudioPortIsActive_Pdm)
		return;
	#if UsingQAR87Board == 1
	//BOARD_SwitchAudioFreq(16000,BtPcmFc2Fc4_AmpFc1Fc3);
	BOARD_SwitchAudioFreq(16000,AmpFc1Fc3); //in case we need Amp and PDM only in standby mode
	#else
	BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
	#endif

#if 0
	/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
	CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
	//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
	CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

	Init_MicDmaCfgCh(0xff,AudioFrameSizeInSamplePerCh_PDM,32);	//mic0,1,2,3,4,5
	BOARD_Init_DMA_PDM(0xff);
	BOARD_Init_DMIC(0xff,0,16000); //0: no skip general Dmic init. If not the first mic init, then should skip.
	ConfigDmicChainedDma(0xff);
	AudioPortIsActive_Pdm=1;



	//init basic clk and PDM
	InitAudioPLLForAllAudioPeripherals();
	InitBaseAudioClkForPdm();

	VarBlockSharedByDspAndMcu.BtFs=8000;	//dsp may check this value, need to set it to either 8000 or 16000

	InitAudioCircularBuf(0,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
	PdmInputMuteCnt=12;			//96ms
#else
	InitPdm(0xff, AudioFrameSizeInSamplePerCh_PDM, 16000);
	VarBlockSharedByDspAndMcu.BtFs=8000;	//dsp may check this value, need to set it to either 8000 or 16000
	InitAudioCircularBuf(0,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
#endif

	//note: DmaTxRxIsExpected is not or ed with AudioI2sPortsBitMapFlag_Fc1 AudioI2sPortsBitMapFlag_Fc3, so after start PDM, fc1 fc3 will not be started
	DmaTxRxIsExpected=(
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

	//start dmic immediately --- but no need to start fc1 fc3 in the PDM callback
	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!

	VarBlockSharedByDspAndMcu.PdmFs_Loc=16000;
	VarBlockSharedByDspAndMcu.PdmFrmSizeInSamples_Loc=AudioFrameSizeInSamplePerCh_PDM;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;
}
void InitAudioInterface_VideoRecording(int Opt)
{
}
void InitAudioInterface_MediaPlayer(int Opt)
{
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

	/* Enable clock */
	//if any audio port is NOT configured, set the clk.

	//in this cfg: audio framesize should be 2ms, PDM is 16KHz
	#define FrmSizeInMs	2

	if((!AudioPortIsActive_I2SToAmp)||!(AudioPortIsActive_Pdm))
	{
		switch(AudioPortI2SAndPdmCfg_GetAmpI2SFs(AudioPortI2SAndPdmCfg))
		{
			case Fs_16000:
					#if UsingQAR87Board == 1
					//MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc2Fc4_AmpFc1Fc3);
					MclkFreq=BOARD_SwitchAudioFreq(16000,AmpFc1Fc3);
					#else
					MclkFreq=BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
					#endif
					fs_I2S=16000;
					frmsizeInSamples_I2S=16*FrmSizeInMs;
				break;
			case Fs_32000:
					#if UsingQAR87Board == 1
					//MclkFreq=BOARD_SwitchAudioFreq(32000,BtPcmFc2Fc4_AmpFc1Fc3);
					MclkFreq=BOARD_SwitchAudioFreq(32000,AmpFc1Fc3);
					#else
					MclkFreq=BOARD_SwitchAudioFreq(32000,BtPcmFc5Fc2_CodecFc1Fc3);
					#endif
					fs_I2S=32000;
					frmsizeInSamples_I2S=32*FrmSizeInMs;
				break;
			case Fs_44100:
					#if AmpIsAlwaysIn48Or16KHz==1
						#if UsingQAR87Board == 1
						//MclkFreq=BOARD_SwitchAudioFreq(48000,BtPcmFc2Fc4_AmpFc1Fc3);//even A2dp stream is 44100, we set I2S to 48KHz --- DSP will convert a2dp 44.1KHz to 48KHz
						MclkFreq=BOARD_SwitchAudioFreq(48000,AmpFc1Fc3);//even A2dp stream is 44100, we set I2S to 48KHz --- DSP will convert a2dp 44.1KHz to 48KHz
						#else
						MclkFreq=BOARD_SwitchAudioFreq(48000,BtPcmFc5Fc2_CodecFc1Fc3);//even A2dp stream is 44100, we set I2S to 48KHz --- DSP will convert a2dp 44.1KHz to 48KHz
						#endif
						fs_I2S=48000;
						frmsizeInSamples_I2S=48*FrmSizeInMs;	//use the same sample numbers as 48KHz, frame duration is a little bit more than 1ms: 1.088ms
					#else
						#if UsingQAR87Board == 1
						//MclkFreq=BOARD_SwitchAudioFreq(44100,BtPcmFc2Fc4_AmpFc1Fc3);//even A2dp stream is 44100, we set I2S to 48KHz --- DSP will convert a2dp 44.1KHz to 48KHz
						MclkFreq=BOARD_SwitchAudioFreq(44100,AmpFc1Fc3);//even A2dp stream is 44100, we set I2S to 48KHz --- DSP will convert a2dp 44.1KHz to 48KHz
						#else
						MclkFreq=BOARD_SwitchAudioFreq(44100,BtPcmFc5Fc2_CodecFc1Fc3);//even A2dp stream is 44100, we set I2S to 48KHz --- DSP will convert a2dp 44.1KHz to 48KHz
						#endif
						fs_I2S=44100;
						frmsizeInSamples_I2S=48*FrmSizeInMs;	//use the same sample numbers as 48KHz, frame duration is a little bit more than 1ms: 1.088ms
					#endif
				break;
			case Fs_48000:
					#if UsingQAR87Board == 1
					//MclkFreq=BOARD_SwitchAudioFreq(48000,BtPcmFc2Fc4_AmpFc1Fc3);
					MclkFreq=BOARD_SwitchAudioFreq(48000,AmpFc1Fc3);
					#else
					MclkFreq=BOARD_SwitchAudioFreq(48000,BtPcmFc5Fc2_CodecFc1Fc3);
					#endif
					fs_I2S=48000;
					frmsizeInSamples_I2S=48*FrmSizeInMs;
			break;
		}
	}

	switch(AudioPortI2SAndPdmCfg_GetAmpI2SBit(AudioPortI2SAndPdmCfg))
	{
		case BitWidth_16:
			bits_I2S=16;
			break;
		case BitWidth_32:
			bits_I2S=32;
			break;
	}

#if UsingQAR87Board == 1
	InitPdm(0xff, FrmSizeInMs*16, 16000);
	InitAmpI2S(frmsizeInSamples_I2S, fs_I2S);
	InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
	InitAndStartCodec(fs_I2S, bits_I2S, MclkFreq); //smart amp MUST start after I2S clock ready
#else
	InitAndStartCodec(fs_I2S, bits_I2S, MclkFreq);
	InitPdm(0xff, FrmSizeInMs*16, 16000);
	InitAmpI2S(frmsizeInSamples_I2S, fs_I2S);
	InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
#endif


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
						AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|
	//B36932 TxToNT maybe need to enabled //AudioI2sPortsBitMapFlag_FcTxToNt|AudioI2sPortsBitMapFlag_FcRxFrNt|
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

	VarBlockSharedByDspAndMcu.I2SFs_Nvt=Fs_I2SToNvt_MicSpkTest;
	VarBlockSharedByDspAndMcu.I2SFs_Loc=fs_I2S;
	VarBlockSharedByDspAndMcu.PdmFs_Loc=16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	VarBlockSharedByDspAndMcu.I2SFrmSizeInSamples_Loc=frmsizeInSamples_I2S;

	VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
	VarBlockSharedByDspAndMcu.PlaySbcFileIdx=0xffff;		//0xffff stands for a2dp sbc stream
}
void InitAudioInterface_Translation(int Opt)
{
}
void InitAudioInterface_AiConversation(int Opt)
{
}
void InitAudioInterface_VideoAi(int Opt)
{
}
#endif

#if 1	//folding --- audio interface Deinit
void Deinit_GeneralAudio(int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec)
{
#if UsingQAR87Board == 1
	//deinit code (amplifier)
#if 0
	if (codec_inited == 0)
	{
		return ;
	}
		hal_amp_aw88166_left_stop();
		hal_amp_aw88166_right_stop();
#endif
	if(ToDeinitCodec)
	{
		DeInitCodec();
	}
	//to do .... codec mute
#else
	//CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
	if(ToDeinitCodec)
	{
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
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
		AudioPortIsActive_I2SToAmp=0;
	}

	if(ToDeinitNvtI2S)
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

	//close PDM all channels
	if(ToDeinitPdm)
	if(AudioPortIsActive_Pdm)
	{
		BOARD_DeInit_DMA_PDM(0xff);
		AudioPortIsActive_Pdm=0;
	}

	(void)BOARD_SwitchAudioFreq(0U,0);
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
void DeInitAudioInterface_HfpCall(int Opt)
{
	Deinit_GeneralAudio(1,0,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(1,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
}
void DeInitAudioInterface_HomeVitStandby(int Opt)
{
	Deinit_GeneralAudio(0,0,1,0);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(0,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
}
void DeInitAudioInterface_VideoRecording(int Opt)
{
}
void DeInitAudioInterface_MediaPlayer(int Opt)
{
}
void DeInitAudioInterface_MusicPlayer(int Opt)
{
	//return;
    VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=1;
	Deinit_GeneralAudio(1,0,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(0,1,1);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
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
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
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
	}
void DeInitAudioInterface_AiConversation(int Opt)
		{
		}
void DeInitAudioInterface_VideoAi(int Opt)
		{
		}
	#endif





