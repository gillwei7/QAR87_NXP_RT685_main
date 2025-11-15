/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __DspMainAudioFlow_H___
#define __DspMainAudioFlow_H___


extern XosMutex 		  g_audio_vitBufferMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task
extern XosMutex			  g_audio_SbcDecoderMutex;
extern XosMutex			  g_audio_OpusDecoderMutex;						//to really use mutex protecting cir buffer --- should move audio signal flow process to a task --- to be done later

extern uint8_t domainId;
extern U32 AudioFrameCnt;
extern int ToTempSkipVitPorcess;

extern S32 SrcIn_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo
extern S32 SrcOut_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo

extern S16 AudioOneFrameBuf_OpusDecodedL [AudioFrameSizeInSamplePerCh];
extern S16 AudioOneFrameBuf_OpusDecodedR [AudioFrameSizeInSamplePerCh];
extern S16 AudioOneFrameBuf_SbcDecodedL [AudioFrameSizeInSamplePerCh];
extern S16 AudioOneFrameBuf_SbcDecodedR [AudioFrameSizeInSamplePerCh];

extern S32 AudioOneFrameBuf_01 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_02 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_03 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_04 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_05 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_06 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_07 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_08 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_09 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_10 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_11 [AudioFrameSizeInSamplePerCh];
extern S32 AudioOneFrameBuf_12 [AudioFrameSizeInSamplePerCh];

extern void DspMainAudioFlowProcOneFrame_AudioIoDbg(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_VideoRecording(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_MediaPlayer(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_MusicPlayer(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_Translation(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_AiConversation(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_VideoAi(int OptionWord);


extern void SimpleSrc3xUp(S32 *DstPtr, S32 *SrcPtr, int LenOfInput);
extern void SimpleSrc3xDn(S32 *DstPtr, S32 *SrcPtr, int LenOfOutput);

#endif

