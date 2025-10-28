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
#include "fsl_i2s_bridge.h"

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



//---------------var for PDM0,1,2,3,4,5 DMA IO------------------ beg------
#if 1 //for folding

#if EnableMic01==1
	dmic_dma_handle_t dmicDmaHandle_0;
	dmic_dma_handle_t dmicDmaHandle_1;
#endif
#if EnableMic23==1
	dmic_dma_handle_t dmicDmaHandle_2;
	dmic_dma_handle_t dmicDmaHandle_3;
#endif
#if EnableMic45==1
	dmic_dma_handle_t dmicDmaHandle_4;
	dmic_dma_handle_t dmicDmaHandle_5;
#endif
#if EnableMic67==1
	dmic_dma_handle_t dmicDmaHandle_6;
	dmic_dma_handle_t dmicDmaHandle_7;
#endif

#if EnableMic01==1
	dma_handle_t      dmicRxDmaHandle_0;
	dma_handle_t      dmicRxDmaHandle_1;
#endif
#if EnableMic23==1
	dma_handle_t      dmicRxDmaHandle_2;
	dma_handle_t      dmicRxDmaHandle_3;
#endif
#if EnableMic45==1
	dma_handle_t      dmicRxDmaHandle_4;
	dma_handle_t      dmicRxDmaHandle_5;
#endif
#if EnableMic67==1
	dma_handle_t      dmicRxDmaHandle_6;
	dma_handle_t      dmicRxDmaHandle_7;
#endif

#if EnableMic01==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_0[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_1[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
#endif
#if EnableMic23==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_2[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_3[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
#endif
#if EnableMic45==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_4[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_5[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
#endif
#if EnableMic67==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_6[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(8)))
	volatile S32 MicInputDmaDualBuf_7[AudioFrameSizeInSamplePerCh_PDM * PDM_Rx_BUFFER_NUM];
#endif

#if EnableMic01==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	volatile dma_descriptor_t MicInputDmaPingpongDscr01[2];
#endif
#if EnableMic23==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	volatile dma_descriptor_t MicInputDmaPingpongDscr23[2];
#endif
#if EnableMic45==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	volatile dma_descriptor_t MicInputDmaPingpongDscr45[2];
#endif
#if EnableMic67==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	volatile dma_descriptor_t MicInputDmaPingpongDscr67[2];
#endif

