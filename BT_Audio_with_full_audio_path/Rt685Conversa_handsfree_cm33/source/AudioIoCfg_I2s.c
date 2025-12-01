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
dma_handle_t I2STxToNvtDmaHandle;
dma_handle_t I2SRxFrNvtDmaHandle;

i2s_dma_handle_t I2STxToAmpHandle;
i2s_dma_handle_t I2SRxFrAmpHandle;
i2s_dma_handle_t I2SRxFrNvtHandle;
i2s_dma_handle_t I2STxToNvtHandle;

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2SRxFrAmpCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2SRxFrAmpCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2STxToAmpCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2STxToAmpCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2STxToNvtCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2STxToNvtCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2SRxFrNvtCh0And1Mixed_A[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data
__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(8)))
volatile S16 I2SRxFrNvtCh0And1Mixed_B[AudioFrameSizeInSamplePerChMaxForDMABuf*2];    //1 frame samples --- for 2 channels mixed I2S audio data

__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2STxToAmpDmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2SRxFrAmpDmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2SRxFrAmpTransfer[2]=
{
	{
        .data     = (unsigned char *)I2SRxFrAmpCh0And1Mixed_A,
        .dataSize = 0,
    },
    {
        .data     = (unsigned char *)I2SRxFrAmpCh0And1Mixed_B,
        .dataSize = 0,
    }
};
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2STxToAmpTransfer[2]=
{
	{
        .data     = (unsigned char *)I2STxToAmpCh0And1Mixed_A,
        .dataSize = 0,
    },
    {
        .data     = (unsigned char *)I2STxToAmpCh0And1Mixed_B,
        .dataSize = 0,
    }
};

__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2STxToNvtDmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
SDK_ALIGN(static dma_descriptor_t I2SRxFrNvtDmaDescriptors[2U], FSL_FEATURE_DMA_LINK_DESCRIPTOR_ALIGN_SIZE);

__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2STxToNvtTransfer[2]=
{
	{
        .data     = (unsigned char *)I2STxToNvtCh0And1Mixed_A,
        .dataSize = 0,
    },
    {
        .data     = (unsigned char *)I2STxToNvtCh0And1Mixed_B,
        .dataSize = 0,
    }
};
__attribute__((__section__(".data.$Audio_IO_Data")))
__attribute__((aligned(16)))
static i2s_transfer_t I2SRxFrNvtTransfer[2]=
{
	{
        .data     = (unsigned char *)I2SRxFrNvtCh0And1Mixed_A,
        .dataSize = 0,
    },
    {
        .data     = (unsigned char *)I2SRxFrNvtCh0And1Mixed_B,
        .dataSize = 0,
    }
};

unsigned int *DmaDscrPtr_I2S1;
unsigned int *DmaDscrPtr_I2S3;
unsigned int *DmaDscrPtr_I2SRxFrNvt;
unsigned int *DmaDscrPtr_I2STxToNvt;

volatile U8 I2S1DmaTransferringIsUsingBufA;
volatile U8 I2S3DmaTransferringIsUsingBufA;
volatile U8 I2STxToNvtDmaTransferringIsUsingBufA;
volatile U8 I2SRxFrNvtDmaTransferringIsUsingBufA;

volatile short StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt;
volatile short StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt;

#if Rt685I2SToNvtIsI2SMaster==0
volatile int I2SToNvtDmaTimePoint=0;
volatile int RestartI2SToNvtCnt=0;
volatile int I2SToNvtIsStarted=0;
#endif

#endif //for folding
//---------------var for I2S Rx0,Tx0,Tx1 DMA IO----------------- end------


#if 1	//folding --- clear and enable
void ClearDmaBuf_I2SFrAmp(void)
{
	memset((void *)I2SRxFrAmpCh0And1Mixed_A,0,sizeof(I2SRxFrAmpCh0And1Mixed_A));
	memset((void *)I2SRxFrAmpCh0And1Mixed_B,0,sizeof(I2SRxFrAmpCh0And1Mixed_B));
}
void ClearDmaBuf_I2SToAmp(void)
{
	memset((void *)I2STxToAmpCh0And1Mixed_A,0,sizeof(I2STxToAmpCh0And1Mixed_A));
	memset((void *)I2STxToAmpCh0And1Mixed_B,0,sizeof(I2STxToAmpCh0And1Mixed_B));
}
void ClearDmaBuf_I2STxToNvt(void)
{
	memset((void *)I2STxToNvtCh0And1Mixed_A,0,sizeof(I2STxToNvtCh0And1Mixed_A));
	memset((void *)I2STxToNvtCh0And1Mixed_B,0,sizeof(I2STxToNvtCh0And1Mixed_B));
}
void ClearDmaBuf_I2SRxFrNvt(void)
{
	memset((void *)I2SRxFrNvtCh0And1Mixed_A,0,sizeof(I2SRxFrNvtCh0And1Mixed_A));
	memset((void *)I2SRxFrNvtCh0And1Mixed_B,0,sizeof(I2SRxFrNvtCh0And1Mixed_B));
}

