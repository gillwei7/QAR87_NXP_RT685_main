/* Copyright 2021 Retune DSP 
 * Copyright 2022,2025 NXP
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP
 * and may only be used strictly in accordance with the applicable license terms.
 * By expressly accepting such terms or by downloading, installing,
 * activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms.
 * If you do not agree to be bound by the applicable license terms,
 * then you may not retain, install, activate or otherwise use the software.
 */

#include "NxpWaveFileUtils.h"
#include "NxpStringUtilsPublic.h"

#include <string.h>

NXP_STATIC void read_bytes(NXP_FILE *fid, void *buf, uint32_t bufsize) {
	if (!nxp_fread(buf, 1, bufsize, fid)) {
#if _WIN32
		fputs("Not enough bytes in file", stderr);
#endif
	}
}

NXP_STATIC void skip_bytes(NXP_FILE *fid, uint32_t bufsize) {
	if (nxp_fseek(fid, bufsize, SEEK_CUR != 0)) {
#if _WIN32
		fputs("Not enough bytes in file", stderr);
#endif
	}
}

NXP_STATIC void read_chunk_header(NXP_FILE *fid, chunk_header_t *hdr) {
	read_bytes(fid, hdr, sizeof(chunk_header_t));
	// if you are on a big-endian system, you need to swap the bytes of hdr.dataSize here...
}

NXP_STATIC int32_t nxp_wav_read_open(nxp_wav_file_t *Awavfile, const char *Afilename) {

#if USE_FATFS
	NXP_FILE *fid = &Awavfile->fid_struct;
	Awavfile->fid = fid;
#else
	NXP_FILE* fid = Awavfile->fid;
#endif

	NxpFileStatus fstat = nxp_fopen(&fid, (char*)Afilename, "rb");
#if !USE_FATFS
	Awavfile->fid = fid;	// Update fid in struct
#endif
	if (fid == NULL) {
		nxp_printf("Failed to open %s, errno = %d\n", Afilename, errno);
		return 1;
	}

	chunk_header_t riff_header;
	read_chunk_header(fid, &riff_header); // should be RIFF
	if ((riff_header.chunk_id[0] != 'R') || (riff_header.chunk_id[1] != 'I') || (riff_header.chunk_id[2] != 'F') || (riff_header.chunk_id[3] != 'F')) {
#if _WIN32
		fputs("Expected chunk 'RIFF' not detected", stderr);
#endif
		return 1;
	}

	char riff_type[4];
	read_bytes(fid, riff_type, 4); // should be WAVE
	if ((riff_type[0] != 'W') || (riff_type[1] != 'A') || (riff_type[2] != 'V') || (riff_type[3] != 'E')) {
#if _WIN32
		fputs("Expected type 'WAVE' not detected", stderr);
#endif
		return 1;
	}

	wav_header_t wav_header;
	chunk_header_t hdr;
	chunk_header_t fmt_header;
	wave_fmt_t fmt_chunk;
	while (!nxp_feof(fid)) {
		read_chunk_header(fid, &hdr);

		// Check for "fmt" chunk
		if ((hdr.chunk_id[0] == 'f') && (hdr.chunk_id[1] == 'm') && (hdr.chunk_id[2] == 't') && (hdr.chunk_id[3] == ' ')) {

			if (hdr.chunk_size == 0) {
#if _WIN32
				fputs("Invalid 'fmt' data size detected", stderr);
#endif
				return 1;
			}

			fmt_header = hdr;

			read_bytes(fid, &fmt_chunk, 16);

			if (fmt_header.chunk_size != 16)
				skip_bytes(fid, fmt_header.chunk_size - 16);

			wav_header.fmt_header = fmt_header;
			wav_header.fmt = fmt_chunk;

		}
		// Check for "data" chunk
		else if ((hdr.chunk_id[0] == 'd') && (hdr.chunk_id[1] == 'a') && (hdr.chunk_id[2] == 't') && (hdr.chunk_id[3] == 'a')) {
			// "data" chunk starts now so break
			wav_header.subchunk2_size = hdr.chunk_size;
			break;
		} else {
			// Skip all other chunks
			skip_bytes(fid, hdr.chunk_size);
		}

		// Skip trailing pad byte if the data size is not even
		if ((hdr.chunk_size % 2) != 0)
			skip_bytes(fid, 1);
	}

	wav_header.riff_header = riff_header;
	wav_header.riff_type[0] = riff_type[0]; // [0-3] = WAVE
	wav_header.riff_type[1] = riff_type[1];
	wav_header.riff_type[2] = riff_type[2];
	wav_header.riff_type[3] = riff_type[3];

	if ((sizeof(wav_header) != sizeof(wav_header_t)) || ((wav_header.fmt.sample_rate != 16000) && (wav_header.fmt.sample_rate != 32000) && (wav_header.fmt.sample_rate != 48000))) {
#if _WIN32
		fputs("Only input files with 16, 32 or 48 kHz sample rates are supported.", stderr);
#endif
		return 1;
	}

	memcpy(&Awavfile->wav_header, &wav_header, sizeof(wav_header_t));
	Awavfile->num_read = 0;

	return 0;
}

