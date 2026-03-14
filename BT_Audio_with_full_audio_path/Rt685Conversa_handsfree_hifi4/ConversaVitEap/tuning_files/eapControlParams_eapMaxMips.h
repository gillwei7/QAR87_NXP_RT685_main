/*
 * Copyright 2020-2021 NXP
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only
 * be used strictly in accordance with the applicable license terms found in
 * file LICENSE.txt
 */

/* For EAP16_6_x_x  */

#ifndef EAP_PARAMETER_EAPMAXMIPS
#define EAP_PARAMETER_EAPMAXMIPS
/*******************************************************************/
/* Include */
/*******************************************************************/
#include "EAP16.h" // EAP library

#ifdef ALGORITHM_PR_EQFIR
#define FIR_SIZE_EAPMAXMIPS			256		/* User defined number of coeffs for FIR filter */
#endif /* ALGORITHM_PR_EQFIR */

/***********************************************************************/
/* 								FLEX BLOCK ORDER CONFIGURATION         */
/***********************************************************************/
/* Pre fixed block :  (Mixer) - (Channel Handle) - (Tone Generator ) - (LIMiter Rms Analysis)
   Flexible order block, order need to be set with control parameter FLEX_ID. here the NXP recommended order:
     	 	 	 	  Concert Sound - PRE Volume Control - user EQualizer N Band - Digital Bass Enhancement -
                      Automatic Volume Leveler - Loudness Maximiser - POST Volume Control - LIMiter Peak -
                      Volume Control Single - PRoduct EQualizer N Band - PRoduct EQualizer FIR - Treeble Enhancement - LIMiter Rms
   Post fixed block:  (DC removal) - (Digital Gain Out) - (PSA) - (XO)*/
LVM_INT32 FlexBlock_Order_eapMaxMips [LVM_MAX_FLEX_ID] = {	//LVM_CS_FLEX_ID,
														LVM_PRE_VC_FLEX_ID,
														LVM_EQNB_FLEX_ID,
														LVM_DBE_FLEX_ID,
														LVM_AVL_FLEX_ID,
														LVM_LM_FLEX_ID,
														LVM_POST_VC_FLEX_ID,
														LVM_LIMP_FLEX_ID,
														LVM_VCS_FLEX_ID,
														LVM_PR_EQNB_FLEX_ID,
														LVM_PR_EQFIR_FLEX_ID,
														LVM_TE_FLEX_ID,
														LVM_LIMR_FLEX_ID,
														LVM_STOP_FLEX_ID };		// always need a stop at the end of the flex chain

/***********************************************************************/
/* 								MIXER CONFIG 						   */
/***********************************************************************/

LVM_MIXER_Stream_Params_t	MIXER_Stream_params_eapMaxMips[LVM_MAX_STREAMS] = {
/*
 *  Stream 1
 */
	[0] = {
		.CallbackParam 		= 0U,
		.pCallback 			= LVM_NULL,
		.pUserData 			= LVM_NULL,
		.TargetGain 		= LVM_MIXER_VOLUME_MAX,
		.TimeConstantMs 	= 0U
	},
/*
 *  Stream 2
 */
	[1] = {
		.CallbackParam 		= 0U,
		.pCallback 			= LVM_NULL,
		.pUserData 			= LVM_NULL,
		.TargetGain 		= LVM_MIXER_VOLUME_MAX,
		.TimeConstantMs 	= 0U
	},
/*
 *  Stream 3
 */
#if LVM_MAX_STREAMS>2
	[2] = {
		.CallbackParam 		= 0U,
		.pCallback 			= LVM_NULL,
		.pUserData 			= LVM_NULL,
		.TargetGain 		= LVM_MIXER_VOLUME_MAX,
		.TimeConstantMs 	= 0U
	}
#endif
};

/***********************************************************************/
/*                           EQUALISER CONFIG                          */
/***********************************************************************/
/* User EQ SET1:
 *
 */
