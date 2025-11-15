/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __SRCProc_H__
#define __SRCProc_H__

#include "xa_type_def.h"

//#define MAX_MEM_ALLOCS 100
#define MAX_MEM_ALLOCS 10			//6 is actual needed

extern WORD  g_w_malloc_count_Ref;
extern WORD  g_w_malloc_count_TxOut;
//extern WORD  g_w_malloc_count_Mic;
//extern WORD  g_w_malloc_count_Spk;
//extern WORD  g_w_malloc_count_RxOut;

extern pVOID g_pv_arr_alloc_memory_Ref[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_TxOut[MAX_MEM_ALLOCS];
//extern pVOID g_pv_arr_alloc_memory_Mic[MAX_MEM_ALLOCS];
//extern pVOID g_pv_arr_alloc_memory_Spk[MAX_MEM_ALLOCS];
//extern pVOID g_pv_arr_alloc_memory_RxOut[MAX_MEM_ALLOCS];

extern xa_codec_handle_t SRC_process_Ref_handle;
extern xa_codec_handle_t SRC_process_TxOut_handle;
//extern xa_codec_handle_t SRC_process_Mic_handle;
//extern xa_codec_handle_t SRC_process_Spk_handle;
//extern xa_codec_handle_t SRC_process_RxOut_handle;





extern xa_codec_handle_t SrcHandle_AlgCtnBus1;
extern xa_codec_handle_t SrcHandle_AlgCtnBus2;
extern xa_codec_handle_t SrcHandle_AlgCtnTx1;
extern xa_codec_handle_t SrcHandle_AlgCtnTx2;
extern xa_codec_handle_t SrcHandle_AlgCtnRx1;
extern xa_codec_handle_t SrcHandle_AlgCtnMic01;
extern xa_codec_handle_t SrcHandle_AlgCtnMic23;
extern xa_codec_handle_t SrcHandle_AlgCtnMic45;

extern WORD  g_w_malloc_SrcHandle_AlgCtnBus1;
extern WORD  g_w_malloc_SrcHandle_AlgCtnBus2;
extern WORD  g_w_malloc_SrcHandle_AlgCtnTx1;
extern WORD  g_w_malloc_SrcHandle_AlgCtnTx2;
extern WORD  g_w_malloc_SrcHandle_AlgCtnRx1;
extern WORD  g_w_malloc_SrcHandle_AlgCtnMic01;
extern WORD  g_w_malloc_SrcHandle_AlgCtnMic23;
extern WORD  g_w_malloc_SrcHandle_AlgCtnMic45;

extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus1[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgCtnBus2[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgTx1[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgTx2[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgRx1[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgMic01[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgMic23[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_SrcHandle_AlgMic45[MAX_MEM_ALLOCS];







extern WORD  g_w_malloc_DecoderSbc;
extern WORD  g_w_malloc_DecoderOpus;
//static heap space for each SRC processor
extern pVOID g_pv_arr_alloc_memory_DecoderSbc[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_DecoderOpus[MAX_MEM_ALLOCS];
//API obj for each SRC processor
extern xa_codec_handle_t DecoderSbc_handle;
extern xa_codec_handle_t DecoderOpus_handle;


//extern int AsrcInBuf[48*2];
//extern int AsrcOutBuf[48*2];

extern void DeinitCadenceAsrc(void **pv_arr_alloc_memory, signed int *w_malloc_count);
extern int InitCadenceAsrc(xa_codec_handle_t *xa_process_handle, int InputBlockSizeInSamples, int inFs, int outFs, int ChNum,  void **pv_arr_alloc_memory, signed int *w_malloc_count, int NeedToDisplay);
extern int ProcCadenceAsrc(xa_codec_handle_t *xa_process_handle, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum);

#endif  //__WAVEIO_H__
