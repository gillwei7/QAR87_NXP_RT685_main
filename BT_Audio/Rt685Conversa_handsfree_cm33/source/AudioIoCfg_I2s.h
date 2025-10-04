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


//having I2S1 and I2S2 swapped, is to aviod using a wire connecting I2STx0 to AMP Data Input --- only use a jump connecting JP8.2 and JP8.3
#define DEMO_I2S3Rx0 (I2S3)	//FlexComm3
#define DEMO_I2S1Tx0 (I2S1)	//FlexComm1

#define I2S1InstanceIdx		1
#define I2S3InstanceIdx		3
#define I2S_FC1Tx_DMA_CHANNEL (3)//(2)		//flexcomm1 rx
#define I2S_FC3Rx_DMA_CHANNEL (6)//(7)		//flexcomm3 tx


#define DEMO_I2S_CLOCK_DIVIDER                                                                                 \
	(24576000U / 48000U / 32U / 2) /* I2S source clock 24.576MHZ, sample rate 48KHZ, bits width 16, 2 channel, \
								  so bitclock should be 48KHZ * 16 = 768KHZ, divider should be 24.576MHZ / 768KHZ */

#define DEMO_DMA (DMA0)			//DMA0 is for MCU side


#define DEMO_I2S3Rx0_MODE 		kI2S_MasterSlaveNormalMaster//kI2S_MasterSlaveNormalSlave
#define DEMO_I2S1Tx0_MODE 		kI2S_MasterSlaveNormalMaster//kI2S_MasterSlaveNormalSlave

///////////////////////////////////////////////////////////////////////////////////////////////////////
extern unsigned int *Ptr_dma_descriptor_table0;


extern volatile S32 I2S3Rx0BufCh2And3Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S32 I2S3Rx0BufCh2And3Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S32 I2S1Tx0BufCh0And1Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
extern volatile S32 I2S1Tx0BufCh0And1Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data

extern volatile U8 I2S1DmaTransferringIsUsingBufA;
extern volatile U8 I2S3DmaTransferringIsUsingBufA;

extern volatile short StartI2SAudioDmaFromDmicDmaIntr_Cnt;

extern i2s_dma_handle_t I2S3Rx0Handle;
extern i2s_dma_handle_t I2S1Tx0Handle;

extern unsigned int *DmaDscrPtr_I2S1;
extern unsigned int *DmaDscrPtr_I2S3;

extern void WaitForRx0LRCKRisingEdge_Fc3(void);
extern void WaitForRx0LRCKFallingEdge_Fc3(void);
extern void WaitForTx0LRCKRisingEdge_Fc1(void);
extern void WaitForTx0LRCKFallingEdge_Fc1(void);

extern void ConfigI2S3ChainedDma(void);
extern void ConfigI2S1ChainedDma(void);

extern void ClearDmaBuf_I2S1Tx0(void);
extern void ClearDmaBuf_I2S3Rx0(void);

extern void ClearDmaBuf_I2S1Tx0(void);
extern void ClearDmaBuf_I2S3Rx0(void);

extern void EnableI2S1Tx0DmaChannel(void);
extern void EnableI2S3Rx0DmaChannel(void);

extern void ImmediatelyStartI2S3Dma(void);
extern void ImmediatelyStartI2S1Dma(void);

extern void BOARD_Init_DMA_I2S_Fc3(void);
extern void BOARD_Init_DMA_I2S_Fc1(void);

extern void BOARD_Init_I2S_Fc3(void);
extern void BOARD_Init_I2S_Fc1(void);

extern int GetI2S3DmaTransferringIsUsingBufAOrB(void);
extern int GetI2S1DmaTransferringIsUsingBufAOrB(void);


extern void CloseI2sAndI2sIntr(I2S_Type *I2SBase);
extern void CloseI2sDma(I2S_Type *I2SBase);


extern U16 UsbUpStreamingStopMonitorCnt;
extern U16 UsbDnStreamingStopMonitorCnt;


#endif

#endif