NXP_STATIC int32_t nxp_wav_write_open(nxp_wav_file_t *Awavfile, const char *Afilename, int32_t Asample_rate, int32_t Anum_channels, int32_t Abit_depth) {

#if USE_FATFS
	NXP_FILE* fid = &Awavfile->fid_struct;
	Awavfile->fid = fid;
#else
	NXP_FILE* fid = Awavfile->fid;
#endif

	NxpFileStatus e = nxp_fopen(&fid, (char*) Afilename, "wb+");
#if !USE_FATFS
	Awavfile->fid = fid;	// Update fid in struct
#endif
	if (fid == NULL) {
		nxp_printf("Failed to open %s, errno = %d\n", Afilename, errno);
		return 1;
	}

	wav_header_t wav_header;

	int32_t subchunk1_size = 16; // PCM
	int32_t bits_per_sample = Abit_depth;
	int32_t bytes_per_sample = bits_per_sample / 8;
	int32_t subchunk2_size = 0; // Update when writing file. Equal to num_samples * num_channels * bits_per_sample / 8

	chunk_header_t riff_header;
	riff_header.chunk_id[0] = 'R';
	riff_header.chunk_id[1] = 'I';
	riff_header.chunk_id[2] = 'F';
	riff_header.chunk_id[3] = 'F';

	riff_header.chunk_size = 4 + (8 + subchunk1_size) + (8 + subchunk2_size); // Update when writing file. 4 + (8 + subchunk1_size) + (8 + subchunk2_size)

	chunk_header_t fmt_header;
	fmt_header.chunk_id[0] = 'f';
	fmt_header.chunk_id[1] = 'm';
	fmt_header.chunk_id[2] = 't';
	fmt_header.chunk_id[3] = ' ';
	fmt_header.chunk_size = subchunk1_size;

	wave_fmt_t wave_fmt;
	wave_fmt.audio_format = 1; // PCM
	wave_fmt.num_channels = (int16_t) Anum_channels;
	wave_fmt.sample_rate = Asample_rate;
	wave_fmt.byte_rate = Asample_rate * Anum_channels * bits_per_sample / 8;
	wave_fmt.block_align = (int16_t) (Anum_channels * bits_per_sample / 8);
	wave_fmt.bits_per_sample = (int16_t) bits_per_sample;

	wav_header.riff_header = riff_header;
	wav_header.riff_type[0] = 'W';
	wav_header.riff_type[1] = 'A';
	wav_header.riff_type[2] = 'V';
	wav_header.riff_type[3] = 'E';
	wav_header.fmt_header = fmt_header;
	wav_header.fmt = wave_fmt;
	wav_header.subchunk2_id[0] = 'd';
	wav_header.subchunk2_id[1] = 'a';
	wav_header.subchunk2_id[2] = 't';
	wav_header.subchunk2_id[3] = 'a';
	wav_header.subchunk2_size = subchunk2_size;

	nxp_fwrite(&wav_header, sizeof(wav_header_t), 1, fid);

	memcpy(&Awavfile->wav_header, &wav_header, sizeof(wav_header_t));
	Awavfile->num_read = 0;

	return 0;
}

