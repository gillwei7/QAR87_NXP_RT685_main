/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __Cirbuf___
#define __Cirbuf___

#if EnableConversa==1
//Note both the BTUp and BTDn cir buffers are at 8KHz
#define BTUpAudioBuf_Len_InSamples	1024		//this is 128ms at 16KHz
#define BTDnAudioBuf_Len_InSamples	1024		//this is 128ms at 16KHz

#define BTUpAudioBuf_MaxReadLen_InSamples	512	//actually no need for this extra space --- when DMA sending the audio data in this cir buffer, MUST move the audio from the cir buffer to a 4 bytes aligned space
#define BTDnAudioBuf_MaxReadLen_InSamples	(AudioFrameSizeInSamplePerCh/2)



#define AodLevel_2_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*2/8)
#define AodLevel_3_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*3/8)
#define AodLevel_4_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*4/8)
#define AodLevel_5_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*5/8)
#define AodLevel_6_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*6/8)




extern T_CommonVarSharedByDspAndMcu VarBlockSharedByDspAndMcu;

extern S32 AllZeroBuf_48PointsSingleCh_16Bit [48*8];


extern U8 UpStreamingIsStarted;



extern T_CircularAudioBuf_S16 BTDnAudioBuf_S16;
extern T_CircularAudioBuf_S16 BTUpAudioBuf_S16;

#if EnableUsbComAndAudio==1
	extern T_CirUacDnAudioBuf_MCh UacDnAudioBuf_MCh;
	extern T_CirUacUpAudioBuf_MCh UacUpAudioBuf_MCh;
#endif

extern void InitAudioCircularBuf(void);


#endif
#endif

