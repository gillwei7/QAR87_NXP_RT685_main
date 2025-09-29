/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdlib.h>
#include <string.h>
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



//---------------var for I2S Rx0,Tx0,Tx1 DMA IO----------------- beg------
#if 1 //for folding
i2s_config_t I2S_Tx_config;
i2s_config_t I2S_Rx_config;

dma_handle_t I2S3Tx0DmaHandle;
dma_handle_t I2S1Rx0DmaHandle;

i2s_dma_handle_t I2S3Tx0Handle;
i2s_dma_handle_t I2S1Rx0Handle;

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(4)))
volatile S32 I2S3Tx0BufCh2And3Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(4)))
volatile S32 I2S3Tx0BufCh2And3Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(4)))
volatile S32 I2S1Rx0BufCh0And1Mixed_A[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data

__attribute__((__section__(".data.$Audio_IO_Data"))) __attribute__((aligned(4)))
volatile S32 I2S1Rx0BufCh0And1Mixed_B[AudioFrameSizeInSamplePerCh*2];    //1 frame samples --- for 2 channels mixed I2S audio data


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
        .data     = (unsigned char *)I2S3Tx0BufCh2And3Mixed_A,
        .dataSize = sizeof(I2S3Tx0BufCh2And3Mixed_A),
    },
    {
        .data     = (unsigned char *)I2S3Tx0BufCh2And3Mixed_B,
        .dataSize = sizeof(I2S3Tx0BufCh2And3Mixed_B),
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

unsigned int *DmaDscrPtr_I2S1;
unsigned int *DmaDscrPtr_I2S3;

volatile U8 I2S1DmaTransferringIsUsingBufA;
volatile U8 I2S3DmaTransferringIsUsingBufA;

volatile short StartI2SAudioDmaFromDmicDmaIntr_Cnt;

#endif //for folding
//---------------var for I2S Rx0,Tx0,Tx1 DMA IO----------------- end------


#if 1	//folding --- clear and enable
void ClearDmaBuf_I2S3Tx0(void)
{
	memset((void *)I2S3Tx0BufCh2And3Mixed_A,0,sizeof(I2S3Tx0BufCh2And3Mixed_A));
	memset((void *)I2S3Tx0BufCh2And3Mixed_B,0,sizeof(I2S3Tx0BufCh2And3Mixed_B));
}
void ClearDmaBuf_I2S1Rx0(void)
{
	memset((void *)I2S1Rx0BufCh0And1Mixed_A,0,sizeof(I2S1Rx0BufCh0And1Mixed_A));
	memset((void *)I2S1Rx0BufCh0And1Mixed_B,0,sizeof(I2S1Rx0BufCh0And1Mixed_B));
}

//in fsl_dma.c handle->base->CHANNEL[channel].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK; is removed --- this is to start all the DMA channel at the same time
void EnableI2S3Tx0DmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_FC3Tx_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}
void EnableI2S1Rx0DmaChannel(void)
{
	((DMA_Type *)(DEMO_DMA))->CHANNEL[I2S_FC1Rx_DMA_CHANNEL].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
}