//in fsl_dma.c handle->base->CHANNEL[channel].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK; is removed --- this is to start all the DMA channel at the same time
void EnableI2SFrAmpDmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_FCTxToAmp_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2SToAmpDmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_FCRxFrAmp_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2STxToNvtDmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_TxToNvt_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2SRxFrNvtDmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_RxFrNvt_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
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
void ImmediatelyStartI2STxToNvtDma(void)
{
	((I2S_Type *)I2STxToNvtInstance)->FIFOCFG |= I2S_FIFOCFG_DMATX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2SRxFrNvtDma(void)
{
	((I2S_Type *)I2SRxFrNvtInstance)->FIFOCFG |= I2S_FIFOCFG_DMARX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}

#endif
#if 1	//folding --- init
void BOARD_Init_DMA_I2S_FcTxToAmp(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL, kDMA_ChannelPriority4);
	DmaDscrPtr_I2S1 =Ptr_dma_descriptor_table0+4*I2S_FCTxToAmp_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_FcRxFrAmp(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL, kDMA_ChannelPriority5);
	DmaDscrPtr_I2S3 =Ptr_dma_descriptor_table0+4*I2S_FCRxFrAmp_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_FcRxFrNvt(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_RxFrNvt_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_RxFrNvt_DMA_CHANNEL, kDMA_ChannelPriority4);
	DmaDscrPtr_I2SRxFrNvt =Ptr_dma_descriptor_table0+4*I2S_RxFrNvt_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_FcTxToNvt(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_TxToNvt_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_TxToNvt_DMA_CHANNEL, kDMA_ChannelPriority5);
	DmaDscrPtr_I2STxToNvt =Ptr_dma_descriptor_table0+4*I2S_TxToNvt_DMA_CHANNEL;
}
void BOARD_Init_I2S_FcRxFrAmp(int Fs, int BitW)
{
	assert((BitW==16)||(BitW==32));
	assert((Fs==16000)||(Fs==32000)||(Fs==44100)||(Fs==48000));

	i2s_config_t I2S_Tx_config;
	I2S_TxGetDefaultConfig(&I2S_Tx_config);

	I2S_Tx_config.dataLength  = BitW;
	I2S_Tx_config.frameLength = BitW*2;

	switch(Fs)
	{
		case 16000:
			I2S_Tx_config.divider     = (24576000U / 16000U / BitW / 2);
			break;
		case 32000:
			I2S_Tx_config.divider     = (24576000U / 32000U / BitW / 2);
			break;
		case 44100:
			I2S_Tx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
		case 48000:
			I2S_Tx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
	}

	I2S_Tx_config.position    =  0;
	I2S_Tx_config.oneChannel  = false;

	I2S_Tx_config.masterSlave = DEMO_I2SToAmpTx_MODE;
	I2S_TxInit(DEMO_I2STxToAmp, &I2S_Tx_config);
}
void BOARD_Init_I2S_FcTxToAmp(int Fs, int BitW)
{
	assert((BitW==16)||(BitW==32));
	assert((Fs==16000)||(Fs==32000)||(Fs==44100)||(Fs==48000));

	i2s_config_t I2S_Rx_config;
	I2S_RxGetDefaultConfig(&I2S_Rx_config);

	I2S_Rx_config.dataLength  = BitW;
	I2S_Rx_config.frameLength = BitW*2;

	switch(Fs)
	{
		case 16000:
			I2S_Rx_config.divider     = (24576000U / 16000U / BitW / 2);
			break;
		case 32000:
			I2S_Rx_config.divider     = (24576000U / 32000U / BitW / 2);
			break;
		case 44100:
			I2S_Rx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
		case 48000:
			I2S_Rx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
	}

	I2S_Rx_config.position    =  0;
	I2S_Rx_config.oneChannel  = false;

	I2S_Rx_config.masterSlave = DEMO_I2SFrAmpRx_MODE;
	I2S_RxInit(DEMO_I2SRxFrAmp, &I2S_Rx_config);
}
void BOARD_Init_I2S_FcTxToNvt(int Fs, int BitW)
{
	assert((BitW==16)||(BitW==32));
	assert((Fs==16000)||(Fs==32000)||(Fs==44100)||(Fs==48000));

	i2s_config_t I2S_Tx_config;

	I2S_TxGetDefaultConfig(&I2S_Tx_config);

	I2S_Tx_config.dataLength  = BitW;
	I2S_Tx_config.frameLength = BitW*2;

	switch(Fs)
	{
		case 16000:
			I2S_Tx_config.divider     = (24576000U / 16000U / BitW / 2);
			break;
		case 32000:
			I2S_Tx_config.divider     = (24576000U / 32000U / BitW / 2);
			break;
		case 44100:
			I2S_Tx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
		case 48000:
			I2S_Tx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
	}

	I2S_Tx_config.position    =  0;
	I2S_Tx_config.oneChannel  = false;

	I2S_Tx_config.masterSlave = I2STxToNvt_ClkMode;
	I2S_TxInit(I2STxToNvtInstance, &I2S_Tx_config);
}
void BOARD_Init_I2S_FcRxFrNvt(int Fs, int BitW)
{
	assert((BitW==16)||(BitW==32));
	assert((Fs==16000)||(Fs==32000)||(Fs==44100)||(Fs==48000));

	i2s_config_t I2S_Rx_config;

	I2S_RxGetDefaultConfig(&I2S_Rx_config);

	I2S_Rx_config.dataLength  = BitW;
	I2S_Rx_config.frameLength = BitW*2;

	switch(Fs)
	{
		case 16000:
			I2S_Rx_config.divider     = (24576000U / 16000U / BitW / 2);
			break;
		case 32000:
			I2S_Rx_config.divider     = (24576000U / 32000U / BitW / 2);
			break;
		case 44100:
			I2S_Rx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
		case 48000:
			I2S_Rx_config.divider     = (24576000U / 48000U / BitW / 2);
			break;
	}

	I2S_Rx_config.position    =  0;
	I2S_Rx_config.oneChannel  = false;

	I2S_Rx_config.masterSlave = I2SRxFrNvt_ClkMode;
	I2S_RxInit(I2SRxFrNvtInstance, &I2S_Rx_config);
}
#endif
#if 1	//folding --- callback
int GetI2S1DmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2S1+3) ==(unsigned int)&I2STxToAmpDmaDescriptors[1]);
}
int GetI2S3DmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2S3+3) ==(unsigned int)&I2SRxFrAmpDmaDescriptors[1]);
}
int GetI2STxToNvtDmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2STxToNvt+3) ==(unsigned int)&I2STxToNvtDmaDescriptors[1]);
}
int GetI2SRxFrNvtDmaTransferringIsUsingBufAOrB(void)
{
	return (*(DmaDscrPtr_I2SRxFrNvt+3) ==(unsigned int)&I2SRxFrNvtDmaDescriptors[1]);
}

