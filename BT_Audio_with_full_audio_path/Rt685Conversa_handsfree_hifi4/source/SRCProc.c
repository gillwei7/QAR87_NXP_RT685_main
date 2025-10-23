/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "xa_type_def.h"
#include "xa_error_standards.h"
#include "xa_error_handler.h"
#include "xa_apicmd_standards.h"
#include "xa_memory_standards.h"
#include "xa_src_pp_api.h"
#include "xa_src_pp_waveio.h"

#include "fsl_sema42.h"
#include "fsl_gpio.h"
#include "fsl_debug_console.h"

#include "GlobalDef.h"
#include "SRCProc.h"
#include "AudioProc_Conversa.h"


//#define POLYPHASE_CUBIC_INTERPOLATION
//#define ASRC_ENABLE
// #define OUTPUT_PING_PONG
// #define INPUT_PING_PONG
// #define SCRATCH_TRASH_TEST

#ifdef ASRC_ENABLE
//#define DISPLAY_MESSAGE_ASRC
#define ASRC_ENABLE_RUNTIME_DRIFT_TEST
#endif

#ifdef __XCC__
#include <xtensa/hal.h>
#endif

#if !defined(PROFILE) && __XCC__
#define PROFILE 1
#endif

#if !defined(SHOW_SIZES)
#define SHOW_SIZES 1
#endif /* SHOW_SIZES */

#if PROFILE
#include <sys/times.h>
#include <xtensa/sim.h>
struct tms start, stop;
double Peak = 0, Ave, Sum = 0;
int Peak_frame;
unsigned long long total_samples = 0;
#endif
#ifdef USE_SRCPLUS_LIBRARY 
#define PARAMFILE "paramfile_srcplus_pp.txt"
#elif USE_SRC384KHZ_TRIMMED_LIBRARY
#define PARAMFILE "paramfile_src384k_pp.txt"
#elif NUM_IOSAMPLES_INSYNC
#define PARAMFILE "paramfile_frio_hifi3.txt"
#else
#define PARAMFILE "paramfile_src_pp.txt"
#endif
#define XA_MAX_CMD_LINE_LENGTH 300
#define XA_MAX_ARGS 20

#define MAX_INPUT_CHUNK_LEN 512
#define MAX_CHANNELS 24

#define ALIGNMENT 8

#define XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED       0xffff8000
#define XA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED       0xffff8001

#define DISPLAY_MESSAGE

extern xa_error_info_struct xa_testbench_error_info_Asrc;
extern xa_error_info_struct xa_src_pp_error_info;

extern uint8_t domainId;

VOID xa_testbench_error_handler_init_Asrc(void);
VOID xa_src_pp_error_handler_init(void);

volatile float AsrcDriftingValueCurrent;
volatile float AsrcDriftingValueTarget;
volatile int AsrcFsInCurrent;
volatile int AsrcFsInTarget;


char *binName = NULL;
//WORD  g_w_malloc_count;
//pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];

WORD  g_w_malloc_count_Ref;
WORD  g_w_malloc_count_TxOut;
pVOID g_pv_arr_alloc_memory_Ref[MAX_MEM_ALLOCS];
pVOID g_pv_arr_alloc_memory_TxOut[MAX_MEM_ALLOCS];



/*  Pointers to the raw input/output buffers used by the file I/O in the main loop. */
pVOID ppin_buffer;
pVOID ppout_buffer;

/* API obj */
//xa_codec_handle_t xa_process_handle;
xa_codec_handle_t SRC_process_Ref_handle;
xa_codec_handle_t SRC_process_TxOut_handle;


xa_error_info_struct *p_proc_err_info;
/* The process API function */
xa_codec_func_t *p_xa_process_api;

/********************************************************************************
 * UserConfig defines a structure for command-line configuration parameters
 ********************************************************************************/
