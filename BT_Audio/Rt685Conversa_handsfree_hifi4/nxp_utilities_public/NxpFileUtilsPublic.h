/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NXP_FILE_UTILS_PUBLIC_
#define NXP_FILE_UTILS_PUBLIC_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if USE_FATFS
#include "ff.h"
typedef FIL NXP_FILE;
#else
typedef FILE NXP_FILE;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	FILE_OK = 0,
    FILE_GENERAL_ERROR = 1,
} NxpFileStatus;

extern NxpFileStatus nxp_fopen(NXP_FILE** Afile, char* Afilename, const char* Arw);
extern int32_t nxp_fread(void* Adata, int32_t Asize, int32_t Anum, NXP_FILE* Afile);
extern int32_t nxp_fwrite(void* Adata, int32_t Asize, int32_t Anum, NXP_FILE* Afile);
extern int32_t nxp_feof(NXP_FILE* Afile);
extern int32_t nxp_fputs(const char * s, NXP_FILE * stream);
extern int32_t nxp_fseek(NXP_FILE *stream, long int offset, int whence);
extern int32_t nxp_fflush(NXP_FILE *stream);
extern NxpFileStatus nxp_fclose(NXP_FILE* Afile);

#ifdef __cplusplus
}
#endif

#endif /* NXP_MEMORY_UTILS_PUBLIC_ */