void WaitForLRCKRisingEdge_FcTxToAmp(void)
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
void WaitForLRCKFallingEdge_FcTxToAmp(void)
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
void WaitForLRCKRisingEdge_FcRxFrAmp(void)
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
void WaitForLRCKFallingEdge_FcRxFrAmp(void)
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
void WaitForLRCKRisingEdge_FcRxFrNvt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNvtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNvtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is just coming into right
}
void WaitForLRCKFallingEdge_FcRxFrNvt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNvtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2SRxFrNvtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is just coming into left
}
void WaitForLRCKRisingEdge_FcTxToNvt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNvtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNvtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is just coming into right
}
void WaitForLRCKFallingEdge_FcTxToNvt(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNvtInstance)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)I2STxToNvtInstance)->STAT) & 0x4) ==0x0	//now it is left channel
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
		case (U32)I2STxToNvtInstance:
			I2S_TransferAbortDMA(I2STxToNvtInstance, &I2STxToNvtHandle);
			DMA_DisableChannel(DEMO_DMA, I2S_TxToNvt_DMA_CHANNEL);
			break;
		case (U32)I2SRxFrNvtInstance:
			I2S_TransferAbortDMA(I2SRxFrNvtInstance, &I2SRxFrNvtHandle);
			DMA_DisableChannel(DEMO_DMA, I2S_RxFrNvt_DMA_CHANNEL);
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
		case (U32)I2STxToNvtInstance:
			DisableIRQ((IRQn_Type)(FLEXCOMM0_IRQn+FcIdx_RxFrNvt));
			break;
		case (U32)I2SRxFrNvtInstance:
			DisableIRQ((IRQn_Type)(FLEXCOMM0_IRQn+FcIdx_TxToNvt));
			break;
		default:
			break;
	}
}