__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
#if EnableMic01==1
	dmic_transfer_t MicDmaCfgCh0[2U] = {
		/* transfer configurations for channel0 */
		{
			.data                   = (void *)MicInputDmaDualBuf_0,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh0[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_0[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh0[0],
		},
	};
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh1[2U] = {
		/* transfer configurations for channel1 */
		{
			.data                   = (void *)MicInputDmaDualBuf_1,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh1[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_1[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh1[0],
		},
	};
#endif
#if EnableMic23==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh2[2U] = {
		/* transfer configurations for channel2 */
		{
			.data                   = (void *)MicInputDmaDualBuf_2,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh2[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_2[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh2[0],
		},
	};
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh3[2U] = {
		/* transfer configurations for channel3 */
		{
			.data                   = (void *)MicInputDmaDualBuf_3,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh3[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_3[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh3[0],
		},
	};
#endif
#if EnableMic45==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh4[2U] = {
		/* transfer configurations for channel4 */
		{
			.data                   = (void *)MicInputDmaDualBuf_4,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh4[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_4[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh4[0],
		},
	};
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh5[2U] = {
		/* transfer configurations for channel5 */
		{
			.data                   = (void *)MicInputDmaDualBuf_5,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh5[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_5[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh5[0],
		},
	};
#endif
#if EnableMic67==1
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh6[2U] = {
		/* transfer configurations for channel6 */
		{
			.data                   = (void *)MicInputDmaDualBuf_6,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh6[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_6[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh6[0],
		},
	};
	__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
	dmic_transfer_t MicDmaCfgCh7[2U] = {
		/* transfer configurations for channel7 */
		{
			.data                   = (void *)MicInputDmaDualBuf_7,
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh7[1],
		},

		{
			.data                   = (void *)&MicInputDmaDualBuf_7[AudioFrameSizeInSamplePerCh_PDM],
			.dataWidth              = sizeof(S32),
			.dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32),
			.dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth,
			.linkTransfer           = &MicDmaCfgCh7[0],
		},
	};
#endif

__attribute__((__section__(".data.$Audio_io_data_non_cached"))) __attribute__((aligned(16)))
volatile dma_descriptor_t MicInputDmaPingpongDscr[8][2];

U8 DmicIdxForCheckingDmaDscrPtr;

unsigned int *DmaDscrPtr_Dmic0;
unsigned int *DmaDscrPtr_Dmic2;
unsigned int *DmaDscrPtr_Dmic4;
unsigned int *DmaDscrPtr_Dmic6;

unsigned int *DmaDscrPtr_Dmic1;		//actually not used
unsigned int *DmaDscrPtr_Dmic3;
unsigned int *DmaDscrPtr_Dmic5;
unsigned int *DmaDscrPtr_Dmic7;

volatile U8 PdmCh0DmaTransferringIsUsingBufA;
volatile U8 PdmCh2DmaTransferringIsUsingBufA;
volatile U8 PdmCh4DmaTransferringIsUsingBufA;
volatile U8 PdmCh6DmaTransferringIsUsingBufA;

#endif //for folding
//---------------var for PDM0,1,2,3,4,5 DMA IO------------------ beg------

#if 1	//folding --- clear and enable
//in fsl_dma.c handle->base->CHANNEL[channel].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK; is removed --- this is to start all the DMA channel at the same time
__attribute__((section("CodeQuickAccess")))
void ImmediatelyStartDmicDmaChannels(U8 MicSelectBits)	//bit0~7 --> dmic ch0~7
{
#if 1
	//this is to speed up, to make all the pdm mics are started at the same time
	DMIC_DoFifoReset(DMIC0, 0);
	DMIC_DoFifoReset(DMIC0, 1);
	DMIC_DoFifoReset(DMIC0, 2);
	DMIC_DoFifoReset(DMIC0, 3);
	DMIC_DoFifoReset(DMIC0, 4);
	DMIC_DoFifoReset(DMIC0, 5);
	DMIC_DoFifoReset(DMIC0, 6);
	DMIC_DoFifoReset(DMIC0, 7);

	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC0].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC1].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC2].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC3].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC4].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC5].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC6].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
	((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC7].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;

#else
	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_DoFifoReset(DMIC0, 0);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC0].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
		if(MicSelectBits&0x02)
		{
			DMIC_DoFifoReset(DMIC0, 1);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC1].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_DoFifoReset(DMIC0, 2);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC2].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
		if(MicSelectBits&0x08)
		{
			DMIC_DoFifoReset(DMIC0, 3);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC3].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_DoFifoReset(DMIC0, 4);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC4].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
		if(MicSelectBits&0x20)
		{
			DMIC_DoFifoReset(DMIC0, 5);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC5].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_DoFifoReset(DMIC0, 6);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC6].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
		if(MicSelectBits&0x80)
		{
			DMIC_DoFifoReset(DMIC0, 7);
			((DMA_Type *)(DEMO_DMA))->CHANNEL[DMAREQ_DMIC7].XFERCFG |= DMA_CHANNEL_XFERCFG_SWTRIG_MASK;
		}
	#endif
#endif
	NVIC_SetPriority(DMA0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY + 1U));
	EnableIRQ(DMA0_IRQn);
}
void Init_MicDmaCfgCh(U8 MicSelectBits)
{
	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			MicDmaCfgCh0[0].data                   = (void *)MicInputDmaDualBuf_0;
			MicDmaCfgCh0[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh0[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh0[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh0[0].linkTransfer           = &MicDmaCfgCh0[1];

			MicDmaCfgCh0[1].data                   = (void *)&MicInputDmaDualBuf_0[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh0[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh0[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh0[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh0[1].linkTransfer           = &MicDmaCfgCh0[0];
		}
		if(MicSelectBits&0x02)
		{
			MicDmaCfgCh1[0].data                   = (void *)MicInputDmaDualBuf_1;
			MicDmaCfgCh1[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh1[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh1[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh1[0].linkTransfer           = &MicDmaCfgCh1[1];

			MicDmaCfgCh1[1].data                   = (void *)&MicInputDmaDualBuf_1[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh1[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh1[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh1[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh1[1].linkTransfer           = &MicDmaCfgCh1[0];
		}
	#endif

	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			MicDmaCfgCh2[0].data                   = (void *)MicInputDmaDualBuf_2;
			MicDmaCfgCh2[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh2[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh2[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh2[0].linkTransfer           = &MicDmaCfgCh2[1];

			MicDmaCfgCh2[1].data                   = (void *)&MicInputDmaDualBuf_2[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh2[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh2[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh2[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh2[1].linkTransfer           = &MicDmaCfgCh2[0];
		}
		if(MicSelectBits&0x08)
		{
			MicDmaCfgCh3[0].data                   = (void *)MicInputDmaDualBuf_3;
			MicDmaCfgCh3[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh3[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh3[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh3[0].linkTransfer           = &MicDmaCfgCh3[1];

			MicDmaCfgCh3[1].data                   = (void *)&MicInputDmaDualBuf_3[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh3[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh3[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh3[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh3[1].linkTransfer           = &MicDmaCfgCh3[0];
		}
	#endif

	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			MicDmaCfgCh4[0].data                   = (void *)MicInputDmaDualBuf_4;
			MicDmaCfgCh4[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh4[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh4[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh4[0].linkTransfer           = &MicDmaCfgCh4[1];

			MicDmaCfgCh4[1].data                   = (void *)&MicInputDmaDualBuf_4[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh4[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh4[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh4[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh4[1].linkTransfer           = &MicDmaCfgCh4[0];
		}
		if(MicSelectBits&0x20)
		{
			MicDmaCfgCh5[0].data                   = (void *)MicInputDmaDualBuf_5;
			MicDmaCfgCh5[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh5[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh5[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh5[0].linkTransfer           = &MicDmaCfgCh5[1];

			MicDmaCfgCh5[1].data                   = (void *)&MicInputDmaDualBuf_5[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh5[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh5[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh5[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh5[1].linkTransfer           = &MicDmaCfgCh5[0];
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			MicDmaCfgCh6[0].data                   = (void *)MicInputDmaDualBuf_6;
			MicDmaCfgCh6[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh6[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh6[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh6[0].linkTransfer           = &MicDmaCfgCh6[1];

			MicDmaCfgCh6[1].data                   = (void *)&MicInputDmaDualBuf_6[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh6[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh6[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh6[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh6[1].linkTransfer           = &MicDmaCfgCh6[0];
		}
		if(MicSelectBits&0x80)
		{
			MicDmaCfgCh7[0].data                   = (void *)MicInputDmaDualBuf_7;
			MicDmaCfgCh7[0].dataWidth              = sizeof(S32);
			MicDmaCfgCh7[0].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh7[0].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh7[0].linkTransfer           = &MicDmaCfgCh7[1];

			MicDmaCfgCh7[1].data                   = (void *)&MicInputDmaDualBuf_7[AudioFrameSizeInSamplePerCh_PDM];
			MicDmaCfgCh7[1].dataWidth              = sizeof(S32);
			MicDmaCfgCh7[1].dataSize               = AudioFrameSizeInSamplePerCh_PDM*sizeof(S32);
			MicDmaCfgCh7[1].dataAddrInterleaveSize = kDMA_AddressInterleave1xWidth;
			MicDmaCfgCh7[1].linkTransfer           = &MicDmaCfgCh7[0];
		}
	#endif
}
#endif

#if 1	//folding --- init
void BOARD_DeInit_DMA_PDM(U8 MicSelectBits)
{

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_0);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC0);
			memset((void *)MicInputDmaDualBuf_0,0,sizeof(MicInputDmaDualBuf_0));
		}
		if(MicSelectBits&0x02)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_1);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC1);
			memset((void *)MicInputDmaDualBuf_1,0,sizeof(MicInputDmaDualBuf_1));
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_2);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC2);
			memset((void *)MicInputDmaDualBuf_2,0,sizeof(MicInputDmaDualBuf_2));
		}
		if(MicSelectBits&0x08)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_3);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC3);
			memset((void *)MicInputDmaDualBuf_3,0,sizeof(MicInputDmaDualBuf_3));
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_4);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC4);
			memset((void *)MicInputDmaDualBuf_4,0,sizeof(MicInputDmaDualBuf_4));
		}
		if(MicSelectBits&0x20)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_5);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC5);
			memset((void *)MicInputDmaDualBuf_5,0,sizeof(MicInputDmaDualBuf_5));
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_6);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC6);
			memset((void *)MicInputDmaDualBuf_6,0,sizeof(MicInputDmaDualBuf_6));
		}
		if(MicSelectBits&0x80)
		{
			DMIC_TransferAbortReceiveDMA(DMIC0, &dmicDmaHandle_7);
			DMA_DisableChannel(DEMO_DMA, DMAREQ_DMIC7);
			memset((void *)MicInputDmaDualBuf_7,0,sizeof(MicInputDmaDualBuf_7));
		}
	#endif

	DMIC_DeInit(DMIC0);
}
void BOARD_Init_DMA_PDM(U8 MicSelectBits)
{
	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC0); DmicIdxForCheckingDmaDscrPtr=0;
		}
		if(MicSelectBits&0x02)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC1); DmicIdxForCheckingDmaDscrPtr=1;
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC2); DmicIdxForCheckingDmaDscrPtr=2;
		}
		if(MicSelectBits&0x08)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC3); DmicIdxForCheckingDmaDscrPtr=3;
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC4); DmicIdxForCheckingDmaDscrPtr=4;
		}
		if(MicSelectBits&0x20)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC5); DmicIdxForCheckingDmaDscrPtr=5;
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC6); DmicIdxForCheckingDmaDscrPtr=6;
		}
		if(MicSelectBits&0x80)
		{
			DMA_EnableChannel(DEMO_DMA, DMAREQ_DMIC7); DmicIdxForCheckingDmaDscrPtr=7;
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC0, kDMA_ChannelPriority2);
		}
		if(MicSelectBits&0x02)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC1, kDMA_ChannelPriority2);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC2, kDMA_ChannelPriority2);
		}
		if(MicSelectBits&0x08)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC3, kDMA_ChannelPriority2);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC4, kDMA_ChannelPriority2);
		}
		if(MicSelectBits&0x20)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC5, kDMA_ChannelPriority2);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC6, kDMA_ChannelPriority2);
		}
		if(MicSelectBits&0x80)
		{
			DMA_SetChannelPriority(DEMO_DMA, DMAREQ_DMIC7, kDMA_ChannelPriority2);
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DmaDscrPtr_Dmic0=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+0);
		}
		if(MicSelectBits&0x02)
		{
			DmaDscrPtr_Dmic1=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+1);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DmaDscrPtr_Dmic2=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+2);
		}
		if(MicSelectBits&0x08)
		{
			DmaDscrPtr_Dmic3=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+3);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DmaDscrPtr_Dmic4=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+4);
		}
		if(MicSelectBits&0x20)
		{
			DmaDscrPtr_Dmic5=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+5);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DmaDscrPtr_Dmic6=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+6);
		}
		if(MicSelectBits&0x80)
		{
			DmaDscrPtr_Dmic7=Ptr_dma_descriptor_table0+4*(DMAREQ_DMIC0+7);
		}
	#endif

}
void BOARD_Init_DMIC(U8 MicSelectBits, U8 SkipInitGlobalDMIC0)
{
	dmic_channel_config_t dmic_channel_cfg;
	memset(&dmic_channel_cfg, 0U, sizeof(dmic_channel_config_t));
	dmic_channel_cfg.divhfclk            = kDMIC_PdmDiv1;

	dmic_channel_cfg.osr                 = 48U;
	dmic_channel_cfg.gainshft            = 6U;

	dmic_channel_cfg.preac2coef          = kDMIC_CompValueZero;
	dmic_channel_cfg.preac4coef          = kDMIC_CompValueZero;
	dmic_channel_cfg.dc_cut_level        = kDMIC_DcCut155;
	dmic_channel_cfg.post_dc_gain_reduce = 0U;
	dmic_channel_cfg.saturate16bit       = 0U;
	dmic_channel_cfg.enableSignExtend	 = true;
	dmic_channel_cfg.sample_rate         = kDMIC_PhyFullSpeed;

	if(!SkipInitGlobalDMIC0)
	{
		DMIC_Init(DMIC0);
#if !(defined(FSL_FEATURE_DMIC_HAS_NO_IOCFG) && FSL_FEATURE_DMIC_HAS_NO_IOCFG)
	DMIC_SetIOCFG(DMIC0, kDMIC_PdmDual);
#endif
	}

	DMIC_Use2fs(DMIC0, false);

	//init DMIC0 ch0
	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel0,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_0, true);
		}
		if(MicSelectBits&0x02)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel1,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_1, true);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel2,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_2, true);
		}
		if(MicSelectBits&0x08)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel3,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_3, true);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel4,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_4, true);
		}
		if(MicSelectBits&0x20)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel5,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_5, true);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel6,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_6, true);
		}
		if(MicSelectBits&0x80)
		{
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel7,true);
			DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_7, true);
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_0, kDMIC_Left,  &dmic_channel_cfg);
		}
		if(MicSelectBits&0x02)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_1, kDMIC_Right, &dmic_channel_cfg);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_2, kDMIC_Left,  &dmic_channel_cfg);
		}
		if(MicSelectBits&0x08)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_3, kDMIC_Right, &dmic_channel_cfg);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_4, kDMIC_Left,  &dmic_channel_cfg);
		}
		if(MicSelectBits&0x20)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_5, kDMIC_Right, &dmic_channel_cfg);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_6, kDMIC_Left,  &dmic_channel_cfg);
		}
		if(MicSelectBits&0x80)
		{
			DMIC_ConfigChannel   (DMIC0, DEMO_DMIC_CHANNEL_7, kDMIC_Right, &dmic_channel_cfg);
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_0, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel0,false);
		}
		if(MicSelectBits&0x02)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_1, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel1,false);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_2, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel2,false);
		}
		if(MicSelectBits&0x08)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_3, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel3,false);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_4, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel4,false);
		}
		if(MicSelectBits&0x20)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_5, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel5,false);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_6, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel6,false);
		}
		if(MicSelectBits&0x80)
		{
			DMIC_FifoChannel     (DMIC0, DEMO_DMIC_CHANNEL_7, PDM_FIFO_DEPTH_InSamples, true, true);
			DMIC_ResetChannelDecimator(DMIC0, kDMIC_EnableChannel7,false);
		}
	#endif


	//enalbe all DMic at the same time
	DMIC_EnableChannnel (DMIC0,
								0
								#if EnableMic01==1
									|(MicSelectBits&0x01)
									|(MicSelectBits&0x02)
								#endif
								#if EnableMic23==1
									|(MicSelectBits&0x04)
									|(MicSelectBits&0x08)
								#endif
								#if EnableMic45==1
									|(MicSelectBits&0x10)
									|(MicSelectBits&0x20)
								#endif
								#if EnableMic67==1
									|(MicSelectBits&0x40)
									|(MicSelectBits&0x80)
								#endif
						);		//all mics must be enabled at the same time
}
#endif

