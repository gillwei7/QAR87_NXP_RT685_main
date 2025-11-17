/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __Cirbuf___
#define __Cirbuf___


//Note both the BTUp and BTDn cir buffers are at 8KHz
#define BTUpAudioBuf_Len_InSamples	1024		//this is 128ms at 16KHz
#define BTDnAudioBuf_Len_InSamples	1024		//this is 128ms at 16KHz

#define BTUpAudioBuf_MaxReadLen_InSamples	512	//actually no need for this extra space --- when DMA sending the audio data in this cir buffer, MUST move the audio from the cir buffer to a 4 bytes aligned space
#define BTDnAudioBuf_MaxReadLen_InSamples	(AudioFrameSizeInSamplePerCh_16KHz/2)




#define AodLevel_2_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*2/8)
#define AodLevel_3_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*3/8)
#define AodLevel_4_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*4/8)
#define AodLevel_5_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*5/8)
#define AodLevel_6_8_LenInSamples ((BTDnAudioBuf_Len_InSamples-512)*6/8)




extern volatile T_CommonVarSharedByDspAndMcu VarBlockSharedByDspAndMcu;

extern S32 AllZeroBuf_48PointsSingleCh_16Bit [48*8];


extern U8 UpStreamingIsStarted;

#if Rt685I2SToNvtIsI2SMaster==0
	//only when NT is I2S master, we use cir buffer
	extern T_CircularAudioBuf_S32 I2SRxFrNt_CirBuf;
	extern T_CircularAudioBuf_S32 I2STxToNt_CirBuf;
#endif

extern T_CircularAudioBuf_S16 BTDnAudioBuf_S16;
extern T_CircularAudioBuf_S16 BTUpAudioBuf_S16;

#if EnableUsbComAndAudio==1
	extern T_CirUacDnAudioBuf_MCh UacDnAudioBuf_MCh;
	extern T_CirUacUpAudioBuf_MCh UacUpAudioBuf_MCh;
#endif

extern void ClearAudioCirBuf(int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir);
extern void InitAudioCircularBuf(int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir);


#endif
