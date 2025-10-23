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

extern WORD  g_w_malloc_count_Mic;
extern WORD  g_w_malloc_count_Spk;
extern WORD  g_w_malloc_count_Ref;
extern WORD  g_w_malloc_count_TxOut;
extern WORD  g_w_malloc_count_RxOut;

extern pVOID g_pv_arr_alloc_memory_Mic[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_Spk[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_Ref[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_TxOut[MAX_MEM_ALLOCS];
extern pVOID g_pv_arr_alloc_memory_RxOut[MAX_MEM_ALLOCS];

extern xa_codec_handle_t SRC_process_Mic_handle;
extern xa_codec_handle_t SRC_process_Spk_handle;
extern xa_codec_handle_t SRC_process_Ref_handle;
extern xa_codec_handle_t SRC_process_TxOut_handle;
extern xa_codec_handle_t SRC_process_RxOut_handle;




extern int AsrcInBuf[48*2];
extern int AsrcOutBuf[48*2];

extern void DeinitCadenceAsrc(void **pv_arr_alloc_memory, signed int *w_malloc_count);
extern int InitCadenceAsrc(xa_codec_handle_t *xa_process_handle, int InputBlockSizeInSamples, int inFs, int outFs, int ChNum,  void **pv_arr_alloc_memory, signed int *w_malloc_count, int NeedToDisplay);
extern int ProcCadenceAsrc(xa_codec_handle_t *xa_process_handle, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum);

#endif  //__WAVEIO_H__
