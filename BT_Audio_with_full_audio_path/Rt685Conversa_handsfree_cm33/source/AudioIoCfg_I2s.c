/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdlib.h>
#include <string.h>
#include "pin_mux.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_device_registers.h"
#include "fsl_dmic.h"
#include "fsl_dmic_dma.h"
#include "fsl_i2c.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "fsl_inputmux.h"

#include "GlobalDef.h"

#if EnableConversa==1

#include "SubFunc.h"
#include "AudioIoCfg_I2s.h"
#include "AudioIoCfg_Pdm.h"
#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "MainAudioFlow.h"
#include "WorkStateManager.h"



//---------------var for I2S Rx0,Tx0,Tx1 DMA IO----------------- beg------
#if 1 //for folding
i2s_config_t I2S_Tx_config;
i2s_config_t I2S_Rx_config;

dma_handle_t I2STxToAmpDmaHandle;
dma_handle_t I2SRxFrAmpDmaHandle;
dma_handle_t I2STxToNtDmaHandle;
dma_handle_t I2SRxFrNtDmaHandle;

i2s_dma_handle_t I2STxToAmpHandle;
i2s_dma_handle_t I2SRxFrAmpHandle;
i2s_dma_handle_t I2SRxFrNtHandle;
i2s_dma_handle_t I2STxToNtHandle;

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S32 I2S3Tx0BufCh0And1Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S32 I2S3Tx0BufCh0And1Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S32 I2S1Rx0BufCh0And1Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S32 I2S1Rx0BufCh0And1Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data

#if Rt685I2SToNvtBitWidth==16
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S16 I2STxToNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S16 I2STxToNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S16 I2SRxFrNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S16 I2SRxFrNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
#endif
#if Rt685I2SToNvtBitWidth==32
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S32 I2STxToNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S32 I2STxToNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S32 I2SRxFrNtCh0And1Mixed_A[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
	__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
	volatile S32 I2SRxFrNtCh0And1Mixed_B[AudioFrameSizeInSamplePerCh_NVT*2];    //1 frame samples --- for 2 channels mixed I2S audio data
#endif

__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2S1Rx0DmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2S3Tx0DmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2S3Tx0Transfer[2]=
{
	{
        .data     = (unsigned char *)I2S3Tx0BufCh0And1Mixed_A,
        .dataSize = sizeof(I2S3Tx0BufCh0And1Mixed_A),
    },
    {
        .data     = (unsigned char *)I2S3Tx0BufCh0And1Mixed_B,
        .dataSize = sizeof(I2S3Tx0BufCh0And1Mixed_B),
    }
};
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2S1Rx0Transfer[2]=
{
	{
        .data     = (unsigned char *)I2S1Rx0BufCh0And1Mixed_A,
        .dataSize = sizeof(I2S1Rx0BufCh0And1Mixed_A),
    },
    {
        .data     = (unsigned char *)I2S1Rx0BufCh0And1Mixed_B,
        .dataSize = sizeof(I2S1Rx0BufCh0And1Mixed_B),
    }
};

__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2STxToNtDmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2SRxFrNtDmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);

__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2STxToNtTransfer[2]=
{
	{
        .data     = (unsigned char *)I2STxToNtCh0And1Mixed_A,
        .dataSize = sizeof(I2STxToNtCh0And1Mixed_A),
    },
    {
        .data     = (unsigned char *)I2STxToNtCh0And1Mixed_B,
        .dataSize = sizeof(I2STxToNtCh0And1Mixed_B),
    }
};
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2SRxFrNtTransfer[2]=
{
	{
        .data     = (unsigned char *)I2SRxFrNtCh0And1Mixed_A,
        .dataSize = sizeof(I2SRxFrNtCh0And1Mixed_A),
    },
    {
        .data     = (unsigned char *)I2SRxFrNtCh0And1Mixed_B,
        .dataSize = sizeof(I2SRxFrNtCh0And1Mixed_B),
    }
};

unsigned int *DmaDscrPtr_I2S1;
unsigned int *DmaDscrPtr_I2S3;
unsigned int *DmaDscrPtr_I2SRxFrNt;
unsigned int *DmaDscrPtr_I2STxToNt;

