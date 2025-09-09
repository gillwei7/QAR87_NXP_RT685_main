/*Copyright 2021 Retune DSP 
* Copyright 2022-2023 NXP 
*
* NXP Confidential. This software is owned or controlled by NXP 
* and may only be used strictly in accordance with the applicable license terms.  
* By expressly accepting such terms or by downloading, installing, 
* activating and/or otherwise using the software, you are agreeing that you have read, 
* and that you agree to comply with and are bound by, such license terms.  
* If you do not agree to be bound by the applicable license terms, 
* then you may not retain, install, activate or otherwise use the software.
*
*/
#include "NxpFileUtilsPublic.h"
#include "NxpStringUtilsPublic.h"


NxpFileStatus nxp_fopen(NXP_FILE** Afile, char* Afilename, const char* Arw) {
#ifdef _WIN32
	errno_t err = fopen_s(Afile, Afilename, Arw);
	if (err != 0) {
		printf("Error opening %s\n", Afilename);
		return FILE_GENERAL_ERROR;
	}
#elif USE_FATFS
	FRESULT res = FR_OK;
	if (strcmp(Arw, "wb") == 0 || strcmp(Arw, "wb+") == 0) {
	    res = f_open(*Afile, Afilename, (FA_WRITE | FA_READ | FA_CREATE_ALWAYS));
	} else if (strcmp(Arw, "rb") == 0) {
	    res = f_open(*Afile, Afilename, FA_READ);
	} else {
		return FILE_GENERAL_ERROR;
	}
	if (res != FR_OK) {
		nxp_printf("Error opening %s\r\n", Afilename);
		nxp_fclose(*Afile);
		return FILE_GENERAL_ERROR;
	}
#else
	FILE** file = (FILE**) Afile;
	*file = fopen(Afilename, Arw);
	if (file == NULL) {
	    nxp_printf("Error opening %s\r\n", Afilename);
        return FILE_GENERAL_ERROR;
	}
#endif

    return FILE_OK;
}

int32_t nxp_fread(void* Adata, int32_t Asize, int32_t Anum, NXP_FILE* Afile) {
#if USE_FATFS
    int32_t num_bytes = Asize * Anum;
    uint32_t num_read;

    FRESULT e = f_read(Afile, Adata, num_bytes, &num_read);
    if (num_read != num_bytes)
            return FILE_GENERAL_ERROR;

//    // Sync file to clear buffers in case we can't close
//    FRESULT res = f_sync(Afile);
//    if (res != FR_OK)
//        return FILE_GENERAL_ERROR;

#else
    if (fread(Adata, Asize, Anum, Afile) != Anum)
        return FILE_GENERAL_ERROR;
#endif
    return Anum;
}

int32_t nxp_fwrite(void* Adata, int32_t Asize, int32_t Anum, NXP_FILE* Afile) {
#if USE_FATFS
    int32_t num_bytes = Asize * Anum;
    FIL* file = (FIL*) Afile;
    uint32_t num_written;

    FRESULT res = f_write(file, Adata, num_bytes, &num_written);

    if (res != FR_OK)
        return FILE_GENERAL_ERROR;

//    // Sync file to clear buffers in case we can't close
//    if (f_sync(file) != FR_OK)
//        return FILE_GENERAL_ERROR;

#else
#if 0
	FILE** file = (FILE**)Afile;
    if (fwrite(Adata, Asize, Anum, *file) != Anum)
        return FILE_GENERAL_ERROR;
#else
    if (fwrite(Adata, Asize, Anum, Afile) != Anum)
        return FILE_GENERAL_ERROR;
#endif
#endif
    return Anum;
}

extern int32_t nxp_feof(NXP_FILE* Afile) {
#if USE_FATFS
	return f_eof(Afile);
#else
	return feof(Afile);
#endif
}

extern int32_t nxp_fputs(const char * s, NXP_FILE * stream) {
#if USE_FATFS
	return f_puts(s, stream);
#else
	return fputs(s, stream);
#endif
}

extern int32_t nxp_fseek(NXP_FILE *stream, long int offset, int whence){
#if USE_FATFS
	return f_lseek (stream, offset);
#else
	return fseek(stream, offset, whence);
#endif
}

extern int32_t nxp_fflush(NXP_FILE *stream) {
#if USE_FATFS
	return f_sync(stream);
#else
	return fflush(stream);
#endif
}

extern NxpFileStatus nxp_fclose(NXP_FILE* Afile) {
#if USE_FATFS
	int fr = f_close(Afile);
	if(fr == 0)
		return FILE_OK;
	else
		return FILE_GENERAL_ERROR;
#else
	return fclose(Afile);
#endif
}

