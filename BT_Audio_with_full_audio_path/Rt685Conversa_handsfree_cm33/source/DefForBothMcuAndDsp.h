/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __DefForBothMcuAndDsp_h__
#define __DefForBothMcuAndDsp_h__



#if EnableUsbComAndAudio==1

//-----------------------------USB audio config that user can change---------------------------------
#if 1	//---folding

//interval value to select, only 0x01, 0x02 or 0x03 is allowed --- recommend not to change this
//NOTE: feedback seems only work when High Speed OUT interval=1: 125us micro frame  --- and with: AUDIO_SAMPLING_RATE_1_TO_10_14 = (6 << 10)
#define HS_ISO_OUT_ENDP_INTERVAL (0x01)			//the reason to have interval value <4 is to to decrease packet size. when having 8 ch, 32bit, 1ms packet is > 1023 byte --- must reduce packet length to have packet size < 1023
#define HS_ISO_IN_ENDP_INTERVAL  (0x02)

#define HS_ISO_FEEDBACK_ENDP_INTERVAL (0x04U)
#define FS_ISO_FEEDBACK_ENDP_INTERVAL (0x01U)

//-----------------UAC Up stream channel number select-----all the following options are available and can be selected--------------
//#define UsbAudioFormat_UpStreamChNumIsOne
//#define UsbAudioFormat_UpStreamChNumIsTwo
//#define UsbAudioFormat_UpStreamChNumIsFour
//#define UsbAudioFormat_UpStreamChNumIsSix
#define UsbAudioFormat_UpStreamChNumIsEight

//-----------------UAC Dn stream channel number select-----all the following options are available and can be selected--------------
//#define UsbAudioFormat_DnStreamChNumIsOne
#define UsbAudioFormat_DnStreamChNumIsTwo
//#define UsbAudioFormat_DnStreamChNumIsFour
//#define UsbAudioFormat_DnStreamChNumIsSix
//#define UsbAudioFormat_DnStreamChNumIsEight

#endif
//-----------------------------USB audio config that user can change---------------------------------



//--------------- USB upstream/downstream audio format def --- user should NOT change this part--------------------
#if 1	//---for folding --- not suggested to change this part
#ifdef UsbAudioFormat_UpStreamChNumIsOne
	#define AUDIO_IN_FORMAT_CHANNELS (0x01)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsTwo
	#define AUDIO_IN_FORMAT_CHANNELS (0x02)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsFour
	#define AUDIO_IN_FORMAT_CHANNELS (0x04)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsSix
	#define AUDIO_IN_FORMAT_CHANNELS (0x06)
#endif
#ifdef UsbAudioFormat_UpStreamChNumIsEight
	#define AUDIO_IN_FORMAT_CHANNELS (0x08)
#endif

#define AUDIO_IN_SAMPLING_RATE_KHZ (16)
#define AUDIO_IN_FORMAT_BITS (32)
#define AUDIO_IN_FORMAT_SIZE (4)



#ifdef UsbAudioFormat_DnStreamChNumIsOne
	#define AUDIO_OUT_FORMAT_CHANNELS (0x01)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsTwo
	#define AUDIO_OUT_FORMAT_CHANNELS (0x02)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsFour
	#define AUDIO_OUT_FORMAT_CHANNELS (0x04)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsSix
	#define AUDIO_OUT_FORMAT_CHANNELS (0x06)
#endif
#ifdef UsbAudioFormat_DnStreamChNumIsEight
	#define AUDIO_OUT_FORMAT_CHANNELS (0x08)
#endif

#define AUDIO_OUT_SAMPLING_RATE_KHZ (48)
#define AUDIO_OUT_FORMAT_BITS (32)
#define AUDIO_OUT_FORMAT_SIZE (4)

#endif	//---for folding
//--------------- USB upstream/downstream audio format def --- user should not change this part--------------------
#endif


//-------------following defines that are not suggested to be changed-------------------
//-------------following defines that are not suggested to be changed-------------------
//-------------following defines that are not suggested to be changed-------------------

#if 1	//folding
#define MuEvtMcuToDsp_AudioFrmIsReady_HfpCall				0x00000001
#define MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy		0x00000002
#define MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg			0x00000003
#define MuEvtMcuToDsp_AudioFrmIsReady_VideoRecording		0x00000004
#define MuEvtMcuToDsp_AudioFrmIsReady_MediaPlayer			0x00000005
#define MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer			0x00000006
#define MuEvtMcuToDsp_AudioFrmIsReady_Translation			0x00000007
#define MuEvtMcuToDsp_AudioFrmIsReady_AiConversation		0x00000008
#define MuEvtMcuToDsp_AudioFrmIsReady_VideoAi				0x00000009

