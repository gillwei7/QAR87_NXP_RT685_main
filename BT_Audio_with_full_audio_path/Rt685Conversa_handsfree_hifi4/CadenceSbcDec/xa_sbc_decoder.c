/*
 * Copyright (c) 2009-2020 Cadence Design Systems, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/config/core.h>
#include <xtensa/xos.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "fsl_debug_console.h"
#include "fsl_sema42.h"
#include "fsl_gpio.h"

#include "GlobalDef.h"

#include "xa_type_def.h"
#include "xa_error_standards.h"
#include "xa_error_handler.h"
#include "xa_apicmd_standards.h"
#include "xa_memory_standards.h"
#include "xa_sbc_dec_api.h"


//#include "TestCode1.h"
//#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "SRCProc.h"
#include "DspMainAudioFlow.h"

#ifdef __XCC__
#include <xtensa/hal.h>
#endif


#if !defined(SHOW_SIZES)
#define SHOW_SIZES 1
#endif	/* SHOW_SIZES */

unsigned char *SbcFileBegPtr;
unsigned char *SbcFileEndPtr;
unsigned char *SbcFileCurrentPtr;

int SbcDecoderIsInited;
int SbcDecoderIsMutedButStillRunning;
int SbcDecoderIsRunning;


#define SbcOutputCirBuf_LRMixed_LengthInSample_Max		(SbcOutputCirBuf_LRMixed_LengthInMs*48)					//when fs is 48KHz, it is max sample size
T_CircularAudioBuf_S32 SbcOutputCirBuf_LRMixed;
int SbcOutputCirBuf_LRMixed_DataArea[SbcOutputCirBuf_LRMixed_LengthInSample_Max+AudioFrameSizeInSamplePerCh_48KHz];	//+ extra read size = AudioFrameSizeInSamplePerCh
int SbcPacketsDecoded;

VOID xa_sbc_dec_error_handler_init();
VOID xa_testbench_error_handler_init();

extern int freadFromFlashBin(unsigned char *DstPtr, int SizeInByte, unsigned char **FileCurPtr, unsigned char *FileEndPtr);
extern xa_error_info_struct xa_testbench_error_info;
extern xa_error_info_struct xa_sbc_dec_error_info;


#if !defined(WAV_HEADER)
#define WAV_HEADER 1
#endif	/* WAV_HEADER */

#if !defined(DISPLAY_MESSAGE)
#define DISPLAY_MESSAGE	1
#endif	/* DISPLAY_MESSAGE */

#if !defined(PROFILE)
#define PROFILE 1
#endif

//#if PROFILE
#if 0
#include <sys/times.h>
#include <xtensa/sim.h>
#endif

#define MAX_MEM_ALLOCS 10
#define XA_MAX_CMD_LINE_LENGTH 300
#define XA_MAX_ARGS 20

#define PARAMFILE_SBC "paramfilesimple_sbc_dec.txt"

/* This is used to verify the scratch memory behavior */
#define TEST_SCRATCH

#define XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED	0xffff8000
#define XA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED	0xffff8001


pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];
WORD  g_w_malloc_count;
//FILE *g_pf_inp, *g_pf_out;

#if 0
/* Store data in little-endian (Intel) format regardless of the
   target processor byte order. */
void
xa_fwrite (void *buffer, int size, int nwords, FILE *fp)
{
  int i, j;
  char *pb_buf = (char *) buffer;
  
  for (i = 0; i < nwords; i++) {
#if __XTENSA_EL__
    for (j = 0; j < size; j++) {
      putc(pb_buf[i * size + j], fp);
    }
#else
    for (j = size - 1; j >= 0; j--) {
      putc(pb_buf[i * size + j], fp);
    }
#endif
  }
}
#endif

//#if WAV_HEADER
#if 0
static void
write16_bits_lh(FILE *fp, WORD32 i)
{
  putc(i & 0xff, fp);
  putc((i >> 8) & 0xff, fp);
}


static void
write32_bits_lh(FILE *fp, WORD32 i)
{
  write16_bits_lh(fp, (WORD32)(i & 0xffffL));
  write16_bits_lh(fp, (WORD32)((i >> 16) & 0xffffL));
}

WORD32
write_wav_header (FILE *fp, /* file to write */
		  WORD32 pcmbytes, /* total bytes in the wav file */
		  WORD32 freq, /* sample rate */
		  WORD32 channels, /* output channels */
		  WORD32 bits /* bits per sample */)
{
  WORD32 bytes = (bits + 7) / 8;
  fwrite("RIFF", 1, 4, fp); /* label */
  write32_bits_lh(fp, pcmbytes + 44 - 8); /* length in bytes without header */
  fwrite("WAVEfmt ", 1, 8, fp); /* 2 labels */
  write32_bits_lh(fp, 2 + 2 + 4 + 4 + 2 + 2); /* length of PCM format decl area */
  write16_bits_lh(fp, 1); /* is pcm? */
  write16_bits_lh(fp, channels);
  write32_bits_lh(fp, freq);
  write32_bits_lh(fp, freq * channels * bytes); /* bps */
  write16_bits_lh(fp, channels * bytes);
  write16_bits_lh(fp, bits);
  fwrite("data", 1, 4, fp);
  write32_bits_lh(fp, pcmbytes);

  return (ferror(fp) ? -1 : 0);
}

#endif /* WAV_HEADER */