/*int user_filter[512];*/
typedef struct _user_config
{
    const char *in_file_name;
    const char *out_file_name;
    int         channels;
    int         fs_in;
    int         fs_out;
    int         input_chunk_size;
    int         bytes_per_sample;
#ifdef POLYPHASE_CUBIC_INTERPOLATION
    int         enable_cubic;
#endif /* #ifdef POLYPHASE_CUBIC_INTERPOLATION */
    int         reset;
#ifdef ASRC_ENABLE
    int         enable_asrc;
    long long   drift_asrc;
#endif /* #ifdef ASRC_ENABLE */
} user_config ;

WORD8 pb_input_file_path[XA_MAX_CMD_LINE_LENGTH] = "";
WORD8 pb_output_file_path[XA_MAX_CMD_LINE_LENGTH] = "";

/* User Defaults */
const user_config default_config =
{
#ifdef ASRC_ENABLE
#ifdef POLYPHASE_CUBIC_INTERPOLATION
    NULL, NULL, 2,  48000, 48000, MAX_INPUT_CHUNK_LEN, 3, 0, -1, 0, 0 
#else /* #ifdef POLYPHASE_CUBIC_INTERPOLATION */
    NULL, NULL, 2,  48000, 48000, MAX_INPUT_CHUNK_LEN, 3, -1, 0, 0 
#endif /* #ifdef POLYPHASE_CUBIC_INTERPOLATION*/
#else /* #ifdef ASRC_ENABLE */
#ifndef POLYPHASE_CUBIC_INTERPOLATION
    NULL, NULL, 2,  48000, 48000, MAX_INPUT_CHUNK_LEN, 3, -1 
#else  /* #ifndef POLYPHASE_CUBIC_INTERPOLATION */
    NULL, NULL, 2,  48000, 48000, MAX_INPUT_CHUNK_LEN, 3, 0, -1
#endif /* #ifndef POLYPHASE_CUBIC_INTERPOLATION */
#endif /* #ifdef ASRC_ENABLE */

};

void DeinitCadenceAsrc(void **pv_arr_alloc_memory, signed int *w_malloc_count)
{
    for(int i = 0; i < *w_malloc_count; i++)
    {
        if(pv_arr_alloc_memory[i])
        {
            free(pv_arr_alloc_memory[i]);
            pv_arr_alloc_memory[i] = NULL;
        }
    }
    *w_malloc_count=0;
}