NXP_STATIC int32_t nxp_wav_read(int32_t **Abuffer, int32_t Anum_samples, nxp_wav_file_t *Awav_file) {
	wav_header_t header = Awav_file->wav_header;
	size_t num_read = 0;
	int32_t bytes_per_sample = header.fmt.bits_per_sample / 8;

	if (((Awav_file->num_read + Anum_samples) * header.fmt.num_channels * bytes_per_sample) > header.subchunk2_size)
		return 0;

	if ((header.fmt.audio_format == WAVE_FORMAT_PCM || header.fmt.audio_format == WAVE_FORMAT_EXTENSIBLE)) {

		// 16 bit integer wav file
		if (header.fmt.bits_per_sample == 16) {
			int16_t *tmp_buf_int = (int16_t*) malloc(Anum_samples * header.fmt.num_channels * sizeof(int16_t));
			num_read = nxp_fread(tmp_buf_int, header.fmt.num_channels * bytes_per_sample, Anum_samples, Awav_file->fid);

			for (uint16_t ich = 0; ich < header.fmt.num_channels; ich++) {
				int16_t *ptr = tmp_buf_int + ich;
				for (size_t isample = 0; isample < num_read; isample++, ptr += header.fmt.num_channels) {
					Abuffer[ich][isample] = *ptr << 16; // Conversa input is 32 bit integer
				}
			}
			free(tmp_buf_int);
		}

		// 24 bit integer wav file
		if (header.fmt.bits_per_sample == 24) {
			char *tmp_buf_char = (char*) malloc(bytes_per_sample * Anum_samples * header.fmt.num_channels);
			num_read = nxp_fread(tmp_buf_char, header.fmt.num_channels * bytes_per_sample, Anum_samples, Awav_file->fid);
			for (uint16_t ich = 0; ich < header.fmt.num_channels; ich++) {
				char *ptr = tmp_buf_char + (bytes_per_sample * ich);
				for (size_t isample = 0; isample < num_read; isample++, ptr += (bytes_per_sample * header.fmt.num_channels)) {
					int32_t tmp = (*(int32_t*) ptr) >> 8; // Discard lower 8 bits, as wav file is 24-bit
					Abuffer[ich][isample] = tmp << 8; // Conversa input is 32 bit integer
				}
			}
			free(tmp_buf_char);
		}

		// 32 bit integer wav file
		else if (header.fmt.bits_per_sample == 32) {
			int32_t *tmp_buf_int = (int32_t*) malloc(Anum_samples * header.fmt.num_channels * sizeof(int32_t));
			num_read = nxp_fread(tmp_buf_int, header.fmt.num_channels * bytes_per_sample, Anum_samples, Awav_file->fid);
			for (uint16_t ich = 0; ich < header.fmt.num_channels; ich++) {
				int32_t *ptr = tmp_buf_int + ich;
				for (size_t isample = 0; isample < num_read; isample++, ptr += header.fmt.num_channels) {
					Abuffer[ich][isample] = *ptr;
				}
			}
			free(tmp_buf_int);
		}
	}

	// 32 bit float wav file
	else if ((header.fmt.bits_per_sample == 32) && (header.fmt.audio_format == WAVE_FORMAT_IEEE_FLOAT)) {
		float *tmp_buf_float = (float*) malloc(Anum_samples * header.fmt.num_channels * sizeof(float));
		num_read = nxp_fread(tmp_buf_float, header.fmt.num_channels * bytes_per_sample, Anum_samples, Awav_file->fid);

		const float scale = 32768.0f * 65536.0f; // Conversa input is 32 bit integer
		for (uint16_t ich = 0; ich < header.fmt.num_channels; ich++) {
			float *ptr = tmp_buf_float + ich;
			for (size_t isample = 0; isample < num_read; isample++, ptr += header.fmt.num_channels) {
				Abuffer[ich][isample] = (int32_t) (*ptr * scale);
			}
		}
		free(tmp_buf_float);
	} else {
		nxp_printf("Only 16/32 bit integer wav files and 32 bit float wav files are supported.", stderr);
	}

	Awav_file->num_read += num_read;
	return (int32_t)num_read;
}