LVM_EQNB_BandDef_t EQNB_BandDefs_UserEq1_eapMaxMips[LVM_EQNB_MAX_BANDS_NBR] = {
        {6, 50, 100},    // gain(dB), freq(Hz) , Qfactor *100
        {-4, 400, 25},
        {-3, 1100, 30},
        {2, 5000, 90},
        {4, 7000, 120},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}};

/* Speaker EQ SET1:
 *
 */
LVM_EQNB_BandDef_t EQNB_BandDefs_ProductEq1_eapMaxMips[LVM_EQNB_MAX_BANDS_NBR] = {
        {3, 50, 96},    // gain(dB), freq(Hz) , Qfactor *100
        {2, 205, 96},
        {2, 837, 96},
        {-5, 3427, 96},
        {-1, 7000, 40},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}};

/***********************************************************************/
/* 								FIR CONFIG 						   	   */
/***********************************************************************/
        
#ifdef ALGORITHM_PR_EQFIR
LVM_INT16 FIR_Coeffs_eapMaxMips[FIR_SIZE_EAPMAXMIPS] = {
    0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,     -1,     -1,     -1,     -2,     -2,     -2,
    -1,     -1,      0,      1,      2,      3,      4,      4,      5,
    4,      3,      2,      0,     -2,     -5,     -7,     -9,    -10,
    -10,     -9,     -7,     -3,      1,      6,     10,     15,     18,
    19,     19,     16,     12,      5,     -3,    -12,    -21,    -28,
    -33,    -35,    -33,    -28,    -18,     -5,      9,     25,     39,
    50,     57,     59,     54,     43,     25,      3,    -21,    -46,
    -68,    -85,    -94,    -93,    -83,    -62,    -32,      5,     44,
    82,    115,    138,    149,    143,    122,     85,     35,    -23,
    -85,   -143,   -191,   -222,   -232,   -218,   -178,   -114,    -31,
    63,    161,    251,    322,    366,    375,    342,    268,    154,
    10,   -155,   -324,   -481,   -606,   -683,   -694,   -629,   -478,
    -242,     76,    463,    901,   1370,   1842,   2291,   2689,   3012,
    3240,   3357,   3357,   3240,   3012,   2689,   2291,   1842,   1370,
    901,    463,     76,   -242,   -478,   -629,   -694,   -683,   -606,
    -481,   -324,   -155,     10,    154,    268,    342,    375,    366,
    322,    251,    161,     63,    -31,   -114,   -178,   -218,   -232,
    -222,   -191,   -143,    -85,    -23,     35,     85,    122,    143,
    149,    138,    115,     82,     44,      5,    -32,    -62,    -83,
    -93,    -94,    -85,    -68,    -46,    -21,      3,     25,     43,
    54,     59,     57,     50,     39,     25,      9,     -5,    -18,
    -28,    -33,    -35,    -33,    -28,    -21,    -12,     -3,      5,
    12,     16,     19,     19,     18,     15,     10,      6,      1,
    -3,     -7,     -9,    -10,    -10,     -9,     -7,     -5,     -2,
    0,      2,      3,      4,      5,      4,      4,      3,      2,
    1,      0,     -1,     -1,     -2,     -2,     -2,     -1,     -1,
    -1,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0
};
#endif /* ALGORITHM_PR_EQFIR */
        
/***********************************************************************/
/*                        INSTANCE PARAM CONFIG                        */
/***********************************************************************/
/* INSTANCE PARAM CONFIG SET1:
 *
 */
