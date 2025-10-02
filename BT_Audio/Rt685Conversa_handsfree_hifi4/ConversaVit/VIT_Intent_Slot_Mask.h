/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __VIT_INTENT_SLOT_MASK_H__
#define __VIT_INTENT_SLOT_MASK_H__

/*
WakeWord supported : 
    WW_Id  : WW_Name
      1    : HEY QUANTA

Speech Intents supported
  Tuple : [Intent]           {Slot Tag}
      [AIConversation]       {Home}
      [Call]                 {answer}
      [Call]                 {hang_up}
      [Call]                 {reject}
      [Home]                 {AIConversation}
      [Home]                 {VideoRecording}
      [Home]                 {MediaPlayer}
      [Home]                 {Photo}
      [Home]                 {Translation}
      [Home]                 {VideoAI}
      [MediaPlayer]          {down}
      [MediaPlayer]          {home}
      [MediaPlayer]          {next}
      [MediaPlayer]          {pause}
      [MediaPlayer]          {play}
      [MediaPlayer]          {previous}
      [MediaPlayer]          {up}
      [MusicPlayer]          {down}
      [MusicPlayer]          {home}
      [MusicPlayer]          {next}
      [MusicPlayer]          {pause}
      [MusicPlayer]          {play}
      [MusicPlayer]          {previous}
      [MusicPlayer]          {up}
      [Photo]                {Home}
      [Translation]          {Home}
      [VideoAI]              {Home}
      [VideoRecording]       {Home}
*/

#define INTENT_SLOT_TUPLE_SIZE  28

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_AIConversation[], VIT_MODEL_ALIGN_BYTES) = {
      PL_TRUE,		//go homeaiconversation
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Call[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_TRUE,	//answer the phone
      PL_TRUE,	//reject the phone
      PL_TRUE,	//hang up the phone
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Home[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//take photo
      PL_TRUE,		//take picture
      PL_TRUE,		//start translation
      PL_TRUE,		//start video ai
      PL_TRUE,		//start ai conversation
      PL_TRUE,		//go media player  -----  start recording ?
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_MediaPlayer[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//play
      PL_TRUE,		//pause
      PL_TRUE,		//previous
      PL_TRUE,		//next
      PL_TRUE,		//volume upmediaplayer
      PL_TRUE,		//volume downmediaplayer
      PL_TRUE,		//go homemediaplayer
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_MusicPlayer[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//play music
      PL_TRUE,		//pause music
      PL_TRUE,		//previous song
      PL_TRUE,		//next song
      PL_TRUE,		//volume upmusicplayer
      PL_TRUE,		//volume downmusicplayer
      PL_TRUE,		//go homemusicplayer
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Photo[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go homephoto
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Translation[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go hometranslation
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_VideoAI[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go homevideoai
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_VideoRecording[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go homevideorecording
};

#endif      /* __VIT_INTENT_SLOT_MASK_H__ */

/* End of file */