volatile U8 I2S1DmaTransferringIsUsingBufA;
volatile U8 I2S3DmaTransferringIsUsingBufA;
volatile U8 I2STxToNtDmaTransferringIsUsingBufA;
volatile U8 I2SRxFrNtDmaTransferringIsUsingBufA;

volatile short StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt;
volatile short StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt;

#if Rt685I2SToNvtIsI2SMaster==0
volatile int I2SToNtDmaTimePoint=0;
volatile int RestartI2SToNtCnt=0;
volatile int I2SToNtIsStarted=0;
#endif

#endif //for folding
//---------------var for I2S Rx0,Tx0,Tx1 DMA IO----------------- end------


#if 1	//folding --- clear and enable
void ClearDmaBuf_I2S3Tx0(void)
{
	memset((void *)I2S3Tx0BufCh0And1Mixed_A,0,sizeof(I2S3Tx0BufCh0And1Mixed_A));
	memset((void *)I2S3Tx0BufCh0And1Mixed_B,0,sizeof(I2S3Tx0BufCh0And1Mixed_B));
}
void ClearDmaBuf_I2S1Rx0(void)
{
	memset((void *)I2S1Rx0BufCh0And1Mixed_A,0,sizeof(I2S1Rx0BufCh0And1Mixed_A));
	memset((void *)I2S1Rx0BufCh0And1Mixed_B,0,sizeof(I2S1Rx0BufCh0And1Mixed_B));
}
void ClearDmaBuf_I2STxToNt(void)
{
	memset((void *)I2STxToNtCh0And1Mixed_A,0,sizeof(I2STxToNtCh0And1Mixed_A));
	memset((void *)I2STxToNtCh0And1Mixed_B,0,sizeof(I2STxToNtCh0And1Mixed_B));
}
void ClearDmaBuf_I2SRxFrNt(void)
{
	memset((void *)I2SRxFrNtCh0And1Mixed_A,0,sizeof(I2SRxFrNtCh0And1Mixed_A));
	memset((void *)I2SRxFrNtCh0And1Mixed_B,0,sizeof(I2SRxFrNtCh0And1Mixed_B));
}


