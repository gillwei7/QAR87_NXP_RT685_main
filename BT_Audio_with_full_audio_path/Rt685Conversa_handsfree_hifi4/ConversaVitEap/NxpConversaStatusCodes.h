/*
 * Copyright 2021 Retune DSP
 * Copyright 2022-2023 NXP
 *
 * NXP Confidential & Proprietary. This software is owned or controlled by NXP
 * and may only be used strictly in accordance with the applicable license terms.
 * By expressly accepting such terms or by downloading, installing,
 *  activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.
 * If you do not agree to be bound by the applicable license terms,
 * then you may not retain, install, activate or otherwise use the software.
 */


#ifndef NXP_CONVERSA_STATUS_CODES_H_
#define NXP_CONVERSA_STATUS_CODES_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @enum NXP_STATUS
 *   Conversa status code returned by Conversa API
 */
typedef enum {
    OK = 0,									///< No error detected.
    GENERAL_ERROR = 1,  					///< General error detected (e.g. NULL pointer)
    MALLOC_FAIL = 2,						///< Internal heap mapping failed (e.g. estimated heap size is smaller than real heap size)
    INVALID_PARAMETER = 3,					///< A parameter is invalid (e.g. nxp_conversa_plugin_config_s or tuning params are incompatible with library constants)
	INVALID_FILTERBANK_PARAMETER = 4,		///< Filterbank configuration is incompatible (e.g. Framesize, num-bands or fb_lp can be wrong)
	INVALID_SE_MODEL_PARAMETER = 5,			///< SE-model failed to create or init due to invalid parameters
    INVALID_SIGNAL = 6,						///< \deprecated.
    LICENSE_EXPIRED = 7,					///< Timeout expired.
} NXP_STATUS;

#ifdef __cplusplus
}
#endif

#endif // NXP_CONVERSA_STATUS_CODES_H_


