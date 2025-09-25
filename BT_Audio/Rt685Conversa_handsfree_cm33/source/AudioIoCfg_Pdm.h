/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __AudioIoCfg_pdm_h__
#define __AudioIoCfg_pdm_h__



#include "GlobalDef.h"

#if EnableConversa==1

#include "fsl_dmic_dma.h"
#include "fsl_i2s_dma.h"



#define EnableMic01		1
#define EnableMic23		1
#define EnableMic45		0
#define EnableMic67		0

#define DMAREQ_DMIC0 16U
#define DMAREQ_DMIC1 17U
#define DMAREQ_DMIC2 18U
#define DMAREQ_DMIC3 19U
#define DMAREQ_DMIC4 20U
#define DMAREQ_DMIC5 21U
#define DMAREQ_DMIC6 22U
#define DMAREQ_DMIC7 23U

#define DEMO_DMIC_CHANNEL_0 kDMIC_Channel0
#define DEMO_DMIC_CHANNEL_1 kDMIC_Channel1
#define DEMO_DMIC_CHANNEL_2 kDMIC_Channel2
#define DEMO_DMIC_CHANNEL_3 kDMIC_Channel3
#define DEMO_DMIC_CHANNEL_4 kDMIC_Channel4
#define DEMO_DMIC_CHANNEL_5 kDMIC_Channel5
#define DEMO_DMIC_CHANNEL_6 kDMIC_Channel6
#define DEMO_DMIC_CHANNEL_7 kDMIC_Channel7


#define PDM_FIFO_DEPTH_InSamples (15U)		//what ever this value is good (0~15)
#define PDM_Rx_BUFFER_NUM (2U)

///////////////////////////////////////////////////////////////////////////////////////////////////////
extern unsigned int *Ptr_dma_descriptor_table0;

extern U32 TimePoint_PrevAudioCallBack;

#if EnableMic01==1
	extern volatile S32 MicInputDmaDualBuf_0[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
	extern volatile S32 MicInputDmaDualBuf_1[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
#endif
#if EnableMic23==1
	extern volatile S32 MicInputDmaDualBuf_2[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
	extern volatile S32 MicInputDmaDualBuf_3[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
#endif
#if EnableMic45==1
	extern volatile S32 MicInputDmaDualBuf_4[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
	extern volatile S32 MicInputDmaDualBuf_5[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
#endif
#if EnableMic67==1
	extern volatile S32 MicInputDmaDualBuf_6[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
	extern volatile S32 MicInputDmaDualBuf_7[AudioFrameSizeInSamplePerCh * PDM_Rx_BUFFER_NUM];
#endif

extern volatile U8 PdmCh0DmaTransferringIsUsingBufA;
extern volatile U8 PdmCh2DmaTransferringIsUsingBufA;
extern volatile U8 PdmCh4DmaTransferringIsUsingBufA;
extern volatile U8 PdmCh6DmaTransferringIsUsingBufA;


extern void Init_MicDmaCfgCh(U8 MicEnableBits);


extern void BOARD_DeInit_DMA_PDM(U8 MicEnableBits);
extern void BOARD_Init_DMA_PDM(U8 MicEnableBits);
extern void BOARD_Init_DMIC(U8 MicEnableBits, U8 SkipInitGlobalDMIC0);

extern int GetPdmCh0DmaTransferringIsUsingBufAOrB(void);
extern int GetPdmCh2DmaTransferringIsUsingBufAOrB(void);
extern int GetPdmCh4DmaTransferringIsUsingBufAOrB(void);
extern int GetPdmCh6DmaTransferringIsUsingBufAOrB(void);

extern int CheckTimePoint_CurrentIntrIsAStartingOne(void);

extern void ConfigDmicChainedDma(U8 MicEnableBits);
extern void ImmediatelyStartDmicDmaChannels(U8 MicEnableBits);

extern void SCO_AudioFlow_SemaphorePost(void);

#endif

#endif