//in fsl_dma.c handle->base->CHANNEL[channel].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK; is removed --- this is to start all the DMA channel at the same time
void EnableI2S3Tx0DmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_FCTxToAmp_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2S1Rx0DmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_FCRxFrAmp_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2STxToNtDmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_TxToNt_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2SRxFrNtDmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_RxFrNt_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2S1Dma(void)
{
	((I2S_Type *)DEMO_I2SRxFrAmp)->FIFOCFG |= I2S_FIFOCFG_DMARX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2S3Dma(void)
{
	((I2S_Type *)DEMO_I2STxToAmp)->FIFOCFG |= I2S_FIFOCFG_DMATX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2STxToNtDma(void)
{
	((I2S_Type *)I2STxToNtInstance)->FIFOCFG |= I2S_FIFOCFG_DMATX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2SRxToNtDma(void)
{
	((I2S_Type *)I2SRxFrNtInstance)->FIFOCFG |= I2S_FIFOCFG_DMARX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}

#endif
#if 1	//folding --- init
void BOARD_Init_DMA_I2S_Fc1(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL, kDMA_ChannelPriority4);
	DmaDscrPtr_I2S1 =Ptr_dma_descriptor_table0+4*I2S_FCRxFrAmp_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_Fc3(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL, kDMA_ChannelPriority5);
	DmaDscrPtr_I2S3 =Ptr_dma_descriptor_table0+4*I2S_FCTxToAmp_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_FcRxFrNt(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_RxFrNt_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_RxFrNt_DMA_CHANNEL, kDMA_ChannelPriority4);
	DmaDscrPtr_I2SRxFrNt =Ptr_dma_descriptor_table0+4*I2S_RxFrNt_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_FcTxToNt(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_TxToNt_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_TxToNt_DMA_CHANNEL, kDMA_ChannelPriority5);
	DmaDscrPtr_I2STxToNt =Ptr_dma_descriptor_table0+4*I2S_TxToNt_DMA_CHANNEL;
}
void BOARD_Init_I2S_Fc3(void)
{
	i2s_config_t I2S_Tx_config;

	I2S_TxGetDefaultConfig(&I2S_Tx_config);

	I2S_Tx_config.dataLength  = 32;
	I2S_Tx_config.frameLength = 64;

	//I2S_Tx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;

	#if EnableOnlyMicSpk_NoBT==0
		//no matter BT side is 16KHz or 8KHz, Fc1 and Fc3 are always 16KHz and 32bit
		I2S_Tx_config.divider     = (24576000U / 16000U / 32U / 2);
	#else
		#if Fs_I2SToAmp_MicSpkTest==16000
			I2S_Tx_config.divider     = (24576000U / 16000U / 32U / 2);
		#endif
		#if Fs_I2SToAmp_MicSpkTest==48000
			I2S_Tx_config.divider     = (24576000U / 48000U / 32U / 2);
		#endif
	#endif

	I2S_Tx_config.position    =  0;
	I2S_Tx_config.oneChannel  = false;

	I2S_Tx_config.masterSlave = DEMO_I2SToAmpTx_MODE;
	I2S_TxInit(DEMO_I2STxToAmp, &I2S_Tx_config);
}
void BOARD_Init_I2S_Fc1(void)
{
	i2s_config_t I2S_Rx_config;

	I2S_RxGetDefaultConfig(&I2S_Rx_config);

	I2S_Rx_config.dataLength  = 32;
	I2S_Rx_config.frameLength = 64;

	//I2S_Rx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;

	#if EnableOnlyMicSpk_NoBT==0
		//no matter BT side is 16KHz or 8KHz, Fc1 and Fc3 are always 16KHz and 32bit
		I2S_Rx_config.divider     = (24576000U / 16000U / 32U / 2);
	#else
		#if Fs_I2SToAmp_MicSpkTest==16000
			I2S_Rx_config.divider     = (24576000U / 16000U / 32U / 2);
		#endif
		#if Fs_I2SToAmp_MicSpkTest==48000
			I2S_Rx_config.divider     = (24576000U / 48000U / 32U / 2);
		#endif
	#endif

	I2S_Rx_config.position    =  0;
	I2S_Rx_config.oneChannel  = false;

	I2S_Rx_config.masterSlave = DEMO_I2SFrAmpRx_MODE;
	I2S_RxInit(DEMO_I2SRxFrAmp, &I2S_Rx_config);
}
void BOARD_Init_I2S_FcTxToNt(void)
{
	i2s_config_t I2S_Tx_config;

	I2S_TxGetDefaultConfig(&I2S_Tx_config);

	I2S_Tx_config.dataLength  = Rt685I2SToNvtBitWidth;
	I2S_Tx_config.frameLength = Rt685I2SToNvtBitWidth*2;

	//I2S_Tx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;
	I2S_Tx_config.divider     = (24576000U / Fs_I2SToNvt_MicSpkTest / Rt685I2SToNvtBitWidth / 2);

	I2S_Tx_config.position    =  0;
	I2S_Tx_config.oneChannel  = false;

	I2S_Tx_config.masterSlave = I2STxToNt_ClkMode;
	I2S_TxInit(I2STxToNtInstance, &I2S_Tx_config);
}
void BOARD_Init_I2S_FcRxFrNt(void)
{
	i2s_config_t I2S_Rx_config;

	I2S_RxGetDefaultConfig(&I2S_Rx_config);

	I2S_Rx_config.dataLength  = Rt685I2SToNvtBitWidth;
	I2S_Rx_config.frameLength = Rt685I2SToNvtBitWidth*2;

	//I2S_Rx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;
	I2S_Rx_config.divider     = (24576000U / Fs_I2SToNvt_MicSpkTest / Rt685I2SToNvtBitWidth / 2);

	I2S_Rx_config.position    =  0;
	I2S_Rx_config.oneChannel  = false;

	I2S_Rx_config.masterSlave = I2SRxFrNt_ClkMode;
	I2S_RxInit(I2SRxFrNtInstance, &I2S_Rx_config);
}
#endif
#if 1	//folding --- callback
int GetI2S1DmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2S1+3) ==(unsigned int)&I2S1Rx0DmaDescriptors[1]);
}
int GetI2S3DmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2S3+3) ==(unsigned int)&I2S3Tx0DmaDescriptors[1]);
}
int GetI2STxToNtDmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2STxToNt+3) ==(unsigned int)&I2STxToNtDmaDescriptors[1]);
}
int GetI2SRxFrNtDmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2SRxFrNt+3) ==(unsigned int)&I2SRxFrNtDmaDescriptors[1]);
}