LVM_InstParams_t eapInstParams_eapMaxMips = {
#if 0 // OLD FILE NOT COMPATIBLE => NEED UPDATE
        /* General */
        .BufferMode         = LVM_MANAGED_BUFFERS,      /* Buffer management mode */
        .MaxBlockSize       = MAX_INTERNAL_BLOCKSIZE,                      /* Maximum processing block size max = 480 */
		.CreateMask		  =   LVM_MIXER_MASK
							| LVM_TG_MASK
							//| LVM_CS_MASK
							| LVM_VC_MASK
							| LVM_EQNB_MASK
							| LVM_DBE_MASK
							| LVM_AVL_MASK
							| LVM_LM_MASK
							| LVM_LIMP_MASK
							| LVM_VCS_MASK
							| LVM_PR_EQNB_MASK
							| LVM_PR_EQFIR_MASK
							| LVM_TE_MASK
							| LVM_LIMR_MASK
							| LVM_PSA_MASK
							| LVM_XO_MASK,

        .EQNB_NumBands      = LVM_EQNB_MAX_BANDS_NBR,   /* Maximum number of band for the equalizer */
        .PR_EQNB_NumBands   = LVM_EQNB_MAX_BANDS_NBR,   /* Maximum number of band for the equalizer */
        .Platform           = LVM_IMXRT500,             /* Choose platform between: LVM_IMXRT1050, LVM_IMXRT1060, LVM_IMXRT1064, LVM_IMXRT1170, LVM_LPC55, LVM_IMXRT500, LVM_IMXRT600 */
#ifdef ALGORITHM_PSA
        /* PSA */
        .PSA_HistorySize        = 1000,         /* PSA History size in ms: 200 to 5000 */
        .PSA_MaxBands           = 64,           /* Maximum number of bands: 6 to 64 */
        .PSA_SpectrumUpdateRate = 25,           /* Spectrum update rate : 10 to 25*/
        .PSA_Included           = LVM_PSA_ON,   /* Controls the instance memory allocation for PSA: ON/OFF */
#endif /* ALGORITHM_PSA */

		.BuildStructAlignCheck = LVM_BUILD_STRUCT_CHECK_VALUE,	// build structure alignment check value
#endif
};

/***********************************************************************/
/*                        HEADROOM PARAM CONFIG                        */
/***********************************************************************/
/* HEADROOM PARAM CONFIG SET1:
 *
 */
#ifdef ALGORITHM_EQNB
LVM_HeadroomBandDef_t HeadroomBandDef_eapMaxMips[LVM_HEADROOM_MAX_NBANDS] = {
        {20, 4999, 3},    /* Limit_Low, Limit_High, Headroom_Offset */
        {5000, 24000, 5},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}};

LVM_HeadroomParams_t eapHeadroomParams_eapMaxMips = {
        .Headroom_OperatingMode = LVM_HEADROOM_ON,                  /* Headroom Control On/Off */
        .pHeadroomDefinition    = &HeadroomBandDef_eapMaxMips[0],   /* Headroom band definition */
        .NHeadroomBands         = 2,                                /* Number of headroom bands */
};
#endif /* ALGORITHM_EQNB */

/***********************************************************************/
/*                         CONTROL PARAM CONFIG                        */
/***********************************************************************/
/* CONTROL PARAM CONFIG SET1:
 *
 */