NXP_STATIC void update_chunk_size(int32_t Anum_channels, int32_t Anum_samples, nxp_wav_file_t *Awav_file) {

#if USE_FATFS
	FRESULT error;

	UINT c = 0;
	// Update chunk_size
	int32_t chunk_size;
	// Move to chunk_size location
	f_lseek(&Awav_file->fid_struct, 4L);
	//fflush(Awav_file->fid);
	// Read old chunk_size
	//int32_t ret = fread(&chunk_size, sizeof(int32_t), 1, Awav_file->fid);
	f_read(&Awav_file->fid_struct, &chunk_size, sizeof(int32_t), &c);

	// Update chunk_size
	chunk_size += Anum_samples * Anum_channels * Awav_file->wav_header.fmt.bits_per_sample/8;
	// Move back one to prepare for write
	f_lseek(&Awav_file->fid_struct, 4L);
	//fflush(&Awav_file->fid_struct);
	// Write chunk_size
	//ret = fwrite(&chunk_size, sizeof(int32_t), 1, &Awav_file->fid_struct);
	f_write(&Awav_file->fid_struct, &chunk_size, sizeof(int32_t), &c);

	// Update subchunk2_size
	int32_t subchunk2_size;
	// Move to subchunk2_size location
	f_lseek(&Awav_file->fid_struct, 40L);
	//fflush(&Awav_file->fid_struct);
	// Read old subchunk2_size
	//ret = fread(&subchunk2_size, sizeof(int32_t), 1, &Awav_file->fid_struct);
	f_read(&Awav_file->fid_struct, &subchunk2_size, sizeof(int32_t), &c);

	//fflush(&Awav_file->fid_struct);
	// Update subchunk2_size
	subchunk2_size += Anum_samples * Anum_channels * Awav_file->wav_header.fmt.bits_per_sample/8;
	// Move back one to prepare for write
	f_lseek(&Awav_file->fid_struct, 40L);
	//fflush(&Awav_file->fid_struct);
	// Write subchunk2_size
	//ret = fwrite(&subchunk2_size, sizeof(int32_t), 1, &Awav_file->fid_struct);
	f_write(&Awav_file->fid_struct, &subchunk2_size, sizeof(int32_t), &c);

	// Move to end of file
	f_lseek(&Awav_file->fid_struct, f_size(&Awav_file->fid_struct));
	//fflush(&Awav_file->fid_struct);
#else
	int32_t bytes_per_sample = Awav_file->wav_header.fmt.bits_per_sample / 8;

	// Update chunk_size
	int32_t chunk_size;
	// Move to chunk_size location
	nxp_fseek(Awav_file->fid, 4L, SEEK_SET);
	fflush(Awav_file->fid);
	// Read old chunk_size
	size_t ret = nxp_fread(&chunk_size, sizeof(int32_t), 1, Awav_file->fid);
	// Update chunk_size
	chunk_size += Anum_samples * Anum_channels * bytes_per_sample;
	// Move back one to prepare for write
	nxp_fseek(Awav_file->fid, 4L, SEEK_SET);
	nxp_fflush(Awav_file->fid);
	// Write chunk_size
	ret = nxp_fwrite(&chunk_size, sizeof(int32_t), 1, Awav_file->fid);

	// Update subchunk2_size
	int32_t subchunk2_size;
	// Move to subchunk2_size location
	nxp_fseek(Awav_file->fid, 40L, SEEK_SET);
	nxp_fflush(Awav_file->fid);
	// Read old subchunk2_size
	ret = nxp_fread(&subchunk2_size, sizeof(int32_t), 1, Awav_file->fid);
	nxp_fflush(Awav_file->fid);
	// Update subchunk2_size
	subchunk2_size += Anum_samples * Anum_channels * bytes_per_sample;
	// Move back one to prepare for write
	fseek(Awav_file->fid, 40L, SEEK_SET);
	nxp_fflush(Awav_file->fid);
	// Write subchunk2_size
	ret = nxp_fwrite(&subchunk2_size, sizeof(int32_t), 1, Awav_file->fid);

	// Move to end of file
	nxp_fseek(Awav_file->fid, 0L, SEEK_END);
	nxp_fflush(Awav_file->fid);
#endif
}

