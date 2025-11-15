
#include <xtensa/config/core.h>
#include <xtensa/xos.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fsl_debug_console.h"

//#define INPUT_PING_PONG
//#define OUTPUT_PING_PONG
//#define SCRATCH_PING_PONG

//#define __PROFILE__

#define ENABLE_OGG_SUPPORT


#ifdef SCRATCH_ANALYSIS
int scratch_max_size = 0;
int scratch_curr_size = 0;
int scratch_beg = 0;
int scratch_lnew_size = 0;
int scratch_new_size = 0;
#endif

#include "GlobalDef.h"

/* Tensilica specific includes. */
#include "xa_opus_codec_api.h"
#ifdef ENABLE_OGG_SUPPORT
#include "xa_ogg_lib_api.h"
#include "opus_header.h"
#include "xa_profiler.h"
#endif
#include "xa_type_def.h"
#include "xa_error_handler.h"


//#include "TestCode1.h"
//#include "AudioProcess.h"
#include "CircularBufManagement.h"
#include "SRCProc.h"
#include "DspMainAudioFlow.h"


#ifdef ENABLE_OGG_SUPPORT
typedef enum 
{
    XA_RAW_OPUS_STREAM = 0,
    XA_OGG_OPUS_STREAM = 1
} xa_opus_stream_type_t;
#endif

#ifdef SCRATCH_PING_PONG
#define SCRATCH_TEST(p_scratch_test, pattern) \
    WORD32 nm; \
    pUWORD8 pnt = (pUWORD8)p_scratch_test; \
    for(nm = 0; nm < scratch_size; nm++) \
    { \
        if(pnt[nm] != pattern) \
        { \
            scratch_ping_pong_flag = 1; \
            break; \
        } \
    } \
    if(scratch_ping_pong_flag == 1) \
        break;
#endif

#ifdef __XTENSA_EB__
#define _SYSTEM_IS_BIG_ENDIAN
#endif

#define Opus_memmove(a, b, c)  memmove((a), (b), (c))    /* Dest, Src, ByteCount */

//#define OPUS_RAND(seed)                   ((WORD64)(((WORD64)907633515) +  (WORD32)(seed * 196314165)))


#ifdef CSTUB_HIFI2
#undef __PROFILE__
#endif

#ifdef __PROFILE__
#include <xtensa/tie/xt_hifi2.h>
#include <sys/times.h>
#include <xtensa/sim.h>
/* Following variable shows the frame number that will be profiled */
/* Set profile_frame = 0 to profile all frames */
int profile_frame = 0;//1;//2822;//5488;
#endif

#ifdef __XCC__
#include <xtensa/hal.h>
#endif

#define XA_MAX_CMD_LINE_LENGTH 1024
#define XA_MAX_ARGS 50
#define PARAMFILE "paramfilesimple_decode.txt"

/* Application and testbench error handler */
#define XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED                0xFFFF8000
#define XA_TESTBENCH_FATAL_FILE_OPEN_FAILED                0xFFFF8001
#define XA_TESTBENCH_FATAL_OPUS_HDR_READ_FAILED            0xFFFF8002

#ifdef ENABLE_OGG_SUPPORT
#define OGG_INP_SIZE    4*1024
#endif

/* These are codec library error*/
VOID xa_opus_codec_error_handler_init();
extern  xa_error_info_struct xa_opus_codec_error_info;
#ifdef ENABLE_OGG_SUPPORT
VOID xa_ogg_lib_error_handler_init();
extern  xa_error_info_struct xa_ogg_lib_error_info;
#endif

/* These are testbench error*/
VOID xa_testbench_error_handler_init();
extern xa_error_info_struct xa_testbench_error_info;
int OpusDecoderIsRunning;
#if 1		//folding

#define OpusOutputCirBuf_LRMixed_LengthInSample		(80*48)		//80ms
T_CircularAudioBuf_S32 OpusOutputCirBuf_LRMixed;
int OpusOutputCirBuf_LRMixed_DataArea[OpusOutputCirBuf_LRMixed_LengthInSample+AudioFrameSizeInSamplePerCh];				//40ms + read size = AudioFrameSizeInSamplePerCh

/* Function to convert a little endian int16 to a */
/* big endian int16 or vica verca                 */
void swap_endian(
    WORD16       vec[],
    WORD         len
)
{
    WORD i;
    WORD16 tmp;
    UWORD8 *p1, *p2;

    for( i = 0; i < len; i++ ){
        tmp = vec[ i ];
        p1 = (UWORD8 *)&vec[ i ]; p2 = (UWORD8 *)&tmp;
        p1[ 0 ] = p2[ 1 ]; p1[ 1 ] = p2[ 0 ];
    }
}

#ifndef _SYSTEM_IS_BIG_ENDIAN
static WORD32 word16_swap(WORD32 data)
{
    return (data << 16)+ ((data >> 16) & 0xFFFF);
}
#endif
/*
static void print_usage( char* argv[] )
{
    printf( "Usage: %s -fs:<sampling rate (8000/12000/16000/24000/48000) Hz; default: 48000> -numch:<channels (1-8); default: 2> "
        "[options] -ifile:<input_file> -ofile:<output_file>\n\n", argv[0]);
    printf( "Options:\n" );
    printf( "-inbandfec:<fec_flag (0/1)>  : enable or disable SILK inband FEC (non-zero value is treated as 1)\n" );
    printf( "-loss:<perc>                 : simulate packet loss, in percent (0-100); default: 0\n" );
#ifdef ENABLE_OGG_SUPPORT
    printf( "-btype:<ogg/raw>             : bitstream type, ogg or raw; default: raw\n" );
    printf( "-maxpage:<n [1,1024]>        : max page size - n in kBytes; default: 64\n");
#endif
    printf( "-strmap:<map>                : Output channel map, string of max. 8 characters, range for each character 0-8\n" );
    printf( "                             : 0-7 to route the corresponding channel from decoder output, 8 for silence,\n" );
    printf( "                             : for streams with less than 8 channels (n) range becomes 0-(n-1)\n" );
}
*/
#ifdef ENABLE_OGG_SUPPORT
#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                           ((buf[base+2]<<16)&0xff0000)| \
                           ((buf[base+1]<<8)&0xff00)| \
                           (buf[base]&0xff))