#if Rt685I2SToNvtIsI2SMaster==0
//when NT is master, RT685 need to check the error state on the I2S clk and handle and possible errors
int I2SRxFrNvtDmaIsStopped(void)
{
	U32 TimeNow;
	GET_CYCLE_COUNTER(TimeNow);
	if(abs(TimeNow-I2SToNvtDmaTimePoint)>(CLOCK_GetFreq(kCLOCK_CoreSysClk)>>6))
		return 1;			//I2SDma intr did not happen for more than 1/64 s (15.625ms)
	else
		return 0;			//I2SDma intr just occurred
}
void DisableI2SToNvtAndI2SIntr_AndSetNeedToRestartI2SToNvt(void)
{
	//clear
	((I2S_Type *)I2SRxFrNvtInstance)->STAT|=0x02;			//clear Slave Frame Error flag error if it is set
	((I2S_Type *)I2SRxFrNvtInstance)->FIFOSTAT|=0xfb;
	((I2S_Type *)I2SRxFrNvtInstance)->FIFOCFG |= (1<<17);	//empty RX fifo

	((I2S_Type *)I2STxToNvtInstance)->STAT|=0x02;			//clear Slave Frame Error flag error if it is set
	((I2S_Type *)I2STxToNvtInstance)->FIFOSTAT|=0xfb;
	((I2S_Type *)I2STxToNvtInstance)->FIFOCFG |= (1<<17);	//empty RX fifo

	//close I2S
	((I2S_Type *)I2SRxFrNvtInstance)->CFG1&=~0x01;			//disable
	((I2S_Type *)I2STxToNvtInstance)->CFG1&=~0x01;			//disable

	//should wait 200ms and restart I2S
	RestartI2SToNvtCnt=I2SToNvtIntrRecoverWaitTime;
	I2SToNvtIsStarted=0;

	I2S_DisableInterrupts(I2SRxFrNvtInstance, (uint32_t)kI2S_RxErrorFlag | (uint32_t)kI2S_RxLevelFlag);
	DisableIRQ((IRQn_Type)FLEXCOMM6_IRQn);
}
int CheckIfI2SToNvtIsStopped(void)
{
	if(I2SToNvtIsStarted)
	{
		static int I2S1DmaIsStoppedCnt=0;
		if(I2SRxFrNvtDmaIsStopped())
			I2S1DmaIsStoppedCnt++;
		else
			I2S1DmaIsStoppedCnt=0;

		if(I2S1DmaIsStoppedCnt>10)
		{
			//confirmed that I2S1 Dma interrupt is stopped
			DisableI2SToNvtAndI2SIntr_AndSetNeedToRestartI2SToNvt();
			return 1;
//			AudioSrcEnabledBitMap&=~AudioSrcEnabledBitMapFlag_Fc1;	//if we don't have this line, main loop process will be blocked cause it is waiting for fc1 --- but even add this line, there is still one frame error on fc4 out
//																	//without this line, it will be 150ms all error on fc4 output
		}
	}
	return 0;
}
#endif

