/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __SRCProc_H__
#define __SRCProc_H__

#include "xa_type_def.h"

#define SRC_MAX_MEM_ALLOCS		10

typedef struct
{
	xa_codec_handle_t xa_process_handle;
	int InputBlockSizeInSamples;
	int inFs;
	int outFs;
	int ChNum;

	void *AllocatedMemPtr[SRC_MAX_MEM_ALLOCS];
	signed int MemAllocCnt;

	int EnableAsrc;
	float AsrcDriftingValueCurrent;
	float AsrcDriftingValueTarget;

	//PID control for adjusting drifting --- inorder to keep AOD is half
	float AodTgtValue;
	float Kp;
	float Ki;
	float KiAcc;
	float KiAccMax;
}TCadenceSRC;

#if UseUacDnAudioForConversaTuning_VoiceCall16KHz==1
extern TCadenceSRC SRC_ConversaUacDn;
#endif
extern TCadenceSRC SRC_ConversaTx1;
extern TCadenceSRC SRC_ConversaRx1;
extern TCadenceSRC SRC_ConversaRx2;
extern TCadenceSRC SRC_DecoderSbc;
extern TCadenceSRC SRC_DecoderOpus;

extern void CadenceSrc_UpdateDrifting(TCadenceSRC *SRCPtr, int CurrentAod, int ForceDriftingTo0);
extern void DeinitCadenceAsrc        (TCadenceSRC *SRCPtr);
extern int InitCadenceAsrc           (TCadenceSRC *SRCPtr, int InputBlockSizeInSamples, int inFs, int outFs, int ChNum,  int EnableAsrc, int NeedToDisplayy);
extern int ProcCadenceAsrc           (TCadenceSRC *SRCPtr, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum);


#endif  //__WAVEIO_H__