void WaitForRx0LRCKRisingEdge_Fc1(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2SRxFrAmp)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2SRxFrAmp)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is just coming into right
}
void WaitForRx0LRCKFallingEdge_Fc1(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2SRxFrAmp)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2SRxFrAmp)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is just coming into left
}
void WaitForTx0LRCKRisingEdge_Fc3(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2STxToAmp)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2STxToAmp)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is just coming into right
}
void WaitForTx0LRCKFallingEdge_Fc3(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2STxToAmp)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2STxToAmp)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is just coming into left
}

void WaitForLRCKRisingEdge_FcRxFrNt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is just coming into right
}
void WaitForLRCKFallingEdge_FcRxFrNt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is just coming into left
}
void WaitForLRCKRisingEdge_FcTxToNt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is just coming into right
}
void WaitForLRCKFallingEdge_FcTxToNt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is just coming into left
}


void CloseI2sDma(I2S_Type *I2SBase)
{
	switch((U32)I2SBase)
	{
		case (U32)DEMO_I2SRxFrAmp:
				I2S_TransferAbortDMA(DEMO_I2SRxFrAmp, &I2SRxFrAmpHandle);
				DMA_DisableChannel(DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL);
			break;
		case (U32)DEMO_I2STxToAmp:
				I2S_TransferAbortDMA(DEMO_I2STxToAmp, &I2STxToAmpHandle);
				DMA_DisableChannel(DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL);
			break;
		case (U32)I2STxToNtInstance:
			I2S_TransferAbortDMA(I2STxToNtInstance, &I2STxToNtHandle);
			DMA_DisableChannel(DEMO_DMA, I2S_TxToNt_DMA_CHANNEL);
			break;
		case (U32)I2SRxFrNtInstance:
			I2S_TransferAbortDMA(I2SRxFrNtInstance, &I2SRxFrNtHandle);
			DMA_DisableChannel(DEMO_DMA, I2S_RxFrNt_DMA_CHANNEL);
			break;
		default:
			break;
	}
}
void CloseI2sAndI2sIntr(I2S_Type *I2SBase)
{
	//clear
	I2SBase->STAT|=0x02;			//clear Slave Frame Error flag error if it is set
	I2SBase->FIFOSTAT|=0xfb;
	I2SBase->FIFOCFG |= (1<<17);	//empty RX fifo

	//close I2S1
	I2SBase->CFG1&=~0x01;			//disable

	//close I2S1 intr
	I2S_DisableInterrupts(I2SBase, (uint32_t)kI2S_RxErrorFlag | (uint32_t)kI2S_RxLevelFlag);
	switch((U32)I2SBase)
	{
		case (U32)DEMO_I2SRxFrAmp:
			DisableIRQ((IRQn_Type)(FLEXCOMM0_IRQn+FcIdx_RxFrAmp));
			break;
		case (U32)DEMO_I2STxToAmp:
			DisableIRQ((IRQn_Type)(FLEXCOMM0_IRQn+FcIdx_TxToAmp));
			break;
		case (U32)I2STxToNtInstance:
			DisableIRQ((IRQn_Type)(FLEXCOMM0_IRQn+FcIdx_RxFrNvt));
			break;
		case (U32)I2SRxFrNtInstance:
			DisableIRQ((IRQn_Type)(FLEXCOMM0_IRQn+FcIdx_TxToNvt));
			break;
		default:
			break;
	}
}