#ifdef EnableSimuPrint
static void print_comments(char *comments, int length)
{
   char *c=comments;
   int len, i, nb_fields, err=0;

   if (length<(8+4+4))
   {
      fprintf (stderr, "Invalid/corrupted comments\n");
      return;
   }
   if (strncmp((char *)c, "OpusTags", 8) != 0)
   {
      fprintf (stderr, "Invalid/corrupted comments\n");
      return;
   }
   c += 8;
   fprintf(stderr, "Encoded with ");
   len=readint(c, 0);
   c+=4;
   if (len < 0 || len>(length-16))
   {
      fprintf (stderr, "Invalid/corrupted comments\n");
      return;
   }
   err&=fwrite(c, 1, len, stderr)!=(unsigned)len;
   c+=len;
   fprintf (stderr, "\n");
   /*The -16 check above makes sure we can read this.*/
   nb_fields=readint(c, 0);
   c+=4;
   length-=16+len;
   if (nb_fields < 0 || nb_fields>(length>>2))
   {
      fprintf (stderr, "Invalid/corrupted comments\n");
      return;
   }
   for (i=0;i<nb_fields;i++)
   {
      if (length<4)
      {
         fprintf (stderr, "Invalid/corrupted comments\n");
         return;
      }
      len=readint(c, 0);
      c+=4;
      length-=4;
      if (len < 0 || len>length)
      {
         fprintf (stderr, "Invalid/corrupted comments\n");
         return;
      }
      err&=fwrite(c, 1, len, stderr)!=(unsigned)len;
      c+=len;
      length-=len;
      fprintf (stderr, "\n");
   }
}
#endif
#ifdef EnableOpusDecodingPrint
static void print_comments_UsbCom(char *comments, int length)
{
   char *c=comments;
   int len, i, nb_fields;

   if (length<(8+4+4))
   {
	  PRINTF("Invalid/corrupted comments\n");
      return;
   }
   if (strncmp((char *)c, "OpusTags", 8) != 0)
   {
	   PRINTF("Invalid/corrupted comments\n");
      return;
   }
   c += 8;
   PRINTF("Encoded with ");
   len=readint(c, 0);
   c+=4;
   if (len < 0 || len>(length-16))
   {
	   PRINTF("Invalid/corrupted comments\n");
      return;
   }
   //err&=fwrite(c, 1, len, stderr)!=(unsigned)len;
   DbgConsole_SendData((unsigned char *)c, len);
   c+=len;
   PRINTF("\n");
   /*The -16 check above makes sure we can read this.*/
   nb_fields=readint(c, 0);
   c+=4;
   length-=16+len;
   if (nb_fields < 0 || nb_fields>(length>>2))
   {
	   PRINTF("Invalid/corrupted comments\n");
      return;
   }
   for (i=0;i<nb_fields;i++)
   {
      if (length<4)
      {
    	  PRINTF("Invalid/corrupted comments\n");
         return;
      }
      len=readint(c, 0);
      c+=4;
      length-=4;
      if (len < 0 || len>length)
      {
    	  PRINTF("Invalid/corrupted comments\n");
         return;
      }
      //err&=fwrite(c, 1, len, stderr)!=(unsigned)len;
      DbgConsole_SendData((unsigned char *)c, len);
      c+=len;
      length-=len;
      PRINTF("\n");
   }
   PRINTF("\n");
}
#endif

#endif


#ifdef WIN32
#define ALIGN(x) _declspec(align(x))
#else
#define ALIGN(x) __attribute__((aligned(x)))
#endif

#endif		//folding


unsigned char *OpusFileBegPtr;
unsigned char *OpusFileEndPtr;
unsigned char *OpusFileCurrentPtr;

#if 1
size_t counter = 0;
WORD32 args = 0, OpusPacketsDecoded = 0, i = 0;
WORD16 len = 0, tot_len = 0;
WORD32 nBytes = 0;
pWORD8 payload = NULL;
pWORD8 payloadEnd = NULL, payloadToDec = NULL;
WORD16 nBytesPerPacket[ XA_OPUS_MAX_PACKET_IN_INP_BUF ];
pWORD16 outPtr= NULL;
char speechOutFileName[ 1024 ] = "NULL", bitInFileName[ 1024 ] = "NULL";
FILE *bitInFile= NULL, *speechOutFile=NULL;
WORD32 frames = 0, frame = 0, lost;

/* default settings */
WORD32 packetLoss_perc = 0;

static WORD32 handle_size = 0, scratch_size = 0, enc_speech_size = 0, synth_speech_size = 0;
xa_codec_handle_t speech_decoder_state = NULL;
pVOID scratch = NULL;
#ifdef SCRATCH_PING_PONG
pVOID scratch_ping = NULL, scratch_pong = NULL;
WORD32 scratch_ping_pong_flag = 0;
#endif
pWORD16 synth_speech = NULL;
#ifdef OUTPUT_PING_PONG
pWORD16 synth_speech_ping = NULL, synth_speech_pong = NULL;
#endif
pUWORD8 enc_speech = NULL;
#ifdef INPUT_PING_PONG
pUWORD8 enc_speech_ping = NULL, enc_speech_pong = NULL;
#endif
xa_opus_dec_control_t dec_control;
xa_error_info_struct *p_proc_codec_err_info;
xa_error_info_struct *p_proc_testbench_err_info;
XA_ERRORCODE error_code = XA_NO_ERROR;
#ifdef ENABLE_OGG_SUPPORT
xa_error_info_struct *p_proc_ogg_err_info;
xa_opus_stream_type_t stream_type;
xa_codec_handle_t ogg_handle = NULL;
xa_ogg_parse_init_cfg_t ogg_cfg;
pVOID ogg_scratch = NULL;
WORD32 read_ogg_inp = 1;
WORD32 ogg_handle_size = 0, ogg_scratch_size = 0;
pUWORD8 p_ogg_inp_buf = NULL;
/* Temporary buffer for processing header and comment packets from ogg stream, size should be at least max of header and comment packet size */
UWORD8 tmp_ogg_out_buf[3*1024];
WORD32 input_over = 0;
OpusHeader opus_hdr;
WORD32 gran_offset = 0;
WORD32 preskip = 0, tmp_skip = 0;
WORD16 tot_samps = 0;
WORD64 page_gran_pos[2] = {0};
WORD64 max_out = 0;
WORD64 link_out = 0;
xa_ogg_parse_param_id_t param_id;
WORD32 eos;
XA_ERRORCODE cfg_err;
xa_profiler ogg_prof;
#endif
//UWORD32 seed = 0;
WORD32 stream_map_set = 0;

