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

//for SRC
extern S32 SrcIn_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo
extern S32 SrcOut_2S32Mixed [48*20*2];		//to hold 20ms at 48KHz, stereo

//for decoding OPUS and SBC
extern S16 AudioOneFrameBuf_OpusDecodedL[AudioFrameSizeInSamplePerCh_48KHz];
extern S16 AudioOneFrameBuf_OpusDecodedR[AudioFrameSizeInSamplePerCh_48KHz];
extern S16 AudioOneFrameBuf_SbcDecodedL [AudioFrameSizeInSamplePerCh_48KHz];
extern S16 AudioOneFrameBuf_SbcDecodedR [AudioFrameSizeInSamplePerCh_48KHz];

//for holding 6 ch mic input --- converted to float
extern S32 AudioOneFrameBuf_16KHz_01 [AudioFrameSizeInSamplePerCh_16KHz];
extern S32 AudioOneFrameBuf_16KHz_02 [AudioFrameSizeInSamplePerCh_16KHz];
extern S32 AudioOneFrameBuf_16KHz_03 [AudioFrameSizeInSamplePerCh_16KHz];
extern S32 AudioOneFrameBuf_16KHz_04 [AudioFrameSizeInSamplePerCh_16KHz];
extern S32 AudioOneFrameBuf_16KHz_05 [AudioFrameSizeInSamplePerCh_16KHz];
extern S32 AudioOneFrameBuf_16KHz_06 [AudioFrameSizeInSamplePerCh_16KHz];


//for holding 48KHz input --- converted to float
extern S32 AudioOneFrameBuf_48KHz_01 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_02 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_03 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_04 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_05 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_06 [AudioFrameSizeInSamplePerCh_48KHz];

//for temp1,2(L R) 48KHz float/S32 and SignalGenerator
extern S32 AudioOneFrameBuf_48KHz_07 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_08 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_09 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_10 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_11 [AudioFrameSizeInSamplePerCh_48KHz];
extern S32 AudioOneFrameBuf_48KHz_12 [AudioFrameSizeInSamplePerCh_48KHz];

//for temp1,2(L R) 48KHz S16
extern S16 AudioOneFrameBuf_48KHz_13 [AudioFrameSizeInSamplePerCh_48KHz];
extern S16 AudioOneFrameBuf_48KHz_14 [AudioFrameSizeInSamplePerCh_48KHz];
extern S16 AudioOneFrameBuf_48KHz_15 [AudioFrameSizeInSamplePerCh_48KHz];
extern S16 AudioOneFrameBuf_48KHz_16 [AudioFrameSizeInSamplePerCh_48KHz];

extern ConversaTuningCfg_t CurrentConversaTuningCfg;

extern void DspMainAudioFlowProcOneFrame_AudioIoDbg(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_VideoRecording(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_MediaPlayer(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_MusicPlayer(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_Translation(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_AiConversation(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_VideoAi(int OptionWord);


extern void SimpleSrc3xUp(S32 *DstPtr, S32 *SrcPtr, int LenOfInput);
extern void SimpleSrc3xDn(S32 *DstPtr, S32 *SrcPtr, int LenOfOutput);

extern void ConversaProcessAndFeedToVit(float **PtrArray_MicIn, float *PtrRxIn, float **PtrArray_OutSignals, S16 *RawMicSigForVitRef);

#endif

