/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef _PLATFORM_TYPE_H_
#define _PLATFORM_TYPE_H_

#ifdef CORE_MCU
	#include "PL_platformTypes_CortexM33.h"
#elif CORE_DSP
	#include "PL_platformTypes_Hifi4.h"
#else
	#error "ERROR: Core not supported"
#endif


// Operating mode
typedef enum
{
    AUDIO_OM_DISABLE,
	AUDIO_OM_ENABLE,
	AUDIO_OM_MAX = PL_MAXENUM,  // to force enum in 32bits
}AUDIO_operatingMode_en;


#endif /*_PLATFORM_TYPE_H_*/