int OpusFrmSizeInSamplesPerCh;
int OpusFrmSizeInBytes;


#ifdef __PROFILE__
double u_seconds;
struct tms start, stop;
clock_t cycles;
double mcps_peak = 0, mcps_curr = 0, mcps_ave = 0, mcps_sum = 0;
int peak_frame = 0;
#endif
#endif

void InitOpusDecoderVarToDefault(void)
{

	counter = 0;
	args = 0;OpusPacketsDecoded = 0;i = 0;
	len = 0; tot_len = 0;
	nBytes = 0;
	payload = NULL;
	payloadEnd = NULL; payloadToDec = NULL;

	outPtr= NULL;
	bitInFile= NULL; speechOutFile=NULL;

	frames = 0;frame = 0;

	/* default settings */
	packetLoss_perc = 0;

	handle_size = 0; scratch_size = 0; enc_speech_size = 0; synth_speech_size = 0;
	speech_decoder_state = NULL;
	scratch = NULL;
	#ifdef SCRATCH_PING_PONG
	pVOID scratch_ping = NULL, scratch_pong = NULL;
	WORD32 scratch_ping_pong_flag = 0;
	#endif
	synth_speech = NULL;
	#ifdef OUTPUT_PING_PONG
	pWORD16 synth_speech_ping = NULL, synth_speech_pong = NULL;
	#endif
	enc_speech = NULL;
	#ifdef INPUT_PING_PONG
	pUWORD8 enc_speech_ping = NULL, enc_speech_pong = NULL;
	#endif
	error_code = XA_NO_ERROR;
	#ifdef ENABLE_OGG_SUPPORT
	ogg_handle = NULL;
	ogg_scratch = NULL;
	read_ogg_inp = 1;
	ogg_handle_size = 0; ogg_scratch_size = 0;
	p_ogg_inp_buf = NULL;
	/* Temporary buffer for processing header and comment packets from ogg stream, size should be at least max of header and comment packet size */
	input_over = 0;
	gran_offset = 0;
	preskip = 0; tmp_skip = 0;
	tot_samps = 0;
	page_gran_pos[0] = 0;
	page_gran_pos[1] = 0;
	max_out = 0;
	link_out = 0;
	#endif
	//seed = 0;
	stream_map_set = 0;

	#ifdef __PROFILE__
	mcps_peak = 0; mcps_curr = 0; mcps_ave = 0; mcps_sum = 0;
	peak_frame = 0;
	#endif
}

