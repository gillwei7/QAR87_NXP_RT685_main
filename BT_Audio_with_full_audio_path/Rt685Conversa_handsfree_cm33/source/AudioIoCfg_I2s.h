/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __AudioIoCfg_i2s_h__
#define __AudioIoCfg_i2s_h__



#include "GlobalDef.h"


#include "fsl_dmic_dma.h"
#include "fsl_i2s_dma.h"

#define USB_DEVICE_INTERRUPT_PRIORITY	3


#define DEMO_I2S_MASTER_CLOCK_FREQUENCY CLOCK_GetMclkClkFreq()

#if UsingQAR87Board == 1

#define FcIdx_RxFrAmp				(3)
#define FcIdx_TxToAmp				(1)
#define I2STxToAmpInstance 			(I2S1)
#define I2SRxFrAmpInstance 			(I2S3)
#define I2S_TxToAmp_DMA_CHANNEL 	(3)				//flexcomm1 tx
#define I2S_RxFrAmp_DMA_CHANNEL 	(6)				//flexcomm3 rx

#define FcIdx_RxFrNvt				(6)
#define FcIdx_TxToNvt				(5)
#define I2STxToNvtInstance		(I2S5)
#define I2SRxFrNvtInstance		(I2S6)
#define I2S_TxToNvt_DMA_CHANNEL (11)			//flexcomm5 tx
#define I2S_RxFrNvt_DMA_CHANNEL (12)			//flexcomm6 rx


#define FcIdx_RxFrBt				(4)
#define FcIdx_TxToBt				(2)
#define PcmTxToBtInstance			(I2S2)
#define PcmRxFrBtInstance			(I2S4)			//B36932 should be I2S4 (I2S5)
#define Pcm_TxToBt_DMA_CHANNEL 		(5)				//flexcomm2 tx to BT
#define Pcm_RxFrBt_DMA_CHANNEL 		(8)				//flexcomm4 rx from BT

#else
//having I2S1 and I2S2 swapped, is to aviod using a wire connecting I2STx0 to AMP Data Input --- only use a jump connecting JP8.2 and JP8.3
#define FcIdx_RxFrAmp			1
#define FcIdx_TxToAmp			3
#define I2STxToAmpInstance 			(I2S3)
#define I2SRxFrAmpInstance 			(I2S1)
#define I2S_RxFrAmp_DMA_CHANNEL 	(2)				//flexcomm1 rx
#define I2S_TxToAmp_DMA_CHANNEL 	(7)				//flexcomm3 tx

//Note --- on RT685 EVK, we use fc2 and 5 for the I2S to NVT --- while using I2S to NVT, BT SCO interface is OFF.
#define FcIdx_RxFrNvt			5
#define FcIdx_TxToNvt			2
#define I2STxToNvtInstance		(I2S2)
#define I2SRxFrNvtInstance		(I2S5)
#define I2S_TxToNvt_DMA_CHANNEL (5)				//flexcomm2 rx
#define I2S_RxFrNvt_DMA_CHANNEL (10)			//flexcomm5 tx

#define FcIdx_RxFrBt			5
#define FcIdx_TxToBt			2
#define PcmTxToBtInstance		(I2S2)
#define PcmRxFrBtInstance		(I2S5)
#define Pcm_TxToBt_DMA_CHANNEL 	(5)				//flexcomm2 rx
#define Pcm_RxFrBt_DMA_CHANNEL 	(10)			//flexcomm5 tx

#endif
/*
 	 	 	 	 	 DMA ch
	fc0		rx,			0
	fc0		tx,			1
	fc1		rx,			2
	fc1		tx,			3
	fc2		rx,			4
	fc2		tx,			5
	fc3		rx,			6
	fc3		tx,			7
	fc4		rx,			8
	fc4		tx,			9
	fc5		rx,			10
	fc5		tx,			11
	fc6		rx,			12
	fc6		tx,			13
*/

#define DEMO_I2S_CLOCK_DIVIDER                                                                                 \
	(24576000U / 48000U / 32U / 2) /* I2S source clock 24.576MHZ, sample rate 48KHZ, bits width 16, 2 channel, \
								  so bitclock should be 48KHZ * 16 = 768KHZ, divider should be 24.576MHZ / 768KHZ */

#define DEMO_DMA (DMA0)			//DMA0 is for MCU side


#if UsingQAR87Board == 1
	#define DEMO_I2SFrAmpRx_MODE 		kI2S_MasterSlaveNormalSlave
	#if Rt685I2SToAmpIsI2SMaster==1
		#define DEMO_I2SToAmpTx_MODE 	kI2S_MasterSlaveNormalMaster
	#else
		#define DEMO_I2SToAmpTx_MODE	kI2S_MasterSlaveNormalSlave
	#endif
#else
	#define DEMO_I2SToAmpTx_MODE 		kI2S_MasterSlaveNormalSlave
	#if Rt685I2SToAmpIsI2SMaster==1
		#define DEMO_I2SFrAmpRx_MODE 	kI2S_MasterSlaveNormalMaster
	#else
		#define DEMO_I2SFrAmpRx_MODE	kI2S_MasterSlaveNormalSlave
	#endif
#endif




