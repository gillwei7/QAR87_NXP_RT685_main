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
      [Home]                 {MediaPlayer}
      [Home]                 {Menu}
      [Home]                 {Photo}
      [Home]                 {Translation}
      [Home]                 {VideoAI}
      [Home]                 {VideoRecording}
      [MediaPlayer]          {Home}
      [MediaPlayer]          {down}
      [MediaPlayer]          {next}
      [MediaPlayer]          {pause}
      [MediaPlayer]          {play}
      [MediaPlayer]          {previous}
      [MediaPlayer]          {up}
      [Menu]                 {Home}
      [Menu]                 {direction}
      [Menu]                 {select}
      [MusicPlayer]          {Home}
      [MusicPlayer]          {down}
      [MusicPlayer]          {next}
      [MusicPlayer]          {pause}
      [MusicPlayer]          {play}
      [MusicPlayer]          {previous}
      [MusicPlayer]          {up}
      [Translation]          {Home}
      [VideoAI]              {Home}
      [VideoRecording]       {Home}
*/

#define INTENT_SLOT_TUPLE_SIZE  28

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_AIConversation[], VIT_MODEL_ALIGN_BYTES) = {
      PL_TRUE,		//go homeaiconversation stop
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
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
};

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Call[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_TRUE,	//answer the phone/call
      PL_TRUE,	//hang up the phone/call
      PL_TRUE,	//reject the phone/call
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

const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Home[], VIT_MODEL_ALIGN_BYTES) = {
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//start ai conversation
      PL_TRUE,		//go media player
      PL_TRUE,		//open menu
      PL_TRUE,		//take photo / take picture
      PL_TRUE,		//start translation
      PL_TRUE,		//start video ai
      PL_TRUE,		//start recording
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
      PL_FALSE,
      PL_TRUE,		//go homemediaplayer
      PL_TRUE,		//volume downmediaplayer
      PL_TRUE,		//next
      PL_TRUE,		//pause
      PL_TRUE,		//play
      PL_TRUE,		//previous
      PL_TRUE,		//volume upmediaplayer
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
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go homemusicplayer
      PL_TRUE,		//volume downmusicplayer
      PL_TRUE,		//next song
      PL_TRUE,		//pause music
      PL_TRUE,		//play music
      PL_TRUE,		//previous song
      PL_TRUE,		//volume upmusicplayer
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
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go hometranslation stop
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
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go homevideoai stop
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
      PL_FALSE,
      PL_FALSE,
      PL_FALSE,
      PL_TRUE,		//go homevideorecording stop
};


const PL_MEM_ALIGN(PL_BOOL VIT_Intent_Slot_Mask_Menu[], VIT_MODEL_ALIGN_BYTES) = {
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
      PL_TRUE,		//go homemainmenu
      PL_TRUE,		//go left/right/next
      PL_TRUE,		//pickup enter
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

#endif      /* __VIT_INTENT_SLOT_MASK_H__ */

/* End of file */





/*
CmdID1 go homeaiconversation
CmdID1 go homemediaplayer
CmdID1 go homemenu
CmdID1 go homemusicplayer
CmdID1 go hometranslation
CmdID1 go homevideoai
CmdID1 go homevideorecording
CmdID3 stopaiconversation
CmdID3 stopmediaplayer
CmdID3 stopmusicplayer
CmdID3 stoptranslation
CmdID3 stopvideoai
CmdID3 stopvideorecording
CmdID4 answer the phone
CmdID5 reject the phone
CmdID6 hang up the phone
CmdID7 take photo
CmdID8 take picture
CmdID9 start translation
CmdID10 start video ai
CmdID11 start ai conversation
CmdID12 start recording
CmdID13 go media player
CmdID14 open menu
CmdID15 play
CmdID16 pause
CmdID17 previous
CmdID18 next
CmdID19 volume upmediaplayer
CmdID19 volume upmusicplayer
CmdID20 volume downmediaplayer
CmdID20 volume downmusicplayer
CmdID22 go left
CmdID23 go right
CmdID24 go next
CmdID25 pickup
CmdID26 enter
CmdID27 play music
CmdID28 pause music
CmdID29 previous song
CmdID30 next song







go home
stop
answer the phone
reject the phone
hang up the phone
take photo
take picture
start translation
start video AI
start AI conversation
start recording
go media player
open menu
play
pause
previous
next
volume up
volume down
go left
go right
go next
pickup
enter
play music
pause music
previous song
next song

*/