LVM_ControlParams_t eapControlParams_eapMaxMips = {
#if 0 // OLD FILE NOT COMPATIBLE => NEED UPDATE
		/* General parameters */
        .OperatingMode          = LVM_MODE_ON,          // LVM_MODE_ON or LVM_MODE_OFF / Bundle operating mode On/Bypass
        .SampleRate             = LVM_FS_48000,         // LVM_FS_8000, LVM_FS_11025, LVM_FS_12000, LVM_FS_16000, LVM_FS_22050, LVM_FS_24000, LVM_FS_32000, LVM_FS_44100, LVM_FS_48000, LVM_FS_96000
        .SourceFormat           = LVM_STEREO,           // LVM_MONO or LVM_MONOINSTEREO or LVM_STEREO
		.SinkFormat             = LVM_STEREO,           // LVM_MONO or LVM_STEREO
		.SpeakerType            = LVM_HEADPHONES,       // LVM_HEADPHONES or LVM_MOBILE_SPEAKERS_SMALL or LVM_MOBILE_SPEAKERS_MEDIUM or LVM_MOBILE_SPEAKERS_LARGE
		.pFB_Order			 	= &FlexBlock_Order_eapMaxMips[0], // re-ordering flexible algorithm, pointer to flexible block order table

#ifdef ALGORITHM_MIXER
	.MIXER_OperatingMode = LVM_MODE_OFF,
	.MIXER_NumStreams = LVM_MAX_STREAMS,
	.pMIXER_StreamParams = MIXER_Stream_params_eapMaxMips,
#endif /* ALGORITHM_MIXER */

#ifdef ALGORITHM_TG
        /* Tone Generator parameters */
        .TG_OperatingMode   = LVM_TG_OFF,           // LVM_TG_OFF or LVM_TG_CONTINUOUS or LVM_TG_ONESHOT
        .TG_SweepMode       = LVM_TG_SWEEPLIN,      // LVM_TG_SWEEPLIN or LVM_TG_SWEEPLOG
        .TG_StartFrequency  = (LVM_UINT16)20,       // Tone Generator Sweep Start Frequency 20 to 24000 Hz
        .TG_StartAmplitude  = (LVM_INT16)-48,       // Tone Generator Sweep Start Amplitude -96 to 0 dB
        .TG_StopFrequency   = (LVM_UINT16)22000,    // Tone Generator Sweep Stop Frequency 20 to 24000 Hz
        .TG_StopAmplitude   = (LVM_INT16)-45,       // Tone Generator Sweep Stop Amplitude -96 to 0 dB
        .TG_SweepDuration   = (LVM_UINT16)1,        // Tone Generator Sweep Duration; Sweep duration in seconds, 0 for infinite duration tone
        .pTG_CallBack       = LVM_NULL,             // WARNING Callback not supported; End of sweep callback
        .TG_CallBackID      = 0x100,                // Callback ID
        .pTGAppMemSpace     = (void *)LVM_NULL,     // Application instance handle or memory area
#endif /* ALGORITHM_TG */

		/* Start flexible order block parameters */

#ifdef ALGORITHM_VIRTUALIZER
        /* Concert Sound Virtualizer parameters */
        .VirtualizerOperatingMode   = LVM_MODE_OFF,     // LVM_MODE_ON or LVM_MODE_OFF;
        .VirtualizerType            = LVM_CONCERTSOUND, // LVM_CONCERTSOUND or LVM_CINEMASOUND_MUSIC or LVM_CINEMASOUND_MOVIE or LVM_CINEMASOUND_STEREO;
        .VirtualizerReverbLevel     = (LVM_UINT16)75,   // Virtualizer reverb level min = 0% to max = 100%
        .CS_EffectLevel             = (LVM_INT16)30000, // Concert Sound effect level min = 0 to max = 32767

		/* Concert sound advanced parameter */
		#ifdef ALGORITHM_CS
				.CS_AP_Mode                 = LVM_AP_DEFAULT,   // concert sound advanced parameter mode: LVM_AP_DEFAULT or LVM_AP_MANUAL
				.CS_AP_MidGain              = 0,                // MidChannelGain: -10 to 10 dB
				.CS_AP_MidCornerFreq        = 500,              // Shelving Filter Corner Frequency: 20 to 24000 Hz
				.CS_AP_SideHighPassCutoff   = 600,              // SideBoost HighPassFilter Corner Frequency: 20 to 24000 Hz
				.CS_AP_SideLowPassCutoff    = 1544,             // SideBoost LowPassFilter Corner Frequency: 20 to 24000 Hz
				.CS_AP_SideGain             = 10,               // Side Channel Gain: 0 to 15 dB
		#endif /* ALGORITHM_CS */
#endif /* ALGORITHM_3DWIDENING */

#ifdef ALGORITHM_VC
	/* Volume Control parameters */
	.VC_EffectLevel = (LVM_INT16)-1, 				    // Volume Control setting in dBs -96 to 0 dB
	.VC_Balance = (LVM_INT16)5,   					    // Left Right Balance control in dB (-96 to 96 dB)
#endif /* ALGORITHM_VC */

#ifdef ALGORITHM_EQNB
        /* N-Band Equalizer parameters */
        .EQNB_OperatingMode     = LVM_EQNB_OFF,                         // LVM_EQNB_ON or LVM_EQNB_OFF;
        .EQNB_LPF_Mode          = LVM_EQNB_FILTER_ON,                   // LVM_EQNB_FILTER_ON or LVM_EQNB_FILTER_OFF;
        .EQNB_LPF_CornerFreq    = (LVM_INT16)7500,                      // EQNB LowPass Corner Frequency;
        .EQNB_HPF_Mode          = LVM_EQNB_FILTER_ON,                   // LVM_EQNB_FILTER_ON or LVM_EQNB_FILTER_OFF;
        .EQNB_HPF_CornerFreq    = (LVM_INT16)20,                        // EQNB HighPass Corner Frequency;
        .EQNB_NBands            = (LVM_UINT16)5,                        // Number of bands 0 to LVM_EQNB_MAX_BANDS_NBR
        .pEQNB_BandDefinition   = &EQNB_BandDefs_UserEq1_eapMaxMips[0], // EQ band configuration
#endif /* ALGORITHM_EQNB */

#ifdef ALGORITHM_DBE
        /* Bass Enhancement parameters */
        .BE_OperatingMode   = LVM_BE_OFF,               // LVM_BE_ON or LVM_BE_OFF;
        .BE_EffectLevel     = (LVM_INT16)LVM_BE_6DB,    // LVM_BE_0DB or LVM_BE_3DB or LVM_BE_6DB or LVM_BE_9DB or LVM_BE_12DB or LVM_BE_15DB;
        .BE_CentreFreq      = LVM_BE_CENTRE_90Hz,       // LVM_BE_CENTRE_55Hz or LVM_BE_CENTRE_66Hz or LVM_BE_CENTRE_78Hz or LVM_BE_CENTRE_90Hz;
        .BE_HPF             = LVM_BE_HPF_ON,            // LVM_BE_HPF_ON or LVM_BE_HPF_OFF;
#endif /* ALGORITHM_DBE */

#ifdef ALGORITHM_AVL
        /* AVL parameters */
        .AVL_OperatingMode  = LVM_AVL_OFF,  // LVM_AVL_ON or LVM_AVL_OFF
#endif /* ALGORITHM_AVL */

#ifdef ALGORITHM_LM
        /* Loudness Maximizer parameters */
        .LM_OperatingMode   = LVM_LM_OFF,       // LVM_LM_ON or LVM_LM_OFF
        .LM_EffectLevel     = LVM_LM_MEDIUM,    // LVM_LM_GENTLE or LVM_LM_MEDIUM or LVM_LM_EXTREME
        .LM_Attenuation     = (LVM_UINT16)1,    // 0 to 6 ; Loudness Maximizer output attenuation
        .LM_CompressorGain  = (LVM_UINT16)3,    // 0 to 6 ; Loudness Maximizer output compressor gain
        .LM_SpeakerCutOff   = (LVM_UINT16)150,  // 150Hz to 1100Hz & 0 to disable; Loudness Maximizer speaker cut off frequency
#endif /* ALGORITHM_LM */

#ifdef ALGORITHM_LIMP
        .LIMP_OperatingMode = LVM_LIMP_OFF,  // LIMP operating mode: LVM_LIMP_ON or LVM_LIMP_OFF
        .LIMP_Threshold     = -12,           // LIMP threshold in dB: -24dB to 0dB
#endif /* ALGORITHM_LIMP */

#ifdef ALGORITHM_VCS
	/* Volume Control Single parameters */
	.VCS_EffectLevel = (LVM_INT16)-3, 		 // Volume Control Single setting in dBs -96 to 0 dB
#endif /* ALGORITHM_VCS */

#ifdef ALGORITHM_PR_EQNB
        /* N-Band Equalizer parameters */
        .PR_EQNB_OperatingMode      = LVM_EQNB_OFF,                             // LVM_EQNB_ON or LVM_EQNB_OFF;
        .PR_EQNB_LPF_Mode           = LVM_EQNB_FILTER_ON,                       // LVM_EQNB_FILTER_ON or LVM_EQNB_FILTER_OFF;
        .PR_EQNB_LPF_CornerFreq     = (LVM_INT16)7300,                          // EQNB LowPass Corner Frequency;
        .PR_EQNB_HPF_Mode           = LVM_EQNB_FILTER_ON,                       // LVM_EQNB_FILTER_ON or LVM_EQNB_FILTER_OFF;
        .PR_EQNB_HPF_CornerFreq     = (LVM_INT16)20,                            // EQNB HighPass Corner Frequency;
        .PR_EQNB_NBands             = (LVM_UINT16)5,                            // Number of bands 0 to MAX_EQNB_BANDS
        .pPR_EQNB_BandDefinition    = &EQNB_BandDefs_ProductEq1_eapMaxMips[0],  // EQ band configuration
#endif /* ALGORITHM_PR_EQNB */

#ifdef ALGORITHM_PR_EQFIR
	/* FIR parameters */
	.FIR_OperatingMode = LVM_FIR_OFF,				// LVM_FIR_ON or LVM_FIR_OFF
	.FIR_CoeffWidth	   = LVM_FIR_COEFF_16BIT,		// LVM_FIR_COEFF_16BIT (Q.12) or LVM_FIR_COEFF_32BIT (Q.27)
	.FIR_NCoeffs	   = FIR_SIZE_EAPMAXMIPS,					// Number of coefficients to be used in provided array
	.FIR_pCoeffs	   = FIR_Coeffs_eapMaxMips,		// Array of filter coefficients
#endif /* ALGORITHM_PR_EQFIR */

#ifdef ALGORITHM_TE
        /* Treble Enhancement parameters */
        .TE_OperatingMode   = LVM_TE_OFF,       // LVM_TE_ON or LVM_TE_OFF
        .TE_EffectLevel     = (LVM_INT16)10,    // Treble Enhancement gain 0dB to 15dB or LVM_TE_LOW_MIPS for saving MIPS
#endif /* ALGORITHM_TE */

#ifdef ALGORITHM_LIMR
        .LIMR_OperatingMode = LVM_LIMR_OFF,         // LVM_LIMR_ON or LVM_LIMR_OFF
        .LIMR_Reference     = LVM_LIMR_REF_0DBFS,   // LIMR reference input: LVM_LIMR_REF_INPUT or LVM_LIMR_REF_0DBFS
        .LIMR_Threshold     = -24,                  // LIMR threshold in dB: -24dB to 0dB
#endif /* ALGORITHM_LIMR */

		/* End flexible order block parameters */

		/* Digital output gain parameters */
		.DG_GainOutput		 = 3,					// digital gain output in dBs from -96 to +20 dBs, 1 dB step

#ifdef ALGORITHM_PSA
        /* General Control */
        .PSA_Enable         = LVM_PSA_OFF,          // LVM_PSA_ON or LVM_PSA_OFF;
        /* Spectrum Analyzer parameters */
        .PSA_PeakDecayRate  = LVM_PSA_SPEED_SLOW,   // LVM_PSA_SPEED_SLOW or LVM_PSA_SPEED_MEDIUM or LVM_PSA_SPEED_FAST; Peak value decay rate
        .PSA_NumBands       = (LVM_UINT16)32,       // 6 to 64; Number of Bands
#endif /* ALGORITHM_PSA */

#ifdef ALGORITHM_XO
        /* Crossover module parameter */
        .XO_OperatingMode   = LVM_MODE_OFF, 		// LVM_MODE_ON or LVM_MODE_OFF
        .XO_cutoffFrequency = 4000,         		// Cutoff frequency in Hz (range  = [60 Hz - 6 000 Hz])
#endif /* ALGORITHM_XO */

		.BuildStructAlignCheck = LVM_BUILD_STRUCT_CHECK_VALUE,	// build structure alignment check value
#endif
};

#endif // end    EAP_PARAMETER_EAPMAXMIPS
