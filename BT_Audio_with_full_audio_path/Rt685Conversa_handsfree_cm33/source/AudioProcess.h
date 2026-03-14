/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AudioProcess_h__
#define __AudioProcess_h__



extern unsigned char NeedToMute_Mic1;
extern unsigned char NeedToMute_Mic2;
extern unsigned char NeedToMute_Mic3;
extern unsigned char NeedToMute_Mic4;
extern unsigned char NeedToMute_SpkL;
extern unsigned char NeedToMute_SpkR;


extern int AUDIOPLL0NUM_StartingUpValue;
extern int AUDIOPLL0NUM_AdjustingValue;

extern void GenerateSinWavFromTable_S32_DualCh        (int WhichOne, S32 *DstPtrL, S32 *DstPtrR, int L);	//L should be <=AudioFrameSizeInSamplePerCh_48KHz
extern void GenerateSinWavFromTable_S32_DualCh_LRMixed(int WhichOne, S32 *DstPtr,                int L);	//L should be <=AudioFrameSizeInSamplePerCh_48KHz
extern void GenerateSinWavFromTable_S32_SingleCh      (int WhichOne, S32 *DstPtr,                int L, int PosNeg);	//L should be <=AudioFrameSizeInSamplePerCh_48KHz
extern void GenerateSinWavFromTable_S16_DualCh        (int WhichOne, S16 *DstPtrL, S16 *DstPtrR, int L);
extern void GenerateSinWavFromTable_S16_DualCh_LRMixed(int WhichOne, S16 *DstPtr,                int L);			//L should be <=AudioFrameSizeInSamplePerCh_48KHz
extern void GenerateSinWavFromTable_S16_SingleCh      (int WhichOne, S16 *DstPtr,                int L, int PosNeg);		//L should be <=AudioFrameSizeInSamplePerCh_48KHz


extern void GraduallySetAudioPllBackToDefault(void);
extern void CheckPcmRxFrBtBufAodAndAdjustAudioPll(int AodValue);



#endif