int InitCadenceAsrc (xa_codec_handle_t *xa_process_handle, int InputBlockSizeInSamples, int inFs, int outFs, int ChNum,  void **pv_arr_alloc_memory, signed int *w_malloc_count, int NeedToDisplay)
{
    /* User Config */
    user_config def_user_config;
    int i;
    *w_malloc_count = 0;

	//PRINTF("Cadence, %d \r\n", *w_malloc_count);


    /* Error code */
    XA_ERRORCODE err_code = XA_NO_ERROR;
    VOID (*p_error_init)(void);
    /* API size */
    UWORD32 ui_proc_mem_tabs_size, pui_api_size;
    WORD32 max_outsize = 0;

    AsrcFsInCurrent=inFs;
    AsrcFsInTarget=inFs;

    /* Memory variables */
    UWORD32 n_mems;

    /* The process API function */
    p_xa_process_api = xa_src_pp;
    p_error_init = xa_src_pp_error_handler_init;
    p_proc_err_info = &xa_src_pp_error_info;

    /********************************************************************
     *  Initialize the error handler                                    *
     ********************************************************************/
    (*p_error_init)();

    /* ******************************************************************/
    /* Get the library name, library version and API version            */
    /* ******************************************************************/

    if(NeedToDisplay)
    {
		#ifdef DISPLAY_MESSAGE
			/* Library Info and Identification strings */
			char pb_process_name[30] = "";
			char pb_lib_version[30] = "";
			char pb_api_version[30] = "";

			/* Get the library name string */
			err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_LIB_ID_STRINGS, XA_CMD_TYPE_LIB_NAME, pb_process_name);
			_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

			/* Get the library version string */
			err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_LIB_ID_STRINGS, XA_CMD_TYPE_LIB_VERSION, pb_lib_version);
			_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

			/* Get the api version string */
			err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_LIB_ID_STRINGS, XA_CMD_TYPE_API_VERSION, pb_api_version);
			_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

			/* Display the Tensilica identification message */
		    //SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
				PRINTF("\n%s LIB version : %s  API version : %s\n", pb_process_name, pb_lib_version, pb_api_version);
				PRINTF("Cadence, Inc. https://www.cadence.com\n\n");
		    //SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
		#endif  //DISPLAY_MESSAGE
    }

    /* ******************************************************************/
    /* Initialize API structure and set config params to default        */
    /* ******************************************************************/
    /* Get the API size */
    err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_API_SIZE, 0, &pui_api_size);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

    if(NeedToDisplay)
    {
		#if SHOW_SIZES
    	PRINTF("\nAPI structure size: %u bytes\n", pui_api_size);
		#endif
    }

    /* Allocate memory for API */
    pv_arr_alloc_memory[*w_malloc_count] = malloc(pui_api_size);

    if (pv_arr_alloc_memory[*w_malloc_count] == NULL)
    {
        _XA_HANDLE_ERROR(&xa_testbench_error_info_Asrc, "API struct alloc", XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
    }

    /* API object requires 4 bytes (WORD32) alignment;
    * malloc() provides at least 8-byte alignment.
    */
    assert((((unsigned int) pv_arr_alloc_memory[*w_malloc_count]) & 3) == 0);

    /* Set API object with the memory allocated */
    *xa_process_handle = (xa_codec_handle_t) pv_arr_alloc_memory[*w_malloc_count];
    //was:   *w_malloc_count++; but report warning, and actual running gives bad result to w_malloc_count ???
    *w_malloc_count=*w_malloc_count+1;

    /* Set the config params to default values */
    err_code = (*p_xa_process_api)(
    		*xa_process_handle,
                   XA_API_CMD_INIT,
                   XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
                   NULL);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

    /* ******************************************************************/
    /* Initialize Memory info tables                                    */
    /* ******************************************************************/
    /* Get memory info tables size */
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_GET_MEMTABS_SIZE, 0,
                                   &ui_proc_mem_tabs_size);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

    if(NeedToDisplay)
    {
		#if SHOW_SIZES
    	PRINTF("MEMTABS size: %u bytes\n", ui_proc_mem_tabs_size);
		#endif
    }

    pv_arr_alloc_memory[*w_malloc_count] = malloc(ui_proc_mem_tabs_size);
    if(pv_arr_alloc_memory[*w_malloc_count] == NULL)
    {
        _XA_HANDLE_ERROR(&xa_testbench_error_info_Asrc, "Mem tables alloc", XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
    }

    /* Memory table requires 4 bytes (WORD32) alignment; malloc()
     * provides at least 8-byte alignment */
    assert((((unsigned int) pv_arr_alloc_memory[*w_malloc_count]) & 3) == 0);

    /* Set pointer for process memory tables */
    err_code = (*p_xa_process_api)(
    		*xa_process_handle,
                   XA_API_CMD_SET_MEMTABS_PTR, 0,
                   (void *) pv_arr_alloc_memory[*w_malloc_count]);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

    //was:   *w_malloc_count++; but report warning, and actual running gives bad result to w_malloc_count ???
    *w_malloc_count=*w_malloc_count+1;

    /* ******************************************************************/
    /* Read user config, set configs for Sample Rate Converter          */
    /* ******************************************************************/
    def_user_config = default_config;

    def_user_config.channels=ChNum;
    def_user_config.fs_in=inFs;
    def_user_config.fs_out=outFs;
    def_user_config.input_chunk_size=InputBlockSizeInSamples;
    def_user_config.bytes_per_sample=3;
	#ifdef POLYPHASE_CUBIC_INTERPOLATION
		def_user_config.enable_cubic=0;
	#endif
    def_user_config.reset=-1;
	#ifdef ASRC_ENABLE
		def_user_config.enable_asrc=0;
	#endif

    /*************** Set Configs **************/
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_INPUT_SAMPLE_RATE,
                                   &def_user_config.fs_in);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Input Fs Error", err_code);

    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_OUTPUT_SAMPLE_RATE,
                                   &def_user_config.fs_out);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Output Fs Error", err_code);

    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_INPUT_CHANNELS,
                                   &def_user_config.channels);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Input Channels Error", err_code);

	err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_BYTES_PER_SAMPLE,
                                   &def_user_config.bytes_per_sample);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Input PCM width Error", err_code);