#if Rt685I2SToNvtIsI2SMaster==1
	#define I2STxToNvt_ClkMode		kI2S_MasterSlaveNormalMaster //B36932 kI2S_MasterSlaveNormalSlave
	#define I2SRxFrNvt_ClkMode		kI2S_MasterSlaveNormalSlave  //B36932 kI2S_MasterSlaveNormalMaster
#else
	#define I2STxToNvt_ClkMode		kI2S_MasterSlaveNormalSlave
	#define I2SRxFrNvt_ClkMode		kI2S_MasterSlaveNormalSlave
#endif

/*
* For configure flexcomm group for different application,
* used in "BOARD_SwitchAudioFreq"
* naming, "connection device name + Flexcomm number(1st is clock source)" _ "connection device name + Flexcomm number(1st is clock source)"
*/
enum FlexcommSetOfApp
{
	BtPcmFc5Fc2_CodecFc1Fc3,
	BtPcmFc2Fc4_AmpFc1Fc3,
	AmpFc1Fc3,
	NvtFc5Fc6_AmpFc1Fc3

};


///////////////////////////////////////////////////////////////////////////////////////////////////////
extern unsigned int *Ptr_dma_descriptor_table0;


extern volatile S16 I2SRxFrAmpCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S16 I2SRxFrAmpCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S16 I2STxToAmpCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S16 I2STxToAmpCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data


extern volatile S16 I2STxToNvtCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S16 I2STxToNvtCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S16 I2SRxFrNvtCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S16 I2SRxFrNvtCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data


extern volatile U8 I2SFrAmpDmaTransferringIsUsingBufA;
extern volatile U8 I2SToAmpDmaTransferringIsUsingBufA;
extern volatile U8 I2STxToNvtDmaTransferringIsUsingBufA;
extern volatile U8 I2SRxFrNvtDmaTransferringIsUsingBufA;

extern volatile short StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt;
extern volatile short StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt;

extern i2s_dma_handle_t I2SRxFrAmpHandle;
extern i2s_dma_handle_t I2STxToAmpHandle;
extern i2s_dma_handle_t I2SRxFrNvtHandle;
extern i2s_dma_handle_t I2STxToNvtHandle;

extern unsigned int *DmaDscrPtr_I2SRxFrAmp;
extern unsigned int *DmaDscrPtr_I2STxToAmp;
extern unsigned int *DmaDscrPtr_I2SRxFrNvt;
extern unsigned int *DmaDscrPtr_I2STxToNvt;


extern void WaitForLRCKRisingEdge_FcTxToAmp(void);
extern void WaitForLRCKFallingEdge_FcTxToAmp(void);
extern void WaitForLRCKRisingEdge_FcRxFrAmp(void);
extern void WaitForLRCKFallingEdge_FcRxFrAmp(void);

extern void WaitForLRCKRisingEdge_FcRxFrNvt(void);
extern void WaitForLRCKFallingEdge_FcRxFrNvt(void);
extern void WaitForLRCKRisingEdge_FcTxToNvt(void);
extern void WaitForLRCKFallingEdge_FcTxToNvt(void);


extern void ConfigI2SRxFrAmpChainedDma(int FrmSizeInSample, int SampleBitW);
extern void ConfigI2STxToAmpChainedDma(int FrmSizeInSample, int SampleBitW);
extern void ConfigI2STxToNvtChainedDma(int FrmSizeInSample, int SampleBitW);
extern void ConfigI2SRxFrNvtChainedDma(int FrmSizeInSample, int SampleBitW);

extern void ClearDmaBuf_I2SFrAmp(void);
extern void ClearDmaBuf_I2SToAmp(void);
extern void ClearDmaBuf_I2STxToNvt(void);
extern void ClearDmaBuf_I2SRxFrNvt(void);

extern void EnableI2SFrAmpDmaChannel(void);
extern void EnableI2SToAmpDmaChannel(void);
extern void EnableI2STxToNvtDmaChannel(void);
extern void EnableI2SRxFrNvtDmaChannel(void);

extern void ImmediatelyStartI2SToAmpDma(void);
extern void ImmediatelyStartI2SFrAmpDma(void);
extern void ImmediatelyStartI2STxToNvtDma(void);
extern void ImmediatelyStartI2SRxFrNvtDma(void);

extern void BOARD_Init_DMA_I2S_FcRxFrAmp(void);
extern void BOARD_Init_DMA_I2S_FcTxToAmp(void);
extern void BOARD_Init_DMA_I2S_FcTxToNvt(void);
extern void BOARD_Init_DMA_I2S_FcRxFrNvt(void);

extern void BOARD_Init_I2S_FcRxFrAmp(int Fs, int BitW);
extern void BOARD_Init_I2S_FcTxToAmp(int Fs, int BitW);
extern void BOARD_Init_I2S_FcTxToNvt(int Fs, int BitW);
extern void BOARD_Init_I2S_FcRxFrNvt(int Fs, int BitW);

extern int GetI2SToAmpDmaTransferringIsUsingBufAOrB(void);
extern int GetI2SFrAmpDmaTransferringIsUsingBufAOrB(void);
extern int GetI2STxToNvtDmaTransferringIsUsingBufAOrB(void);
extern int GetI2SRxFrNvtDmaTransferringIsUsingBufAOrB(void);


extern void CloseI2sAndI2sIntr(I2S_Type *I2SBase);
extern void CloseI2sDma(I2S_Type *I2SBase);


extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;

extern int CheckIfI2SToNvtIsStopped(void);

#endif

