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

#include "NxpFreqBinFileUtils.h"
#include "NxpStringUtilsPublic.h"

#include <string.h>

/**
 * Open file and write header to it.
 *
 *  @param [in] Afbin_file Pointer to file handle
 *  @param [in] Afilename Pointer to string containing name of the file
 *  @param [in] Asample_rate Sample rate
 *  @param [in] Anum_channels Number of channels
 *  @param [in] Anum_bands Number of bands (correspond to number of complex data that will be written later)
 *  @return 0 if success, 1 if issue during file creation
 */
NXP_STATIC int32_t nxp_fbin_write_open(nxp_fbin_file_t * Afbin_file, const char *Afilename, int32_t Asample_rate, int32_t Anum_channels, int32_t Anum_bands) {

#if USE_FATFS
	NXP_FILE* fid = &Afbin_file->fid_struct;
	Afbin_file->fid = fid;
#else
	NXP_FILE* fid = Afbin_file->fid;
#endif

	NxpFileStatus e = nxp_fopen(&fid, (char*) Afilename, "wb+");
#if !USE_FATFS
	Afbin_file->fid = fid;	// Update fid in struct
#endif
	if (fid == NULL || e != FILE_OK) {
		nxp_printf("Failed to open %s, errno = %d\n", Afilename, errno);
		return 1;
	}

	fbin_header_t fbin_header;
	fbin_header.chunk_id[0] = 'F';
	fbin_header.chunk_id[1] = 'B';
	fbin_header.chunk_id[2] = 'I';
	fbin_header.chunk_id[3] = 'N';

	fbin_fmt_t fbin_fmt;
	fbin_fmt.num_channels = (int16_t) Anum_channels;
	fbin_fmt.num_bands = (int16_t) Anum_bands;
	fbin_fmt.sample_rate = Asample_rate;

	fbin_header.fmt = fbin_fmt;

	nxp_fwrite(&fbin_header, sizeof(fbin_header_t), 1, fid);

	memcpy(&Afbin_file->fbin_header, &fbin_header, sizeof(fbin_header_t));

	return 0;
}

/**
 * Write buffer (containing complex data) to file.
 *
 *  @param [in] Abuffer Pointer to the complex data (containing float real, float img ... data)
 *  @param [in] Anum_channels Number of channels
 *  @param [in] Anum_samples Number of samples (complex) to write
 *  @param [in] Afbin_file Pointer to file handle
 *  @return Number of samples (nxp_complex) written (shall be equals to Anum_channels * Anum_samples)
 */
NXP_STATIC int32_t nxp_fbin_write_complex(nxp_complex** Abuffer, int32_t Anum_channels, int32_t Anum_samples, nxp_fbin_file_t* Afbin_file) {
	if (Anum_samples <= 0)
		return 0;

#if defined(HIFI4) || defined(FUSIONDSP) ||defined(HIFI1)
	return nxp_fwrite(&Abuffer[0][0], sizeof(nxp_complex), Anum_samples * Anum_channels, Afbin_file->fid);
#else
	return nxp_fwrite(&Abuffer[0][0][0], sizeof(nxp_complex), Anum_samples * Anum_channels, Afbin_file->fid);
#endif
}

/**
 * Close file.
 *
 *  @param [in] Afbin_file Pointer to file handle
 *  @return void
 */
NXP_STATIC void nxp_fbin_close(nxp_fbin_file_t *Afbin_file) {
	if (Afbin_file->fid)
		nxp_fclose(Afbin_file->fid);
}