#if 1	//folding --- callback
int GetPdmCh0DmaTransferringIsUsingBufAOrB(void)
{
	return ((*(DmaDscrPtr_Dmic0+3)==(unsigned int)&MicInputDmaPingpongDscr[0][0]));
}
int GetPdmCh2DmaTransferringIsUsingBufAOrB(void)
{
	return ((*(DmaDscrPtr_Dmic2+3)==(unsigned int)&MicInputDmaPingpongDscr[2][0]));
}
int GetPdmCh4DmaTransferringIsUsingBufAOrB(void)
{
	return ((*(DmaDscrPtr_Dmic4+3)==(unsigned int)&MicInputDmaPingpongDscr[4][0]));
}
int GetPdmCh6DmaTransferringIsUsingBufAOrB(void)
{
	return ((*(DmaDscrPtr_Dmic6+3)==(unsigned int)&MicInputDmaPingpongDscr[6][0]));
}

#if EnableMic01==1
__attribute__((section("CodeQuickAccess")))
void DMicRx_Callback0(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin7Up();

	AllowAudioInterfaceReInit_PdmI2S=0;
	if(DmaTxRxIsExpected & AudioI2sPortsBitMapFlag_Fc1)	//fc1 and fc3 are to be on/off together
	{
		StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt++;
		if(StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt>2)
		{
			//start Rx0 after edge
			#if __OPTIMIZE__==0
				WaitForRx0LRCKFallingEdge_Fc1();
			#else
				WaitForRx0LRCKRisingEdge_Fc1();
			#endif

					((I2S_Type *)DEMO_I2SRxFrAmp)->FIFOCFG |= (1<<16);	//empty fifo
					#if 1
						//this is to ensure rx intr comes after dmic intr --- don't close this part
						for(int i=0;i<8;i++)
						{
							volatile U32 t;
							t=((I2S_Type *)DEMO_I2SRxFrAmp)->FIFORD;
						}
					#endif
					ImmediatelyStartI2S1Dma();	//after calling this, I2S dma intr occurs one frame later!

			//start tx0 after edge
			#if __OPTIMIZE__==0
				WaitForRx0LRCKRisingEdge_Fc1();
			#else
				WaitForRx0LRCKFallingEdge_Fc1();
			#endif
					((I2S_Type *)DEMO_I2STxToAmp)->FIFOCFG |= (1<<16);	//empty fifo
					#if 1
						//this is to ensure tx intr comes after dmic intr --- don't close this part
						for(int i=0;i<8;i++)
						{
							((I2S_Type *)DEMO_I2STxToAmp)->FIFOWR=0;
							//((I2S_Type *)DEMO_I2S4_TX1)->FIFOWR=i*0x20000;		//full the fifo tx buffer, so that tx intr can be aligned with rx and dmic
						}
					#endif
					ImmediatelyStartI2S3Dma();	//after calling this, I2S dma intr occurs one frame later!

			//set the flag variables to initial value
			StartI2SToAmpAudioDmaFromDmicDmaIntr_Cnt=0;
		}
	}

	if(DmaTxRxIsExpected & AudioI2sPortsBitMapFlag_FcTxToNt)	//fc1 and fc3 are to be on/off together
	{
		StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt++;
		if(StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt>2)
		{
			//start Rx0 after edge
			#if __OPTIMIZE__==0
				WaitForLRCKFallingEdge_FcRxFrNt();
			#else
				WaitForLRCKRisingEdge_FcRxFrNt();
			#endif

					((I2S_Type *)I2SRxFrNtInstance)->FIFOCFG |= (1<<16);	//empty fifo
					#if 1
						//this is to ensure rx intr comes after dmic intr --- don't close this part
						for(int i=0;i<8;i++)
						{
							volatile U32 t;
							t=((I2S_Type *)I2SRxFrNtInstance)->FIFORD;
						}
					#endif
					ImmediatelyStartI2SRxToNtDma();	//after calling this, I2S dma intr occurs one frame later!

			//start tx0 after edge
			#if __OPTIMIZE__==0
					WaitForLRCKRisingEdge_FcRxFrNt();
			#else
					WaitForLRCKFallingEdge_FcRxFrNt();
			#endif
					((I2S_Type *)I2STxToNtInstance)->FIFOCFG |= (1<<16);	//empty fifo
					#if 1
						//this is to ensure tx intr comes after dmic intr --- don't close this part
						for(int i=0;i<8;i++)
						{
							((I2S_Type *)I2STxToNtInstance)->FIFOWR=0;
							//((I2S_Type *)DEMO_I2S4_TX1)->FIFOWR=i*0x20000;		//full the fifo tx buffer, so that tx intr can be aligned with rx and dmic
						}
					#endif
					ImmediatelyStartI2STxToNtDma();	//after calling this, I2S dma intr occurs one frame later!

			//set the flag variables to initial value
			StartI2SToNvtAudioDmaFromDmicDmaIntr_Cnt=0;
		}
	}

	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioPdmPortsBitMapFlag_Mic01;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady_InCall();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif

	DbgPin7Dn();
	return;
}
#endif
#if EnableMic23==1
__attribute__((section("CodeQuickAccess")))
void DMicRx_Callback2(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioPdmPortsBitMapFlag_Mic23;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady_InCall();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif

	DbgPin7Dn();
	return;
}
#endif
#if EnableMic45==1
__attribute__((section("CodeQuickAccess")))
void DMicRx_Callback4(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioPdmPortsBitMapFlag_Mic45;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady_InCall();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif

	DbgPin7Dn();
	return;
}
#endif
#if EnableMic67==1
__attribute__((section("CodeQuickAccess")))
void DMicRx_Callback6(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin7Up();
	AllowAudioInterfaceReInit_PdmI2S=0;
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaTxRxIsDone=0;
	}
	DmaTxRxIsDone|=AudioPdmPortsBitMapFlag_Mic67;

	if(DmaTxRxIsDone==DmaTxRxIsExpected)
		#if CallAudioFrameProcessInIntr==1
			ProcessAudio_AfterAudioInputBufIsReady_InCall();
		#else
			SCO_AudioFlow_SemaphorePost();
		#endif

	DbgPin7Dn();
	return;
}
#endif
#if 0
#if EnableMic01==1
U32 ToUpdateActivedI2sPorts=0;
void DMicRx_Callback0(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	//DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();
	LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();

	if(ToUpdateActivedI2sPorts)
	{
		ActivedI2sPorts=ToUpdateActivedI2sPorts;
		ToUpdateActivedI2sPorts=0;
	}

	if(RequestToStartI2sInPdmIntr)
	{
		//LedPin1Up();

		//start I2S1,4 Tx
		WaitForRx0LRCKRisingEdge_Fc1();		//this is to make sure I2S intr comes after DMIC			//start tx0 after rising edge
		//start Tx after rising edge
			((I2S_Type *)DEMO_I2SRxFrAmp)->FIFOCFG |= (1<<16);	//empty fifo
			//((I2S_Type *)DEMO_I2S4_TX1)->FIFOCFG |= (1<<16);	//empty fifo
			#if 1
			//this is to ensure tx intr comes after dmic intr --- don't close this part
				for(int i=0;i<8;i++)
				{
					((I2S_Type *)DEMO_I2SRxFrAmp)->FIFOWR=0;
					//((I2S_Type *)DEMO_I2S4_TX1)->FIFOWR=0;
					//((I2S_Type *)DEMO_I2S_TX1)->FIFOWR=i*0x20000;		//fpll the fifo tx buffer, so that tx intr can be aligned with rx and dmic --- using special values for debug checking
				}
			#endif
				ImmediatelyStartI2S1Dma();	//after calling this, I2S dma intr occurs one frame later!
				//ImmediatelyStartI2S4Dma();	//after calling this, I2S dma intr occurs one frame later!


		//start I2S2,3 Rx
		WaitForRx0LRCKFallingEdge_Fc1();
		//start Rx0 after falling edge
			//((I2S_Type *)DEMO_I2S2_RX0)->FIFOCFG |= (1<<16);	//empty fifo
			((I2S_Type *)DEMO_I2STxToAmp)->FIFOCFG |= (1<<16);	//empty fifo
			#if 1
				//this is to ensure rx intr comes after dmic intr --- don't close this part
				for(int i=0;i<8;i++)
				{
					volatile U32 t;
					//t=((I2S_Type *)DEMO_I2S2_RX0)->FIFORD;
					t=((I2S_Type *)DEMO_I2STxToAmp)->FIFORD;
					t++;	//just to remove a warning
				}
			#endif
				//ImmediatelyStartI2S2Dma();
				ImmediatelyStartI2S3Dma();

		//ActivedI2sPorts=RequestToOpenI2sPortsBitMap;
		ToUpdateActivedI2sPorts=RequestToOpenI2sPortsBitMap;			//one intr later to update ActivedI2sPorts
		RequestToOpenI2sPortsBitMap=0;
//		UsbDnStreamingStopMonitorCnt=0;
		I2sIsStarted=1;

		RequestToStartI2sInPdmIntr=0;

		//LedPin1Dn();
	}

	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic01;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	LedPin4Dn();
}
void DMicRx_Callback1(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic01;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	DbgPin1Dn();
}
#endif
#if EnableMic23==1
void DMicRx_Callback2(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	//DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();
	LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic23;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	LedPin4Dn();
}
void DMicRx_Callback3(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic23;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	DbgPin1Dn();
}
#endif
#if EnableMic45==1
void DMicRx_Callback4(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	//DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();
	LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic45;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	LedPin4Dn();
}
void DMicRx_Callback5(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic45;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	DbgPin1Dn();
}
#endif
#if EnableMic67==1
void DMicRx_Callback6(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	//DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();DbgPin2Up();
	LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();LedPin4Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic67;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	LedPin4Dn();
}
void DMicRx_Callback7(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
	DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();DbgPin1Up();
	if(CheckTimePoint_CurrentIntrIsAStartingOne())
	{
		DmaDonePdmPorts=0;
		DmaDoneI2sPorts=0;
	}
	DmaDonePdmPorts|=AudioPdmPortsBitMapFlag_Mic67;
#if CallAudioFrameProcessInIntr==1
	if((DmaDonePdmPorts==ActivedPdmPorts)&&(DmaDoneI2sPorts==ActivedI2sPorts))
		ProcessAudio_AfterAudioInputBufIsReady_InCall();
#endif
	DbgPin1Dn();
}
#endif
#endif
#endif