#define MuEvtDspToMcu_AudioProcIsFinished_HfpCall			0x00000010
#define MuEvtDspToMcu_AudioProcIsFinished_HomeVitStandBy	0x00000020
#define MuEvtDspToMcu_AudioProcIsFinished_AudioIoDbg		0x00000030
#define MuEvtDspToMcu_AudioProcIsFinished_VideoRecording	0x00000040
#define MuEvtDspToMcu_AudioProcIsFinished_MediaPlayer		0x00000050
#define MuEvtDspToMcu_AudioProcIsFinished_MusicPlayer		0x00000060
#define MuEvtDspToMcu_AudioProcIsFinished_Translation		0x00000070
#define MuEvtDspToMcu_AudioProcIsFinished_AiConversation	0x00000080
#define MuEvtDspToMcu_AudioProcIsFinished_VideoAi			0x00000090



#define SEMA42_ID_SbcRawCirBufferProtect	0x00
#define SEMA42_ID_PrintProtect				0x01

#define APP_SEMA42								SEMA42
#define SEMA42_GATE0							SEMA42_ID_PrintProtect
#define SEMA42_GATE1							SEMA42_ID_SbcRawCirBufferProtect
#define SEMA42_GATE2							2U

#define AudioFrameSizeInSamplePerChMaxForDMABuf		(128*3)
//both PDM and I2S are 16KHz
#define AudioFrameSizeInSamplePerCh_16KHz 			(128)			//conversa must work with framesize=128, should never change this value, unless Conversa lib's frame size is changed --- this is 8ms
#define AudioFrameSizeInSamplePerCh_48KHz 			(128*3)			//this is 8ms

#if Rt685I2SToNvtIsI2SMaster==0
#define AudioFrameSizeInSamplePerCh_I2SToNvt		48
#define I2SNt_CirBuf_LenInSamples					(48*6+AudioFrameSizeInSamplePerCh_16KHz)			//this is 8.66ms
#endif


#define AUDIO_OUT_TRANSFER_LENGTH_ONE_FRAME (AUDIO_OUT_SAMPLING_RATE_KHZ * AUDIO_OUT_FORMAT_CHANNELS * AUDIO_OUT_FORMAT_SIZE)

#endif


typedef enum _VoiceCommandMenu
{
	ASR_Menu_None=0,
    ASR_Menu_Home,
	ASR_Menu_VidioAi,
	ASR_Menu_AiConversation,
	ASR_Menu_Translation,
	ASR_Menu_VideoRecording,
	ASR_Menu_MediaPlayer,
	ASR_Menu_PhoneCall,
	ASR_Menu_MusicPlayer,
	ASR_Menu_MainMenu,
} VoiceCommandMenu_t;

typedef enum _VoiceCommandItem
{
	ASR_VoiceCommand_None=0,
	ASR_VoiceCommand_GoHomeAiconversation,

	ASR_VoiceCommand_AnswerThePhone,
	ASR_VoiceCommand_RejectThePhone,
	ASR_VoiceCommand_HangUpThePhone,

	ASR_VoiceCommand_TakePhoto,				//this one covers both take photo and take picture
	ASR_VoiceCommand_StartRecording,
	ASR_VoiceCommand_StartTranslation,
	ASR_VoiceCommand_StartVideoAi,
	ASR_VoiceCommand_StartAiConversation,
	ASR_VoiceCommand_OpenMenu,
	ASR_VoiceCommand_GoMediaPlayer,

	ASR_VoiceCommand_Play,
	ASR_VoiceCommand_Pause,
	ASR_VoiceCommand_Previous,
	ASR_VoiceCommand_Next,
	ASR_VoiceCommand_VolumeUpMediaplayer,
	ASR_VoiceCommand_VolumeDownMediaplayer,
	ASR_VoiceCommand_GoHomeMediaplayer,

	ASR_VoiceCommand_GoHomeMainMenu,
	ASR_VoiceCommand_MenuDirection_Left,
	ASR_VoiceCommand_MenuDirection_Right,
	ASR_VoiceCommand_MenuDirection_Next,
	ASR_VoiceCommand_MenuSelect_Pickup,
	ASR_VoiceCommand_MenuSelect_Enter,

	ASR_VoiceCommand_PlayMusic,
	ASR_VoiceCommand_PauseMusic,
	ASR_VoiceCommand_PreviousSong,
	ASR_VoiceCommand_NextSong,
	ASR_VoiceCommand_VolumeUpMusicplayer,
	ASR_VoiceCommand_VolumeDownMusicplayer,
	ASR_VoiceCommand_GoHomeMusicplayer,

	ASR_VoiceCommand_GoHomeTranslation,
	ASR_VoiceCommand_GoHomeVideoAi,
	ASR_VoiceCommand_GoHomeVideoRecording,
} VoiceCommandItem_t;


#define CirBuf_SbcRaw_LengthInBytes 20000				//this value was checked by printing MCU writing and DSP reading rd/wr pointers --- at the beginning, MCU will write 8000 bytes in, so 20K byte size is a proper size
#define CirBuf_SbcRaw_MaxReadSizeLengthInBytes 512		//using 512 because Cadence Sbc decoder uses 512 bytes as input buffer size, this is the maximum size reading from this cir buffer