__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2S1Dma(void)
{
	((I2S_Type *)DEMO_I2S1Rx0)->FIFOCFG |= I2S_FIFOCFG_DMARX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartI2S3Dma(void)
{
	((I2S_Type *)DEMO_I2S3Tx0)->FIFOCFG |= I2S_FIFOCFG_DMATX_MASK;
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
#endif
#if 1	//folding --- init
void BOARD_Init_DMA_I2S_Fc1(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_FC1Rx_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_FC1Rx_DMA_CHANNEL, kDMA_ChannelPriority4);
	DmaDscrPtr_I2S1 =Ptr_dma_descriptor_table0+4*I2S_FC1Rx_DMA_CHANNEL;
}
void BOARD_Init_DMA_I2S_Fc3(void)
{
	DMA_EnableChannel(DEMO_DMA, I2S_FC3Tx_DMA_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, I2S_FC3Tx_DMA_CHANNEL, kDMA_ChannelPriority5);
	DmaDscrPtr_I2S3 =Ptr_dma_descriptor_table0+4*I2S_FC3Tx_DMA_CHANNEL;
}
void BOARD_Init_I2S_Fc3(void)
{
	i2s_config_t I2S_Tx_config;

	I2S_TxGetDefaultConfig(&I2S_Tx_config);

	I2S_Tx_config.dataLength  = 32;
	I2S_Tx_config.frameLength = 64;

	//I2S_Tx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;
	//no matter BT side is 16KHz or 8KHz, Fc1 and Fc3 are always 16KHz and 32bit
	I2S_Tx_config.divider     = (24576000U / 16000U / 32U / 2);

	I2S_Tx_config.position    =  0;
	I2S_Tx_config.oneChannel  = false;
    // gill
	//I2S_Tx_config.masterSlave = DEMO_I2S1Rx0_MODE;
	I2S_Tx_config.masterSlave = DEMO_I2S3Tx0_MODE;
	I2S_TxInit(DEMO_I2S3Tx0, &I2S_Tx_config);
}
void BOARD_Init_I2S_Fc1(void)
{
	i2s_config_t I2S_Rx_config;

	I2S_RxGetDefaultConfig(&I2S_Rx_config);

	I2S_Rx_config.dataLength  = 32;
	I2S_Rx_config.frameLength = 64;

	//I2S_Rx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;
	//no matter BT side is 16KHz or 8KHz, Fc1 and Fc3 are always 16KHz and 32bit
	I2S_Rx_config.divider     = (24576000U / 16000U / 32U / 2);

	I2S_Rx_config.position    =  0;
	I2S_Rx_config.oneChannel  = false;

	//I2S_Rx_config.masterSlave = DEMO_I2S3Tx0_MODE;
	I2S_Rx_config.masterSlave = DEMO_I2S1Rx0_MODE;
	I2S_RxInit(DEMO_I2S1Rx0, &I2S_Rx_config);
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
void WaitForRx0LRCKRisingEdge_Fc1(void)
{
	//this part is to make sure I2S tx rx starts immediately after the LR edge
	//wait till it is I2S left  (LRCK falling edge)
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2S1Rx0)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2S1Rx0)->STAT) & 0x4) ==0x4	//now it is right channel
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
				((((I2S_Type *)DEMO_I2S1Rx0)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2S1Rx0)->STAT) & 0x4) ==0x0	//now it is left channel
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
				((((I2S_Type *)DEMO_I2S3Tx0)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is left
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2S3Tx0)->STAT) & 0x4) ==0x4	//now it is right channel
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
				((((I2S_Type *)DEMO_I2S3Tx0)->STAT) & 0x4) ==0x4	//now it is right channel
		  )
			break;
	}
	//now it is right
	while(1)
	{
		if(
				((((I2S_Type *)DEMO_I2S3Tx0)->STAT) & 0x4) ==0x0	//now it is left channel
		  )
			break;
	}
	//now it is just coming into left
}
void CloseI2sDma(I2S_Type *I2SBase)
{
	switch((U32)I2SBase)
	{
		case (U32)DEMO_I2S1Rx0:
				I2S_TransferAbortDMA(DEMO_I2S1Rx0, &I2S1Rx0Handle);
				DMA_DisableChannel(DEMO_DMA, I2S_FC1Rx_DMA_CHANNEL);
			break;
		case (U32)DEMO_I2S3Tx0:
				I2S_TransferAbortDMA(DEMO_I2S3Tx0, &I2S3Tx0Handle);
				DMA_DisableChannel(DEMO_DMA, I2S_FC3Tx_DMA_CHANNEL);
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
		case (U32)DEMO_I2S1Rx0:
			DisableIRQ((IRQn_Type)FLEXCOMM1_IRQn);
			break;
		case (U32)DEMO_I2S3Tx0:
			DisableIRQ((IRQn_Type)FLEXCOMM3_IRQn);
			break;
		default:
			break;
	}
}