#if 0
void usage()
{
  fprintf(stdout,
	  "\n"
	  "Usage:  xt-run <binfile> [options] -ifile:<inputfile> -ofile:<outputfile>\n"
	  "        <binfile>     HiFi2 SBC Decoder executable\n"
	  "        <inputfile>   Input SBC file\n"
//#if WAV_HEADER
#if 0
	  "        <outputfile>  Output WAV file\n"
#else
	  "        <outputfile>  Output PCM file\n"
#endif
	  "        -h            Print this help message\n");
  exit(0);
}
#endif

#if 1
//decoder variables
#if DISPLAY_MESSAGE
  /* Library Info and Identification strings */
  WORD8 pb_process_name[30] = "";
  WORD8 pb_lib_version[30] = "";
#endif

  /* Error code */
  XA_ERRORCODE err_code = XA_NO_ERROR;
  XA_ERRORCODE err_code_exec = XA_NO_ERROR;

  /* API obj */
  xa_codec_handle_t xa_process_handle;

#ifdef TEST_SCRATCH
  static int scratch_size = 0;
#endif

  /* First part                                        */
  /* Error Handler Init                                */
  /* Get Library Name, Library Version and API Version */
  /* Initialize API structure + Default config set     */
  /* Set config params from user                       */
  /* Initialize memory tables                          */
  /* Get memory information and allocate memory        */

  /* Memory variables */
  UWORD32 n_mems = 0;
  UWORD32 ui_proc_mem_tabs_size = 0;
  /* API size */
  UWORD32 pui_api_size = 0;
  /* Process initing done query variable */
  UWORD32 ui_init_done = 0;
  UWORD32 ui_exec_done = 0;
  pWORD8 pb_inp_buf = NULL;
  pWORD8 pb_out_buf = NULL;

  UWORD32 ui_inp_size = 0;
  WORD32 i_bytes_consumed = 0;
  WORD32 i_bytes_read = 0;
  WORD32 i_buff_size = 0;

  WORD32 i_out_bytes = 0, TotalAudioDataBytesGenerated = 0, TotalSbcDataBytesConsumed = 0;
  WORD32 i_bitrate = 0, i_samp_freq = 0, i_num_chan = 0, i_pcm_wd_sz = 0;
  int SbcAudioSamplesGeneratedPerCh;
  int SbcFrmSizeReadIn;
  int SbcFrmSizeToBeProcessed;


  /* The process API function */
  static xa_codec_func_t *p_xa_process_api;

  /* The process error info structure */
  static xa_error_info_struct *p_proc_err_info;

  /* The get config from API */
  XA_ERRORCODE (*p_get_config_param) (xa_codec_handle_t p_xa_process_api_obj,
				      pWORD32           pi_bitrate,
				      pWORD32           pi_samp_freq,
				      pWORD32           pi_num_chan,
				      pWORD32           pi_pcm_wd_sz);

  /* The error init function */
  VOID (*p_error_init)();

#endif

XA_ERRORCODE xa_sbc_dec_get_config_param (xa_codec_handle_t p_xa_process_api_obj,
			     pWORD32           pi_bitrate,
			     pWORD32           pi_samp_freq,
			     pWORD32           pi_num_chan,
			     pWORD32           pi_pcm_wd_sz)
{
  XA_ERRORCODE err_code = XA_NO_ERROR;
  /* the process API function */
  xa_codec_func_t *p_xa_process_api = xa_sbc_dec;
  xa_error_info_struct *p_proc_err_info = &xa_sbc_dec_error_info;

  /* Data rate */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
				   XA_API_CMD_GET_CONFIG_PARAM,
				   XA_SBC_DEC_CONFIG_PARAM_BITRATE, pi_bitrate);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  }
  /* Sampling frequency */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
				   XA_API_CMD_GET_CONFIG_PARAM,
				   XA_SBC_DEC_CONFIG_PARAM_SAMP_FREQ, pi_samp_freq);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  }
  /* Number of channels */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
				   XA_API_CMD_GET_CONFIG_PARAM,
				   XA_SBC_DEC_CONFIG_PARAM_NUM_CHANNELS, pi_num_chan);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);
  }
  /* PCM word size */
  {
    err_code = (*p_xa_process_api)(p_xa_process_api_obj,
				   XA_API_CMD_GET_CONFIG_PARAM,
				   XA_SBC_DEC_CONFIG_PARAM_PCM_WDSZ, pi_pcm_wd_sz);
    _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);
  }
  return XA_NO_ERROR;
}

static unsigned int output_wordsize (unsigned int sample_bits)
{
  /* Round up to the next 2-byte size: 16 -> 2; 24 -> 4. */
  return 2 * ((sample_bits + 15) / 16);
}

void InitSbcOutputCirBuf(int fs, int ToClearMem)
{
	int SampleNum;

	SampleNum=fs/1000*SbcOutputCirBuf_LRMixed_LengthInMs;
	assert(SampleNum <= SbcOutputCirBuf_LRMixed_LengthInSample_Max);

	InitCirAudioBuf_S32(&SbcOutputCirBuf_LRMixed,SbcOutputCirBuf_LRMixed_DataArea,SampleNum);

	if(ToClearMem)
		memset(SbcOutputCirBuf_LRMixed_DataArea,0,sizeof(SbcOutputCirBuf_LRMixed_DataArea));
}

