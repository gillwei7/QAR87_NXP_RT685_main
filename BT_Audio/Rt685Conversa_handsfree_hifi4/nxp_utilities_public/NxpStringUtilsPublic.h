/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NXP_STRING_UTILS_PUBLIC_
#define NXP_STRING_UTILS_PUBLIC_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#if SDK_DEBUGCONSOLE
// Use FSL driver for MCUXpresso builds
extern int DbgConsole_Printf(const char *formatString, ...);
#define nxp_printf(...) DbgConsole_Printf(__VA_ARGS__)
#else
// Use stdio
#define nxp_printf(...) printf(__VA_ARGS__)
#endif

#ifdef __cplusplus
}
#endif

#endif /* NXP_STRING_UTILS_PUBLIC_ */