__attribute__((section("CodeQuickAccess")))
void I2S3Tx0_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin7Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_Fc3;
	StartI2SAudioDmaFromDmicDmaIntr_Cnt=0;

	//if(DmaTxRxIsDone==(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67))
	//if(DmaTxRxIsDone==(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23))
	if(DmaTxRxIsDone==(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3
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
					))

		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif
	DbgPin7Dn();
}
__attribute__((section("CodeQuickAccess")))
void I2S1Rx0_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	DbgPin7Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioI2sPortsBitMapFlag_Fc1;
	StartI2SAudioDmaFromDmicDmaIntr_Cnt=0;

	//if(DmaTxRxIsDone==(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67))
	//if(DmaTxRxIsDone==(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23))
	if(DmaTxRxIsDone==(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3
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
					))
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif
	DbgPin7Dn();
}
/*
void I2S2Rx0_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	//DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();
	LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();

	if(ToUpdateActivedPdmPorts)
	{
		ActivedPdmPorts=ToUpdateActivedPdmPorts;
		ToUpdateActivedPdmPorts=0;
	}

	//should start pdm in i2s intr
	if(RequestToStartPdmInI2sIntr)
	{
		//we open pdm ports here
		U8 OpenAudioSrcBitMap=0;

		if(RequestToOpenPdmPortsBitMap&AudioPdmPortsBitMapFlag_Mic01)
			OpenAudioSrcBitMap|=0x03;
		if(RequestToOpenPdmPortsBitMap&AudioPdmPortsBitMapFlag_Mic23)
			OpenAudioSrcBitMap|=0x0c;
		if(RequestToOpenPdmPortsBitMap&AudioPdmPortsBitMapFlag_Mic45)
			OpenAudioSrcBitMap|=0x30;
		if(RequestToOpenPdmPortsBitMap&AudioPdmPortsBitMapFlag_Mic67)
			OpenAudioSrcBitMap|=0xc0;

		ImmediatelyStartDmicDmaChannels(OpenAudioSrcBitMap); //after calling this, dmic dma intr occurs one frame later!
		//ActivedPdmPorts=RequestToOpenPdmPortsBitMap;
		ToUpdateActivedPdmPorts=RequestToOpenPdmPortsBitMap;		//this is to update ActivedPdmPorts one intr later, cause the new activated pdm intr will be one frame later
		RequestToOpenPdmPortsBitMap=0;
		UsbUpStreamingStopMonitorCnt=0;
		PdmIsStarted=1;
		RequestToStartPdmInI2sIntr=0;
	}

	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDoneI2sPorts|=AudioI2sPortsBitMapFlag_Fc2;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady();
#endif
	LedPin4Dn();
}
void I2S4Tx1_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
	//DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();
	LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDoneI2sPorts|=AudioI2sPortsBitMapFlag_Fc4;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady();
#endif
	LedPin4Dn();
}
*/
#endif
#if 1	//folding --- configure/create and start
void ConfigI2S3ChainedDma(void)
{
	DMA_CreateHandle(&I2S3Tx0DmaHandle, DEMO_DMA, I2S_FC3Tx_DMA_CHANNEL);
	I2S_TxTransferCreateHandleDMA(DEMO_I2S3Tx0,  &I2S3Tx0Handle, &I2S3Tx0DmaHandle, I2S3Tx0_Callback,I2S3Tx0Transfer);
	I2S_TransferInstallLoopDMADescriptorMemory   (&I2S3Tx0Handle,  I2S3Tx0DmaDescriptors, 2U);
    if (I2S_TransferSendLoopDMA   (DEMO_I2S3Tx0, &I2S3Tx0Handle, &I2S3Tx0Transfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
	StartI2SAudioDmaFromDmicDmaIntr_Cnt=0;
}
void ConfigI2S1ChainedDma(void)
{
	DMA_CreateHandle(&I2S1Rx0DmaHandle, DEMO_DMA, I2S_FC1Rx_DMA_CHANNEL);
	I2S_RxTransferCreateHandleDMA(DEMO_I2S1Rx0,  &I2S1Rx0Handle, &I2S1Rx0DmaHandle, I2S1Rx0_Callback,I2S1Rx0Transfer);
    I2S_TransferInstallLoopDMADescriptorMemory   (&I2S1Rx0Handle,  I2S1Rx0DmaDescriptors, 2U);
    if (I2S_TransferReceiveLoopDMA(DEMO_I2S1Rx0, &I2S1Rx0Handle, &I2S1Rx0Transfer[0], 2U) != kStatus_Success)
    {
        assert(false);
    }
	StartI2SAudioDmaFromDmicDmaIntr_Cnt=0;
}
#endif


#endif