int freadFromFlashBin(unsigned char *DstPtr, int SizeInByte, unsigned char **FileCurPtr, unsigned char *FileEndPtr)
{
	if(SizeInByte)
	{
		int RemainlingFileLengthInByte= FileEndPtr - *FileCurPtr;
		if(SizeInByte>RemainlingFileLengthInByte)
			SizeInByte=RemainlingFileLengthInByte;
		memcpy(DstPtr,*FileCurPtr,SizeInByte);
		*FileCurPtr+=SizeInByte;
	}
	return SizeInByte;
}
int InitOpusDecoder(void)
{
	OpusDecoderIsRunning=1;
	InitOpusDecoderVarToDefault();

	/* ******************************************************************/
	/* Initialize the error handler testbench                           */
	/* ******************************************************************/
	p_proc_testbench_err_info = NULL;
	p_proc_testbench_err_info = (xa_error_info_struct*)&xa_testbench_error_info;
	xa_testbench_error_handler_init();

	/* ******************************************************************/
	/* Initialize the error handler library                             */
	/* ******************************************************************/
	p_proc_codec_err_info = NULL;
	p_proc_codec_err_info = (xa_error_info_struct*)&xa_opus_codec_error_info;
	xa_opus_codec_error_handler_init();
#ifdef  ENABLE_OGG_SUPPORT
	p_proc_ogg_err_info = NULL;
	p_proc_ogg_err_info = (xa_error_info_struct*)&xa_ogg_lib_error_info;
	xa_ogg_lib_error_handler_init();
	INIT_XA_PROFILER(ogg_prof, (char *) "OGG-PARSE");
#endif

	/* ******************************************************************/
	/* Library Info print.                                              */
	/* ******************************************************************/

	#ifdef EnableSimuPrint
		fprintf(stderr, "\n");
		fprintf(stderr, "%s library version %s\n", xa_opus_get_lib_name_string(), xa_opus_get_lib_version_string());
		fprintf(stderr, "API version: %s\n", xa_opus_get_lib_api_version_string());
		fprintf(stderr, "Cadence Design Systems, Inc. http://www.cadence.com\n");
		fprintf(stderr, "\n");
	#endif
	#ifdef EnableOpusDecodingPrint
		PRINTF("\n");
		PRINTF("%s library version %s\n", xa_opus_get_lib_name_string(), xa_opus_get_lib_version_string());
		PRINTF("API version: %s\n", xa_opus_get_lib_api_version_string());
		PRINTF("Cadence Design Systems, Inc. http://www.cadence.com\n");
		PRINTF("\n");
	#endif


	/************************************************/
	/* Reset config and control structures          */
	/************************************************/
	memset( &dec_control, 0, sizeof(dec_control) );
	memset( dec_control.stream_map, 255, sizeof(dec_control.stream_map) );
#ifdef ENABLE_OGG_SUPPORT
	memset(&ogg_cfg, 0, sizeof(xa_ogg_parse_init_cfg_t));
	memset(&opus_hdr, 0, sizeof(OpusHeader));
	/* Default stream type - raw */
	//stream_type = XA_RAW_OPUS_STREAM;
	/* Default Ogg page size */
	ogg_cfg.max_page_size = 64;      /* in KB */
#endif

	stream_type = XA_OGG_OPUS_STREAM;
	dec_control.no_range_dec_state = 1;    /* There is no losing of packet at start time */

	dec_control.lostFlag = 0;

	/************************************************************
	*               Get Memory sizes from Ogg Parser            *
	************************************************************/
#ifdef ENABLE_OGG_SUPPORT
	if(stream_type == XA_OGG_OPUS_STREAM)
	{
		ogg_handle_size = xa_ogg_parse_get_handle_byte_size(&ogg_cfg);
		ogg_scratch_size = xa_ogg_parse_get_scratch_byte_size(&ogg_cfg);
	}

	if(stream_type == XA_OGG_OPUS_STREAM)
	{
		#ifdef EnableSimuPrint
			fprintf(stdout, "\nOgg Parser runtime memory usage:");
			fprintf(stdout, "\nPersistent state size: %6d bytes\n", ogg_handle_size);
			fprintf(stdout, "Scratch buffer size:   %6d bytes\n", ogg_scratch_size);
		#endif
		#ifdef EnableOpusDecodingPrint
			PRINTF("\nOgg Parser runtime memory usage:");
			PRINTF("\nPersistent state size: %6d bytes\n", ogg_handle_size);
			PRINTF("Scratch buffer size:   %6d bytes\n", ogg_scratch_size);
		#endif
	}
#endif

	/************************************************************
	*             Allocate Memory for Ogg Parser                *
	************************************************************/
#ifdef ENABLE_OGG_SUPPORT
	if(stream_type == XA_OGG_OPUS_STREAM)
	{
		ogg_handle = (xa_codec_handle_t)malloc(ogg_handle_size);
		if(ogg_handle == NULL)
		{
			_XA_HANDLE_ERROR(p_proc_testbench_err_info, "API state alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
		}else
		{
			#ifdef EnableOpusDecodingPrint
				PRINTF(" Memory region address %p\n",(unsigned int)ogg_handle);
			#endif
		}
		if(ogg_scratch_size != 0)
		{
			ogg_scratch = (pVOID)malloc(ogg_scratch_size);
			if(ogg_scratch == NULL)
			{
				_XA_HANDLE_ERROR(p_proc_testbench_err_info, "API state alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
			}else
			{
				#ifdef EnableOpusDecodingPrint
					PRINTF(" Memory region address %p\n",(unsigned int)ogg_scratch);
				#endif
			}
		}
		p_ogg_inp_buf = (pUWORD8)malloc(OGG_INP_SIZE);
		if(p_ogg_inp_buf == NULL)
		{
			_XA_HANDLE_ERROR(p_proc_testbench_err_info, "API state alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
		}else
		{
			#ifdef EnableOpusDecodingPrint
				PRINTF(" Memory region address %p\n",(unsigned int)p_ogg_inp_buf);
			#endif
		}
	}
#endif
	return 0;
}

int InitOpusDecoderForOneOpusFile(int OpusFileIdx)
//int xa_opus_dec_main_process( int argc, char* argv[] )
{

	OpusFileBegPtr=(unsigned char *)(PtrVarBlockSharedByDspAndMcu->FileAddrTable_Opus[2*OpusFileIdx+0]);
	OpusFileEndPtr=(unsigned char *)(PtrVarBlockSharedByDspAndMcu->FileAddrTable_Opus[2*OpusFileIdx+1]);
	OpusFileCurrentPtr=OpusFileBegPtr;

    dec_control.API_sampleRate = 0;
    dec_control.API_numChannels = 0;
    /*******************************************************************
     *                  Initialization of Ogg Parser                   *
     *******************************************************************/
#ifdef ENABLE_OGG_SUPPORT
    if(stream_type == XA_OGG_OPUS_STREAM)
    {
        error_code = xa_ogg_parse_init(ogg_handle, ogg_scratch, &ogg_cfg);
        _XA_HANDLE_ERROR(p_proc_ogg_err_info, "", error_code);
    }
#endif

#ifdef ENABLE_OGG_SUPPORT
    if(stream_type == XA_OGG_OPUS_STREAM)
    {
        WORD32 packet_count = 0;
        read_ogg_inp = 1;
        while(1)
        {
            nBytes = sizeof(tmp_ogg_out_buf);
            counter = 0;
            if(read_ogg_inp && !input_over)
            {
                //counter = fread(p_ogg_inp_buf, sizeof(WORD8), OGG_INP_SIZE, bitInFile);
                counter = freadFromFlashBin(p_ogg_inp_buf, sizeof(WORD8)*OGG_INP_SIZE, &OpusFileCurrentPtr, OpusFileEndPtr);
                read_ogg_inp = 0;
                if(counter < OGG_INP_SIZE)
                {
                    input_over = 1;
                }
            }
            error_code = xa_ogg_parse_process(ogg_handle,
                                              p_ogg_inp_buf,
                                              (pUWORD8)tmp_ogg_out_buf,
                                              (pWORD32)&counter,
                                              &nBytes);
            _XA_HANDLE_ERROR(p_proc_ogg_err_info, "", error_code);

            if(error_code == XA_OGG_EXECUTE_NONFATAL_INSUFFICIENT_DATA)
                read_ogg_inp = 1;

            if(nBytes > 0)
            {
                if(packet_count == 0)
                {
                    /* parse header */
                    if (!opus_header_parse((pUWORD8)tmp_ogg_out_buf, nBytes, &opus_hdr))
                    {
                        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "", XA_TESTBENCH_FATAL_OPUS_HDR_READ_FAILED);
                    }
                    /* If sample rate is not passed on command line, pick input_sample_rate from ogg stream header */
                    if(dec_control.API_sampleRate == 0)
                        dec_control.API_sampleRate = opus_hdr.input_sample_rate;
                    if(dec_control.API_numChannels == 0)
                        dec_control.API_numChannels = opus_hdr.channels;
                    dec_control.nb_streams         = opus_hdr.nb_streams;
                    dec_control.nb_coupled         = opus_hdr.nb_coupled;
                    dec_control.gain               = opus_hdr.gain;
                    dec_control.version            = opus_hdr.version;
                    /* Channel mapping 1 implies multistream input, 0 implies single stream input */
                    dec_control.channel_mapping    = opus_hdr.channel_mapping;
                    /* Set stream_map from opus header if not passed on command line */
                    if(stream_map_set == 0)
                        memcpy(dec_control.stream_map, opus_hdr.stream_map, 8);

                    gran_offset                    = opus_hdr.preskip;
                    preskip                        = opus_hdr.preskip;
                }
                else if(packet_count == 1)
                {
					#ifdef EnableSimuPrint
						fprintf(stdout, "\nOgg Stream Comments: \n");
						/* Print comments */
						print_comments((char *)tmp_ogg_out_buf, nBytes);
					#endif
					#ifdef EnableOpusDecodingPrint
						PRINTF("\nOgg Stream Comments: \n");
						/* Print comments */
						print_comments_UsbCom((char *)tmp_ogg_out_buf, nBytes);
					#endif
                    break;
                }
                packet_count++;
            }
            if(input_over && read_ogg_inp)
            {
                /* File ended, break */
				#ifdef EnableSimuPrint
					printf( "Error: Ran out of input without finding header and comment packets");
				#endif
				#ifdef EnableOpusDecodingPrint
					PRINTF( "Error: Ran out of input without finding header and comment packets");
				#endif
                exit(0);
            }
        }
    }
    else
#endif /* #ifdef ENABLE_OGG_SUPPORT */
    {
        /* If stream type is XA_RAW_OPUS_STREAM and sample rate and number of channels are not passed from command line revert to default */
        if(dec_control.API_sampleRate == 0)
            dec_control.API_sampleRate = 48000;     /* Default for raw opus stream */
        if(dec_control.API_numChannels == 0)
            dec_control.API_numChannels = 2;        /* Default for raw opus stream */
    }
    /* Channel mapping 0 (default) implies single stream input, nb_streams and nb_coupled should be set according to API_numChannels */
    if(dec_control.channel_mapping == 0)
    {
        dec_control.nb_streams = 1;
        if(dec_control.API_numChannels == 2)
            dec_control.nb_coupled = 1;
        else
            dec_control.nb_coupled = 0;
    }

	#ifdef EnableSimuPrint
		printf( "API sampling rate:                             %d Hz\n",  dec_control.API_sampleRate );
		printf( "API number of channels:                        %d\n",     dec_control.API_numChannels );
	#endif
	#ifdef EnableOpusDecodingPrint
		PRINTF( "API sampling rate:                             %d Hz\n",  dec_control.API_sampleRate );
		PRINTF( "API number of channels:                        %d\n",     dec_control.API_numChannels );
	#endif

    /************************************************************
    *            Get Memory sizes from Opus library             *
    ************************************************************/
    handle_size = xa_opus_dec_get_handle_byte_size(dec_control.nb_streams, dec_control.nb_coupled);
    scratch_size = xa_opus_dec_get_scratch_byte_size(dec_control.channel_mapping);
#ifdef SCRATCH_ANALYSIS
    /* Add extra scratch size to be safe */
    scratch_size += 30*1024;
#endif
    enc_speech_size = XA_OPUS_MAX_DEC_INP_BYTES*dec_control.nb_streams;
    synth_speech_size = XA_OPUS_MAX_BYTES_CHANNEL_PACKET * dec_control.API_numChannels;


	#ifdef EnableSimuPrint
		fprintf(stdout, "\nOpus Decoder runtime memory usage:");
		fprintf(stdout, "\nPersistent state size: %6d bytes\n", handle_size);
		fprintf(stdout, "Scratch buffer size:   %6d bytes\n", scratch_size);
		fprintf(stdout, "Input buffer size:     %6d bytes\n", enc_speech_size);
		fprintf(stdout, "Output buffer size:    %6d bytes\n\n", synth_speech_size);
	#endif
	#ifdef EnableOpusDecodingPrint
		PRINTF("\nOpus Decoder runtime memory usage:");
		PRINTF("\nPersistent state size: %6d bytes\n", handle_size);
		PRINTF("Scratch buffer size:   %6d bytes\n", scratch_size);
		PRINTF("Input buffer size:     %6d bytes\n", enc_speech_size);
		PRINTF("Output buffer size:    %6d bytes\n\n", synth_speech_size);
	#endif


    /************************************************************
    *                     Allocate Memory                       *
    ************************************************************/
    speech_decoder_state = (xa_codec_handle_t)malloc(handle_size);
    if(speech_decoder_state == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API state alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
	}else
	{
		#ifdef EnableOpusDecodingPrint
			PRINTF(" Memory region address %p\n",(unsigned int)speech_decoder_state);
		#endif
	}

#ifndef SCRATCH_PING_PONG
    scratch = (pVOID)malloc(scratch_size);
    if(scratch == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API scratch alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
	}else
	{
		#ifdef EnableOpusDecodingPrint
			PRINTF(" Memory region address %p\n",(unsigned int)scratch);
		#endif
	}
    memset(scratch,0x7f,scratch_size);
#else
    scratch_ping = (pVOID)malloc(scratch_size);
    if(scratch_ping == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API scratch alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
    }
    memset(scratch_ping,0xcb,scratch_size);
    scratch_pong = (pVOID)malloc(scratch_size);
    if(scratch_pong == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API scratch alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
    }
    memset(scratch_pong,0xcb,scratch_size);
    scratch = scratch_ping;
#endif

#ifndef INPUT_PING_PONG
    enc_speech = (pUWORD8)malloc(enc_speech_size);
    if(enc_speech == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API input alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
	}else
	{
		#ifdef EnableOpusDecodingPrint
			PRINTF(" Memory region address %p\n",(unsigned int)enc_speech);
		#endif
	}
#else
    enc_speech_ping = (pUWORD8)malloc(enc_speech_size);
    if(enc_speech_ping == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API input alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
    }
    /* To avoid ferrets while switching buffers for first time */
    memset(enc_speech_ping, 0xce, enc_speech_size);
    enc_speech_pong = (pUWORD8)malloc(enc_speech_size);
    if(enc_speech_pong == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API input alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
    }
    enc_speech = enc_speech_ping;
#endif

#ifndef OUTPUT_PING_PONG
    synth_speech = (pWORD16)malloc(synth_speech_size);
    if(synth_speech == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API output alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
	}else
	{
		#ifdef EnableOpusDecodingPrint
			PRINTF(" Memory region address %p\n",(unsigned int)synth_speech);
		#endif
	}
#else
    synth_speech_ping = (pWORD16)malloc(synth_speech_size);
    if(synth_speech_ping == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API output alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
    }
    synth_speech_pong = (pWORD16)malloc(synth_speech_size);
    if(synth_speech_pong == NULL) 
    {
        _XA_HANDLE_ERROR(p_proc_testbench_err_info, "API output alloc", XA_TESTBENCH_FATAL_MEM_ALLOC_FAILED);
    }
    synth_speech = synth_speech_ping;
#endif

#ifdef SCRATCH_ANALYSIS
    scratch_beg = (int)scratch;
#endif

    /*******************************************************************
     *                  Initialization of Opus Decoder                 *
     *******************************************************************/
    error_code = xa_opus_dec_init(speech_decoder_state, &dec_control);
    _XA_HANDLE_ERROR(p_proc_codec_err_info, "", error_code);

    /* Set i/p buffer pointer for opus decoder */
    OpusPacketsDecoded = 0;
    payloadEnd = payload = (pWORD8)(enc_speech);

    /* Initialized array with -1 */
    for( i = 0; i < XA_OPUS_MAX_PACKET_IN_INP_BUF; i++ ) 
    {
        nBytesPerPacket[i] = -1;
    }

    /* set output buffer pointer for opus decoder */
    outPtr = synth_speech;

#ifdef ENABLE_OGG_SUPPORT
    /* Put one packet in input buffer to opus decoder */
    if(stream_type == XA_OGG_OPUS_STREAM)
    {
        read_ogg_inp = 0;
        while(1)
        {
            nBytes = enc_speech_size;
            counter = 0;
            if(read_ogg_inp && !input_over)
            {
                //counter = fread(p_ogg_inp_buf, sizeof(WORD8), OGG_INP_SIZE, bitInFile);
                counter = freadFromFlashBin(p_ogg_inp_buf, sizeof(WORD8)*OGG_INP_SIZE, &OpusFileCurrentPtr, OpusFileEndPtr);
                read_ogg_inp = 0;
                if(counter < OGG_INP_SIZE)
                {
                    input_over = 1;
                }
            }
            error_code = xa_ogg_parse_process(ogg_handle,
                                              p_ogg_inp_buf,
                                              (pUWORD8)payloadEnd,
                                              (pWORD32)&counter,
                                              &nBytes);
            _XA_HANDLE_ERROR(p_proc_ogg_err_info, "", error_code);

            if(error_code == XA_OGG_EXECUTE_NONFATAL_INSUFFICIENT_DATA)
                read_ogg_inp = 1;

            if(nBytes > 0)
            {
                /* Got one packet, break */
                break;
            }
            if(input_over && read_ogg_inp) 
            {
                /* File ended, exit */
				#ifdef EnableSimuPrint
					printf( "Error: Ran out of input, no opus packet found \n");
				#endif
				#ifdef EnableOpusDecodingPrint
					PRINTF( "Error: Ran out of input, no opus packet found \n");
				#endif
				return 0;
            }
        }
        //seed = OPUS_RAND(seed);
        if (nBytes == 0)// || (packetLoss_perc>0 && seed%100 < (UWORD32)packetLoss_perc))
        {
            nBytesPerPacket[ 0 ] = 0;
        }
        else
        {
            nBytesPerPacket[ 0 ] = nBytes;
            payloadEnd          += nBytes;
        }
        param_id.type = XA_OGG_PARSE_PARAM_PAGE_GRANULE;
        cfg_err = xa_ogg_parse_get_param(ogg_handle, param_id, &page_gran_pos[0]);
        _XA_HANDLE_ERROR(p_proc_ogg_err_info, "", cfg_err);
    }
    else
#endif /* #ifdef ENABLE_OGG_SUPPORT */
    {
    }

	//init SRC
	//             (ptr to handle.     int InputBlockSizeInSamples,            int inFs,                            int outFs,             int ChNum,         alloc_memory ptr array,              ptr numbers,      int NeedToDisplay)
	InitCadenceAsrc(&DecoderOpus_handle, DecoderOpus_SrcInSizeInSamples,dec_control.API_sampleRate,   PtrVarBlockSharedByDspAndMcu->I2SFs_Loc,  2, (void **)&g_pv_arr_alloc_memory_DecoderOpus, &g_w_malloc_DecoderOpus,         1    );
	//                                   DecoderOpus_SrcInSizeInSamples is to reserve enough space for output, later the input block size in samples will be set again in the src processing

    return 0;
}

//return 0: good and should continue
//return 1: finished, or fail, should stop
#define ProcessOneFrameOpusOnly		0
int OpusDecodeProcess(void)
//int xa_opus_dec_main_process( int argc, char* argv[] )
{
	#if ProcessOneFrameOpusOnly==1
	while( 1 )
	{
	#else
	for(int ii=0;ii<2;ii++)	//to run the decoding twice to generate 40ms
	{
		int FreeAod;

		xos_mutex_lock(&g_audio_OpusDecoderMutex);
			FreeAod=CirAudioBuf_SpaceAvailableInSamples_S32(&OpusOutputCirBuf_LRMixed);
		xos_mutex_unlock(&g_audio_OpusDecoderMutex);

		if(FreeAod >= 20*48)
		//if(FreeAod >= 20*16)
	#endif
		{
			/* Set Input Pointer */
			payloadEnd = (pWORD8)enc_speech + (payloadEnd-payload);
			payload = (pWORD8)(enc_speech);

	#ifdef ENABLE_OGG_SUPPORT
			if(stream_type == XA_OGG_OPUS_STREAM)
			{
				read_ogg_inp = 0;
				while(1)
				{
					nBytes = enc_speech_size;
					counter = 0;
					if(read_ogg_inp && !input_over)
					{
						//counter = fread(p_ogg_inp_buf, sizeof(WORD8), OGG_INP_SIZE, bitInFile);
						counter = freadFromFlashBin(p_ogg_inp_buf, sizeof(WORD8)*OGG_INP_SIZE, &OpusFileCurrentPtr, OpusFileEndPtr);
						OpusFrmSizeInBytes=counter;

						read_ogg_inp = 0;
						if( counter < OGG_INP_SIZE )
						{
							input_over = 1;
						}
					}
					START_TIME_XA_PROFILER(ogg_prof);
					error_code = xa_ogg_parse_process(ogg_handle,
													p_ogg_inp_buf,
													(pUWORD8)payloadEnd,
													(pWORD32)&counter,
													&nBytes);
					STOP_TIME_XA_PROFILER(ogg_prof);
					_XA_HANDLE_ERROR(p_proc_ogg_err_info, "", error_code);

					if(error_code == XA_OGG_EXECUTE_NONFATAL_INSUFFICIENT_DATA)
					{
						if(input_over == 1)
						{
							break;
						}
						read_ogg_inp = 1;
					}

					if(nBytes > 0)
					{
						/* Got one packet, break */
						break;
					}

					param_id.type = XA_OGG_PARSE_PARAM_END_OF_STREAM;
					cfg_err = xa_ogg_parse_get_param(ogg_handle, param_id, &eos);
					_XA_HANDLE_ERROR(p_proc_ogg_err_info, "", cfg_err);
					if (eos)
					{
						#ifdef EnableSimuPrint
							fprintf(stderr, "End of stream reached\n");
						#endif
						#ifdef EnableOpusDecodingPrint
							PRINTF("End of stream reached\n");
						#endif
					}
				}
				if(error_code == XA_OGG_EXECUTE_NONFATAL_INSUFFICIENT_DATA && input_over == 1)
				{
					OpusDecoderIsRunning=2;
					PRINTF("Finish\n");
					#if ProcessOneFrameOpusOnly==1
						break;
					#else
						return(1);
					#endif
				}
				/* Simulate losses */
				//seed = OPUS_RAND(seed);
				if (nBytes == 0)// || (packetLoss_perc>0 && seed%100 < (UWORD32)packetLoss_perc))
				{
					PRINTF("NEVER_B\n");
					nBytesPerPacket[ 1 ] = 0;
				}
				else
				{
					nBytesPerPacket[ 1 ] = nBytes;
					payloadEnd          += nBytes;
				}
				param_id.type = XA_OGG_PARSE_PARAM_PAGE_GRANULE;
				cfg_err = xa_ogg_parse_get_param(ogg_handle, param_id, &page_gran_pos[1]);
				_XA_HANDLE_ERROR(p_proc_ogg_err_info, "", cfg_err);
			}
	#endif /* #ifdef ENABLE_OGG_SUPPORT */

			if( nBytesPerPacket[ 0 ] == 0 )
			{
				PRINTF("NEVER_A\n");
				lost = 1;
				nBytes = nBytesPerPacket[ 1 ];
			}
			else
			{
				lost = 0;
				nBytes = nBytesPerPacket[ 0 ];
			}

			payloadToDec = payload;

			/* Opus decoder */
			outPtr = synth_speech;
			tot_len = 0;

			/* No Loss: Decode all frames in the packet */
			frames = 0;

			#if ProcessOneFrameOpusOnly==1
				do
				{
			#endif

				PRINTF("Opus Packet: %d, %d, %d \n", OpusPacketsDecoded, OpusFrmSizeInSamplesPerCh, OpusFrmSizeInBytes);
				OpusFrmSizeInBytes=0;

				/* Decode frame */
				dec_control.lostFlag = 0; // Indicate no-loss frame

				error_code = xa_opus_dec( speech_decoder_state,
										  (pUWORD8)payloadToDec,
										  outPtr, nBytes, &dec_control, &len, scratch );
				#if 0
					//fill the circular buffer of OPUS decoder
					signed short tmpInterpolationBuf[20*16*2];
					if(dec_control.API_sampleRate==48000)
					{
						if(dec_control.API_numChannels==1)
						{
							//mono
							//simple decimation 1/3
							for(int i=0;i<20*16;i++)
							{
								tmpInterpolationBuf[2*i+0]=outPtr[3*i];
								tmpInterpolationBuf[2*i+1]=outPtr[3*i];
							}
						}else
						{
							//stereo
							//simple decimation 1/3
							for(int i=0;i<20*16;i++)
							{
								tmpInterpolationBuf[2*i+0]=outPtr[6*i + 0];
								tmpInterpolationBuf[2*i+1]=outPtr[6*i + 1];
							}
						}
						OpusFrmSizeInSamplesPerCh=20*48;
					}
					if(dec_control.API_sampleRate==16000)
					{
						if(dec_control.API_numChannels==1)
						{
							//mono
							//no decimation
							for(int i=0;i<20*16;i++)
							{
								tmpInterpolationBuf[2*i+0]=outPtr[i];
								tmpInterpolationBuf[2*i+1]=outPtr[i];
							}
						}else
						{
							//stereo
							//no decimation
							for(int i=0;i<20*16;i++)
							{
								tmpInterpolationBuf[2*i+0]=outPtr[2*i + 0];
								tmpInterpolationBuf[2*i+1]=outPtr[2*i + 1];
							}
						}
						OpusFrmSizeInSamplesPerCh=20*16;
					}
					xos_mutex_lock(&g_audio_OpusDecoderMutex);
						CirAudioBuf_WriteSamples_S32(&OpusOutputCirBuf_LRMixed, 20*16, (S32 *)tmpInterpolationBuf);
					xos_mutex_unlock(&g_audio_OpusDecoderMutex);
				#else
					S16 *TmpSrcPtr=(S16 *)outPtr;
					int OutSampleNum;

					if(dec_control.API_sampleRate==48000)
					{
						OpusFrmSizeInSamplesPerCh=20*48;
					}

					if(dec_control.API_sampleRate==44100)			//??? OPUS 44100 is not used in this project. but what if OPUS 44100 is used? --- still 20ms, 882 samples?
					{
						OpusFrmSizeInSamplesPerCh=882;
					}

					if(dec_control.API_sampleRate==16000)
					{
						OpusFrmSizeInSamplesPerCh=20*16;
					}

					int SrcSampleLeftUnProcessed=OpusFrmSizeInSamplesPerCh;
					while(SrcSampleLeftUnProcessed)
					{
						int SamplesToProcessInThisLoop;
						if(SrcSampleLeftUnProcessed>=(DecoderOpus_SrcInSizeInSamples/2))
						{
							SamplesToProcessInThisLoop=DecoderOpus_SrcInSizeInSamples/2;
						}else
						{
							SamplesToProcessInThisLoop=SrcSampleLeftUnProcessed;
						}

						//process amount of SamplesToProcessInThisLoop
						#if 1	//folding
							if(dec_control.API_numChannels == 2)
							{
								//stereo
								for(int i=0;i<SamplesToProcessInThisLoop;i++)
								{
									SrcIn_2S32Mixed[2*i+0]=((*TmpSrcPtr++)<<16);
									SrcIn_2S32Mixed[2*i+1]=((*TmpSrcPtr++)<<16);
								}
							}else
							{
								//mono
								for(int i=0;i<SamplesToProcessInThisLoop;i++)
								{
									SrcIn_2S32Mixed[2*i+0]=((*TmpSrcPtr  )<<16);
									SrcIn_2S32Mixed[2*i+1]=((*TmpSrcPtr++)<<16);
								}
							}
							//     (xa_codec_handle_t *xa_process_handle, int *AudioS32DstPtr,  int *AudioS32SrcPtr,         int InSampleNum,    int *OutputSampleNum)
							ProcCadenceAsrc(       &DecoderOpus_handle,     SrcOut_2S32Mixed,    SrcIn_2S32Mixed,      SamplesToProcessInThisLoop,    &OutSampleNum  );

							//convert 32bit LRMixed buffer to 16Bit LRMixed buffer
							S16 *TmpDstPtr=(S16 *)SrcIn_2S32Mixed;
							for(int i=0;i<OutSampleNum;i++)
							{
								*TmpDstPtr++=(SrcOut_2S32Mixed[2*i+0]>>16);
								*TmpDstPtr++=(SrcOut_2S32Mixed[2*i+1]>>16);
							}

							//use SrcIn_2S32Mixed as the S16 mixed LR channel as input buffer for the cir buffer
							xos_mutex_lock(&g_audio_OpusDecoderMutex);
								CirAudioBuf_WriteSamples_S32(&OpusOutputCirBuf_LRMixed, OutSampleNum, SrcIn_2S32Mixed);
							xos_mutex_unlock(&g_audio_OpusDecoderMutex);
						#endif


						SrcSampleLeftUnProcessed-=SamplesToProcessInThisLoop;
					}

				#endif

				_XA_HANDLE_ERROR(p_proc_codec_err_info, "", error_code);
				frame++;
				frames++;
				outPtr  += len;
				tot_len += len;
				if( frames > XA_OPUS_MAX_FRAMES_PER_PACKET )
				{
					/* Hack for corrupt stream that could generate too many frames */
					outPtr  = synth_speech;
					tot_len = 0;
					frames  = 0;
				}


			#if ProcessOneFrameOpusOnly==1
					/* Until last 20 ms frame of packet has been decoded */
				} while( dec_control.moreInternalDecoderFrames );
			#endif

				OpusPacketsDecoded++;

			/* Update buffers */
			Opus_memmove( payload, &payload[ nBytesPerPacket[ 0 ] ], nBytesPerPacket[ 1 ] * sizeof( UWORD8 ) );
			payloadEnd -= nBytesPerPacket[ 0 ];
			nBytesPerPacket[ 0 ] = nBytesPerPacket[ 1 ];
		}
	}
    return 0;
}

void DeInitOpusDecoder(void)
{
    /* Free allocated memory. */
	if(speech_decoder_state!=NULL)
		free(speech_decoder_state);
#ifndef SCRATCH_PING_PONG
	if(scratch!=NULL)
		free(scratch);
#else
	if(scratch_ping!=NULL)
		free(scratch_ping);
	if(scratch_pong!=NULL)
		free(scratch_pong);
#endif
#ifndef INPUT_PING_PONG
	if(enc_speech!=NULL)
		free(enc_speech);
#else
	if(enc_speech_ping!=NULL)
		free(enc_speech_ping);
	if(enc_speech_pong!=NULL)
		free(enc_speech_pong);
#endif
#ifndef OUTPUT_PING_PONG
	if(synth_speech!=NULL)
		free(synth_speech);
#else
	if(synth_speech_ping!=NULL)
		free(synth_speech_ping);
	if(synth_speech_pong!=NULL)
		free(synth_speech_pong);
#endif
}

void DeInitOpusDecoderForOneOpusFile(void)
{
#ifdef ENABLE_OGG_SUPPORT
    if(stream_type == XA_OGG_OPUS_STREAM)
    {
        SUMMARY_XA_PROFILER(ogg_prof);

    	if(ogg_handle!=NULL)
    		free(ogg_handle);

        if (ogg_scratch_size)
        	if(ogg_scratch!=NULL)
        		free(ogg_scratch);

    	if(p_ogg_inp_buf!=NULL)
    		free(p_ogg_inp_buf);
    }
#endif
	DeinitCadenceAsrc((void **)&g_pv_arr_alloc_memory_DecoderOpus, &g_w_malloc_DecoderOpus);
	OpusDecoderIsRunning=0;
}

void InitOpusOutputCirBuf(void)
{
	InitCirAudioBuf_S32(&OpusOutputCirBuf_LRMixed,OpusOutputCirBuf_LRMixed_DataArea,OpusOutputCirBuf_LRMixed_LengthInSample);
	memset(OpusOutputCirBuf_LRMixed_DataArea,0,sizeof(OpusOutputCirBuf_LRMixed_DataArea));
}
