/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __AudioProcConversa_H___
#define __AudioProcConversa_H___


#include "PL_platformTypes.h"
#include "NxpConversaPlugin.h"


#define	NXP_CONVERSA_AEC_REFERENCE 			0		 // Conversa AEC reference input channel type(1)current sensing or (0)Rx path output

#define NXP_CONVERSA_AEC_FILTER_LENGTH_FOR_SPEAKERPHONE_MS (128.0f) // in ms
#define NXP_CONVERSA_AEC_FILTER_LENGTH_MIN_MS	   		   (15.0f)  // in ms (lower has no impact as the TAP filter is already at minimum)

/* Pre-setup Conversa Speech enhancement parameters
 *    - Parameters when Se model enabled are extracted from the header of the Se model
 *    - Parameters bellow are only used when Se model is not enabled
 *    - Parameters bellow are initially set to reproduce similar configuration than Se model but they can be updated
 */
#define	CONVERSA_NUM_BAND_16K				128			 	// Set number of band for the 16kHz
#define	CONVERSA_FILTER_BANK_SIZE_16K   	768			 	// Set filter bank window size for the 16kHz
#define	CONVERSA_NUM_BAND_32K_48K  		    128			 	// Set number of band for the 32-48kHz
#define	CONVERSA_FILTER_BANK_SIZE_32K_48K  	768			 	// Set filter bank window size for the 32k_48kkHz

// Offset to decode speech enhancement model
#define SE_MODEL_U32_OFFSET_SIZE		3
#define SE_MODEL_U32_OFFSET_SAMPLE_RATE	12
#define SE_MODEL_U32_OFFSET_NUM_BANDS	13
#define SE_MODEL_U32_OFFSET_FRAMESIZE	14
#define SE_MODEL_U32_OFFSET_FB_LP  		15

#define RT600_CACHE_MEM_START_ADR		0x20040000

// Conversa Operating mode
typedef enum
{
	CONVERSA_DISABLE = 0,
	CONVERSA_ENABLE  = 1,
	CONVERSA_OM_MAX = PL_MAXENUM,  // to force enum in 32bits
}CONVERSA_operatingMode_en;

// CONVERSA_ML_MODEL
typedef enum
{
	CONVERSA_ML_MODEL_DISABLE = 0,
	CONVERSA_ML_MODEL_SMALL   = 1,
	CONVERSA_ML_MODEL_LARGE   = 2,
	CONVERSA_ML_MODEL_MAX = PL_MAXENUM,  // to force enum in 32bits
} CONVERSA_ML_MODEL_en;

typedef enum
{
	CONVERSA_BF_MODE_DISABLE 		 = 0,
	CONVERSA_BF_MODE_ADAPTIVE 		 = 1,
	CONVERSA_BF_MODE_ADAPTIVE_OR_FIX = 2,
	CONVERSA_BF_MODE_ADAPTIVE_MAX = PL_MAXENUM,  // to force enum in 32bits
} CONVERSA_BF_MODE_en;

// data format type
typedef enum
{
	CONVERSA_PROCESS_FLOAT,				// Conversa process in float
	CONVERSA_PROCESS_Q31,				// Conversa process in Q31
	CONVERSA_PROCESS_MAX = PL_MAXENUM,  // to force enum in 32bits
} CONVERSA_processFormat_en;


/* Conversa tuning parameters config */
typedef struct
{
	PL_UINT32 settings_format_version;		// Conversa tuning parameters format version
	PL_UINT32 conversa_lib_version[3];		// Conversa tuning parameters conversa library version
	PL_UINT32 num_mic;						// Conversa tuning parameters number of input microphone
	PL_UINT32 num_spk;						// Conversa tuning parameters number of output speaker
	PL_UINT32 tx_samplerate;				// Conversa tuning parameters tx path sample rate
	PL_UINT32 rx_samplerate;				// Conversa tuning parameters rx path sample rate
	PL_UINT32 dsp_blocksize;				// Conversa tuning parameters dsp block size
	PL_UINT32 num_bands;					// Conversa tuning parameters number of bands
	PL_INT8   info_str[128];   				// Conversa tuning parameters string information
	PL_UINT32 parameter_data_size;			// Conversa tuning parameters parameter data size
	PL_UINT32 parameter_data[];				// Conversa tuning parameters parameters table
} conversa_parameter_config_t;

/* parameter file structure */
typedef struct
{
	const conversa_parameter_config_t* 		p_address;				// Conversa tuning parameter address
	//CONVERSA_operatingMode_en				enable;  				// Conversa tuning parameters enable external Conversa tuning parameter. If disable then Conversa used default library parameters
	const PL_FLOAT*							p_fixBFCalib;			// Conversa tuning fix steering beam former calibration address
} AUDIO_ConversaTuningParam_st;

/* model file structure */
typedef struct
{
	const PL_UINT8*  						p_address;			    // pointer to Conversa Se model
	PL_UINT32								size_byte;				// size of the Conversa Se model in bytes
	//CONVERSA_ML_MODEL_en 					type;					// enable/disable and select Conversa Speech enhancement ML algorithm
} AUDIO_ConversaSeModel_st;

/* Conversa overall structure */
typedef struct
{
	AUDIO_operatingMode_en   				operatingModeTx;				 // enable disable conversa Sw Ip Tx path
	AUDIO_operatingMode_en   				operatingModeRx;				 // enable disable conversa Sw Ip Rx path
	PL_UINT32							    conversaDataControlAddress; 	 // Address control the conversa data (used by the tuning tool)
	nxp_conversa_plugin_constants_t			conversaPluginConst;    		 // constant which decribe the conversa limit
	nxp_conversa_plugin_t					conversaPluginParams;   		 // conversa parameters
	AUDIO_ConversaTuningParam_st			conversaTuningParams; 		 	 // conversa tuning parameters definition
	AUDIO_ConversaSeModel_st				conversaSeModel;				 // conversa Se model definition
	PL_BOOL 								conversaLicenseTimeOut;	     	 // conversa license time out False True
	PL_UINT16								conversaDoaAngle;				 // conversa DOA angle
	PL_FLOAT								conversaFBfPfWeight;			 // conversa fix beam former post filter weight (Who is talking ? feature)
	//AUDIO_deviceType_en					    conversaAudioDeviceType;		 // conversa audio device type
} AUDIO_conversa_st;

enum
{
	CONVERSA_OutSignalIdx_RxOut = 0,
	CONVERSA_OutSignalIdx_TxOut,
	CONVERSA_OutSignalIdx_AecOut,
	CONVERSA_OutSignalIdx_BfOut,
	CONVERSA_OutSignalIdx_NlpOut
};

extern uint8_t domainId;

extern U32 AudioFrameCnt;


extern int ConversaIsNowBeingUsed;
extern nxp_conversa_plugin_t conversaPluginParams;

extern S32 *RawMic32BitBuf0;
extern S32 *RawMic32BitBuf1;
extern S32 *RawMic32BitBuf2;
extern S32 *RawMic32BitBuf3;

extern S32 *ConversaTxOut32BitBuf;
extern S32 *AecOut32BitBuf;
extern S32 *BfOut32BitBuf;
extern S32 *NlpOut32BitBuf;

extern void DeInitConversa(void);
extern void InitConversa(ConversaTuningCfg_t TuningCfg, int NeedToPrintInfo);

extern void DspMainAudioFlowProcOneFrame_HfpCall_(int OptionWord);
extern void DspMainAudioFlowProcOneFrame_HfpCall(int OptionWord);

#endif