#if Rt685I2SToNvtIsI2SMaster==0
//when NT is master, RT685 need to check the error state on the I2S clk and handle and possible errors
int I2SRxFrNtDmaIsStopped(void)
{
	U32 TimeNow;
	GET_CYCLE_COUNTER(TimeNow);
	if(abs(TimeNow-I2SToNtDmaTimePoint)>(CLOCK_GetFreq(kCLOCK_CoreSysClk)>>6))
		return 1;			//I2SDma intr did not happen for more than 1/64 s (15.625ms)
	else
		return 0;			//I2SDma intr just occurred
}
void DisableI2SToNtAndI2SIntr_AndSetNeetToRestartI2SToNt(void)
{
	//clear
	((I2S_Type *)I2SRxFrNtInstance)->STAT|=0x02;			//clear Slave Frame Error flag error if it is set
	((I2S_Type *)I2SRxFrNtInstance)->FIFOSTAT|=0xfb;
	((I2S_Type *)I2SRxFrNtInstance)->FIFOCFG |= (1<<17);	//empty RX fifo

	((I2S_Type *)I2STxToNtInstance)->STAT|=0x02;			//clear Slave Frame Error flag error if it is set
	((I2S_Type *)I2STxToNtInstance)->FIFOSTAT|=0xfb;
	((I2S_Type *)I2STxToNtInstance)->FIFOCFG |= (1<<17);	//empty RX fifo

	//close I2S
	((I2S_Type *)I2SRxFrNtInstance)->CFG1&=~0x01;			//disable
	((I2S_Type *)I2STxToNtInstance)->CFG1&=~0x01;			//disable

	//should wait 200ms and restart I2S
	RestartI2SToNtCnt=I2SToNtIntrRecoverWaitTime;
	I2SToNtIsStarted=0;

	I2S_DisableInterrupts(I2SRxFrNtInstance, (uint32_t)kI2S_RxErrorFlag | (uint32_t)kI2S_RxLevelFlag);
	DisableIRQ((IRQn_Type)FLEXCOMM6_IRQn);
}
int CheckIfI2SToNtIsStopped(void)
{
	if(I2SToNtIsStarted)
	{
		static int I2S1DmaIsStoppedCnt=0;
		if(I2SRxFrNtDmaIsStopped())
			I2S1DmaIsStoppedCnt++;
		else
			I2S1DmaIsStoppedCnt=0;

		if(I2S1DmaIsStoppedCnt>10)
		{
			//confirmed that I2S1 Dma interrupt is stopped
			DisableI2SToNtAndI2SIntr_AndSetNeetToRestartI2SToNt();
			return 1;
//			AudioSrcEnabledBitMap&=~AudioSrcEnabledBitMapFlag_Fc1;	//if we don't have this line, main loop process will be blocked cause it is waiting for fc1 --- but even add this line, there is still one frame error on fc4 out
//																	//without this line, it will be 150ms all error on fc4 output
		}
	}
	return 0;
}
#endif

