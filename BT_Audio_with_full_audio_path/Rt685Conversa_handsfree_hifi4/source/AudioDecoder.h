/*
 * Copyright (c) 2009-2019 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __AudioDecoder_H___
#define __AudioDecoder_H___



#if EnableOpusDec==1
extern int OpusDecoderIsRunning;
extern T_CircularAudioBuf_S32 OpusOutputCirBuf_LRMixed;

extern int InitOpusDecoder(void);
extern void DeInitOpusDecoder(void);
extern int InitOpusDecoderForOneOpusFile(int OpusFileIdx);
extern void DeInitOpusDecoderForOneOpusFile(void);

extern void OpusDecodeProcess(void);
extern void InitOpusOutputCirBuf(void);
#endif

#if EnableSbcDec==1
extern int SbcDecoderIsMutedButStillRunning;
extern int SbcOutputCirBuf_LRMixed_IsHalfFull;
extern int SbcDecoderIsInited;
extern int SbcDecoderIsRunning;
extern T_CircularAudioBuf_S32 SbcOutputCirBuf_LRMixed;

extern int  InitSbcDecoder(void);
extern void DeInitSbcDecoder(void);
extern int InitSbcDecoderForOneSbcFile(int SbcFileIdx);
extern void DeInitSbcDecoderForOneSbcFile(void);

extern int SbcDecodeProcess(int SbcFileIdx);
extern void InitSbcOutputCirBuf(void);

#endif

#endif /* __CORE_DSP_H_GENERIC */
