/*
 * Copyright 2025 NXP
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP
 * and may only be used strictly in accordance with the applicable license terms.
 * By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.
 * If you do not agree to be bound by the applicable license terms,
 * then you may not retain, install, activate or otherwise use the software.
 */

#ifndef NXP_FREQBINFILE_UTILS_H
#define NXP_FREQBINFILE_UTILS_H

#include "NxpTypes.h"
#include "NxpFileUtilsPublic.h"

#include <stdio.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NXP_STATIC
#define NXP_STATIC
#endif


typedef struct fbin_fmt_s {
	uint16_t num_channels;
	uint16_t num_bands;
	uint32_t sample_rate;
} fbin_fmt_t;

typedef struct fbin_header_s {
	char chunk_id[4];
	fbin_fmt_t fmt;
} fbin_header_t;

typedef struct nxp_fbin_file_s {
#if USE_FATFS
	NXP_FILE fid_struct;
#endif
	NXP_FILE* fid;
	fbin_header_t fbin_header;
} nxp_fbin_file_t;

NXP_STATIC int32_t nxp_fbin_write_open(nxp_fbin_file_t *Afbin_file, const char* Afilename, int32_t Asample_rate, int32_t Anum_channels, int32_t Anum_bands);

NXP_STATIC int32_t nxp_fbin_write_complex(nxp_complex** Abuffer, int32_t Anum_channels, int32_t Anum_samples, nxp_fbin_file_t* Afbin_file);

NXP_STATIC void nxp_fbin_close(nxp_fbin_file_t* Afbin_file);

#ifdef __cplusplus
}
#endif

#endif /* NXP_FREQBINFILE_UTILS_H */
