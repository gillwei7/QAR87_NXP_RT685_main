
#include "qar87_config.h"

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

#include "SubFunc.h"
#include "AudioIoCfg_I2s.h"
#include "AudioIoCfg_Pdm.h"
#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "MainAudioFlow.h"

int UartComReportValue_S32_1;
int UartComReportValue_S32_2;
int UartComReportValue_S32_3;
int UartComReportValue_S32_4;

int UsbComReportValue_S32_1;
int UsbComReportValue_S32_2;
int UsbComReportValue_S32_3;
int UsbComReportValue_S32_4;



AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t s_currSendBuf    [1024], 4);

volatile U8 DmaTxRxIsDone;

volatile U8 PdmIsStarted;
volatile U8 I2sIsStarted;


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

volatile S32 *I2SDmaOtCh01Ptr;
volatile S32 *I2SDmaInCh01Ptr;

extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;

__attribute__((section("CodeQuickAccess")))
void ProcessAudio_AfterAudioInputBufIsReady(void)
{
	volatile S32 *MicInputCh0Ptr;
	volatile S32 *MicInputCh1Ptr;
	volatile S32 *MicInputCh2Ptr;
	volatile S32 *MicInputCh3Ptr;
	volatile S32 *MicInputCh4Ptr;
	volatile S32 *MicInputCh5Ptr;
	volatile S32 *MicInputCh6Ptr;
	volatile S32 *MicInputCh7Ptr;

	S32 *TmpPtrS32;
	S16 i,j;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];

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
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 1 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 1 * AudioFrameSizeInSamplePerCh;
		}else
		{
			//now DMA is using PDM BufB, the MCU code should use PDM DMA buffer A (the first half), which is just ready
			MicInputCh0Ptr=MicInputDmaDualBuf_0 + 0 * AudioFrameSizeInSamplePerCh;
			MicInputCh1Ptr=MicInputDmaDualBuf_1 + 0 * AudioFrameSizeInSamplePerCh;
		}
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

		if(I2S1DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaInCh01Ptr=I2S1Rx0BufCh0And1Mixed_A;
		}
		if(I2S3DmaTransferringIsUsingBufA)
		{	//now DMA is using I2S BufA, the MCU code should use I2S DMA buffer B, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh2And3Mixed_B;
		}else
		{	//now DMA is using I2S BufB, the MCU code should use I2S DMA buffer A, which is just ready
			I2SDmaOtCh01Ptr=I2S3Tx0BufCh2And3Mixed_A;
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

	#if 1	//folding --- read and write BT cir buffer
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
					PRINTF("BT Dn CirBuf is E \r\n");
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
					PRINTF("BT Up CirBuf is F \r\n");
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
					PRINTF("BT Dn CirBuf is E \r\n");
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
					PRINTF("BT Up CirBuf is F \r\n");
				}
				OSA_EXIT_CRITICAL();
			#endif
		}else
		{
			//should never come here
			PRINTF("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
		}
	#endif

	#if 1	//folding
		UsbUpStreamingStopMonitorCnt++;
		if(UsbUpStreamingStopMonitorCnt>200)
		{
			//uac upstreaming request is stopped
			OSA_ENTER_CRITICAL();
			CirUacUpAudioBuf_ClearAllSamples_MultiCh(&UacUpAudioBuf_MCh);
			UsbUpStreamingIsStarted=0;
			OSA_EXIT_CRITICAL();
		}

		UsbDnStreamingStopMonitorCnt++;
		if(UsbDnStreamingStopMonitorCnt>200)
		{
			//uac dnstreaming request is stopped
			OSA_ENTER_CRITICAL();
			CirUacDnAudioBuf_ClearAllSamples_MultiCh(&UacDnAudioBuf_MCh);
			UsbDnStreamingIsStarted=0;
			OSA_EXIT_CRITICAL();
		}
	#endif

	#if 1	//folding --- final process --- send event to dsp, and set values for COM printing, and read button IO pin to generate button event
		#if 1
			//no need to skip the first several frame
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, EvtFlag_AudioFrameIsReadyToProcess);		//send 0x00000001 to DSP, to trigger DSP MU interrupt --- after 300ms delay
		#else
			if(!AudioIoFrameCntForMuteMicInputAtStartingUp)
			{
				//mow mic input is ready, DSP needs to process
				MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, EvtFlag_AudioFrameIsReadyToProcess);		//send 0x00000001 to DSP, to trigger DSP MU interrupt --- after 300ms delay

			}else
			{
				//no need to trigger dsp processing
			}
		#endif

		//report monitor info to Uart com host through uart
		static int CycCntInfoIdx1=0;
		if((AudioIoFrameCnt%100)==1)
		{

			CycCntInfoIdx1=0;	//this is to only display one selected row of the total 4 rows

			VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]=AOD_BTDnBuf;
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[1]=AOD_BTUpBuf;

			#if EnableAudioPllAdjustingToSyncBetweenBtFsAndLocalFs==1
				VarBlockSharedByDspAndMcu.MonitorInfoArray1[2]=AUDIOPLL0NUM_AdjustingValue;
			#else
				VarBlockSharedByDspAndMcu.MonitorInfoArray1[2]=0;
			#endif

			UartComReportValue_S32_1=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+0];
			UartComReportValue_S32_2=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+1];
			UartComReportValue_S32_3=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+2];
			UartComReportValue_S32_4=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray1[CycCntInfoIdx1*4+3];

            sprintf((char *)s_currSendBuf,"W#%d,%d,%d,%d,%d\r\n",CycCntInfoIdx1,UartComReportValue_S32_1,UartComReportValue_S32_2,UartComReportValue_S32_3,UartComReportValue_S32_4);
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[3]=0;

			SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
			PRINTF(s_currSendBuf);
			SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

			CycCntInfoIdx1++;						//using CycCntInfoIdx1 from 0 to 3, can have 16 cycle cnt values to be printed
			if(CycCntInfoIdx1>3) CycCntInfoIdx1=0;
		}


		#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
			static int CycCntInfoIdx2=0;
			//report cycle count info to USB com host through VCOM
			if((AudioIoFrameCnt%20)==2)
			{
				CycCntInfoIdx2=2;	//this is to only display one selected row of the total 4 rows
				UsbComReportValue_S32_1=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+0];
				UsbComReportValue_S32_2=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+1];
				UsbComReportValue_S32_3=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+2];
				UsbComReportValue_S32_4=(int)VarBlockSharedByDspAndMcu.MonitorInfoArray2[CycCntInfoIdx2*4+3];
				s_sendSize=CycCntInfoIdx2+1;				//command to do VCOM send in the main loop!
				CycCntInfoIdx2++;						//using CycCntInfoIdx2 from 0 to 3, can have 16 cycle cnt values to be printed
				if(CycCntInfoIdx2>3) CycCntInfoIdx2=0;
			}
		#endif
#if !DEV_AUDIO_DEBUG_GPIO
		GenBtnEvt();
#endif
	#endif


	DmaTxRxIsDone=0;
	DbgPin5Dn();
}

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
    	CloseI2sDma((I2S_Type *)DEMO_I2S3Tx0);
    	CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2S3Tx0);
    	ClearDmaBuf_I2S3Tx0();
	}

	//close active audio ports --- I2S out ports
	//if(ActivedI2sPorts&AudioI2sPortsBitMapFlag_Fc1)
	{
    	CloseI2sDma((I2S_Type *)DEMO_I2S1Rx0);
    	CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2S1Rx0);
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