#if 0
// Set cache attribute to Write Back No Allocate when the last argument is -wbna
void set_wbna(int *argc, char *argv[])
{
    if ( *argc > 1 && !strcmp(argv[*argc-1], "-wbna") ) {
#ifdef __XCC__
        extern char _memmap_cacheattr_wbna_trapnull;
     
        xthal_set_cacheattr((unsigned)&_memmap_cacheattr_wbna_trapnull);
#endif
        (*argc)--;
    }
}
#endif
void InitSbcDecoderVarToDefault(void)
{
#if DISPLAY_MESSAGE
  /* Library Info and Identification strings */
  pb_process_name[0] = 0;
  pb_lib_version[0] = 0;
#endif

  /* Error code */
  err_code = XA_NO_ERROR;
  err_code_exec = XA_NO_ERROR;

#ifdef TEST_SCRATCH
  scratch_size = 0;
#endif

  /* First part                                        */
  /* Error Handler Init                                */
  /* Get Library Name, Library Version and API Version */
  /* Initialize API structure + Default config set     */
  /* Set config params from user                       */
  /* Initialize memory tables                          */
  /* Get memory information and allocate memory        */

  /* Memory variables */
  n_mems = 0;
  ui_proc_mem_tabs_size = 0;
  /* API size */
  pui_api_size = 0;
  /* Process initing done query variable */
  ui_init_done = 0;
  ui_exec_done = 0;
  pb_inp_buf = NULL;
  pb_out_buf = NULL;

  ui_inp_size = 0;
  i_bytes_consumed = 0;
  i_bytes_read = 0;
  i_buff_size = 0;

  i_out_bytes = 0; TotalAudioDataBytesGenerated = 0; TotalSbcDataBytesConsumed=0;
  i_bitrate = 0; i_samp_freq = 0; i_num_chan = 0; i_pcm_wd_sz = 0;

  /* Stack process struct initing */
  p_xa_process_api = xa_sbc_dec;
  p_get_config_param = xa_sbc_dec_get_config_param;
  p_error_init = xa_sbc_dec_error_handler_init;
  p_proc_err_info = &xa_sbc_dec_error_info;
  /* Stack process struct initing end */

  /* ******************************************************************/
  /* Initialize the error handler                                     */
  /* ******************************************************************/
  (*p_error_init)();

}
int InitSbcDecoder(void)
{
	int i;

	SbcPacketsDecoded=0;
	SbcDecoderIsRunning=0;
	SbcDecoderIsMutedButStillRunning=0;
	InitSbcDecoderVarToDefault();

  /* ******************************************************************/
  /* Get the library name, library version and API version            */
  /* ******************************************************************/

#if EnableSbcDecodingPrint
  /* Get the library name string */
  err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_LIB_ID_STRINGS,
				 XA_CMD_TYPE_LIB_NAME, pb_process_name);

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  /* Get the library version string */
  err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_LIB_ID_STRINGS,
				 XA_CMD_TYPE_LIB_VERSION, pb_lib_version);

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  /* Display the Tensilica identification message */
  PRINTF("\n%s version %s\n", pb_process_name, pb_lib_version);
  PRINTF("Tensilica, Inc. http://www.tensilica.com\n\n");
#endif

  /* ******************************************************************/
  /* Initialize API structure and set config params to default        */
  /* ******************************************************************/

  /* Get the API size */
  err_code = (*p_xa_process_api)(NULL, XA_API_CMD_GET_API_SIZE, 0,
				 &pui_api_size);
#if EnableSbcDecodingPrint
  PRINTF("\nAPI structure size: %u bytes\n", pui_api_size);
