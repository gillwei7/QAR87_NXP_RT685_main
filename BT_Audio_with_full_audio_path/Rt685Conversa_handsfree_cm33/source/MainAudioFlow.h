/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __MainAudioFlow_h__
#define __MainAudioFlow_h__


#define AudioPdmPortsBitMapFlag_Mic01			0x01
#define AudioPdmPortsBitMapFlag_Mic23			0x02

#if EnableMic45==1
	#define AudioPdmPortsBitMapFlag_Mic45			0x04
#endif

#if EnableMic67==1
	#define AudioPdmPortsBitMapFlag_Mic67			0x08
#endif

#define AudioI2sPortsBitMapFlag_Fc1				0x10
#define AudioI2sPortsBitMapFlag_Fc3				0x20
#define AudioI2sPortsBitMapFlag_FcTxToNt		0x40
#define AudioI2sPortsBitMapFlag_FcRxFrNt		0x80

extern int BTAudioBitWidth;
extern int BTAudioFs;

extern volatile U8 PdmIsStarted;
extern volatile U8 I2sIsStarted;

extern volatile U8 DmaTxRxIsDone;
extern volatile U8 DmaTxRxIsExpected;

extern int AOD_BTDnBuf;
extern int AOD_BTUpBuf;
extern uint8_t domainId;


extern U32 MainAudioFlowMode;		//0: no audio flow, 1,2,3: flow1,2,3

extern volatile S32 *MicInputCh0Ptr;
extern volatile S32 *MicInputCh1Ptr;
extern volatile S32 *MicInputCh2Ptr;
extern volatile S32 *MicInputCh3Ptr;
extern volatile S32 *MicInputCh4Ptr;
extern volatile S32 *MicInputCh5Ptr;
extern volatile S32 *MicInputCh6Ptr;
extern volatile S32 *MicInputCh7Ptr;

extern int CheckTimePoint_CurrentIntrIsAStartingOne(void);

extern void CloseAllActivedPDMPorts(void);
extern void CloseAllActivedI2SPorts(void);


extern void SetFcClkSharing(int ClkSrcId, int ClkDstId, int InternalSharingGroupIdx);
extern void InitAudioPLLForAllAudioPeripherals(void);
extern void InitBaseAudioClkForPdm(void);
extern void Deinit_Board_Audio(void);


extern void USB_MicUpStreamDataRateControl_AdjustPacketLength(int AodInCirBuf);


extern void ProcessAudio_AfterAudioInputBufIsReady_InCall(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_HomeVitStandBy(void);

extern void ProcessAudio_AfterAudioInputBufIsReady_AudioIoDbg(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_VideoRecording(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_MediaPlayer(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_MusicPlayer(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_Translation(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_AiConversation(void);
extern void ProcessAudio_AfterAudioInputBufIsReady_VideoAi(void);

extern void ProcessAudio_AfterAudioInputBufIsReady_TestMicSpk(void);

extern void McuMainAudioFlowFinalize_AudioIoDbg(void);
extern void McuMainAudioFlowFinalize_VideoRecording(void);
extern void McuMainAudioFlowFinalize_MediaPlayer(void);
extern void McuMainAudioFlowFinalize_MusicPlayer(void);
extern void McuMainAudioFlowFinalize_Translation(void);
extern void McuMainAudioFlowFinalize_AiConversation(void);
extern void McuMainAudioFlowFinalize_VideoAi(void);


extern void InitAudioInterface_AudioIoDebug(int Opt);
extern void DeInitAudioInterface_AudioIoDebug(int Opt);
extern void InitAudioInterface_HfpCall(int Opt);
extern void DeInitAudioInterface_HfpCall(int Opt);
extern void InitAudioInterface_HomeVitStandby(int Opt);
extern void DeInitAudioInterface_HomeVitStandby(int Opt);
extern void InitAudioInterface_VideoRecording(int Opt);
extern void DeInitAudioInterface_VideoRecording(int Opt);
extern void InitAudioInterface_MediaPlayer(int Opt);
extern void DeInitAudioInterface_MediaPlayer(int Opt);
extern void InitAudioInterface_MusicPlayer(int Opt);
extern void DeInitAudioInterface_MusicPlayer(int Opt);
extern void InitAudioInterface_Translation(int Opt);
extern void DeInitAudioInterface_Translation(int Opt);
extern void InitAudioInterface_AiConversation(int Opt);
extern void DeInitAudioInterface_AiConversation(int Opt);
extern void InitAudioInterface_VideoAi(int Opt);
extern void DeInitAudioInterface_VideoAi(int Opt);


#endif

