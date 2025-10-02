/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef NXP_PLATFORMS_H_
#define NXP_PLATFORMS_H_

#include <stdint.h>
#include <stddef.h>

/*
 * Platform specific includes
 */

#ifdef __ARM_NEON
#include "arm_neon.h"
#elif NXP_POWERQUAD
#include "fsl_powerquad.h"
#elif defined(FUSIONDSP)
#include <xtensa/tie/xt_fusion.h>
#elif defined(HIFI1)
#include <xtensa/tie/xt_hifi3.h>
#include <xtensa/tie/xt_hifi2.h>
#include <xtensa/tie/xt_FP.h>
#elif defined(HIFI3)
#include <xtensa/tie/xt_hifi3.h>
#elif defined(HIFI4)
#include <xtensa/tie/xt_hifi4.h>
#endif

#if defined(HIFI3) || defined(HIFI1) || defined(HIFI4) || defined(FUSIONDSP)
#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"
#endif

#ifdef _WIN32
#define NXP_PRAGMA_USED 
#else
#define NXP_PRAGMA_USED __attribute__((used))
#endif

#ifdef __ARM_NEON
#define NXP_PACKED __attribute__((__packed__))
#else
#define NXP_PACKED
#endif

#ifdef LITTLE_KERNEL
#define __ALWAYS_INLINE __attribute__((always_inline))
#endif

/**
 * Memory sections
 */

#if ((defined(NXP_RT600_HIFI4)) && (NXP_SIM==0))|| defined(NXP_RT700_HIFI1)  || ((defined(NXP_RT700_HIFI4)) && (NXP_SIM==0))
#define NXP_CONVERSA_NON_CACHED_ATTR __attribute__ ((section("NonCacheable")))
#else
#define NXP_CONVERSA_NON_CACHED_ATTR
#endif

#if ( MEMORY_MAPPING_CUSTOM == 1 )
#define NXP_PUT_FUNCTION_IN_RODATA_FLASH	__attribute__((__section__(".CreateCodeInRODataFlash")))	// will be placed in RO Data Flash
#define NXP_PUT_FUNCTION_IN_DATA_FLASH		__attribute__((__section__(".CreateCodeInDataFlash")))		// will be placed in Data Flash
#define NXP_PUT_FUNCTION_IN_TEXT_FLASH		__attribute__((__section__(".CreateCodeInTextFlash")))		// will be placed in Text Flash
#define NXP_PUT_FUNCTION_IN_DATA_RAM		__attribute__((__section__(".CodeQuickAccessInDataRam")))	// will be placed in Data RAM
#define NXP_PUT_FUNCTION_IN_CODE_RAM		__attribute__((__section__(".CodeQuickAccessInCodeRam")))	// will be placed in Code RAM
#else
#define NXP_PUT_FUNCTION_IN_RODATA_FLASH
#define NXP_PUT_FUNCTION_IN_DATA_FLASH
#define NXP_PUT_FUNCTION_IN_TEXT_FLASH
#define NXP_PUT_FUNCTION_IN_DATA_RAM
#define NXP_PUT_FUNCTION_IN_CODE_RAM
#endif

#endif // NXP_PLATFORMS_H_