#endif

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  /* Allocate memory for API */
  g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(pui_api_size);

  if (g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
    _XA_HANDLE_ERROR(&xa_testbench_error_info, "API struct alloc",
		     XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
  }

  /* API object requires 4 bytes (WORD32) alignment;
   * malloc() provides at least 8-byte alignment.
   */
  assert((((unsigned int) g_pv_arr_alloc_memory[g_w_malloc_count]) & 3) == 0);

  /* Set API object with the memory allocated */
  xa_process_handle = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

  g_w_malloc_count++;

  /* Set the config params to default values */
  err_code = (*p_xa_process_api)(xa_process_handle,
				 XA_API_CMD_INIT,
				 XA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS,
				 NULL);

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  /* ******************************************************************/
  /* Initialize Memory info tables                                    */
  /* ******************************************************************/

  /* Get memory info tables size */
  err_code = (*p_xa_process_api)(xa_process_handle,
				 XA_API_CMD_GET_MEMTABS_SIZE, 0,
				 &ui_proc_mem_tabs_size);
#if EnableSbcDecodingPrint
  PRINTF("\nMEMTABS size: %u bytes\n\n", ui_proc_mem_tabs_size);
#endif

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);


  g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(ui_proc_mem_tabs_size);

  if(g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
    _XA_HANDLE_ERROR(&xa_testbench_error_info, "Mem tables alloc",
		     XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
  }

  /* Memory table requires 4 bytes (WORD32) alignment; malloc()
   * provides at least 8-byte alignment.
   */
  assert((((unsigned int) g_pv_arr_alloc_memory[g_w_malloc_count]) & 3) == 0);

  /* Set pointer for process memory tables	*/
  err_code = (*p_xa_process_api)(xa_process_handle,
				 XA_API_CMD_SET_MEMTABS_PTR, 0,
				 (void *) g_pv_arr_alloc_memory[g_w_malloc_count]);

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  g_w_malloc_count++;

  /* initialize the API, post config, fill memory tables	*/
  err_code = (*p_xa_process_api)(xa_process_handle,
				 XA_API_CMD_INIT,
				 XA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS,
				 NULL);

  _XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

  /* ******************************************************************/
  /* Allocate Memory with info from library                           */
  /* ******************************************************************/

	/* Get number of memory tables required */
	err_code = (*p_xa_process_api)(xa_process_handle,
				 XA_API_CMD_GET_N_MEMTABS,
				 0,
				 &n_mems);

	_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

	for (i = 0; i < (WORD32) n_mems; i++)
	{
		int ui_size, ui_alignment, ui_type;
		pVOID pv_alloc_ptr;

		/* Get memory size */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_GET_MEM_INFO_SIZE,
					   i,
					   &ui_size);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		#ifdef TEST_SCRATCH
			if (i == 1)
			{
				scratch_size = ui_size;
			}
		#endif

		/* Get memory alignment */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_GET_MEM_INFO_ALIGNMENT,
					   i,
					   &ui_alignment);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		/* Get memory type */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_GET_MEM_INFO_TYPE,
					   i,
					   &ui_type);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(ui_size);

		if(g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
		  _XA_HANDLE_ERROR(&xa_testbench_error_info, "Mem tables alloc",
				   XA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
		}

		/* The following assertion should never fail because malloc() on
		 * Xtensa always returns memory aligned on at least an 8-byte
		 * boundary.
		 */
		assert((((unsigned int) g_pv_arr_alloc_memory[g_w_malloc_count]) % ui_alignment) == 0);

		pv_alloc_ptr = (void *) g_pv_arr_alloc_memory[g_w_malloc_count];

		g_w_malloc_count++;

		/* Set the buffer pointer */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_SET_MEM_PTR,
					   i,
					   pv_alloc_ptr);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		if(ui_type == XA_MEMTYPE_INPUT)
		{
		  pb_inp_buf = pv_alloc_ptr;
		  ui_inp_size = ui_size;
		}
		if(ui_type == XA_MEMTYPE_OUTPUT)
		{
		  pb_out_buf = pv_alloc_ptr;
		}

		#if EnableSbcDecodingPrint
			switch (ui_type)
			{
				case XA_MEMTYPE_INPUT:
					PRINTF("Input buffer size: %u bytes\n", ui_size);
					break;
				case XA_MEMTYPE_OUTPUT:
					PRINTF("Output buffer size: %u bytes\n", ui_size);
					break;
				case XA_MEMTYPE_SCRATCH:
					PRINTF("Scratch buffer size: %u bytes\n", ui_size);
					break;
				case XA_MEMTYPE_PERSIST:
					PRINTF("Persistent buffer size: %u bytes\n", ui_size);
					break;
				default:
					assert(0);
			}
		#endif	/* SHOW_SIZES */
	}

  /* End first part */
	SbcDecoderIsInited=1;

	return 0;
}
int InitSbcDecoderForOneSbcFile(int SbcFileIdx)
{
	int i;
	if(SbcFileIdx!=0xffff)
	{
		SbcFileBegPtr=(unsigned char *)(PtrVarBlockSharedByDspAndMcu->FileAddrTable_Sbc[2*SbcFileIdx+0]);
		SbcFileEndPtr=(unsigned char *)(PtrVarBlockSharedByDspAndMcu->FileAddrTable_Sbc[2*SbcFileIdx+1]);
		SbcFileCurrentPtr=SbcFileBegPtr;
	}

	i_bytes_consumed = ui_inp_size;

	/* Second part        */
	/* Initialize process */
	/* Get config params  */

	/* ******************************************************************/
	/* Initialize process in a loop (to handle junk data at beginning)  */
	/* ******************************************************************/
	i_buff_size = ui_inp_size;

	//U32 U32ToPrint1;
	//U32 U32ToPrint2;
	do
	{

		//move unprocessed bytes to the beginning
		for(i = 0; i < (WORD32)ui_inp_size - i_bytes_consumed; i++)
		{
			pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
		}

		#if 1
			//get sbc raw data
			if(SbcFileIdx==0xffff)
			{
				//this is a2dp sbc streaming, not reading sbc from a file
				int l;
			    SEMA42_Lock(APP_SEMA42, SEMA42_GATE1, domainId);
			    	l=CirAudioBuf_SpaceOccupiedInSamples_S8((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw));
			    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE1);

				//PRINTF("CirBuf_SbcRaw start\n");
				if(l>=i_bytes_consumed)
				{
					/*
					PRINTF("CirBuf_SbcRaw head: %x\n",
							(U32)(

							((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw))->PtrBufHead
								 )
					);
					PRINTF("CirBuf_SbcRaw head: %x\n",
							(U32)(

							((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw))->PtrRd
								 )
					);
					PRINTF("CirBuf_SbcRaw head: %x\n",
							(U32)(

							((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw))->PtrWr
								 )
					);
					*/

					if(i_bytes_consumed)
					{
						SEMA42_Lock(APP_SEMA42, SEMA42_GATE1, domainId);
							CirAudioBuf_ReadSamples_S8((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw), i_bytes_consumed, (S8 *)(pb_inp_buf + (ui_inp_size - i_bytes_consumed)));
						SEMA42_Unlock(APP_SEMA42, SEMA42_GATE1);
					}
					i_bytes_read=i_bytes_consumed;
				}else
				{
					//not enough data, exit and do nothing
					return 1;
				}
			}else
			{
				//this is to read sbc from a file
				//read i_bytes_consumed bytes, and put to after earlier unprocessed section, to make a complete buffer which total length is ui_inp_size
				i_bytes_read = freadFromFlashBin((unsigned char *)(pb_inp_buf + (ui_inp_size - i_bytes_consumed)), sizeof(WORD8)*i_bytes_consumed, &SbcFileCurrentPtr, SbcFileEndPtr);
			}

			//U32ToPrint1=*(U32 *)(pb_inp_buf);
			//U32ToPrint2=*(1+(U32 *)(pb_inp_buf));

		#else
			i_bytes_read = fread(pb_inp_buf + (ui_inp_size - i_bytes_consumed),
					 sizeof(WORD8),
					 i_bytes_consumed,
					 g_pf_inp);
		#endif


		/* New buffer size */
		i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);

		if( i_buff_size <= 0 )
		{
			i_buff_size = 0;

			/* Tell that the input is over in this buffer */
			err_code = (*p_xa_process_api)(xa_process_handle,
						 XA_API_CMD_INPUT_OVER,
						 0,
						 NULL);

			_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);
		}

