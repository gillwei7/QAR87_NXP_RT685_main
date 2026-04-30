/*
 * Copyright 2021 by Retune DSP
 * Copyright 2022-2023, 2025 NXP
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP
 * and may only be used strictly in accordance with the applicable license terms.
 * By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.
 * If you do not agree to be bound by the applicable license terms,
 * then you may not retain, install, activate or otherwise use the software.
 */

#ifndef NXP_TYPES_H_
#define NXP_TYPES_H_

#include "NxpPlatforms.h"

#define BIT(x)          (1UL << x)

#ifdef __cplusplus
extern "C" {
#endif

/** @enum conversa_signal_types
 *   Conversa Signal types enum used in Conversa.
 *   these types can be used in NxpConversa_Plugin_GetSignalID to retrieve Conversa internal signal
 */
typedef enum
{
	NXP_FLOAT = 0,								///< size: float
	NXP_FLOATX2 = 1,							///< size: float[2]
	NXP_DB = 2,									///< size: float
	NXP_COMPLEX = 3,							///< size: float[2]
	NXP_DOUBLE = 4,								///< size: double
	NXP_INT32 = 5,								///< size: int32_t
	NXP_SIG_TYPE_MAXENUM = 2147483647
}conversa_signal_types;


/** @enum
 *   Bit position of Conversa internal output enum used in Conversa.
 *   These enum can be used in nxp_conversa_plugin_config_t::internal_output_selector to construct output_selector
 *
 */
enum
{
    NXP_AEC_OUT_BITPOS           = 0,                            ///< Bit position for Accoustic Echo Canceller linear output
    NXP_BF_OUT_BITPOS            = 1,                            ///< Bit position for BeamFormer output
    NXP_NLP_OUT_BITPOS           = 2,                            ///< Bit position for Non-Linear Processing output
    NXP_AEC_FREQ_OUT_BITPOS      = 3,                            ///< Bit position for Accoustic Echo Canceller linear output in spectrum domain
    NXP_BF_FREQ_OUT_BITPOS       = 4,                            ///< Bit position for BeamFormer output in spectrum domain
    NXP_NLP_FREQ_OUT_BITPOS      = 5,                            ///< Bit position for Non-Linear Processing output in spectrum domain
    NXP_AEC_REF_FREQ_OUT_BITPOS  = 6,                            ///< Bit position for Accoustic Echo Canceller linear Refernce in spectrum domain
};

/** @enum conversa_internal_output
 *   Conversa internal output enum used in Conversa.
 *   These fields can be used in nxp_conversa_plugin_config_t::internal_output_selector to select which intermediate internal buffer to extract.
 *
 */
typedef enum
{
    NXP_AEC_OUT          = BIT(NXP_AEC_OUT_BITPOS),          ///< extract Accoustic Echo Canceller linear output
    NXP_BF_OUT           = BIT(NXP_BF_OUT_BITPOS),           ///< extract BeamFormer output
    NXP_NLP_OUT          = BIT(NXP_NLP_OUT_BITPOS),          ///< extract Non-Linear Processing output
    NXP_AEC_FREQ_OUT     = BIT(NXP_AEC_FREQ_OUT_BITPOS),     ///< extract Accoustic Echo Canceller linear output in spectrum domain
    NXP_BF_FREQ_OUT      = BIT(NXP_BF_FREQ_OUT_BITPOS),      ///< extract BeamFormer output in spectrum domain
    NXP_NLP_FREQ_OUT     = BIT(NXP_NLP_FREQ_OUT_BITPOS),     ///< extract Non-Linear Processing output in spectrum domain
    NXP_AEC_REF_FREQ_OUT = BIT(NXP_AEC_REF_FREQ_OUT_BITPOS), ///< extract Accoustic Echo Canceller Reference output in spectrum domain
    NXP_INTERNAL_OUT_MAXENUM = 2147483647
} conversa_internal_output;

/*
 * NXP types				IEEE754			HIFI4 / FUSIONF1
 *----------------------------------------------------------
 * nxp_float				float			xtfloat
 * nxp_dB:					float			xtfloat
 * nxp_floatx2:				float[2]		xtfloatx2
 * nxp_complex:				float[2]		xtfloatx2
 * nxp_double:				double			xtfloatx2
 */
#if defined(HIFI4) || defined(FUSIONDSP) || defined(HIFI1)
typedef xtfloat nxp_float;
typedef xtfloat nxp_dB;
typedef xtfloatx2 nxp_floatx2;
typedef xtfloatx2 nxp_complex;
typedef xtfloatx2 nxp_double;
#else
typedef float nxp_float;
typedef float nxp_dB;
typedef float nxp_floatx2[2];
typedef float nxp_complex[2];
typedef double nxp_double;

#endif

typedef nxp_float nxp_xyz_t[3];

#ifdef __cplusplus
}
#endif

#endif // NXP_TYPES_H_
