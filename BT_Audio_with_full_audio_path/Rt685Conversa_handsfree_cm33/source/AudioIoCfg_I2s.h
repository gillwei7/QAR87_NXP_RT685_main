/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __AudioIoCfg_i2s_h__
#define __AudioIoCfg_i2s_h__



#include "GlobalDef.h"

#if EnableConversa==1

#include "fsl_dmic_dma.h"
#include "fsl_i2s_dma.h"

#define USB_DEVICE_INTERRUPT_PRIORITY	3

#define AudioPortEnabledBitMapFlag_Dmic	0x10
#define AudioPortEnabledBitMapFlag_Fc1	0x01
#define AudioPortEnabledBitMapFlag_Fc3	0x04


#define DEMO_I2S_MASTER_CLOCK_FREQUENCY CLOCK_GetMclkClkFreq()

#if UsingQAR87Board == 1

#define FcIdx_RxFrAmp			3
#define FcIdx_TxToAmp			1
#define DEMO_I2STxToAmp 			(I2S1)
#define DEMO_I2SRxFrAmp 			(I2S3)
#define I2S_FCTxToAmp_DMA_CHANNEL 	(3)				//flexcomm1 tx
#define I2S_FCRxFrAmp_DMA_CHANNEL 	(6)				//flexcomm3 rx

//#define DEMO_I2S3Tx0 			(I2S3)
//#define DEMO_I2S1Rx0 			(I2S1)
//#define I2S_FC1Rx_DMA_CHANNEL 	(2)				//flexcomm1 rx
//#define I2S_FC3Tx_DMA_CHANNEL 	(7)				//flexcomm3 tx

#define FcIdx_RxFrNvt			6
#define FcIdx_TxToNvt			5
#define I2STxToNtInstance		(I2S5)
#define I2SRxFrNtInstance		(I2S6)
#define I2S_TxToNt_DMA_CHANNEL 	(11)			//flexcomm5 tx
#define I2S_RxFrNt_DMA_CHANNEL 	(12)			//flexcomm6 rx

#else
//having I2S1 and I2S2 swapped, is to aviod using a wire connecting I2STx0 to AMP Data Input --- only use a jump connecting JP8.2 and JP8.3
#define FcIdx_RxFrAmp			1
#define FcIdx_TxToAmp			3
#define DEMO_I2STxToAmp 			(I2S3)
#define DEMO_I2SRxFrAmp 			(I2S1)
#define I2S_FCRxFrAmp_DMA_CHANNEL 	(2)				//flexcomm1 rx
#define I2S_FCTxToAmp_DMA_CHANNEL 	(7)				//flexcomm3 tx

//Note --- on RT685 EVK, we use fc2 and 5 for the I2S to NVT --- while using I2S to NVT, BT SCO interface is OFF.
#define FcIdx_RxFrNvt			5
#define FcIdx_TxToNvt			2
#define I2STxToNtInstance		(I2S2)
#define I2SRxFrNtInstance		(I2S5)
#define I2S_TxToNt_DMA_CHANNEL 	(5)				//flexcomm2 rx
#define I2S_RxFrNt_DMA_CHANNEL 	(10)			//flexcomm5 tx
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


#define DEMO_I2SToAmpTx_MODE 		kI2S_MasterSlaveNormalMaster //kI2S_MasterSlaveNormalSlave //B36932, need to check whether is "kI2S_MasterSlaveNormalMaster"

#if Rt685I2SToAmpIsI2SMaster==1
	#define DEMO_I2SFrAmpRx_MODE 	kI2S_MasterSlaveNormalMaster
#else
	#define DEMO_I2SFrAmpRx_MODE	kI2S_MasterSlaveNormalSlave
#endif

#if Rt685I2SToNvtIsI2SMaster==1
	#define I2STxToNt_ClkMode		kI2S_MasterSlaveNormalMaster //B36932 kI2S_MasterSlaveNormalSlave
	#define I2SRxFrNt_ClkMode		kI2S_MasterSlaveNormalSlave  //B36932 kI2S_MasterSlaveNormalMaster
#else
	#define I2STxToNt_ClkMode		kI2S_MasterSlaveNormalSlave
	#define I2SRxFrNt_ClkMode		kI2S_MasterSlaveNormalSlave
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////
extern unsigned int *Ptr_dma_descriptor_table0;