#ifdef POLYPHASE_CUBIC_INTERPOLATION

    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_ENABLE_CUBIC,
                                   &def_user_config.enable_cubic);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Enable Cubic Interpolation Error", err_code);

#endif /* #ifdef POLYPHASE_CUBIC_INTERPOLATION */


#ifdef ASRC_ENABLE

        err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_ENABLE_ASRC,
                                   &def_user_config.enable_asrc);
        _XA_HANDLE_ERROR(p_proc_err_info, " Set enable asrc value Error", err_code);

        if ( def_user_config.enable_asrc == 1 )
        {
            err_code = (*p_xa_process_api)(*xa_process_handle,
                                          XA_API_CMD_SET_CONFIG_PARAM,
                                          XA_SRC_PP_CONFIG_PARAM_DRIFT_ASRC,
                                          &def_user_config.drift_asrc);
            _XA_HANDLE_ERROR(p_proc_err_info, "ASRC drift value Error", err_code);
        }


#endif /* #ifdef ASRC_ENABLE */
    /* Set number of samples in the input buffer */
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_INPUT_CHUNK_SIZE,
                                   &def_user_config.input_chunk_size);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Input Chunk Size Error", err_code);

    /* initialize the API, post config, fill memory tables */
    err_code = (*p_xa_process_api)(
    		*xa_process_handle,
                   XA_API_CMD_INIT,
                   XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
                   NULL);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

    /* ******************************************************************/
    /* Allocate Memory with info from library                           */
    /* ******************************************************************/

    /* Get number of memory tables required */
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_GET_N_MEMTABS,
                                   0,
                                   &n_mems);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

    for (i = 0; i < (WORD32) n_mems; i++)
    {
        int ui_size, ui_alignment, ui_type;
        pVOID pv_alloc_ptr;


        WORD32 n_input;

        /* Get memory size */
        err_code = (*p_xa_process_api)(*xa_process_handle,
                                       XA_API_CMD_GET_MEM_INFO_SIZE,
                                       i,
                                       &ui_size);
        _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

        /* Get memory alignment */
        err_code = (*p_xa_process_api)(*xa_process_handle,
                                       XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
                                       i,
                                       &ui_alignment);
        _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

        /* Get memory type */
        err_code = (*p_xa_process_api)(
        		*xa_process_handle,
                       XA_API_CMD_GET_MEM_INFO_TYPE,
                       i,
                       &ui_type);
        _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

        pv_arr_alloc_memory[*w_malloc_count] = malloc(ui_size);
        if(pv_arr_alloc_memory[*w_malloc_count] == NULL)
        {
            _XA_HANDLE_ERROR(&xa_testbench_error_info_Asrc, "Mem tables alloc",
                             XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
        }

        /* The following assertion should never fail because malloc() on
         * Xtensa always returns memory aligned on at least an 8-byte
         * boundary.
         */
        assert((((unsigned int) pv_arr_alloc_memory[*w_malloc_count]) % ui_alignment) == 0);

        pv_alloc_ptr = (void *) pv_arr_alloc_memory[*w_malloc_count];
        *w_malloc_count=*w_malloc_count+1;

        /* Set the buffer pointer */
        err_code = (*p_xa_process_api)(
        		*xa_process_handle,
                       XA_API_CMD_SET_MEM_PTR,
                       i,
                       pv_alloc_ptr);
        _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

        switch (ui_type)
        {
        case XA_MEMTYPE_INPUT:


            n_input = def_user_config.input_chunk_size;
#if SHOW_SIZES
            fprintf(stdout, "Input buffer size: %u bytes\n", ui_size);
#endif /* SHOW_SIZES */

            ppin_buffer = pv_alloc_ptr;

#ifdef INPUT_PING_PONG
            p_input_buffer_ping = (WORD32 *)pv_alloc_ptr;

            pv_arr_alloc_memory[*w_malloc_count] = malloc(ui_size);
            if(pv_arr_alloc_memory[*w_malloc_count] == NULL) {
            _XA_HANDLE_ERROR(&xa_testbench_error_info, "Mem tables alloc",
                             XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
            }

            /* 8-byte alignment check, should never fail on xtensa */
            assert((((unsigned int) pv_arr_alloc_memory[*w_malloc_count]) % ui_alignment) == 0);
            p_input_buffer_pong = (void *) pv_arr_alloc_memory[*w_malloc_count];
            *w_malloc_count=*w_malloc_count+1;

            in_idx = i;
#endif /* INPUT_PING_PONG */

            break;

        case XA_MEMTYPE_OUTPUT:

            ppout_buffer = (WORD32 *)pv_alloc_ptr;
#if SHOW_SIZES
            fprintf(stdout, "Output buffer size: %u bytes\n", ui_size);
#endif /* SHOW_SIZES */

#ifdef OUTPUT_PING_PONG
            p_output_buffer_ping = (WORD32 *)pv_alloc_ptr;

            pv_arr_alloc_memory[*w_malloc_count] = malloc(ui_size);
            if(pv_arr_alloc_memory[*w_malloc_count] == NULL) {
            _XA_HANDLE_ERROR(&xa_testbench_error_info_Asrc, "Mem tables alloc",
                             XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
            }

            /* 8-byte alignment check, should never fail on xtensa */
            assert((((unsigned int) pv_arr_alloc_memory[*w_malloc_count]) % ui_alignment) == 0);
            p_output_buffer_pong = (void *) pv_arr_alloc_memory[*w_malloc_count];
            *w_malloc_count=*w_malloc_count+1;

            out_idx = i;
#endif /* #ifdef OUTPUT_PING_PONG */

            max_outsize = ui_size/(4* def_user_config.channels);

            break;

        case XA_MEMTYPE_SCRATCH:
#if SHOW_SIZES
            fprintf(stdout, "Scratch buffer size: %u bytes\n", ui_size);
#endif /* SHOW_SIZES */

#ifdef SCRATCH_TRASH_TEST
            scratch_size = ui_size;
            p_scratch = (void *)pv_alloc_ptr;
#endif /* #ifdef SCRATCH_TRASH_TEST */

            break;

        case XA_MEMTYPE_PERSIST:
#if SHOW_SIZES
            fprintf(stdout, "Persistent buffer size: %u bytes\n", ui_size);
#endif /* SHOW_SIZES */
            break;

        default:
            assert(0);
        }
    } /* for (i = 0; i < (WORD32) n_mems; i++)  */



    /* ******************************************************************/
    /* Initialization of the Sample Rate Converter                      */
    /* ******************************************************************/
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_INIT,
                                   XA_CMD_TYPE_INIT_PROCESS,
                                   NULL);
    _XA_HANDLE_ERROR(p_proc_err_info, "Initialization Error", err_code);

    fprintf(stdout, "\n");

    /* ******************************************************************/
    /* Convey the SRC process details to the user                       */
    /* ******************************************************************/
    {
        int num_of_stages;

        /* Get the number of stages */
        err_code = (*p_xa_process_api)(*xa_process_handle,
                                       XA_API_CMD_GET_CONFIG_PARAM,
                                       XA_SRC_PP_CONFIG_PARAM_GET_NUM_STAGES,
                                       &num_of_stages);
        _XA_HANDLE_ERROR(p_proc_err_info, "Get number of stages Error", err_code);

        if(NeedToDisplay)
        {
        	PRINTF("\nConverting from %d to %d", def_user_config.fs_in, def_user_config.fs_out);
        	PRINTF(" in %d stages\n", num_of_stages);

        	PRINTF("\nChannels: %d\n", def_user_config.channels);
        	PRINTF("\nInput Sample Rate: %d\n", def_user_config.fs_in);
        	PRINTF("\nOutput Sample Rate: %d\n", def_user_config.fs_out);

        	PRINTF("\nMaximum output size: %d samples\n",max_outsize);
        }
    }

    if(NeedToDisplay)
    {
	    //SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
		PRINTF("malloc_count: %d\n", *w_malloc_count);
	    //SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
    }

	return XA_NO_ERROR;
}
//xa_codec_handle_t SRC_process_Mic_handle

int ProcCadenceAsrc(xa_codec_handle_t *xa_process_handle, int *AudioS32DstPtr, int *AudioS32SrcPtr, int InSampleNum, int *OutputSampleNum)
{
    XA_ERRORCODE err_code = XA_NO_ERROR;

    /* Set number of samples in the input buffer */
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_INPUT_CHUNK_SIZE,
                                   &InSampleNum);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Input Chunk Size Error", err_code);

    /* Set Input Buffer Pointer */
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_SET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_SET_INPUT_BUF_PTR,
								   AudioS32SrcPtr);
    _XA_HANDLE_ERROR(p_proc_err_info, "Set Input Buffer Pointer Error", err_code);

	/* Set Output Buffer Pointer */
	err_code = (*p_xa_process_api)(*xa_process_handle,
								   XA_API_CMD_SET_CONFIG_PARAM,
								   XA_SRC_PP_CONFIG_PARAM_SET_OUTPUT_BUF_PTR,
								   AudioS32DstPtr);
	_XA_HANDLE_ERROR(p_proc_err_info, "Set Output Buffer Pointer Error", err_code);

	#ifdef ASRC_ENABLE
		//update drifting value
		if(AsrcDriftingValueCurrent!=AsrcDriftingValueTarget)
		{
			long long DriftValue = ( long long)( (AsrcDriftingValueTarget)*  ( (long long)1 << 31) ); /* converting drift_float to Q31 fixed value */

			err_code = (*p_xa_process_api)(*xa_process_handle,
										  XA_API_CMD_SET_CONFIG_PARAM,
										  XA_SRC_PP_CONFIG_PARAM_DRIFT_ASRC,
										  &DriftValue);
			_XA_HANDLE_ERROR(p_proc_err_info, "ASRC drift value Error", err_code);

			AsrcDriftingValueCurrent=AsrcDriftingValueTarget;
			PtrVarBlockSharedByDspAndMcu->CycCnt[2]++;
		}
	#endif
	//main process
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_EXECUTE,
                                   XA_CMD_TYPE_DO_EXECUTE,
                                   NULL);
    _XA_HANDLE_ERROR(p_proc_err_info, "Exec Error", err_code);


    /* Get the number of samples in the output buffer */
    err_code = (*p_xa_process_api)(*xa_process_handle,
                                   XA_API_CMD_GET_CONFIG_PARAM,
                                   XA_SRC_PP_CONFIG_PARAM_OUTPUT_CHUNK_SIZE,
								   OutputSampleNum);
    _XA_HANDLE_ERROR(p_proc_err_info, "Get Output Chunk Size Error", err_code);

	return XA_NO_ERROR;
}

