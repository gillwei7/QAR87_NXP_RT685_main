/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __AudioProcVit_H___
#define __AudioProcVit_H___

#ifndef CORE_DSP
#define CORE_DSP
#endif

// platform types
#include "PL_platformTypes.h"
#include "VIT.h"

#define ASE_DetectDisplayInUacWav				1

#define APP_VIT_FRAME_SIZE_InSAMPLEs			VIT_SAMPLES_PER_30MS_FRAME
//#define DSP_AUDIO_VIT_THREAD_STACK_SIZE_BYTE 	(3*1024U)						// DSP audio VIT thread stack size in bytes
//#define DSP_AUDIO_VIT_THREAD_PRIORITY   		(XOS_MAX_PRIORITY - 4)			// DSP audio VIT thread priority


#define MEMORY_ALIGNMENT            8       		// In bytes
//#define DEVICE_ID					VIT_IMXRT600	// Device ID


#define VIT_OPERATING_MODE_WW  (VIT_WAKEWORD_ENABLE)
#define VIT_OPERATING_MODE_S2I (VIT_SPEECHTOINTENT_ENABLE)

#define configPRINTF PRINTF


/* Configure the detection period in second for each command
   VIT will return UNKNOWN if no command is recognized during this time span.
   When only activating one mode, WWD or CMD, even timeout event trigger,
   no model switching happens automatically. Set it to 60s aligned with UI session.
 */
#define VIT_COMMAND_TIME_SPAN 8


typedef enum _asr_session
{
    ASR_SESSION_STOPPED,
    ASR_SESSION_WAKE_WORD,
    ASR_SESSION_INTENT,
} asr_session_t;


typedef enum
{
	AUDIO_VIT_USE_CONVERSA_TXOUT =  0,	// VIT use Conversa TxOut data as input
	AUDIO_VIT_USE_CONVERSA_AECOUT = 1,	// VIT use Conversa AecOut data as input
	AUDIO_VIT_USE_CONVERSA_BFOUT =  2,	// VIT use Conversa BfOut data as input
	AUDIO_VIT_USE_CONVERSA_NLPOUT = 3,	// VIT use Conversa AecOut data as input
	AUDIO_VIT_USE_RAW_MIC = 		4,	// VIT use raw mic data as input
	AUDIO_VIT_MAX = PL_MAXENUM,			// to force enum in 32bits
}AUDIO_VIT_INPUT_en;

typedef struct
{
	PL_INT16 * p_buffer;					// pointer to buffer
	PL_INT16 * p_bufferStart;    			// beginning of the circular buffer
	PL_INT16 * p_bufferEnd;	  				// end of the circular buffer
	PL_INT16 * p_currentRead;	  			// first readable data
	PL_INT16 * p_currentWrite;	  			// first writable data
	PL_UINT32	threshold;					// number of empty samples to write
	PL_UINT32   bufferSampleWidth; 			// number of bytes per sample
}TOOLS_ConvToVit_st;

typedef struct
{
	PL_INT16*  p_vitBuffer;					// Memory used for the circular buffer
	PL_UINT32  bufferSizeInBytes;			// size of the memory used for the circular buffer
	PL_UINT32  bufferSampleWidth;			// size of each samples in bytes

}TOOLS_ConvToVit_param;

typedef struct
{
	AUDIO_VIT_INPUT_en 	 vitInputSelect;		// VIT selection input
	PL_UINT32 			 num_mic;				// VIT num of mics
	PL_UINT32 			 framesize;				// VIT framesize
	VIT_OperatingMode_en operatingMode;			// VIT operating mode (LPVAD, WW, CMD, S2I)
	PL_FLOAT			 timeSpan;				// VIT time span for command detection
} VIT_parameter_config_t;

/* model file structure */
typedef struct
{
	const PL_UINT8*  						p_address;			    // pointer to VIT model
	PL_UINT32								size_byte;				// size of the VIT model in bytes
	VIT_Model_Location_en	  				vitModelMem;			// memory location of VIT model
} AUDIO_VITModel_st;

typedef struct
{
	TOOLS_ConvToVit_param					vitBufferParam;					// Parameters for VIT buffer
	TOOLS_ConvToVit_st						vitBufferHandle;				// Handle for VIT buffer
	VIT_parameter_config_t					vitConfig;
	AUDIO_VITModel_st						vitModel;						// VIT model information
	VIT_Handle_t							vitHandle;						// VIT handle
	VIT_InstanceParams_st					vitInstParams;					// VIT instance configuration
	VIT_ControlParams_st					vitControlParams;				// VIT control parameter structure
	PL_MemoryTable_st						vitMemoryTable;					// VIT memory table
	PL_INT8                   				*pMemory[PL_NR_MEMORY_REGIONS]; // VIT memory pool address
} AUDIO_vit_st;

#if ASE_DetectDisplayInUacWav==1
typedef enum _ASR_WavPulseType
{
	ASR_WavPulse_NothingDetected =  1,
	ASR_WavPulse_WakeWordDetected,
	ASR_WavPulse_VoiceCmdDetected,
}ASR_WavPulseType;

extern ASR_WavPulseType ASR_WavPulse;
#endif

extern AUDIO_vit_st	vitPluginParams;


extern XosMutex 		  g_audio_vitBufferMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task
extern XosMutex			  g_audio_SbcDecoderMutex;
extern XosMutex			  g_audio_OpusDecoderMutex;						//to really use mutex protecting cir buffer --- should move audio signal flow process to a task --- to be done later

extern void InitVit(void);
extern void ConfigAndStartVitTask(void);



#endif