//MCU program must has the exact same struct def as the following --- this h file should be included by both MCU and DSP prj
typedef struct
{
	//part 1 --- one frame of audio buf for each source and sink port
	__attribute__((aligned(32))) S32 PdmInAudioBuf[8][AudioFrameSizeInSamplePerCh_16KHz];

	__attribute__((aligned(32))) S32 UacUpAudioBuf[AudioFrameSizeInSamplePerCh_16KHz*8];		//this buffer is channel mixed, and to be used as cir buffer data source
	__attribute__((aligned(32))) S32 UacDnAudioBufL[AudioFrameSizeInSamplePerCh_48KHz*3];		//when local fs=16KHz, Uac dn is 48KHz, need 3 times of AudioFrameSizeInSamplePerCh_16KHz space
	__attribute__((aligned(32))) S32 UacDnAudioBufR[AudioFrameSizeInSamplePerCh_48KHz*3];		//when local fs=16KHz, Uac dn is 48KHz, need 3 times of AudioFrameSizeInSamplePerCh_16KHz space
	__attribute__((aligned(32))) S16 I2SLineInBufL[AudioFrameSizeInSamplePerChMaxForDMABuf]; 		// from amp, for AEC, not use now
	__attribute__((aligned(32))) S16 I2SLineInBufR[AudioFrameSizeInSamplePerChMaxForDMABuf];
	__attribute__((aligned(32))) S16 I2SInNvtBufL[AudioFrameSizeInSamplePerChMaxForDMABuf]; 		//from nvt, cm33 write in, DSP conversa process
	__attribute__((aligned(32))) S16 I2SInNvtBufR[AudioFrameSizeInSamplePerChMaxForDMABuf];

	__attribute__((aligned(32))) S16 I2SLineOtBufL[AudioFrameSizeInSamplePerChMaxForDMABuf]; 		//tx to amp, ex. Conversa RX output
	__attribute__((aligned(32))) S16 I2SLineOtBufR[AudioFrameSizeInSamplePerChMaxForDMABuf];
	__attribute__((aligned(32))) S16 I2SOtNvtBufL[AudioFrameSizeInSamplePerChMaxForDMABuf]; 		//out to nvt, DSP write, cm33 deliver to nvt
	__attribute__((aligned(32))) S16 I2SOtNvtBufR[AudioFrameSizeInSamplePerChMaxForDMABuf];

	__attribute__((aligned(32))) S32 BTRxInAudio[AudioFrameSizeInSamplePerCh_16KHz];	//MCU side writes in the full frame if BT is at 16KHz, writes in half if BT is at 8KHz
	__attribute__((aligned(32))) S32 BTTxOtAudio[AudioFrameSizeInSamplePerCh_16KHz];	//MCU side takes out the full frame if BT is at 16KHz, takes out half if BT is at 8KHz


	//part 2 --- others
	U32 BtHfpFs;
	U32 I2SFs_Nvt;
	U32 I2SFs_Amp;
	U32 PdmFs;
	U32 UacUpFs;
	U32 UacDnFs;
	U32 I2SFrmSizeInSamples_Amp;
	U32 I2SFrmSizeInSamples_Nvt;
	U32 PdmFrmSizeInSamples;

	U32 U32CycCntHistory	[100];
	U32 MonitorInfoArray1	[20];
	U32 MonitorInfoArray2	[20];
	U32 U32ControlPara		[40];
	U32 FileAddrTable_Opus	[20];
	U32 FileAddrTable_Sbc	[10];

	U32 NeedToStartPlayOpus;
	U32 PlayOpusFileIdx;
	U32 NeedToStartPlaySbc;
	U32 PlaySbcFileIdx;
	U32 NeedToStopA2dpSbc;

	#if 0
		//if using this type, command info can not be passed to MCU ???
		VoiceCommandMenu_t CurVoiceMenu;
		VoiceCommandMenu_t PreVoiceMenu;
		VoiceCommandMenu_t CurrentVoiceCommandIntent;
		VoiceCommandItem_t CurrentVoiceCommandTagName;
	#else
		//have to use U32 as the command info type
		U32 CurVoiceMenu;
		U32 PreVoiceMenu;
		U32 CurrentVoiceCommandIntent;
		U32 CurrentVoiceCommandTagName;
		U32 WWIsDetected;
	#endif

	U32 U32Tmp2;
	U32 Cm33Hifi1HandShakeCheck;

	//SBC decoding RAW data from MCU to DSP
	int  CirBuf_SbcRaw;	//use dummy variable here is to avoid the T_CircularAudioBuf_S8 type missing error
	int  CirBuf_SbcRaw_Dummy1;		//dummy is to reserve the space for (T_CircularAudioBuf_S8 *)&CirBuf_SbcRaw
	int  CirBuf_SbcRaw_Dummy2;		//dummy is to reserve the space for (T_CircularAudioBuf_S8 *)&CirBuf_SbcRaw
	int  CirBuf_SbcRaw_Dummy3;		//dummy is to reserve the space for (T_CircularAudioBuf_S8 *)&CirBuf_SbcRaw

	U32 CirBuf_SbcRaw_DataArea[(CirBuf_SbcRaw_LengthInBytes+CirBuf_SbcRaw_MaxReadSizeLengthInBytes)/4];
} T_CommonVarSharedByDspAndMcu;




#endif