__attribute__((section("CodeQuickAccess")))
void I2S3Tx0_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_Fc3;
	StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady_InCall();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif
	DbgPin7Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2S1Rx0_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_Fc1;
	StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady_InCall();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif
	DbgPin7Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2SRxFrNt_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin6Up();
	#if Rt685I2SToNvtIsI2SMaster==1
		AllowAudioInterfaceReInit_PdmI2S=0;
		if(CheckTimePoint_CurrentIntrIsAStartingOne())
		{
			DmaTxRxIsDone=0;
		}
		DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_FcRxFrNt;
		StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;

		if(DmaTxRxIsDone==DmaTxRxIsExpected)
			#if CallAudioFrameProcessInIntr==1
				ProcessAudio_AfterAudioInputBufIsReady_InCall();
			#else
				SCO_AudioFlow_SemaphorePost();
			#endif
	#else

		I2SToNtIsStarted=1;
		GET_CYCLE_COUNTER(I2SToNtDmaTimePoint);

		//when NT is master, RT685 need to check the error state on the I2S clk and handle and possible errors
		//check to terminate I2S, if I2S clk is gone/error
		uint32_t I2Sstat = ((I2S_Type *)I2STxToNtInstance)->STAT;
		if(I2Sstat&0x02)
		{
			DbgPin5Up();
			//LRCK BICK error --- I2S line BICK or LRCK detached !!!
			//LRCK BICK error --- I2S line BICK or LRCK detached !!!
			//LRCK BICK error --- I2S line BICK or LRCK detached !!!
			DisableI2SToNtAndI2SIntr_AndSetNeetToRestartI2SToNt();
			DbgPin5Dn();
		}

		//copy received samples from I2S Rx DMA buffer to cir buffer
		//dual buffer select, and copy samples to circular buffer
		I2SRxFrNtDmaTransferringIsUsingBufA=GetI2SRxFrNtDmaTransferringIsUsingBufAOrB();

		#if Rt685I2SToNvtBitWidth==16
			S16 *SrcPtr;
			if(I2SRxFrNtDmaTransferringIsUsingBufA)
			{
				SrcPtr=(S16 *)I2SRxFrNtCh0And1Mixed_B;
			}else
			{
				SrcPtr=(S16 *)I2SRxFrNtCh0And1Mixed_A;
			}
			if (CirAudioBuf_SpaceAvailableInSamples_S32(&I2SRxFrNt_CirBuf) >= (AudioFrameSizeInSamplePerCh_I2SToNvt))
			{
				CirAudioBuf_WriteSamples_S32(&I2SRxFrNt_CirBuf, AudioFrameSizeInSamplePerCh_I2SToNvt, (S32 *)SrcPtr);
			}
		#endif
		#if Rt685I2SToNvtBitWidth==32
			S32 *SrcPtr;
			if(I2SRxFrNtDmaTransferringIsUsingBufA)
			{
				SrcPtr=(S32 *)I2SRxFrNtCh0And1Mixed_B;
			}else
			{
				SrcPtr=(S32 *)I2SRxFrNtCh0And1Mixed_A;
			}
			if (CirAudioBuf_SpaceAvailableInSamples_S64(&I2SRxFrNt_CirBuf) >= (AudioFrameSizeInSamplePerCh_I2SToNvt))
			{
				CirAudioBuf_WriteSamples_S64(&I2SRxFrNt_CirBuf, AudioFrameSizeInSamplePerCh_I2SToNvt, (S64 *)SrcPtr);
			}
		#endif
	#endif
	DbgPin6Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2STxToNt_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin6Up();
	#if Rt685I2SToNvtIsI2SMaster==1
		AllowAudioInterfaceReInit_PdmI2S=0;
		if(CheckTimePoint_CurrentIntrIsAStartingOne())
		{
			DmaTxRxIsDone=0;
		}
		DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_FcTxToNt;
		StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;

		if(DmaTxRxIsDone==DmaTxRxIsExpected)
			#if CallAudioFrameProcessInIntr==1
				ProcessAudio_AfterAudioInputBufIsReady_InCall();
			#else
				SCO_AudioFlow_SemaphorePost();
			#endif
	#else

		//copy samples from cir buffer to I2S Tx DMA buffer
		//dual buffer select, and copy samples to circular buffer
		I2STxToNtDmaTransferringIsUsingBufA=GetI2STxToNtDmaTransferringIsUsingBufAOrB();


		#if Rt685I2SToNvtBitWidth==16
			S16 *SrcPtr;
			S16 *DstPtr;
			if(I2STxToNtDmaTransferringIsUsingBufA)
			{
				DstPtr=(S16 *)I2STxToNtCh0And1Mixed_B;
			}else
			{
				DstPtr=(S16 *)I2STxToNtCh0And1Mixed_A;
			}

			if (CirAudioBuf_SpaceOccupiedInSamples_S32(&I2STxToNt_CirBuf) >= (AudioFrameSizeInSamplePerCh_I2SToNvt))
			{
				SrcPtr=(S16 *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&I2STxToNt_CirBuf, AudioFrameSizeInSamplePerCh_I2SToNvt);
			}
			memcpy(DstPtr,SrcPtr,sizeof(S16)*2*AudioFrameSizeInSamplePerCh_I2SToNvt);	//samples to I2S tx DMA buffer
		#endif
		#if Rt685I2SToNvtBitWidth==32
			S32 *SrcPtr;
			S32 *DstPtr;
			if(I2STxToNtDmaTransferringIsUsingBufA)
			{
				DstPtr=(S32 *)I2STxToNtCh0And1Mixed_B;
			}else
			{
				DstPtr=(S32 *)I2STxToNtCh0And1Mixed_A;
			}

			if (CirAudioBuf_SpaceOccupiedInSamples_S64(&I2STxToNt_CirBuf) >= (AudioFrameSizeInSamplePerCh_I2SToNvt))
			{
				SrcPtr=(S32 *)CirAudioBuf_ReadSamples_GetRdPtr_S64(&I2STxToNt_CirBuf, AudioFrameSizeInSamplePerCh_I2SToNvt);
			}
			memcpy(DstPtr,SrcPtr,sizeof(S32)*2*AudioFrameSizeInSamplePerCh_I2SToNvt);	//samples to I2S tx DMA buffer
		#endif
	#endif
	DbgPin6Dn();
}
#endif
#if 1	//folding --- configure/create and start
void ConfigI2S3ChainedDma(void)
{
	DMA_CreateHandle(&I2STxToAmpDmaHandle, DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL);
	I2S_TxTransferCreateHandleDMA(DEMO_I2STxToAmp,  &I2STxToAmpHandle, &I2STxToAmpDmaHandle, I2S3Tx0_Callback,I2S3Tx0Transfer);
	I2S_TransferInstallLoopDMADescriptorMemory   (&I2STxToAmpHandle,  I2S3Tx0DmaDescriptors, 2U);
    if (I2S_TransferSendLoopDMA   (DEMO_I2STxToAmp, &I2STxToAmpHandle, &I2S3Tx0Transfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2S1ChainedDma(void)
{
	DMA_CreateHandle(&I2SRxFrAmpDmaHandle, DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL);
	I2S_RxTransferCreateHandleDMA(DEMO_I2SRxFrAmp,  &I2SRxFrAmpHandle, &I2SRxFrAmpDmaHandle, I2S1Rx0_Callback,I2S1Rx0Transfer);
    I2S_TransferInstallLoopDMADescriptorMemory   (&I2SRxFrAmpHandle,  I2S1Rx0DmaDescriptors, 2U);
    if (I2S_TransferReceiveLoopDMA(DEMO_I2SRxFrAmp, &I2SRxFrAmpHandle, &I2S1Rx0Transfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2STxToNtChainedDma(void)
{
	DMA_CreateHandle(&I2STxToNtDmaHandle, DEMO_DMA, I2S_TxToNt_DMA_CHANNEL);
	I2S_TxTransferCreateHandleDMA(I2STxToNtInstance,  &I2STxToNtHandle, &I2STxToNtDmaHandle, I2STxToNt_Callback,I2STxToNtTransfer);
	I2S_TransferInstallLoopDMADescriptorMemory   (&I2STxToNtHandle,  I2STxToNtDmaDescriptors, 2U);
    if (I2S_TransferSendLoopDMA   (I2STxToNtInstance, &I2STxToNtHandle, &I2STxToNtTransfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2SRxFrNtChainedDma(void)
{
	DMA_CreateHandle(&I2SRxFrNtDmaHandle, DEMO_DMA, I2S_RxFrNt_DMA_CHANNEL);
	I2S_RxTransferCreateHandleDMA(I2SRxFrNtInstance,  &I2SRxFrNtHandle, &I2SRxFrNtDmaHandle, I2SRxFrNt_Callback,I2SRxFrNtTransfer);
    I2S_TransferInstallLoopDMADescriptorMemory   (&I2SRxFrNtHandle,  I2SRxFrNtDmaDescriptors, 2U);
    if (I2S_TransferReceiveLoopDMA(I2SRxFrNtInstance, &I2SRxFrNtHandle, &I2SRxFrNtTransfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;

	#if Rt685I2SToNvtIsI2SMaster==0
		I2SToNtDmaTimePoint=0;
		RestartI2SToNtCnt=0;
		I2SToNtIsStarted=0;
	#endif
}
#endif


#endif