#if 1
		/* Set number of bytes to be processed */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_SET_INPUT_BYTES,
					   0,
					   &i_buff_size);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		/* Initialize the process */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_INIT,
					   XA_CMD_TYPE_INIT_PROCESS,
					   NULL);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		/* Checking for end of initialization */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_INIT,
					   XA_CMD_TYPE_INIT_DONE_QUERY,
					   &ui_init_done);

		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

		/* How much buffer is used in input buffers */
		err_code = (*p_xa_process_api)(xa_process_handle,
					   XA_API_CMD_GET_CURIDX_INPUT_BUF,
					   0,
					   &i_bytes_consumed);

		//PRINTF("InitSbcDecoderForOneSbcFile i_bytes_consumed: %d \n", i_bytes_consumed);
		_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);
#else
		i_bytes_consumed=0;
		ui_init_done=1;
		PRINTF("InitSbcDecoderForOneSbcFile i_bytes_consumed: %d \n", i_bytes_consumed);
#endif

	} while (!ui_init_done);
	/* Init end of process condition end */

#if 1
	/* ******************************************************************/
	/* Get config params from API                                       */
	/* ******************************************************************/
	err_code = (*p_get_config_param)(xa_process_handle,
				   &i_bitrate,
				   &i_samp_freq,
				   &i_num_chan,
				   &i_pcm_wd_sz);
#else
	   i_bitrate=1;
	   i_samp_freq=44100;
	   i_num_chan=2;
	   i_pcm_wd_sz=16;
#endif

	_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);
	PRINTF("InitSbcDecoderForOneSbcFile %d %d %d %d  \r\n",i_bitrate,i_samp_freq,i_num_chan,i_pcm_wd_sz);
	//PRINTF("InitSbcDecoderForOneSbcFile read: %d %d %x %x\n", i_bytes_read, i_bytes_consumed, U32ToPrint1, U32ToPrint2);

	//this is to check if the input data is changed or not --- it is not changed
	//U32ToPrint1=*(U32 *)(pb_inp_buf);
	//U32ToPrint2=*(1+(U32 *)(pb_inp_buf));
	//PRINTF("InitSbcDecoderForOneSbcFile exit: %x %x\n", U32ToPrint1, U32ToPrint2);

	#if EnableSbcDecodingPrint
		PRINTF("\nData Rate: %d bps\n", i_bitrate);
		PRINTF("Sample Rate: %d Hz\n", i_samp_freq);
		PRINTF("Number of Channels: %d\n", i_num_chan);
		PRINTF("PCM Sample Size: %d bits\n", i_pcm_wd_sz);
		//PRINTF("Standard: SBC\n");
	#endif
	/* End second part */


	//init SRC
	//             (ptr to handle.     int InputBlockSizeInSamples,  int inFs,                 int outFs,                  int ChNum,  EnableAsrc NeedToDisplay)
	InitCadenceAsrc(&SRC_DecoderSbc,DecoderSbc_SrcInSizeInSamples, i_samp_freq,   PtrVarBlockSharedByDspAndMcu->I2SFs_Amp,      2,           1,          1     );
	//                                 DecoderSbc_SrcInSizeInSamples is to reserve enough space for output, later the input block size in samples will be set again in the src processing

	SRC_DecoderSbc.AodTgtValue=((T_CircularAudioBuf_S8 *)&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw)->LengthInSamples/2;
	SRC_DecoderSbc.KiAccMax=1000000;
	SRC_DecoderSbc.Kp=(float)(1.0f/20000000.0f);			//kp ki should be set to proper value according to each kind of SBC stream --- now only good for 44.1KHz sbc, 581 byte packet
	SRC_DecoderSbc.Ki=(float)(1.0f/100000000.0f);			//kp ki should be set to proper value according to each kind of SBC stream --- now only good for 44.1KHz sbc, 581 byte packet


	InitSbcOutputCirBuf(PtrVarBlockSharedByDspAndMcu->I2SFs_Amp,0);

	//#if WAV_HEADER
	#if 0
	  write_wav_header(g_pf_out, 0x7fffffff, i_samp_freq, i_num_chan, i_pcm_wd_sz);
	#endif

	//#if PROFILE
	#if 0
		struct tms start, stop;
		double Peak = 0, Ave = 0, Sum = 0;
		int frame = 0;
		int Peak_frame = 0;
		unsigned long long total_samples = 0;
		PRINTF("\n");
	#endif

	SbcDecoderIsInited=2;
	return 0;
}

