/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __AlgoContainer_H___
#define __AlgoContainer_H___

typedef enum
{
	ALGO_Idx_0_None = 0,
	ALGO_Idx_1_ConversaVoiceCall,
	ALGO_Idx_2_ConversaVoiceCall,
	ALGO_Idx_3_ConversaVoiceCall,
}TAlgo;

extern int ProcAudioContainer(int WorkMode, int FrmLenInSamples, int Fs);
extern int InitAudioContainer(int WorkMode, int FrmLenInSamples, int Fs);




#endif

