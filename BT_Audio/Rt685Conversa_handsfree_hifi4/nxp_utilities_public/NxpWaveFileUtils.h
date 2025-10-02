/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef NXP_WAVFILE_UTILS_H
#define NXP_WAVFILE_UTILS_H

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

// WAVE PCM soundfile format, see e.g. http://soundfile.sapp.org/doc/WaveFormat/ and http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_EXTENSIBLE 65534
#define EXTENSIBLE_FORMAT_ADDITION_SIZE (2 + 2 + 4 + 16 + 4 + 4 + 4)    // Additional data stored in WAVE_FORMAT_EXTENSIBLE compared to WAVE_FORMAT_PCM
typedef struct chunk_header_s {
	char chunk_id[4];
	uint32_t chunk_size;
} chunk_header_t;

//typedef struct riff_chunk_s {
//	chunk_header_t riff_header;
//	char riff_type[4];
//} riff_chunk_t;

typedef struct wave_fmt_s {
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
} wave_fmt_t;

typedef struct wav_header_s {
	chunk_header_t riff_header;
	char riff_type[4];
	chunk_header_t fmt_header;
	wave_fmt_t fmt;
	char subchunk2_id[4];
	int32_t subchunk2_size; // subchunk2_size denotes the number of samples
} wav_header_t;

typedef struct nxp_wav_file_s {
#if USE_FATFS
	NXP_FILE fid_struct;
#endif
	NXP_FILE* fid;
	wav_header_t wav_header;
	size_t num_read;
} nxp_wav_file_t;

NXP_STATIC void read_bytes(NXP_FILE* fid, void* buf, uint32_t bufsize);

NXP_STATIC void skip_bytes(NXP_FILE* fid, uint32_t bufsize);

NXP_STATIC void read_chunk_header(NXP_FILE* fid, chunk_header_t* hdr);

NXP_STATIC void update_chunk_size(int32_t Anum_channels, int32_t Anum_samples, nxp_wav_file_t* Awav_file);

NXP_STATIC int32_t nxp_wav_read_open(nxp_wav_file_t *Awavfile, const char *Afilename);

NXP_STATIC int32_t nxp_wav_write_open(nxp_wav_file_t *Awavfile, const char* Afilename, int32_t Asample_rate, int32_t Anum_channels, int32_t Abit_depth);

NXP_STATIC int32_t nxp_wav_read(int32_t** Abuffer, int32_t Anum_samples, nxp_wav_file_t* Awav_file);

NXP_STATIC int32_t nxp_wav_write_int16(int16_t** Abuffer, int32_t num_channels, int32_t Anum_samples, nxp_wav_file_t* Awav_file);

NXP_STATIC int32_t nxp_wav_write_int32(int32_t** Abuffer, int32_t num_channels, int32_t Anum_samples, nxp_wav_file_t* Awav_file);

NXP_STATIC int32_t nxp_wav_write_float(nxp_float** Abuffer, int32_t Anum_channels, int32_t Anum_samples, nxp_wav_file_t* Awav_file);

NXP_STATIC void nxp_wav_close(nxp_wav_file_t* Awav_file);

#ifdef __cplusplus
}
#endif

#endif /* NXP_WAVFILE_UTILS_H */