__attribute__((section("CodeQuickAccess")))
void I2SFrAmp_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_FcRxFrAmp;
	StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		SCO_AudioFlow_SemaphorePost();
	DbgPin7Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2SToAmp_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_FcTxToAmp;
	StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		SCO_AudioFlow_SemaphorePost();
	DbgPin7Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2SRxFrNvt_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin6Up();
	#if Rt685I2SToNvtIsI2SMaster==1
		AllowAudioInterfaceReInit_PdmI2S=0;
		if(CheckTimePoint_CurrentIntrIsAStartingOne())
		{
			DmaTxRxIsDone=0;
		}
		DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_FcRxFrNvt;
		StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;

		if(DmaTxRxIsDone==DmaTxRxIsExpected)
			SCO_AudioFlow_SemaphorePost();
	#else

		I2SToNvtIsStarted=1;
		GET_CYCLE_COUNTER(I2SToNvtDmaTimePoint);

		//when NT is master, RT685 need to check the error state on the I2S clk and handle and possible errors
		//check to terminate I2S, if I2S clk is gone/error
		uint32_t I2Sstat = ((I2S_Type *)I2STxToNvtInstance)->STAT;
		if(I2Sstat&0x02)
		{
			DbgPin5Up();
			//LRCK BICK error --- I2S line BICK or LRCK detached !!!
			//LRCK BICK error --- I2S line BICK or LRCK detached !!!
			//LRCK BICK error --- I2S line BICK or LRCK detached !!!
			DisableI2SToNvtAndI2SIntr_AndSetNeedToRestartI2SToNvt();
			DbgPin5Dn();
		}

		//copy received samples from I2S Rx DMA buffer to cir buffer
		//dual buffer select, and copy samples to circular buffer
		I2SRxFrNvtDmaTransferringIsUsingBufA=GetI2SRxFrNvtDmaTransferringIsUsingBufAOrB();

		S16 *SrcPtr;
		if(I2SRxFrNvtDmaTransferringIsUsingBufA)
		{
			SrcPtr=(S16 *)I2SRxFrNvtCh0And1Mixed_B;
		}else
		{
			SrcPtr=(S16 *)I2SRxFrNvtCh0And1Mixed_A;
		}
		if (CirAudioBuf_SpaceAvailableInSamples_S32(&I2SRxFrNvt_CirBuf) >= (AudioFrameSizeInSamplePerCh_I2SToNvt))
		{
			CirAudioBuf_WriteSamples_S32(&I2SRxFrNvt_CirBuf, AudioFrameSizeInSamplePerCh_I2SToNvt, (S32 *)SrcPtr);
		}
	#endif
	DbgPin6Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2STxToNvt_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin6Up();
	#if Rt685I2SToNvtIsI2SMaster==1
		AllowAudioInterfaceReInit_PdmI2S=0;
		if(CheckTimePoint_CurrentIntrIsAStartingOne())
		{
			DmaTxRxIsDone=0;
		}
		DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_FcTxToNvt;
		StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;

		if(DmaTxRxIsDone==DmaTxRxIsExpected)
			SCO_AudioFlow_SemaphorePost();
	#else

		//copy samples from cir buffer to I2S Tx DMA buffer
		//dual buffer select, and copy samples to circular buffer
		I2STxToNvtDmaTransferringIsUsingBufA=GetI2STxToNvtDmaTransferringIsUsingBufAOrB();

		S16 *SrcPtr;
		S16 *DstPtr;
		if(I2STxToNvtDmaTransferringIsUsingBufA)
		{
			DstPtr=(S16 *)I2STxToNvtCh0And1Mixed_B;
		}else
		{
			DstPtr=(S16 *)I2STxToNvtCh0And1Mixed_A;
		}

		if (CirAudioBuf_SpaceOccupiedInSamples_S32(&I2STxToNvt_CirBuf) >= (AudioFrameSizeInSamplePerCh_I2SToNvt))
		{
			SrcPtr=(S16 *)CirAudioBuf_ReadSamples_GetRdPtr_S32(&I2STxToNvt_CirBuf, AudioFrameSizeInSamplePerCh_I2SToNvt);
		}
		memcpy(DstPtr,SrcPtr,sizeof(S16)*2*AudioFrameSizeInSamplePerCh_I2SToNvt);	//samples to I2S tx DMA buffer
	#endif
	DbgPin6Dn();
}
#endif
#if 1	//folding --- configure/create and start
void ConfigI2SRxFrAmpChainedDma(int FrmSizeInSample, int SampleBitW)
{
	assert((SampleBitW==16)||(SampleBitW==32));
	assert(FrmSizeInSample<=AudioFrameSizeInSamplePerChMaxForDMABuf);

	I2SRxFrAmpTransfer[0].dataSize = FrmSizeInSample*(SampleBitW/8)*2;
	I2SRxFrAmpTransfer[1].dataSize = FrmSizeInSample*(SampleBitW/8)*2;

	DMA_CreateHandle(&I2SRxFrAmpDmaHandle, DEMO_DMA, I2S_FCRxFrAmp_DMA_CHANNEL);
	I2S_RxTransferCreateHandleDMA(DEMO_I2SRxFrAmp,  &I2SRxFrAmpHandle, &I2SRxFrAmpDmaHandle, I2SFrAmp_Callback,I2SRxFrAmpTransfer);
	I2S_TransferInstallLoopDMADescriptorMemory   (&I2SRxFrAmpHandle,  I2SRxFrAmpDmaDescriptors, 2U);
    if (I2S_TransferReceiveLoopDMA   (DEMO_I2SRxFrAmp, &I2SRxFrAmpHandle, &I2SRxFrAmpTransfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2STxToAmpChainedDma(int FrmSizeInSample, int SampleBitW)
{
	assert((SampleBitW==16)||(SampleBitW==32));
	assert(FrmSizeInSample<=AudioFrameSizeInSamplePerChMaxForDMABuf);

	I2STxToAmpTransfer[0].dataSize = FrmSizeInSample*(SampleBitW/8)*2;
	I2STxToAmpTransfer[1].dataSize = FrmSizeInSample*(SampleBitW/8)*2;

	DMA_CreateHandle(&I2STxToAmpDmaHandle, DEMO_DMA, I2S_FCTxToAmp_DMA_CHANNEL);
	I2S_TxTransferCreateHandleDMA(DEMO_I2STxToAmp,  &I2STxToAmpHandle, &I2STxToAmpDmaHandle, I2SToAmp_Callback,I2STxToAmpTransfer);
    I2S_TransferInstallLoopDMADescriptorMemory   (&I2STxToAmpHandle,  I2STxToAmpDmaDescriptors, 2U);
    if (I2S_TransferSendLoopDMA(DEMO_I2STxToAmp, &I2STxToAmpHandle, &I2STxToAmpTransfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2STxToNvtChainedDma(int FrmSizeInSample, int SampleBitW)
{
	assert((SampleBitW==16)||(SampleBitW==32));
	if(SampleBitW==16)
		assert(FrmSizeInSample<=128*3);
	if(SampleBitW==32)
		assert(FrmSizeInSample<=AudioFrameSizeInSamplePerChMaxForDMABuf);

	I2STxToNvtTransfer[0].dataSize = FrmSizeInSample*(SampleBitW/8)*2;
	I2STxToNvtTransfer[1].dataSize = FrmSizeInSample*(SampleBitW/8)*2;

	DMA_CreateHandle(&I2STxToNvtDmaHandle, DEMO_DMA, I2S_TxToNvt_DMA_CHANNEL);
	I2S_TxTransferCreateHandleDMA(I2STxToNvtInstance,  &I2STxToNvtHandle, &I2STxToNvtDmaHandle, I2STxToNvt_Callback,I2STxToNvtTransfer);
	I2S_TransferInstallLoopDMADescriptorMemory   (&I2STxToNvtHandle,  I2STxToNvtDmaDescriptors, 2U);
    if (I2S_TransferSendLoopDMA   (I2STxToNvtInstance, &I2STxToNvtHandle, &I2STxToNvtTransfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2SRxFrNvtChainedDma(int FrmSizeInSample, int SampleBitW)
{
	assert((SampleBitW==16)||(SampleBitW==32));
	if(SampleBitW==16)
		assert(FrmSizeInSample<=128*3);
	if(SampleBitW==32)
		assert(FrmSizeInSample<=AudioFrameSizeInSamplePerChMaxForDMABuf);

	I2SRxFrNvtTransfer[0].dataSize = FrmSizeInSample*(SampleBitW/8)*2;
	I2SRxFrNvtTransfer[1].dataSize = FrmSizeInSample*(SampleBitW/8)*2;

	DMA_CreateHandle(&I2SRxFrNvtDmaHandle, DEMO_DMA, I2S_RxFrNvt_DMA_CHANNEL);
	I2S_RxTransferCreateHandleDMA(I2SRxFrNvtInstance,  &I2SRxFrNvtHandle, &I2SRxFrNvtDmaHandle, I2SRxFrNvt_Callback,I2SRxFrNvtTransfer);
    I2S_TransferInstallLoopDMADescriptorMemory   (&I2SRxFrNvtHandle,  I2SRxFrNvtDmaDescriptors, 2U);
    if (I2S_TransferReceiveLoopDMA(I2SRxFrNvtInstance, &I2SRxFrNvtHandle, &I2SRxFrNvtTransfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
    StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;

	#if Rt685I2SToNvtIsI2SMaster==0
		I2SToNvtDmaTimePoint=0;
		RestartI2SToNvtCnt=0;
		I2SToNvtIsStarted=0;
	#endif
}

#endif