NXP_STATIC int32_t nxp_wav_write_int16(int16_t **Abuffer, int32_t Anum_channels, int32_t Anum_samples, nxp_wav_file_t *Awav_file) {
	if (Anum_samples <= 0)
		return 0;

	int16_t *tmp_buf_int = (int16_t*) malloc(Anum_samples * Anum_channels * sizeof(int16_t));

	// Interleave samples
	int16_t *p = tmp_buf_int;
	for (int i = 0; i < Anum_samples; i++) {
		for (int j = 0; j < Anum_channels; j++)
			*p++ = Abuffer[j][i];
	}

	int32_t write_count = 0;
	int32_t bits_per_sample = Awav_file->wav_header.fmt.bits_per_sample;
	if (bits_per_sample == 16) {
		write_count = nxp_fwrite(tmp_buf_int, sizeof(int16_t), Anum_channels * Anum_samples, Awav_file->fid);
	} else {
		nxp_printf("Cannot write 16 bit buffer to %d bit wav_file\n", bits_per_sample);
	}

	free(tmp_buf_int);

	update_chunk_size(Anum_channels, Anum_samples, Awav_file);

	return write_count;
}

NXP_STATIC int32_t nxp_wav_write_int32(int32_t **Abuffer, int32_t Anum_channels, int32_t Anum_samples, nxp_wav_file_t *Awav_file) {

	if (Anum_samples <= 0)
		return 0;

	int32_t *tmp_buf_int = (int32_t*) malloc(Anum_samples * Anum_channels * sizeof(int32_t));

	// Interleave samples
	int32_t *p = tmp_buf_int;
	for (int i = 0; i < Anum_samples; i++) {
		for (int j = 0; j < Anum_channels; j++)
			*p++ = Abuffer[j][i];
	}

	int32_t write_count = 0;
	int32_t bits_per_sample = Awav_file->wav_header.fmt.bits_per_sample;
	if (bits_per_sample == 32) {
		write_count = nxp_fwrite(tmp_buf_int, sizeof(int32_t), Anum_channels * Anum_samples, Awav_file->fid);
	} else {
		nxp_printf("Cannot write 32 bit buffer to %d bit wav_file\n", bits_per_sample);
	}

	free(tmp_buf_int);

	update_chunk_size(Anum_channels, Anum_samples, Awav_file);

	return write_count;
}

NXP_STATIC int32_t nxp_wav_write_float(nxp_float **Abuffer, int32_t Anum_channels, int32_t Anum_samples, nxp_wav_file_t *Awav_file) {
	if (Anum_samples <= 0)
		return 0;

	int32_t bits_per_sample = Awav_file->wav_header.fmt.bits_per_sample;

	size_t write_count = 0;
	if (bits_per_sample == 32) {
		const float scale = 2147483648.0f; // buffer is 32 bit floating-point
		int32_t *tmp_buf_int = (int32_t*) malloc(Anum_samples * Anum_channels * sizeof(int32_t));
		int32_t *ptr = tmp_buf_int;
		for (int isample = 0; isample < Anum_samples; isample++) {
			for (int ich = 0; ich < Anum_channels; ich++, ptr++) {
				*ptr = (int32_t) (Abuffer[ich][isample] * scale);
			}
		}
		write_count = nxp_fwrite(tmp_buf_int, sizeof(int32_t), Anum_channels * Anum_samples, Awav_file->fid);

		free(tmp_buf_int);

	} else if (bits_per_sample == 16) {
		const float scale = 32768.0f; // buffer is 32 bit floating-point
		int16_t *tmp_buf_int = (int16_t*) malloc(Anum_samples * Anum_channels * sizeof(int16_t));
		int16_t *ptr = tmp_buf_int;
		for (int isample = 0; isample < Anum_samples; isample++) {
			for (int ich = 0; ich < Anum_channels; ich++, ptr++) {
				*ptr = (int16_t) (Abuffer[ich][isample] * scale);
			}
		}

		write_count = nxp_fwrite(tmp_buf_int, Anum_samples, sizeof(tmp_buf_int[0]), Awav_file->fid);

		free(tmp_buf_int);

	} else {
		nxp_printf("%d bit wav_file not supported\n", bits_per_sample);
	}

	update_chunk_size(Anum_channels, Anum_samples, Awav_file);

	return (int32_t)write_count;
}

NXP_STATIC void nxp_wav_close(nxp_wav_file_t *Awav_file) {
	if (Awav_file->fid)
		nxp_fclose(Awav_file->fid);
}