#if 1	//folding --- configure/create and start
void ConfigDmicChainedDma(U8 MicSelectBits)
{
	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_0, DEMO_DMA, DMAREQ_DMIC0);
		}
		if(MicSelectBits&0x02)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_1, DEMO_DMA, DMAREQ_DMIC1);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_2, DEMO_DMA, DMAREQ_DMIC2);
		}
		if(MicSelectBits&0x08)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_3, DEMO_DMA, DMAREQ_DMIC3);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_4, DEMO_DMA, DMAREQ_DMIC4);
		}
		if(MicSelectBits&0x20)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_5, DEMO_DMA, DMAREQ_DMIC5);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_6, DEMO_DMA, DMAREQ_DMIC6);
		}
		if(MicSelectBits&0x80)
		{
			DMA_CreateHandle(&dmicRxDmaHandle_7, DEMO_DMA, DMAREQ_DMIC7);
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_0, DMicRx_Callback0,	NULL, &dmicRxDmaHandle_0);
		}
		if(MicSelectBits&0x02)
		{	//the second mic of this pair doesn't need a callback
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_1, NULL,            	NULL, &dmicRxDmaHandle_1);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_2, DMicRx_Callback2,	NULL, &dmicRxDmaHandle_2);
		}
		if(MicSelectBits&0x08)
		{	//the second mic of this pair doesn't need a callback
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_3, NULL,            	NULL, &dmicRxDmaHandle_3);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_4, DMicRx_Callback4,	NULL, &dmicRxDmaHandle_4);
		}
		if(MicSelectBits&0x20)
		{	//the second mic of this pair doesn't need a callback
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_5, NULL,            	NULL, &dmicRxDmaHandle_5);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_6, DMicRx_Callback6,	NULL, &dmicRxDmaHandle_6);
		}
		if(MicSelectBits&0x80)
		{	//the second mic of this pair doesn't need a callback
			DMIC_TransferCreateHandleDMA(DMIC0, &dmicDmaHandle_7, NULL,            	NULL, &dmicRxDmaHandle_7);
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_0, (void *)MicInputDmaPingpongDscr[0], 2U);
		}
		if(MicSelectBits&0x02)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_1, (void *)MicInputDmaPingpongDscr[1], 2U);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_2, (void *)MicInputDmaPingpongDscr[2], 2U);
		}
		if(MicSelectBits&0x08)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_3, (void *)MicInputDmaPingpongDscr[3], 2U);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_4, (void *)MicInputDmaPingpongDscr[4], 2U);
		}
		if(MicSelectBits&0x20)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_5, (void *)MicInputDmaPingpongDscr[5], 2U);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_6, (void *)MicInputDmaPingpongDscr[6], 2U);
		}
		if(MicSelectBits&0x80)
		{
			DMIC_InstallDMADescriptorMemory(&dmicDmaHandle_7, (void *)MicInputDmaPingpongDscr[7], 2U);
		}
	#endif

	#if EnableMic01==1
		if(MicSelectBits&0x01)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_0, MicDmaCfgCh0, DEMO_DMIC_CHANNEL_0);
		}
		if(MicSelectBits&0x02)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_1, MicDmaCfgCh1, DEMO_DMIC_CHANNEL_1);
		}
	#endif
	#if EnableMic23==1
		if(MicSelectBits&0x04)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_2, MicDmaCfgCh2, DEMO_DMIC_CHANNEL_2);
		}
		if(MicSelectBits&0x08)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_3, MicDmaCfgCh3, DEMO_DMIC_CHANNEL_3);
		}
	#endif
	#if EnableMic45==1
		if(MicSelectBits&0x10)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_4, MicDmaCfgCh4, DEMO_DMIC_CHANNEL_4);
		}
		if(MicSelectBits&0x20)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_5, MicDmaCfgCh5, DEMO_DMIC_CHANNEL_5);
		}
	#endif
	#if EnableMic67==1
		if(MicSelectBits&0x40)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_6, MicDmaCfgCh6, DEMO_DMIC_CHANNEL_6);
		}
		if(MicSelectBits&0x80)
		{
			DMIC_TransferReceiveDMA(DMIC0, &dmicDmaHandle_7, MicDmaCfgCh7, DEMO_DMIC_CHANNEL_7);
		}
	#endif
}
#endif

#endif