extern volatile S32 I2S3Tx0BufCh0And1Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S32 I2S3Tx0BufCh0And1Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S32 I2S1Rx0BufCh0And1Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S32 I2S1Rx0BufCh0And1Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data


#if Rt685I2SToNvtBitWidth==16
	extern volatile S16 I2STxToNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	extern volatile S16 I2STxToNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	extern volatile S16 I2SRxFrNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	extern volatile S16 I2SRxFrNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
#endif
#if Rt685I2SToNvtBitWidth==32
	extern volatile S32 I2STxToNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	extern volatile S32 I2STxToNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	extern volatile S32 I2SRxFrNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	extern volatile S32 I2SRxFrNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
#endif

extern volatile U8 I2S1DmaTransferringIsUsingBufA;
extern volatile U8 I2S3DmaTransferringIsUsingBufA;
extern volatile U8 I2STxToNtDmaTransferringIsUsingBufA;
extern volatile U8 I2SRxFrNtDmaTransferringIsUsingBufA;

extern volatile short StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt;
extern volatile short StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt;

extern i2s_dma_handle_t I2SRxFrAmpHandle;
extern i2s_dma_handle_t I2STxToAmpHandle;
extern i2s_dma_handle_t I2SRxFrNtHandle;
extern i2s_dma_handle_t I2STxToNtHandle;

extern unsigned int *DmaDscrPtr_I2S1;
extern unsigned int *DmaDscrPtr_I2S3;
extern unsigned int *DmaDscrPtr_I2SRxFrNt;
extern unsigned int *DmaDscrPtr_I2STxToNt;


extern void WaitForRx0LRCKRisingEdge_Fc1(void);
extern void WaitForRx0LRCKFallingEdge_Fc1(void);
extern void WaitForTx0LRCKRisingEdge_Fc3(void);
extern void WaitForTx0LRCKFallingEdge_Fc3(void);

extern void WaitForLRCKRisingEdge_FcRxFrNt(void);
extern void WaitForLRCKFallingEdge_FcRxFrNt(void);
extern void WaitForLRCKRisingEdge_FcTxToNt(void);
extern void WaitForLRCKFallingEdge_FcTxToNt(void);


extern void ConfigI2S3ChainedDma(void);
extern void ConfigI2S1ChainedDma(void);
extern void ConfigI2STxToNtChainedDma(void);
extern void ConfigI2SRxFrNtChainedDma(void);

extern void ClearDmaBuf_I2S3Tx0(void);
extern void ClearDmaBuf_I2S1Rx0(void);
extern void ClearDmaBuf_I2STxToNt(void);
extern void ClearDmaBuf_I2SRxFrNt(void);

extern void EnableI2S3Tx0DmaChannel(void);
extern void EnableI2S1Rx0DmaChannel(void);
extern void EnableI2STxToNtDmaChannel(void);
extern void EnableI2SRxFrNtDmaChannel(void);

extern void ImmediatelyStartI2S3Dma(void);
extern void ImmediatelyStartI2S1Dma(void);
extern void ImmediatelyStartI2STxToNtDma(void);
extern void ImmediatelyStartI2SRxToNtDma(void);

extern void BOARD_Init_DMA_I2S_Fc3(void);
extern void BOARD_Init_DMA_I2S_Fc1(void);
extern void BOARD_Init_DMA_I2S_FcTxToNt(void);
extern void BOARD_Init_DMA_I2S_FcRxFrNt(void);

extern void BOARD_Init_I2S_Fc3(void);
extern void BOARD_Init_I2S_Fc1(void);
extern void BOARD_Init_I2S_FcTxToNt(void);
extern void BOARD_Init_I2S_FcRxFrNt(void);

extern int GetI2S3DmaTransferringIsUsingBufAOrB(void);
extern int GetI2S1DmaTransferringIsUsingBufAOrB(void);
extern int GetI2STxToNtDmaTransferringIsUsingBufAOrB(void);
extern int GetI2SRxFrNtDmaTransferringIsUsingBufAOrB(void);


extern void CloseI2sAndI2sIntr(I2S_Type *I2SBase);
extern void CloseI2sDma(I2S_Type *I2SBase);


extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;

extern int CheckIfI2SToNtIsStopped(void);

#endif

#endif

