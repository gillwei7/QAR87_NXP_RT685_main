/*
 * Copyright 2020-2025 NXP
 *
 * NXP Confidential. This software is owned or controlled by NXP and may only
 * be used strictly in accordance with the applicable license terms found in
 * file LICENSE.txt
 */

/*
 * ** DO NOT MODIFY THIS SECTION **
 * ATT API version: 3.0
 */

#ifndef EAP_CONFIG_MIXER_ON_EFFECTS_OFF_H
#define EAP_CONFIG_MIXER_ON_EFFECTS_OFF_H
/***********************************************************************/
/*                               Include                               */
/***********************************************************************/
#include "EAP16.h"  // EAP library
LVM_MIXER_Stream_Params_t MIXER_Stream_params_mixer[LVM_MAX_STREAMS] = {
    /*
     *  Stream 1
     */
    {
        .TargetGain = LVM_MIXER_VOLUME_MAX,
        .TimeConstantMs = 0U,
        .pCallback = LVM_NULL,
        .CallbackParam = 0U,
        .pUserData = LVM_NULL,
    },
    /*
     *  Stream 2
     */
    {
        .TargetGain = LVM_MIXER_VOLUME_MIN,
        .TimeConstantMs = 10U,

        .pCallback = LVM_NULL,
        .CallbackParam = 1U,
        .pUserData = LVM_NULL,
    },
    /*
     *  Stream 3
     */
    {
        .TargetGain = LVM_MIXER_VOLUME_MIN,
        .TimeConstantMs = 0U,
        .pCallback = LVM_NULL,
        .CallbackParam = 2U,
        .pUserData = LVM_NULL,
    }};

/* HEADROOM PARAM CONFIG SET1:
 *
 */
#ifdef ALGORITHM_EQNB
LVM_HeadroomBandDef_t HeadroomBandDef_default[LVM_HEADROOM_MAX_NBANDS] = {
        {20, 4999, 3},    /* Limit_Low, Limit_High, Headroom_Offset */
        {5000, 5500, 5},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}};

LVM_HeadroomParams_t HeadroomParams_default = {
        .Headroom_OperatingMode = LVM_HEADROOM_OFF,                  /* Headroom Control On/Off */
        .pHeadroomDefinition    = &HeadroomBandDef_default[0],  			/* Headroom band definition */
        .NHeadroomBands         = 1                               /* Number of headroom bands */
};
#endif /* ALGORITHM_EQNB */

LVM_ControlParams_t ControlParamSet_default = {
    /* control Parameters */
    .OperatingMode = LVM_MODE_ON,
    .SampleRate = LVM_FS_48000,
    .SourceFormat = LVM_STEREO,
    .SpeakerType = LVM_HEADPHONES,
    .SpeakerTypeInternal = LVM_SPEAKER_STEREO,
    .MIXER_OperatingMode = LVM_MODE_OFF,
    .MIXER_NumStreams = LVM_MAX_STREAMS,
    .pMIXER_StreamParams = MIXER_Stream_params_mixer,

    //.VirtualizerOperatingMode = LVM_MODE_OFF,
    //.VirtualizerType = LVM_CONCERTSOUND,
    //.VirtualizerReverbLevel = 75,

    //.CS_EffectLevel = 30000,

    .EQNB_OperatingMode = LVM_EQNB_OFF,
    .EQNB_LPF_Mode = LVM_EQNB_FILTER_OFF,
    .EQNB_LPF_CornerFreq = 5000,
    .EQNB_HPF_Mode = LVM_EQNB_FILTER_OFF,
    .EQNB_HPF_CornerFreq = 1000,
    .EQNB_NBands = 0,
    .pEQNB_BandDefinition = LVM_NULL,
    .PR_EQNB_OperatingMode = LVM_EQNB_OFF,
    .PR_EQNB_LPF_Mode = LVM_EQNB_FILTER_OFF,
    .PR_EQNB_LPF_CornerFreq = 5000,
    .PR_EQNB_HPF_Mode = LVM_EQNB_FILTER_OFF,
    .PR_EQNB_HPF_CornerFreq = 1000,
    .PR_EQNB_NBands = 0,
    .pPR_EQNB_BandDefinition = LVM_NULL,
    .BE_OperatingMode = LVM_BE_OFF,
    .BE_EffectLevel = 15,
    .BE_CentreFreq = LVM_BE_CENTRE_55Hz,
    .BE_HPF = LVM_BE_HPF_OFF,
    .VC_EffectLevel = 0,
    .VC_Balance = 0,
    .TE_OperatingMode = LVM_TE_OFF,
    .TE_EffectLevel = 9,
    .LM_OperatingMode = LVM_LM_OFF,
    .LM_EffectLevel = LVM_LM_GENTLE,
    .LM_Attenuation = 0,
    .LM_CompressorGain = 2,
    .LM_SpeakerCutOff = 250,
    .AVL_OperatingMode = LVM_AVL_OFF,
    .TG_OperatingMode = LVM_TG_OFF,
    .TG_SweepMode = LVM_TG_SWEEPLOG,
    .TG_StartFrequency = 20,
    .TG_StartAmplitude = -48,
    .TG_StopFrequency = 2000,
    .TG_StopAmplitude = -45,
    .TG_SweepDuration = 1,
    .pTG_CallBack = LVM_NULL,
    .TG_CallBackID = 0,
    .pTGAppMemSpace = LVM_NULL,

    //.PSA_Enable = LVM_PSA_OFF,
    //.PSA_PeakDecayRate = LVM_PSA_SPEED_SLOW,
    //.PSA_NumBands = 32,

    .LIMP_OperatingMode = LVM_LIMP_OFF,
    .LIMP_Threshold = -3,
    .LIMR_OperatingMode = LVM_LIMR_OFF,
    .LIMR_Reference = LVM_LIMR_REF_0DBFS,
    .LIMR_Threshold = -24,

    //.XO_OperatingMode = LVM_MODE_OFF,
    //.XO_cutoffFrequency = 100,

};
LVM_InstParams_t InstParams_default = {
    .BufferMode = LVM_MANAGED_BUFFERS,
    .MaxBlockSize = MAX_INTERNAL_BLOCKSIZE,
    .EQNB_NumBands = LVM_EQNB_MAX_BANDS_NBR,
    .PR_EQNB_NumBands = LVM_EQNB_MAX_BANDS_NBR,
    .Platform = LVM_IMXRT600,
}; /* EAP_InstParams */

#endif  // end    EAP_CONFIG_MIXER_ON_EFFECTS_OFF_H