int SbcDecodeProcess(int SbcFileIdx)
{
	int i;
	DbgPin8Up();

	#if 0
		//empty run
		static int tmpCnt=0;

		if(tmpCnt==10)
		{
			PRINTF("Sbc %d\r\n",tmpCnt);
			SbcDecoderIsRunning=2;
			return 0;
		}else
		{
			tmpCnt++;
			PRINTF("Sbc %d\r\n",tmpCnt);
			return 0;
		}
	#endif

	//#if WAV_HEADER
	#if 0
	  write_wav_header(g_pf_out, 0x7fffffff, i_samp_freq, i_num_chan, i_pcm_wd_sz);
	#endif

	//#if PROFILE
	#if 0
		struct tms start, stop;
		double Peak = 0, Ave = 0, Sum = 0;
		int frame = 0;
		int Peak_frame = 0;
		unsigned long long total_samples = 0;
		fprintf(stdout, "\n");
	#endif

	int loopTime=0;

	if(i_samp_freq==16000)
		loopTime=4*2;		//4*8ms=32ms, this is > 30ms	*2 to double sure call the decoding enough times

	if(i_samp_freq==48000)
		loopTime=12*2;	//12*2.666ms=32ms, this is > 30ms	*2 to double sure call the decoding enough times

	if(i_samp_freq==44100)
		loopTime=11*2;	//11*2.902ms=31.9ms, this is > 30ms	*2 to double sure call the decoding enough times

	int FreeAod_Audio;
	int AodSbc;

	for(int ii=0;ii<loopTime;ii++)	//to run the decoding multiple times to generate longer than 32ms samples  --- signal flow side triggers this decoding once every 24ms or 32ms
	{
		xos_mutex_lock(&g_audio_SbcDecoderMutex);
		FreeAod_Audio=CirAudioBuf_SpaceAvailableInSamples_S32(&SbcOutputCirBuf_LRMixed);
		xos_mutex_unlock(&g_audio_SbcDecoderMutex);

		int SamplesToGeGeneratedEachFrame=	PtrVarBlockSharedByDspAndMcu->I2SFs_Amp /1000 * 10;		//each frame is no more than 10ms --- SBC generates no more than 10ms

		if(FreeAod_Audio>=SamplesToGeGeneratedEachFrame)
		{
			/* Execute process */
			U32 U32ToPrint1;
			U32 U32ToPrint2;

			//PRINTF("Sbc Packet Strt: %d, %d\n", ui_inp_size, i_bytes_consumed);

			//move unprocessed bytes to the beginning
			for(i = 0; i < (WORD32)ui_inp_size - i_bytes_consumed; i++)
			{
				pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
			}

			#if 1
				//get sbc raw data
				//i_bytes_consumed: means last time used, now this time must read back this amount of bytes to make a complete ui_inp_size(512) buffer(pb_inp_buf)
				if(SbcFileIdx==0xffff)
				{
					//this is a2dp sbc streaming, not reading sbc from a file
					SEMA42_Lock(APP_SEMA42, SEMA42_GATE1, domainId);
					AodSbc=CirAudioBuf_SpaceOccupiedInSamples_S8((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw));
					SEMA42_Unlock(APP_SEMA42, SEMA42_GATE1);

					/*
					PRINTF("Hifi: %x, %x, %d, %d\n",
									(U32)(
											((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw))->PtrRd
										 ),
									(U32)(
											((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw))->PtrWr
										 ),
									l,i_bytes_consumed
								);
					*/

					if(AodSbc>=i_bytes_consumed)
					{
						SEMA42_Lock(APP_SEMA42, SEMA42_GATE1, domainId);
							CirAudioBuf_ReadSamples_S8((T_CircularAudioBuf_S8 *)(&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw), i_bytes_consumed, (S8 *)(pb_inp_buf + (ui_inp_size - i_bytes_consumed)));
						SEMA42_Unlock(APP_SEMA42, SEMA42_GATE1);
						i_bytes_read=i_bytes_consumed;
					}else
					{
						//not enough data, exit and do nothing
						//PRINTF("%d, %d,\n",l,i_bytes_consumed);
						DbgPin8Dn();
						return 123;
					}
				}else
				{
					//this is to read sbc from a file
					//read i_bytes_consumed bytes, and put to after earlier unprocessed section, to make a complete buffer which total length is ui_inp_size
					i_bytes_read = freadFromFlashBin((unsigned char *)(pb_inp_buf + (ui_inp_size - i_bytes_consumed)), sizeof(char)*i_bytes_consumed, &SbcFileCurrentPtr, SbcFileEndPtr);
				}

				SbcFrmSizeReadIn=i_bytes_read;
				//U32ToPrint1=*   (U32 *)(pb_inp_buf + (ui_inp_size - i_bytes_consumed));
				//U32ToPrint2=*(1+(U32 *)(pb_inp_buf + (ui_inp_size - i_bytes_consumed)));		//this is the new read in data
				U32ToPrint1=*(U32 *)(pb_inp_buf);
				U32ToPrint2=*(1+(U32 *)(pb_inp_buf));											//this is the data to be processed later, the head of the input buffer

			#else
				i_bytes_read = fread(pb_inp_buf + (ui_inp_size - i_bytes_consumed),
						 sizeof(char),
						 i_bytes_consumed,
						 g_pf_inp);
			#endif

			#if 1	//folding
				/* New buffer size */
				i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);
				SbcFrmSizeToBeProcessed=i_buff_size;

				//PRINTF("i_buff_size %d \r\n", i_buff_size);

				if( i_buff_size <= 0 )
				{
					i_buff_size = 0;

					SbcDecoderIsRunning=2;

					/* Tell that the input is over in this buffer */
					err_code = (*p_xa_process_api)(xa_process_handle,
								 XA_API_CMD_INPUT_OVER,
								 0,
								 NULL);

					_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);
				}

				/* Set number of bytes to be processed */
				err_code = (*p_xa_process_api)(xa_process_handle,
							   XA_API_CMD_SET_INPUT_BYTES,
							   0,
							   &i_buff_size);

				_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

				//#if PROFILE
				#if 0
					xt_iss_client_command("all","enable");
					times(&start);
				#endif

				/* Execute process */
				err_code_exec = (*p_xa_process_api)(xa_process_handle,
							   XA_API_CMD_EXECUTE,
							   XA_CMD_TYPE_DO_EXECUTE,
							   NULL);

				//#if PROFILE
				#if 0
					times(&stop);
					xt_iss_client_command("all","disable");
				#endif

				_XA_HANDLE_ERROR(p_proc_err_info, "", err_code_exec);

				/* Checking for end of processing */
				err_code = (*p_xa_process_api)(xa_process_handle,
							   XA_API_CMD_EXECUTE,
							   XA_CMD_TYPE_DONE_QUERY,
							   &ui_exec_done);

				_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

				/* Get the output bytes */
				err_code = (*p_xa_process_api)(xa_process_handle,
							   XA_API_CMD_GET_OUTPUT_BYTES,
							   0,
							   &i_out_bytes);

				_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);


				//#if PROFILE
				#if 0
					/* Assume that if no output was produced, no
					 * significant cycles were consumed.
					 */
					if (i_out_bytes)
					  {
					frame++;

					clock_t cycles = stop.tms_utime - start.tms_utime;
					int samples = i_out_bytes / (output_wordsize(i_pcm_wd_sz) * i_num_chan);
					double Curr = ((double) cycles / samples * i_samp_freq / 1000000);
					Sum += Curr;
					Ave = Sum / frame;

					if (Peak < Curr) {
					  Peak = Curr;
					  Peak_frame = frame;
					}

					/*
					  Calculate the current time based on the total number of
					  samples produced by the decoder and the sample rate:

					  time [ms] = samples / sample_rate [kHz]

					  Alternatively, the current time can be calculated based on
					  the total number of bytes consumed by the decoder and the
					  data rate:

					  time [ms] = 8 * bytes / data_rate [kbps]
					*/
					total_samples += samples;

					unsigned long long total_msec =
					  (unsigned long long)((double)total_samples / i_samp_freq * 1000.0);
					int msec = (int)(total_msec % 1000);
					unsigned long long total_seconds = total_msec / 1000;
					int seconds = (int)(total_seconds % 60);
					int minutes = (int)(total_seconds / 60);

					fprintf(stdout, "[%d] %d:%02d.%03d MCPS: %.2f Average: %.2f Peak: %.2f @ [%d]\n",
						frame, minutes, seconds, msec,
						Curr, Ave, Peak, Peak_frame);
					  }
				#endif

				err_code = (*p_get_config_param)(xa_process_handle,
								 &i_bitrate,
								 &i_samp_freq,
								 &i_num_chan,
								 &i_pcm_wd_sz);      //value here is not correct

				_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

				unsigned int ui_nsamples = i_out_bytes / output_wordsize(i_pcm_wd_sz);

				TotalAudioDataBytesGenerated += (ui_nsamples * i_pcm_wd_sz) / 8;
				//PRINTF("i_buff_size %d %d %d %d %d  \r\n",i_bitrate,i_samp_freq,i_num_chan,i_pcm_wd_sz,ui_nsamples);
			#endif

			#if 1	//folding
				//fill output buffer
				SbcAudioSamplesGeneratedPerCh=ui_nsamples/i_num_chan;



				int OutSampleNum;
				S16 *TmpSrcPtr=(S16 *)pb_out_buf;

				int SrcSampleLeftUnProcessed=SbcAudioSamplesGeneratedPerCh;
				while(SrcSampleLeftUnProcessed)
				{
					int SamplesToProcessInThisLoop;
					if(SrcSampleLeftUnProcessed>=(3*DecoderSbc_SrcInSizeInSamples/4))
					{
						SamplesToProcessInThisLoop=3*DecoderSbc_SrcInSizeInSamples/4;
					}else
					{
						SamplesToProcessInThisLoop=SrcSampleLeftUnProcessed;
					}
					//PRINTF("i_buff_size 222 \n");

					//process amount of SamplesToProcessInThisLoop
					#if 1	//folding
						//move S16 samples to S32 buffers --- and use SbcDecoderIsMutedButStillRunning to control mute/pause
						if(i_num_chan==2)
						{
							//stereo
							if(SbcDecoderIsMutedButStillRunning)
							{
								//muted, all zeros
								for(int i=0;i<SamplesToProcessInThisLoop;i++)
								{
									SrcIn_2S32Mixed[2*i+0]=0;
									SrcIn_2S32Mixed[2*i+1]=0;
								}
							}else
							{
								for(int i=0;i<SamplesToProcessInThisLoop;i++)
								{
									//not muted
									SrcIn_2S32Mixed[2*i+0]=((*TmpSrcPtr++)<<16);
									SrcIn_2S32Mixed[2*i+1]=((*TmpSrcPtr++)<<16);
									//SrcIn_2S32Mixed[2*i+0]=  0x100000*i;
									//SrcIn_2S32Mixed[2*i+1]=0-0x100000*i;
								}
							}
						}else
						{
							//mono
							if(SbcDecoderIsMutedButStillRunning)
							{
								for(int i=0;i<SamplesToProcessInThisLoop;i++)
								{
									//muted, all zeros
									SrcIn_2S32Mixed[2*i+0]=0;
									SrcIn_2S32Mixed[2*i+1]=0;
								}
							}else
							{
								for(int i=0;i<SamplesToProcessInThisLoop;i++)
								{
									//not muted
									SrcIn_2S32Mixed[2*i+0]=((*TmpSrcPtr  )<<16);
									SrcIn_2S32Mixed[2*i+1]=((*TmpSrcPtr++)<<16);
								}
							}
						}
						//PRINTF("i_buff_size 333 \n");

						//            (TCadenceSRC *SRCPtr, int *AudioS32DstPtr,  int *AudioS32SrcPtr,        int InSampleNum,    int *OutputSampleNum)
						ProcCadenceAsrc(&SRC_DecoderSbc,      SrcOut_2S32Mixed,    SrcIn_2S32Mixed,       SamplesToProcessInThisLoop,    &OutSampleNum);

						//PRINTF("i_buff_size 444 \n");
						//convert 32bit LRMixed buffer to 16Bit LRMixed buffer
						S16 *TmpDstPtr=(S16 *)SrcIn_2S32Mixed;
						for(int i=0;i<OutSampleNum;i++)
						{
							*TmpDstPtr++=(SrcOut_2S32Mixed[2*i+0]>>16);
							*TmpDstPtr++=(SrcOut_2S32Mixed[2*i+1]>>16);
							//*TmpDstPtr++=  0x10*i;
							//*TmpDstPtr++=0-0x10*i;
						}

						//use SrcIn_2S32Mixed as the S16 mixed LR channel as input buffer for the cir buffer
						xos_mutex_lock(&g_audio_SbcDecoderMutex);
							CirAudioBuf_WriteSamples_S32(&SbcOutputCirBuf_LRMixed, OutSampleNum, SrcIn_2S32Mixed);
						xos_mutex_unlock(&g_audio_SbcDecoderMutex);
					#endif

					SrcSampleLeftUnProcessed-=SamplesToProcessInThisLoop;
				}

			#else
				xa_fwrite((pVOID)pb_out_buf, (i_pcm_wd_sz/8), ui_nsamples, g_pf_out);
			#endif

			/* How much buffer is used in input buffers */
			err_code = (*p_xa_process_api)(xa_process_handle,
						   XA_API_CMD_GET_CURIDX_INPUT_BUF,
						   0,
						   &i_bytes_consumed);


			//PRINTF("i_buff_size 666 \n");
			_XA_HANDLE_ERROR(p_proc_err_info, "", err_code);

			assert(i_bytes_consumed <= i_buff_size);
			TotalSbcDataBytesConsumed+=i_bytes_consumed;

			DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();
			DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();DbgPin8Dn();
