/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __AudioProcEap_H___
#define __AudioProcEap_H___

#include "EAP16.h"


#define NOTIF_VOLUME_TRANSITION_TIME_MS	50	// Time in milliseconds of the volume slopes for notification sound playing


/* EAP */
typedef struct
{
	PL_BOOL					enable;
	LVM_Handle_t            hInstance;                          /* Instance handle */
	LVM_ControlParams_t     *pControlParams;                    /* control Parameters */
    LVM_InstParams_t        *pInstParams;                       /* EAP_InstParams */
    LVM_HeadroomParams_t    *pHeadroomParams;               /* Headroom parameters */
    LVM_MemTab_t            memTab;                             /* Memory allocation table */
} AUDIO_Eap_st;


extern int EapIsNowBeingUsed;

extern PL_BOOL 			EapAllowsWritingStrm1AsPromptAudio;
extern PL_BOOL			EapRefusesStartingANewPromptPlaying;

//the main eap handle for music play and media play
extern AUDIO_Eap_st		eap_1_Handle;					    // EAP overall structure. If CORE DSP is used then it must be placed in non cached memory section so no volatile is not needed
//the second eap handler together with the first one, being used for video recording
//extern AUDIO_Eap_st		eap_2_Handle;					    // EAP overall structure. If CORE DSP is used then it must be placed in non cached memory section so no volatile is not needed

extern void InitEap(EapTuningCfg_t CfgIdx, int ToPrint_Level);
extern status_t DeInitEap(AUDIO_Eap_st* 		p_eapDefinition);


extern void eapNotificationCallbackDown(void* Instance, void* UserData, short int CallbackArg);
extern void eapNotificationCallbackUp(void* Instance, void* UserData, short int CallbackArg);
extern void EapProcess(S16 **DstPtrs, S16 **SrcPtrs, int SampleNum, EapTuningCfg_t EapTuneCfg);



#endif



