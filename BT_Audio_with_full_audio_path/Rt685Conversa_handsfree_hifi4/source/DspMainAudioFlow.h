/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __DspMainAudioFlow_H___
#define __DspMainAudioFlow_H___

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