//			PRINTF("Sbc Packet: %d, %d, %d %d %d H: %x %x Fs: %d %d\n", SbcPacketsDecoded, SbcAudioSamplesGeneratedPerCh, SbcFrmSizeReadIn, SbcFrmSizeToBeProcessed, i_bytes_consumed, U32ToPrint1, U32ToPrint2, i_samp_freq, ui_exec_done);
			//PRINTF("Sbc Packet: %d, %d, %d %d %d \n",                   SbcPacketsDecoded, SbcAudioSamplesGeneratedPerCh, SbcFrmSizeReadIn, SbcFrmSizeToBeProcessed, i_bytes_consumed);
			//PRINTF("Sbc Packet: %d, %d, %d \n",                         SbcPacketsDecoded, SbcAudioSamplesGeneratedPerCh, SbcFrmSizeReadIn);
			//PRINTF("Sbc Packet: %d, %d, %d %d %d \n",                   SbcPacketsDecoded, SbcAudioSamplesGeneratedPerCh, SbcFrmSizeReadIn, TotalAudioDataBytesGenerated, TotalSbcDataBytesConsumed);
			DbgPin8Up();

			SbcPacketsDecoded++;

			/* Do till the process execution is done */
		}
	}

	if(SbcFileIdx==0xffff)
	{
		//this is a2dp sbc stream
		CadenceSrc_UpdateDrifting(&SRC_DecoderSbc, ((T_CircularAudioBuf_S8 *)&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw)->LengthInSamples - AodSbc, 0);
	}else
	{

	}

	DbgPin8Dn();
	return 0;
}

void DeInitSbcDecoder(void)
{
    /* Free allocated memory. */
    for(int i = 0; i < g_w_malloc_count; i++)
	{
		if(g_pv_arr_alloc_memory[i])
			free(g_pv_arr_alloc_memory[i]);
	}
    g_w_malloc_count=0;
}
void DeInitSbcDecoderForOneSbcFile(void)
{
	DeinitCadenceAsrc(&SRC_DecoderSbc);
}


