/****************************************************************************************/
/*  Copyright 2004-2026 NXP                                                             */
/*                                                                                      */
/*  NXP Confidential and Proprietary.                                                   */
/*  This software is owned or controlled by NXP and may only be used                    */ 
/*  strictly in accordance with the applicable license terms.                           */ 
/*  By expressly accepting such terms or by downloading, installing,                    */ 
/*  activating and/or otherwise using the software, you are agreeing                    */ 
/*  that you have read, and that you agree to comply with and are                       */
/*  bound by, such license terms.  If you do not agree to be bound                      */
/*  by the applicable license terms, then you may not retain,                           */
/*  install, activate or otherwise use the software.                                    */
/*  Reproduction in whole or in part is prohibited without the prior                    */
/*  written consent of the copyright owner.                                             */
/*                                                                                      */
/*  This software and any compilation or derivative thereof is and                      */
/*  shall remain the proprietary information of NXP Software and is                     */
/*  highly confidential in nature. Any and all use hereof is restricted                 */
/*  and is subject to the terms and conditions set forth in the                         */
/*  software license agreement concluded with NXP Software.                             */
/*                                                                                      */
/*  Under no circumstances is this software or any derivative thereof                   */
/*  to be combined with any Open Source Software in any way or                          */
/*  licensed under any Open License Terms without the express prior                     */
/*  written  permission of NXP Software.                                                */
/*                                                                                      */
/*  For the purpose of this clause, the term Open Source Software means                 */
/*  any software that is licensed under Open License Terms. Open                        */
/*  License Terms means terms in any license that require as a                          */
/*  condition of use, modification and/or distribution of a work                        */
/*                                                                                      */
/*           1. the making available of source code or other materials                  */
/*              preferred for modification, or                                          */
/*                                                                                      */
/*           2. the granting of permission for creating derivative                      */
/*              works, or                                                               */
/*                                                                                      */
/*           3. the reproduction of certain notices or license terms                    */
/*              in derivative works or accompanying documentation, or                   */
/*                                                                                      */
/*           4. the granting of a royalty-free license to any party                     */
/*              under Intellectual Property Rights                                      */
/*                                                                                      */
/*  regarding the work and/or any work that contains, is combined with,                 */
/*  requires or otherwise is based on the work.                                         */
/*                                                                                      */
/*  This software is provided for ease of recompilation only.                           */
/*  Modification and reverse engineering of this software are strictly                  */
/*  prohibited.                                                                         */
/*                                                                                      */
/****************************************************************************************/

/****************************************************************************************/
/*                                                                                      */
/*  Header file for the Essential Audio Processor                                       */                               
/*                                                                                      */
/*  This files includes all definitions, types, structures and function                 */
/*  prototypes required by the calling layer. All other types, structures and           */
/*  functions are private.                                                              */
/*                                                                                      */        
/*  File version: 8.0.0.0                                                               */
/*                                                                                      */
/****************************************************************************************/
/*                                                                                      */
/*  Note: 1                                                                             */
/*  =======                                                                             */
/*  The algorithm can execute either with separate input and output buffers or with     */
/*  a common buffer, i.e. the data is processed in-place.                               */
/*                                                                                      */
/****************************************************************************************/
/*                                                                                      */
/*  Note: 2                                                                             */
/*  =======                                                                             */
/*  Source and Sink data formats depends of their respective format (Stereo or Mono).   */
/*  Output format can be Mono or Stereo whatever the input format.                      */
/*                                                                                      */                                        
/*  An audio stream can have 1 channel (mono) or 2 channels (stereo)                    */
/*  It is organized as follows:                                                         */
/*                                                                                      */
/*  Byte Offset         Stereo Input/Output     Mono Input/Output						*/
/*  ===========         ===================    ====================						*/
/*      0               Left Sample #1          Mono Sample #1							*/
/*      2               Right Sample #1         Mono Sample #2							*/
/*      4               Left Sample #2          Mono Sample #3							*/
/*      6               Right Sample #2         Mono Sample #4							*/
/*      .                      .                     .                                  */
/*      .                      .                     .                                  */
/*                                                                                      */
/* Details:                                                                             */
/*                                                                                      */
/* 1- If input format is Mono then output format could be Stereo but output             */
/*    audio content between the 2 channels will be very similar. No stereo effect       */
/*    will be present.                                                                  */
/*                                                                                      */    
/* 2- If input format is Stereo and output format is Mono then the 2 channels are mixed */ 
/*    together after have been processed by the EAP chain.                              */
/*                                                                                      */
/* 2- There is no MIPS consumption advantage to run EAP with input format in Mono       */
/*                                                                                      */        
/* 3- If CrossOver is enabled:		                                                    */
/*      a) Output format is Stereo              					                    */
/*          pOutData[0] will be the output Low band and pOutData[1] the High band       */	
/*              		    pOutData[0] in stereo       pOutData[1] in stereo 			*/
/*                         =====================       =====================			*/
/*                           Left Sample LB  #1           Left Sample HB  #1			*/
/*                           Right Sample LB #1           Right Sample HB #1			*/
/*                           Left Sample LB  #2           Left Sample HB  #2			*/
/*                           Right Sample LB #2           Right Sample HB #2			*/
/*                                   .                            .                     */
/*                                   .                            .                     */
/*																						*/
/*      b) Output format in Mono									                    */
/*          pOutData[0] will be the output Low band and pOutData[1] the High band       */
/*                          pOutData[0] in mono         pOutData[1] in mono 			*/
/*                         =====================       =====================			*/
/*                           MONO Sample LB  #1           MONO Sample HB  #1			*/
/*                           MONO Sample LB  #2           MONO Sample HB  #2			*/
/*                           MONO Sample LB  #3           MONO Sample HB  #3			*/
/*                           MONO Sample LB  #4           MONO Sample HB  #4			*/
/*                                   .                            .                     */
/*                                   .                            .                     */
/****************************************************************************************/
/*                                                                                      */
/*  Note: 3                                                                             */
/*  =======                                                                             */
/*  Even when EAP is turn Off the input mono/Stereo to output mono/Stereo convertion    */
/*  is still executed.                                                                  */
/*                                                                                      */
/****************************************************************************************/

#ifndef __LVM_H__
#define __LVM_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************************/
/*                                                                                      */
/*  Includes                                                                            */
/*                                                                                      */
/****************************************************************************************/
#include <stdio.h>
#include "LVC_Types.h"

/****************************************************************************************/
/*                                                                                      */
/*  Definitions - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR                           */
/*   Library has been built with following define and values are exposed here           */
/*   only for integration information purpose                                           */
/*                                                                                      */
/****************************************************************************************/

/* Audio Streams  - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR  */
#define LVM_MIN_STREAMS                  1       // Minimum number of audio streams
#define LVM_MAX_STREAMS                  3       // Maximum number of audio streams
#define LVM_MAX_NUM_CHANNELS             2       // Maximum number of interleaved input channels in a stream

/* percentage vlue MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR  */
#define LVM_PERCENT_MIN				     0		 // min value of a percentage value
#define LVM_PERCENT_MAX				   100       // max value of a percentage value

/* block size MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_HEADROOM_MAX_NBANDS               5     // Headroom management */
#define MAX_INTERNAL_BLOCKSIZE             1024     // Maximum internal block size authorized (multiple of 64)
#define LVM_MANAGED_MAX_MAXBLOCKSIZE       1024     // Maximum MaxBlockSzie Limit for Managed Buffer Mode
#define LVM_UNMANAGED_MAX_MAXBLOCKSIZE     1024     // Maximum MaxBlockSzie Limit for Unmanaged Buffer Mode

#define MIN_INTERNAL_BLOCKSIZE_DEFAULT           16  // Minimum internal block size by default
#define MIN_INTERNAL_BLOCKSHIFT_DEFAULT           4  // Minimum internal block size as a power of 2 by default
#define MIN_INTERNAL_BLOCKMASK_DEFAULT       0xFFF0  // Minimum internal block size mask by default
// ALGORITHM_AVL
#define MIN_INTERNAL_BLOCKSIZE_AVL_PRESENT       64  // Minimum internal block size if AVL is compiled in the lib
#define MIN_INTERNAL_BLOCKSHIFT_AVL_PRESENT       6  // Minimum internal block size as a power of 2 if AVL is compiled in the lib
#define MIN_INTERNAL_BLOCKMASK_AVL_PRESENT   0xFFC0  // Minimum internal block size mask if AVL is compiled in the lib
// ALGORITHM_PB
#define MIN_INTERNAL_BLOCKSIZE_PB_PRESENT        64  // Minimum internal block size if PB is compiled in the lib
#define MIN_INTERNAL_BLOCKSHIFT_PB_PRESENT        6  // Minimum internal block size as a power of 2 if PB is compiled in the lib
#define MIN_INTERNAL_BLOCKMASK_PB_PRESENT    0xFFC0  // Minimum internal block size mask if PB is compiled in the lib

/* Mixer MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_MIXER_MIN_TIMESLOPE        0        // Minimum duration of a gain transition time slope (in milliseconds)
#define LVM_MIXER_MAX_TIMESLOPE    20000        // Maximum duration of a gain transition time slope (in milliseconds)
#define LVM_MIXER_VOLUME_MAX       32767        // Maximum settable volume for a mixer stream in linear Q0D15 (0 dB)
#define LVM_MIXER_VOLUME_MIN           0        // Minimum settable volume for a mixer stream in linear Q0D15 (-inf dB)

/* Power spectrum analysis MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_PSA_MAX_NUMBANDS           64       // Maximum Number of PSA Bands
#define LVM_PSA_MIN_NUMBANDS           6        // Minimum Number of PSA Bands
#define LVM_PSA_MIN_HISTORY            200      // Minimum PSA History in ms
#define LVM_PSA_MAX_HISTORY            5000     // Maximum PSA History in ms
#define LVM_PSA_MIN_UPDATERATE         10       // Minimum PSA Update rate
#define LVM_PSA_MAX_UPDATERATE         25       // Maximum PSA Update rate

/* Tone Generator MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_TG_MIN_FREQUENCY           20       // TG Minimum Sweep Frequency
#define LVM_TG_MAX_FREQUENCY           24000    // TG Maximum Sweep Frequency
#define LVM_TG_MIN_AMPLITUDE           -96      // TG Minimum Sweep signal Amplitude
#define LVM_TG_MIX_AMPLITUDE           0        // TG Maximum Sweep signal Amplitude

/* Loudness maximizer MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_LM_MAX_ATTENUATION         6        // LM Maximum Attanuation
#define LVM_LM_MAX_COMPRESSOR_GAIN     6        // LM Maximum compressor gain
#define LVM_LM_MIN_SPEAKER_CUTOFF      150      // LM Minimum Speaker Cutoff frequency
#define LVM_LM_MAX_SPEAKER_CUTOFF      1100     // LM Maximum Speaker Cutoff frequency
#define LVM_LM_MIN_COMPRESSOR_KNEE     -96      // LM Minimum output Sample value limit
#define LVM_LM_MAX_COMPRESSOR_KNEE     0       // LM Maximum output Sample value limit

/* Treeble enhancement MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_TE_LOW_MIPS            32767        // Treble enhancement 6dB Mips saving mode 
#define LVM_TE_MIN_EFFECTLEVEL         0        //TE Minimum EffectLevel
#define LVM_TE_MAX_EFFECTLEVEL         15       //TE Maximum Effect level

/* Digital output gain MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */ 
#define LVM_DG_MIN_GAIN_OUTPUT         -96      //DG Minimum Gain
#define LVM_DG_MAX_GAIN_OUTPUT         20       //DG Maximal Gain

/* Bass enhancer MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */ 
#define LVM_BE_MIN_EFFECTLEVEL         0        //BE Minimum EffectLevel
#define LVM_BE_MAX_EFFECTLEVEL         15       //BE Maximum Effect level

/* IIR multiband EQ MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */ 
#define LVM_EQNB_MAX_BANDS_NBR         10        // EQNB Maximal band number 
#define LVM_EQNB_MIN_BAND_FREQ         20        // EQNB Minimum Band Frequency
#define LVM_EQNB_MAX_BAND_FREQ         24000     // EQNB Maximum Band Frequency
#define LVM_EQNB_MIN_BAND_GAIN         -15       // EQNB Minimum Band Frequency
#define LVM_EQNB_MAX_BAND_GAIN         15        // EQNB Maximum Band Frequency
#define LVM_EQNB_MIN_QFACTOR           25        // EQNB Minimum Q Factor
#define LVM_EQNB_MAX_QFACTOR           1200      // EQNB Maximum Q Factor
#define LVM_EQNB_MIN_LPF_FREQ          1000      // EQNB Minimum Low Pass Corner frequency
#define LVM_EQNB_MIN_HPF_FREQ          20        // EQNB Minimum High Pass Corner frequency
#define LVM_EQNB_MAX_HPF_FREQ          1000      // EQNB Maximum High Pass Corner frequency
#define LVM_PR_EQNB_MAX_BAND_GAIN       3        // PRODUCT EQNB Maximum Band Frequency, not 15 dB to avoid saturation 

/* FIR  - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */ 
#define LVM_FIR_MAX_NCOEFFS            256       // Maximum supported FIR size (in number of coefficients) 

/* Volume control MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */ 
#define LVM_VC_MIN_EFFECTLEVEL         -96      // VC Minimum EffectLevel
#define LVM_VC_MAX_EFFECTLEVEL           0      // VC Maximum Effect level
#define LVM_VC_BALANCE_MAX              96      // VC balance max value
#define LVM_VC_BALANCE_MIN             -96      // VC balance min value

/* Limiter MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_LIM_THRESHOLD_MIN		   -24		 // min value in dBFs 
#define LVM_LIM_THRESHOLD_MAX		   0         // max value in dBFs 

/* Virtualizer and Cinema sound experience MIN MAX - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_VIRTUALIZER_MAX_REVERB_LEVEL  LVM_PERCENT_MAX  //Vitrualizer Maximum Reverb Level
#define LVM_CS_MIN_EFFECT_LEVEL        0	     //CS Minimum Effect Level
#define LVM_CS_MAX_EFFECT_LEVEL        32767	 //CS Maximal Effect Level
#define LVM_CS_MIDGAIN_MINMAX            10
#define LVM_CS_SIDEGAIN_MIN               0
#define LVM_CS_SIDEGAIN_MAX              15
#define LVM_CS_FREQ_MIN                  20
#define LVM_CS_FREQ_MAX               24000

/* Algorithm masks - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR */
#define LVM_NONE_MASK                  0
#define LVM_CS_MASK                    (LVM_INT32) (1)                     // 0x00001
#define LVM_EQNB_MASK                  (LVM_INT32)(LVM_CS_MASK      << 1)
#define LVM_DBE_MASK                   (LVM_INT32)(LVM_EQNB_MASK    << 1)
#define LVM_PB_MASK                    (LVM_INT32)(LVM_DBE_MASK     << 1)
#define LVM_VC_MASK                    (LVM_INT32)(LVM_PB_MASK      << 1)  // 0x00010
#define LVM_VCS_MASK                   (LVM_INT32)(LVM_VC_MASK      << 1)   
#define LVM_TE_MASK                    (LVM_INT32)(LVM_VCS_MASK     << 1)
#define LVM_LM_MASK                    (LVM_INT32)(LVM_TE_MASK      << 1)
#define LVM_AVL_MASK                   (LVM_INT32)(LVM_LM_MASK      << 1)  // 0x00100
#define LVM_TG_MASK                    (LVM_INT32)(LVM_AVL_MASK     << 1)
#define LVM_PSA_MASK                   (LVM_INT32)(LVM_TG_MASK      << 1)
#define LVM_LIMP_MASK                  (LVM_INT32)(LVM_PSA_MASK     << 1)
#define LVM_LIMR_MASK                  (LVM_INT32)(LVM_LIMP_MASK    << 1)  // 0x01000
#define LVM_XO_MASK                    (LVM_INT32)(LVM_LIMR_MASK    << 1)
#define LVM_PR_EQNB_MASK			   (LVM_INT32)(LVM_XO_MASK      << 1)
#define LVM_MIXER_MASK                 (LVM_INT32)(LVM_PR_EQNB_MASK << 1) 
#define LVM_PR_EQFIR_MASK              (LVM_INT32)(LVM_MIXER_MASK   << 1)  // 0x10000
#define LVM_MBLM_MASK                  (LVM_INT32)(LVM_PR_EQFIR_MASK<< 1)
#define LVM_ALL_MASK                   (LVM_INT32)((LVM_MBLM_MASK   << 1) - 1) 

/* Flexible algorithm ID - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR * /
 *  Pre fixed algorithm :  (Mixer) - (Channel Handle) - (Tone Generator ) - (LIMiter Rms Analysis)
 *
 *  Flex order algorithm, following order example:
 *                         Concert Sound - PRE Volume Control - user EQualizer N Band - Digital Bass Enhancement -
                           Automatic Volume Leveler - Loudness Maximiser - POST Volume Control - LIMiter Peak -
                           Volume Control Single - PRoduct EQualizer N Band - PRoduct equalizer FIR - Treeble Enhancement - LIMiter Rms
 *
 *  Post fixed algorithm:  (PSA) - (Gain Out) - (XO)
*/
#define LVM_NONE_FLEX_ID               0
#define LVM_CS_FLEX_ID                 (LVM_INT32) (1)                          // 0x0001
#define LVM_EQNB_FLEX_ID               (LVM_INT32)(LVM_CS_FLEX_ID      << 1)
#define LVM_DBE_FLEX_ID                (LVM_INT32)(LVM_EQNB_FLEX_ID    << 1)
#define LVM_PB_FLEX_ID                 (LVM_INT32)(LVM_DBE_FLEX_ID     << 1)
#define LVM_PRE_VC_FLEX_ID             (LVM_INT32)(LVM_PB_FLEX_ID      << 1)    // 0x0010
#define LVM_POST_VC_FLEX_ID            (LVM_INT32)(LVM_PRE_VC_FLEX_ID  << 1)
#define LVM_VCS_FLEX_ID                (LVM_INT32)(LVM_POST_VC_FLEX_ID << 1)
#define LVM_TE_FLEX_ID                 (LVM_INT32)(LVM_VCS_FLEX_ID     << 1)
#define LVM_LM_FLEX_ID                 (LVM_INT32)(LVM_TE_FLEX_ID      << 1)    // 0x0100
#define LVM_AVL_FLEX_ID                (LVM_INT32)(LVM_LM_FLEX_ID      << 1)
#define LVM_LIMP_FLEX_ID               (LVM_INT32)(LVM_AVL_FLEX_ID     << 1)
#define LVM_LIMR_FLEX_ID               (LVM_INT32)(LVM_LIMP_FLEX_ID    << 1)
#define LVM_PR_EQNB_FLEX_ID			   (LVM_INT32)(LVM_LIMR_FLEX_ID    << 1)    // 0x1000
#define LVM_PR_EQFIR_FLEX_ID           (LVM_INT32)(LVM_PR_EQNB_FLEX_ID << 1)
#define LVM_MBLM_FLEX_ID               (LVM_INT32)(LVM_PR_EQFIR_FLEX_ID<< 1)
#define LVM_STOP_FLEX_ID               (LVM_INT32)(LVM_MBLM_FLEX_ID    << 1)    // 0x8000
#define LVM_MAX_FLEX_ID                (16)                                     // maximum number of algo which can be re-ordered by parameter (called flex algorithm)

#define LVM_BUILD_STRUCT_CHECK_VALUE  0xA5A50123

/****************************************************************************************/
/*                                                                                      */
/*  Definitions                                                                         */
/*                                                                                      */
/****************************************************************************************/

/* Concert Sound effect level presets */
// ALGORITHM_VIRTUALIZER = ALGORITHM_CS
#define LVM_CS_EFFECT_NONE                    0     // 0% effect, minimum value
#define LVM_CS_EFFECT_LOW                 16384     // 50% effect
#define LVM_CS_EFFECT_MED                 24576     // 75% effect
#define LVM_CS_EFFECT_HIGH                32767     // 100% effect, maximum value

/* Bass enhancement effect level presets */
// ALGORITHM_BASS => ALGORITHM_DBE or ALGORITHM_PB
#define LVM_BE_0DB                            0     // 0dB boost, no effect
#define LVM_BE_3DB                            3     // +3dB boost
#define LVM_BE_6DB                            6     // +6dB boost
#define LVM_BE_9DB                            9     // +9dB boost
#define LVM_BE_12DB                          12     // +12dB boost
#define LVM_BE_15DB                          15     // +15dB boost

/****************************************************************************************/
/*                                                                                      */
/*  Types - NOT ALLOWED TO BE CHANGED BY THE INTEGRATOR                                 */
/*                                                                                      */
/****************************************************************************************/

/* Instance handle */
typedef void *LVM_Handle_t;


/* Status return values */
typedef enum
{
    LVM_SUCCESS                             = 0,   // Successful return from a routine
    LVM_ALIGNMENTERROR                      = 1,   // Input data or output data pointer memory alignment error. The input and output data buffers must be 32-bit aligned.
    LVM_NULLADDRESS                         = 2,   // NULL allocation address
    LVM_INVALIDNUMSAMPLES                   = 3,   // Could also be named Invalid Block Size
												   // Invalid number of samples :
                                                   //     - MIN_INTERNAL_BLOCKMASK / MAX_INTERNAL_BLOCKSIZE range must be respected.
                                                   //     - In LVM_UNMANAGED_BUFFERS mode, block size modulo must be respected according the create mask:
                                                   //               modulo 64 for LVM_PB_MASK LVM_AVL_MASK
                                                   //               modulo  4 for LVM_CS_MASK LVM_DBE_MASK LVM_EQNB_MASK LVM_TE_MASK LVM_VC_MASK
                                                   //               modulo  2 for LVM_LM_MASK LVM_MBLM_MASK
												   //	  - Instance parameter .MaxBlockSize must be < to MAX_INTERNAL_BLOCKSIZE
												   //     - Instance parameter .MaxBlockSize is forced to MIN_INTERNAL_BLOCKSIZE if < to MIN_INTERNAL_BLOCKSIZE
												   //     - Instance parameter .MaxBlockSize is forced to a multiple of MIN_INTERNAL_BLOCKSIZE
    LVM_WRONGAUDIOTIME                      = 4,   // Wrong time value for audio time when get spectrum is used
    LVM_ALGORITHMDISABLED                   = 5,   // Dedicated function is used (ex: LVM_GetSpectrum) but the associated algorithm is disabled (ex: PSA_Enable)
    LVM_NOT_INITIALIZED                     = 6,   // Process function was called with a non-initialized module. Call LVM_SetControlParameters before call process function
	LVM_INVALIDNXPPLATFORM                  = 7,   // Invalid platform is used. Check Platform instance parameter is matching the platform.
    LVM_INVALIDMASK                         = 8,   // built in library doesn't include the required algorithm define in CreateMask instance parameter
                                                   // define at library build
    LVM_INVALIDBUILDSTRUCTALIGN             = 9,   // built in library structure is different than user structure
                                                   // pre-processor define are different that the library buil (check ALGORITHM_XX define)
    LVM_OUTOFRANGE_FLEX_ORDER               = 10,  // Flex order is not supported:
                                                   //     - LVM_STOP_FLEX_ID is missing
                                                   //     - Mismatch between FLEX_ID list and create mask. Not possible to set a FLEX_ID of a algorithm not set in the create mask at GetInstance.

    LVM_SUBALGOERROR						= 97,  // Internal algorithm error witch are not converted to a dedicated EAP error
	LVM_INVALIDFUNCTION                     = 98,  // Invalid function used. Invalid function called with the current EAP Library built
	LVM_OUTOFRANGE                          = 99,  // Out of range control parameter (without details)

    /* OUT OF RANGE - PARAMETERS DETAILS */
	LVM_OUTOFRANGE_GENERAL_PARAMS			= 100,
	LVM_OUTOFRANGE_SPEAKER_TYPES			= 101,
	LVM_OUTOFRANGE_VIRTUALIZER_OM			= 110,
	LVM_OUTOFRANGE_VIRTUALIZER_TYPE         = 111,
	LVM_OUTOFRANGE_VIRTUALIZER_REVERB       = 112,
	LVM_OUTOFRANGE_CS_EFFECT				= 120,
	LVM_OUTOFRANGE_USER_EQNB				= 130,
	LVM_OUTOFRANGE_USER_EQNB_BAND_DEF       = 131,
	LVM_OUTOFRANGE_PRODUCT_EQNB				= 140,
	LVM_OUTOFRANGE_PRODUCT_EQNB_BAND_DEF    = 141,
	LVM_OUTOFRANGE_BE						= 150,
	LVM_OUTOFRANGE_PB						= 160,
	LVM_OUTOFRANGE_VC_LEVEL					= 170,
	LVM_OUTOFRANGE_VC_BALANCE				= 171,
    LVM_OUTOFRANGE_VCS_LEVEL                = 180,
    LVM_OUTOFRANGE_DG_GAIN_OUTPUT           = 190,
	LVM_OUTOFRANGE_TE						= 200,
	LVM_OUTOFRANGE_LM						= 210,
	LVM_OUTOFRANGE_LM_SPEAKER_CUTOFF        = 211,
	LVM_OUTOFRANGE_AVL						= 220,
	LVM_OUTOFRANGE_TG						= 230,
	LVM_OUTOFRANGE_PSA_RATE					= 241,
	LVM_OUTOFRANGE_PSA_ENABLE				= 242,
	LVM_OUTOFRANGE_PSA_NUMBAND				= 243,
	LVM_OUTOFRANGE_LIMP_OM					= 250,
	LVM_OUTOFRANGE_LIMP_THRESHOLD			= 251,
	LVM_OUTOFRANGE_LIMR_OM					= 260,
	LVM_OUTOFRANGE_LIMR_THRESHOLD			= 261,
	LVM_OUTOFRANGE_LIMR_REFERENCE			= 262,
	LVM_OUTOFRANGE_CS_AP_MODE				= 270,
	LVM_OUTOFRANGE_CS_AP					= 271,
	LVM_OUTOFRANGE_XO_OPERATINGMODE			= 280,
	LVM_OUTOFRANGE_XO_CUTOFFFREQUENCY		= 281,
    LVM_OUTOFRANGE_MIXER                    = 290,
    LVM_OUTOFRANGE_FIR                      = 300,
    LVM_OUTOFRANGE_MBLM_OPERATINGMODE       = 310,
    LVM_OUTOFRANGE_MBLM_XO_CUTOFFFREQUENCY  = 311,
    LVM_OUTOFRANGE_MBLM_LMLB                = 312,
    LVM_OUTOFRANGE_MBLM_LMLB_SPEAKER_CUTOFF = 313,
    LVM_OUTOFRANGE_MBLM_LMHB                = 314,
    LVM_OUTOFRANGE_MBLM_LIMPLB_OM           = 315,
    LVM_OUTOFRANGE_MBLM_LIMPLB_THRESHOLD    = 316,

    /* INVALID MASK => built in library doesn't include the required algorithm define by CreateMask instance parameter */
    LVM_INVALIDMASK_CS                     = 1000,
    LVM_INVALIDMASK_USER_EQNB              = 1010,
    LVM_INVALIDMASK_PRODUCT_EQNB           = 1020,
    LVM_INVALIDMASK_DBE                    = 1030,
    LVM_INVALIDMASK_PB                     = 1040,
    LVM_INVALIDMASK_VC                     = 1050,
    LVM_INVALIDMASK_VCS                    = 1060,
    LVM_INVALIDMASK_DG_GAIN                = 1070,
    LVM_INVALIDMASK_TE                     = 1080,
    LVM_INVALIDMASK_LM                     = 1090,
    LVM_INVALIDMASK_AVL                    = 1100,
    LVM_INVALIDMASK_TG                     = 1110,
    LVM_INVALIDMASK_PSA                    = 1120,
    LVM_INVALIDMASK_LIMP                   = 1130,
    LVM_INVALIDMASK_LIMR                   = 1140,
    LVM_INVALIDMASK_XO                     = 1150,
    LVM_INVALIDMASK_MIXER                  = 1160,
    LVM_INVALIDMASK_PRODUCT_EQFIR          = 1170,
    LVM_INVALIDMASK_MBLM                   = 1180,

	/* INTERNAL ALGO FUNCTION ERROR =>  Internal algorithm error witch are not converted to a dedicated EAP error */
    LVM_SUBALGOERROR_CS                    = 1500,
    LVM_SUBALGOERROR_USER_EQNB             = 1510,
    LVM_SUBALGOERROR_PRODUCT_EQNB          = 1520,
    LVM_SUBALGOERROR_DBE                   = 1530,
    LVM_SUBALGOERROR_PB                    = 1540,
    //LVM_SUBALGOERROR_VC                    = 1550, // no dedicated error present
    //LVM_SUBALGOERROR_VCS                   = 1560, // no dedicated error present
    //LVM_SUBALGOERROR_DG_GAIN               = 1570, // no dedicated error present
    //LVM_SUBALGOERROR_TE                    = 1580, // no dedicated error present
    LVM_SUBALGOERROR_LM                    = 1590,
    LVM_SUBALGOERROR_AVL                   = 1600,
    LVM_SUBALGOERROR_TG                    = 1610,
    LVM_SUBALGOERROR_PSA                   = 1620,
    //LVM_SUBALGOERROR_LIMP                  = 1630, // no dedicated error present
    //LVM_SUBALGOERROR_LIMR                  = 1640, // no dedicated error present
    LVM_SUBALGOERROR_XO                    = 1650,
    LVM_SUBALGOERROR_MIXER                 = 1660,
    LVM_SUBALGOERROR_PRODUCT_EQFIR         = 1670,
    LVM_SUBALGOERROR_MBLM_XO               = 1680,
	LVM_SUBALGOERROR_MBLM_LMLB             = 1681,
	LVM_SUBALGOERROR_MBLM_LMHB             = 1682,
	//LVM_SUBALGOERROR_MBLM_LIMP           = 1683, // no dedicated error present

	/* INVALID FLEXORDER => Current instance doesn't include the required algorithm set by CreateMask instance parameter at EAP create instance */
    LVM_INVALIDFLEXORDER_CS                = 2000,
    LVM_INVALIDFLEXORDER_USER_EQNB         = 2010,
    LVM_INVALIDFLEXORDER_PRODUCT_EQNB      = 2020,
    LVM_INVALIDFLEXORDER_DBE               = 2030,
    LVM_INVALIDFLEXORDER_PB                = 2040,
    LVM_INVALIDFLEXORDER_PRE_VC            = 2050,
	LVM_INVALIDFLEXORDER_POST_VC           = 2060,
    LVM_INVALIDFLEXORDER_VCS               = 2070,
    LVM_INVALIDFLEXORDER_TE                = 2080,
    LVM_INVALIDFLEXORDER_LM                = 2090,
    LVM_INVALIDFLEXORDER_AVL               = 2100,
    LVM_INVALIDFLEXORDER_LIMP              = 2110,
    LVM_INVALIDFLEXORDER_LIMR              = 2120,
    LVM_INVALIDFLEXORDER_MIXER             = 2130,
    LVM_INVALIDFLEXORDER_PRODUCT_EQFIR     = 2140,
    LVM_INVALIDFLEXORDER_MBLM              = 2150,

	LVM_RETURNSTATUS_DUMMY				   = LVM_MAXENUM
} LVM_ReturnStatus_en;


/* Buffer Management mode */
typedef enum
{
    LVM_MANAGED_BUFFERS   = 0,
    LVM_UNMANAGED_BUFFERS = 1,
    LVM_BUFFERS_DUMMY     = LVM_MAXENUM
} LVM_BufferMode_en;

/* Output device type */
typedef enum
{
    LVM_HEADPHONES             = 0,
    LVM_MOBILE_SPEAKERS_SMALL  = 2,
    LVM_MOBILE_SPEAKERS_MEDIUM = 3,
    LVM_MOBILE_SPEAKERS_LARGE  = 4,
    LVM_SPEAKERTYPE_MAX        = LVM_MAXENUM
} LVM_OutputDeviceType_en;

/* platform authorized */
typedef enum
{
	LVM_IMXRT1050 = 1,						   // I.MXRT1050 : EAP running on Cortex-M7
	LVM_IMXRT1060 = 2,                         // I.MXRT1060 : EAP running on Cortex-M7
	LVM_IMXRT1064 = 3,						   // I.MXRT1064 : EAP running on Cortex-M7
	LVM_IMXRT1170 = 4,                         // I.MXRT1170 : EAP running on Cortex-M7
	LVM_LPC55     = 5,						   // LPC55	     : EAP running on Cortex-M33
	LVM_IMXRT500  = 6,                         // I.MXRT500  : EAP running on RT500
	LVM_IMXRT600  = 7,                         // I.MXRT600  : EAP running on RT600
    LVM_IMXRT700  = 8,                         // I.MXRT700  : EAP running on RT700
	LVM_MAX_PLATFORM = LVM_MAXENUM,
}EAP_NXPPlatform_en;

/* Build information */
typedef enum
{
    LVM_BUILD_DETAILS_NONE                  = 0 ,
    LVM_BUILD_DETAILS_RT500_FUSIONF1_RI2021 = ((LVM_IMXRT500 * 10) + 0),
    LVM_BUILD_DETAILS_RT500_FUSIONF1_RI2023 = ((LVM_IMXRT500 * 10) + 1),
    LVM_BUILD_DETAILS_RT600_HIFI4_RI2021    = ((LVM_IMXRT600 * 10) + 0),
    LVM_BUILD_DETAILS_RT600_HIFI4_RI2023    = ((LVM_IMXRT600 * 10) + 1),
    LVM_BUILD_DETAILS_RT700_HIFI1_RI2023    = ((LVM_IMXRT700 * 10) + 0),
    LVM_BUILD_DETAILS_RT700_HIFI4_RI2023    = ((LVM_IMXRT700 * 10) + 1),
    LVM_BUILD_DETAILS_WINDOWS_X64           = 1000,
    LVM_BUILD_DETAILS_DUMMY                 = LVM_MAXENUM,
} LVM_BUILD_DETAILS_en;

/* Virtualizer mode selection*/
// ALGORITHM_VIRTUALIZER = ALGORITHM_CS
typedef enum
{
    LVM_CONCERTSOUND       = 0,
    LVM_VIRTUALIZERTYPE_DUMMY   = LVM_MAXENUM
} LVM_VirtualizerType_en;
// endif ALGORITHM_VIRTUALIZER

/* N-Band Equaliser operating mode */
// (ALGORITHM_EQNB) || (ALGORITHM_PR_EQNB)
typedef enum
{
    LVM_EQNB_OFF   = 0,
    LVM_EQNB_ON    = 1,
    LVM_EQNB_DUMMY = LVM_MAXENUM
} LVM_EQNB_Mode_en;
// end (ALGORITHM_EQNB) || (ALGORITHM_PR_EQNB)

/* Filter mode control */
typedef enum
{
    LVM_EQNB_FILTER_OFF = 0,
    LVM_EQNB_FILTER_ON  = 1,
    LVM_EQNB_FILTER_DUMMY = LVM_MAXENUM
} LVM_EQNB_FilterMode_en;

/* Bass Enhancement operating mode */
// ALGORITHM_BASS => ALGORITHM_DBE or ALGORITHM_PB
typedef enum
{
    LVM_BE_OFF   = 0,
    LVM_BE_ON    = 1,
    LVM_BE_DUMMY = LVM_MAXENUM
} LVM_BE_Mode_en;

/* Bass Enhancement centre frequency selection control */
typedef enum
{
    LVM_BE_CENTRE_55Hz  = 0,
    LVM_BE_CENTRE_66Hz  = 1,
    LVM_BE_CENTRE_78Hz  = 2,
    LVM_BE_CENTRE_90Hz  = 3,
    LVM_BE_CENTRE_DUMMY = LVM_MAXENUM
} LVM_BE_CentreFreq_en;

/* Bass Enhancement HPF selection control */
typedef enum
{
    LVM_BE_HPF_OFF   = 0,
    LVM_BE_HPF_ON    = 1,
    LVM_BE_HPF_DUMMY = LVM_MAXENUM
} LVM_BE_FilterSelect_en;
// end ALGORITHM_BASS

/* Volume Control operating mode */
typedef enum
{
    LVM_VC_OFF   = 0,
    LVM_VC_ON    = 1,
    LVM_VC_DUMMY = LVM_MAXENUM
} LVM_VC_Mode_en;

/* Treble Enhancement operating mode */
// ALGORITHM_TE
typedef enum
{
    LVM_TE_OFF   = 0,
    LVM_TE_ON    = 1,
    LVM_TE_DUMMY = LVM_MAXENUM
} LVM_TE_Mode_en;
/* end ALGORITHM_TE */

/* FIR Operating Mode */
// ALGORITHM_PR_EQFIR
typedef enum
{
    LVM_FIR_OFF   = 0,
    LVM_FIR_ON    = 1,
    LVM_FIR_DUMMY = LVM_MAXENUM
} LVM_FIR_Mode_en;
/* FIR Coefficients Width */
typedef enum
{
    LVM_FIR_COEFF_16BIT   = 0,
    LVM_FIR_COEFF_32BIT   = 1,
    LVM_FIR_COEFF_DUMMY   = LVM_MAXENUM
} LVM_FIR_CoeffWidth_en;
// end ALGORITHM_PR_EQFIR */

/* Loudness Maximiser operating mode */
// ALGORITHM_LM
typedef enum
{
    LVM_LM_OFF   = 0,
    LVM_LM_ON    = 1,
    LVM_LM_DUMMY = LVM_MAXENUM
}LVM_LM_Mode_en;
/* Loudness Maximiser effect setting */
typedef enum
{
    LVM_LM_GENTLE       = 0,
    LVM_LM_MEDIUM       = 1,
    LVM_LM_EXTREME      = 2,
    LVM_LM_EFFECT_DUMMY = LVM_MAXENUM
}LVM_LM_Effect_en;
// end ALGORITHM_LM

/* AVL operating mode */
// ALGORITHM_AVL
typedef enum
{
    LVM_AVL_OFF   = 0,
    LVM_AVL_ON    = 1,
    LVM_AVL_DUMMY = LVM_MAXENUM
} LVM_AVL_Mode_en;
// end ALGORITHM_AVL

/* Headroom management operating mode */
typedef enum
{
    LVM_HEADROOM_OFF   = 0,
    LVM_HEADROOM_ON    = 1,
    LVM_Headroom_DUMMY = LVM_MAXENUM
} LVM_Headroom_Mode_en;

/* Tone Generator operating mode */
// ALGORITHM_TG
typedef enum
{
    LVM_TG_OFF        = 0,
    LVM_TG_CONTINUOUS = 1,
    LVM_TG_ONESHOT    = 2,
    LVM_TG_DUMMY      = LVM_MAXENUM
} LVM_TG_Mode_en;
/* Tone Generator sweep mode */
typedef enum
{
    LVM_TG_SWEEPLIN    = 0,
    LVM_TG_SWEEPLOG    = 1,
    LVM_TG_SWEEP_DUMMY = LVM_MAXENUM
} LVM_TG_SweepMode_en;
// end ALGORITHM_TG

/* Cross over operating mode */
// ALGORITHM_XO
typedef enum
{
	LVM_XO_OFF 		= 0,
	LVM_XO_ON  	    = 1,
    LVM_XO_DUMMY    = LVM_MAXENUM
}LVM_XO_Mode_en;
// end ALGORITHM_XO

/* Power spectrum parameters */
// ALGORITHM_PSA
typedef enum
{
    LVM_PSA_SPEED_SLOW   = 0,                                /* Peak decaying at slow speed */
    LVM_PSA_SPEED_MEDIUM = 1,                                /* Peak decaying at medium speed */
    LVM_PSA_SPEED_FAST   = 2,                                /* Peak decaying at fast speed */
    LVM_PSA_SPEED_DUMMY  = LVM_MAXENUM
} LVM_PSA_DecaySpeed_en;
typedef enum
{
    LVM_PSA_OFF   = 0,
    LVM_PSA_ON    = 1,
    LVM_PSA_DUMMY = LVM_MAXENUM
} LVM_PSA_Mode_en;
// end  ALGORITHM_PSA */

/* Peak limiter operating mode */
// ALGORITHM_LIMP
typedef enum
{
    LVM_LIMP_OFF   = 0,
    LVM_LIMP_ON    = 1,
    LVM_LIMP_DUMMY = LVM_MAXENUM
} LVM_LIMP_Mode_en;
// end  ALGORITHM_LIMP */

/* RMS limiter operating mode & parameters */
// ALGORITHM_LIMR
typedef enum
{
    LVM_LIMR_OFF   = 0,
    LVM_LIMR_ON    = 1,
    LVM_LIMR_DUMMY = LVM_MAXENUM
} LVM_LIMR_Mode_en;
typedef enum
{
    LVM_LIMR_REF_INPUT  = 0,
    LVM_LIMR_REF_0DBFS  = 1,
    LVM_LIMR_REF_DUMMY  = LVM_MAXENUM
} LVM_LIMR_Reference_en;
// end  ALGORITHM_LIMR */

// Adavanced parameter mode
typedef enum
{
    LVM_AP_DEFAULT  = 0,
    LVM_AP_MANUAL   = 1,
    LVM_AP_DUMMY  = LVM_MAXENUM
} LVM_AP_MODE_en;

/****************************************************************************************/
/*                                                                                      */
/*  Structures                                                                          */
/*                                                                                      */
/****************************************************************************************/

/* Version information */
typedef struct
{
    LVM_UINT8                   versionMajor;           /* Major for major update or API update */
    LVM_UINT8                   versionMedium;          /* Medium for feature update */
    LVM_UINT8                   versionMinor;           /* minor for minor and bug fix update */
    LVM_UINT32                  libAlgorithmdMask;      /* library algorithm mask at build */
    LVM_BUILD_DETAILS_en        libBuildDetails;        /* library build details if any */
} LVM_VersionInfo_st;

/* Memory table containing the region definitions */
typedef struct
{
    LVM_MemoryRegion_st         Region[LVM_NR_MEMORY_REGIONS];  /* One definition for each region */
} LVM_MemTab_t;


/* N-Band equaliser band definition */
typedef struct
{
    LVM_INT16                   Gain;                   /* Band gain in dB */
    LVM_UINT16                  Frequency;              /* Band centre frequency in Hz */
    LVM_UINT16                  QFactor;                /* Band quality factor (x100) */
} LVM_EQNB_BandDef_t;

/* Headroom band definition */
typedef struct
{
    LVM_UINT16                  Limit_Low;              /* Low frequency limit of the band in Hertz */
    LVM_UINT16                  Limit_High;             /* High frequency limit of the band in Hertz */
    LVM_INT16                   Headroom_Offset;        /* Headroom = biggest band gain - Headroom_Offset */
} LVM_HeadroomBandDef_t;

/* Mixer Stream Param Definition */
// ALGORITHM_MIXER
typedef struct
{
    LVM_INT16              TargetGain;
    LVM_INT16              TimeConstantMs;
    LVM_Callback           pCallback;
    LVM_INT16              CallbackParam;
    void* pUserData;
} LVM_MIXER_Stream_Params_t;
// end ALGORITHM_MIXER

/* Control Parameter structure */
typedef struct
{
    /*
     * General parameters
     */
    LVM_Mode_en                 OperatingMode;          /* Bundle operating mode On/Bypass */
    LVM_Fs_en                   SampleRate;             /* Sample rate */
    LVM_Format_en               SourceFormat;           /* Input data format */
    LVM_Format_en               SinkFormat;             /* Output data format */
    LVM_OutputDeviceType_en     SpeakerType;            /* Output device type */
    LVM_INT32                   *pFB_Order;             /* re-ordering flexible algorithm, pointer to flex block order table */

    // ALGORITHM_MIXER
    /* Mixer input parameters */
    LVM_Mode_en                 MIXER_OperatingMode;
    LVM_INT16                   MIXER_NumStreams;
    LVM_MIXER_Stream_Params_t*  pMIXER_StreamParams;
    // end  ALGORITHM_MIXER

    // ALGORITHM_TG
    /* Tone Generator parameters */
    LVM_TG_Mode_en              TG_OperatingMode;       /* Tone generator mode */
    LVM_TG_SweepMode_en         TG_SweepMode;           /* Log or linear sweep */
    LVM_UINT16                  TG_StartFrequency;      /* Sweep start frequency in Hz */
    LVM_INT16                   TG_StartAmplitude;      /* Sweep start amplitude in dBr */
    LVM_UINT16                  TG_StopFrequency;       /* Sweep stop frequency in Hz */
    LVM_INT16                   TG_StopAmplitude;       /* Sweep stop amplitude in dBr */
    LVM_UINT16                  TG_SweepDuration;       /* Sweep duration in seconds, 0 for infinite duration tone */
    LVM_Callback                pTG_CallBack;           /* End of sweep callback */
    LVM_INT16                   TG_CallBackID;          /* Callback ID*/
    void*						pTGAppMemSpace;        /* Application instance handle or memory area */
    // end  ALGORITHM_TG

    /* Start flexible order block parameters */

    // ALGORITHM_VIRTUALIZER => ALGORITHM_CS
    /* Concert Sound Virtualizer parameters*/
    LVM_Mode_en                 VirtualizerOperatingMode; /* Virtualizer operating mode On/Off */
    LVM_VirtualizerType_en      VirtualizerType;          /* Virtualizer type: ConcertSound, CinemaSound Music or CinemaSound Movie */
    LVM_UINT16                  VirtualizerReverbLevel;   /* Virtualizer reverb level in % */
    LVM_INT16                   CS_EffectLevel;           /* Concert Sound effect level */
    
    /* Concert sound advanced parameter */
    // ALGORITHM_CS
    LVM_AP_MODE_en				CS_AP_Mode;				  /* concert sound advanced paramameter mode */
    LVM_INT16                   CS_AP_MidGain;            /* MidChannelGain */
    LVM_UINT16                  CS_AP_MidCornerFreq;      /* Shelving Filter Corner Frequency */
    LVM_UINT16                  CS_AP_SideHighPassCutoff; /* SideBoost HighPassFilter Corner Frequency */
    LVM_UINT16                  CS_AP_SideLowPassCutoff;  /* SideBoost LowPassFilter Corner Frequency */
    LVM_INT16                   CS_AP_SideGain;           /* Side Channel Gain */
    // end ALGORITHM_CS

    // ALGORITHM_VC
    /* Volume Control parameters, split into 2 part (pre and post) + balance */
    LVM_INT16                   VC_EffectLevel;         /* Volume Control setting in dBs */
    LVM_INT16                   VC_Balance;             /* Left Right Balance control in dB (-96 to 96 dB) */
    // end ALGORITHM_VC

    // ALGORITHM_EQNB
    /* N-Band Equaliser parameters */
    LVM_EQNB_Mode_en            EQNB_OperatingMode;     /* N-Band Equaliser operating mode */
    LVM_EQNB_FilterMode_en      EQNB_LPF_Mode;          /* Low pass filter */
    LVM_INT16                   EQNB_LPF_CornerFreq;
    LVM_EQNB_FilterMode_en      EQNB_HPF_Mode;          /* High pass filter */
    LVM_INT16                   EQNB_HPF_CornerFreq;
    LVM_UINT16                  EQNB_NBands;            /* Number of bands */
    LVM_EQNB_BandDef_t*         pEQNB_BandDefinition;   /* Pointer to equaliser definitions */
    // end ALGORITHM_EQNB

    // ALGORITHM_BASS => ALGORITHM_DBE or ALGORITHM_PB
    /* Bass Enhancement parameters */
    LVM_BE_Mode_en              BE_OperatingMode;       /* Bass Enhancement operating mode */
    LVM_INT16                   BE_EffectLevel;         /* Bass Enhancement effect level */
    LVM_BE_CentreFreq_en        BE_CentreFreq;          /* Bass Enhancement centre frequency */
    LVM_BE_FilterSelect_en      BE_HPF;                 /* Bass Enhancement high pass filter selector */
    // end ALGORITHM_BASS */

    // ALGORITHM_AVL
    /* AVL parameters */
    LVM_AVL_Mode_en             AVL_OperatingMode;      /* AVL operating mode */
    // end ALGORITHM_AVL

    // ALGORITHM_MBLM
    /* Multi band Loudness Maximiser parameters */
    LVM_Mode_en					MBLM_OperatingMode;		       /* Multi band Loudness Maximiser - operating mode*/
    // Multi band Loudness Maximiser - Crossover parameters
    LVM_UINT16					MBLM_XO_cutoffFrequency;	   /* Multi band Loudness Maximiser - Crossover cut-off frequency*/
    // Multi band Loudness Maximiser - Loudness Maximiser Low band parameters
    LVM_LM_Mode_en              MBLM_LMLB_OperatingMode;       /* Multi band Loudness Maximiser - Loudness Maximiser low band operating mode */
    LVM_LM_Effect_en            MBLM_LMLB_EffectLevel;         /* Multi band Loudness Maximiser - Loudness Maximiser low band effect level */
    LVM_UINT16                  MBLM_LMLB_Attenuation;         /* Multi band Loudness Maximiser - Loudness Maximiser low band output attenuation */
    LVM_UINT16                  MBLM_LMLB_CompressorGain;      /* Multi band Loudness Maximiser - Loudness Maximiser low band output compressor gain */
    LVM_UINT16                  MBLM_LMLB_SpeakerCutOff;       /* Multi band Loudness Maximiser - Loudness Maximiser low band speaker cut off frequency */
    LVM_INT16					MBLM_LMLB_CompressorKnee;      /* Multi band Loudness Maximiser - Loudness Maximiser low band compressor knee in dB */
	// Multi band Loudness Maximiser - Low Band Peak limiter parameters
    LVM_LIMP_Mode_en			MBLM_LIMPLB_OperatingMode;	   /* Multi band Loudness Maximiser - Low band Peak limiter operating mode */
    LVM_INT16					MBLM_LIMPLB_Threshold;		   /* Multi band Loudness Maximiser - Low band Peak limiter threshold in dB */
    // Multi band Loudness Maximiser - Loudness Maximiser High band high band parameters
    LVM_LM_Mode_en              MBLM_LMHB_OperatingMode;       /* Multi band Loudness Maximiser - Loudness Maximiser high band operating mode */
    LVM_LM_Effect_en            MBLM_LMHB_EffectLevel;         /* Multi band Loudness Maximiser - Loudness Maximiser high band effect level */
    LVM_UINT16                  MBLM_LMHB_Attenuation;         /* Multi band Loudness Maximiser - Loudness Maximiser high band output attenuation */
    LVM_UINT16                  MBLM_LMHB_CompressorGain;      /* Multi band Loudness Maximiser - Loudness Maximiser high band output compressor gain */
    LVM_INT16					MBLM_LMHB_CompressorKnee;      /* Multi band Loudness Maximiser - Loudness Maximiser high band compressor knee in dB */
    // end ALGORITHM_MBLM

    // ALGORITHM_LM
    /* Loudness Maximiser parameters */
    LVM_LM_Mode_en              LM_OperatingMode;       /* Loudness Maximiser operating mode */
    LVM_LM_Effect_en            LM_EffectLevel;         /* Loudness Maximiser effect level */
    LVM_UINT16                  LM_Attenuation;         /* Loudness Maximiser output attenuation */
    LVM_UINT16                  LM_CompressorGain;      /* Loudness Maximiser output compressor gain */
    LVM_UINT16                  LM_SpeakerCutOff;       /* Loudness Maximiser speaker cut off frequency */
    LVM_INT16					LM_CompressorKnee;      /* Loudness Maximiser compressor knee in dB */
    // end ALGORITHM_LM

    // ALGORITHM_LIMP
    LVM_LIMP_Mode_en			LIMP_OperatingMode;		/* LIMP operating mode */
    LVM_INT16					LIMP_Threshold;		    /* LIMP threshold in dB */
    // end ALGORITHM_LIMP

    // ALGORITHM_VCS
    /* Single Volume Control parameters in 1 unique step */
    LVM_INT16                   VCS_EffectLevel;       /* Single Volume Control setting in dBs */
    // end ALGORITHM_VCS

    // ALGORITHM_PR_EQNB
    /* N-Band Equaliser parameters */
    LVM_EQNB_Mode_en            PR_EQNB_OperatingMode;     /* N-Band Equaliser operating mode */
    LVM_EQNB_FilterMode_en      PR_EQNB_LPF_Mode;          /* Low pass filter */
    LVM_INT16                   PR_EQNB_LPF_CornerFreq;
	LVM_EQNB_FilterMode_en      PR_EQNB_HPF_Mode;          /* High pass filter */
    LVM_INT16                   PR_EQNB_HPF_CornerFreq;
    LVM_UINT16                  PR_EQNB_NBands;            /* Number of bands */
    LVM_EQNB_BandDef_t          *pPR_EQNB_BandDefinition;  /* Pointer to equaliser definitions */
    // end ALGORITHM_PR_EQNB

    // ALGORITHM_PR_EQFIR
    LVM_FIR_Mode_en             FIR_OperatingMode;          /* FIR Operating Mode */
    LVM_FIR_CoeffWidth_en       FIR_CoeffWidth;             /* FIR Coefficients Bit Width */
    LVM_UINT16                  FIR_NCoeffs;                /* FIR number of coefficients */
    void*                       FIR_pCoeffs;                /* FIR Pointer to coefficients */
    // end ALGORITHM_PR_EQFIR */

    // ALGORITHM_TE
    /* Treble Enhancement parameters */
    LVM_TE_Mode_en              TE_OperatingMode;           /* Treble Enhancement On/Off */
    LVM_INT16                   TE_EffectLevel;             /* Treble Enhancement gain dBs */
    // end ALGORITHM_TE

    // ALGORITHM_LIMR
    LVM_LIMR_Mode_en			LIMR_OperatingMode;		    /* LIMR operating mode */
    LVM_LIMR_Reference_en		LIMR_Reference;	 		    /* LIMR reference input */
    LVM_INT16					LIMR_Threshold;		        /* LIMR threshold in dB */
    // end ALGORITHM_LIMR

    /* End flexible order block parameters */

    /* Digital output gain parameters */
    LVM_INT16                   DG_GainOutput;              /* digital gain output in dBs from -96 to +20 dBs, 1 dB step */

    // ALGORITHM_PSA
    /* General Control */
    LVM_PSA_Mode_en             PSA_Enable;

    /* Spectrum Analyzer parameters */
    LVM_PSA_DecaySpeed_en       PSA_PeakDecayRate;          /* Peak value decay rate*/
    LVM_UINT16                  PSA_NumBands;               /* Number of Bands*/
    // end ALGORITHM_PSA

    // ALGORITHM_XO
	LVM_XO_Mode_en				XO_OperatingMode;		    /* Crossover operating mode*/
	LVM_UINT16					XO_cutoffFrequency;		    /* Crossover cut-off frequency*/
    // end ALGORITHM_XO
    
    LVM_UINT32                  BuildStructAlignCheck;      /* Structure aligned check */
} LVM_ControlParams_t;
 

/* Instance Parameter structure */
typedef struct
{
    /* General */
    LVM_BufferMode_en           BufferMode;             /* Buffer management mode */
    LVM_UINT16                  MaxBlockSize;           /* Maximum processing block size */
    LVM_INT32                   CreateMask;             /* Mask for algorithm enablemnt in EAP instance (See Algorithm Masks) */

    /* N-Band Equaliser */
    LVM_UINT16                  EQNB_NumBands;          /* Maximum number of User equaliser bands */
	LVM_UINT16                  PR_EQNB_NumBands;       /* Maximum number of Product equaliser bands */
	EAP_NXPPlatform_en			Platform;				/* NXP Platform where EAP is playing on (LVM_IMXRT1050,LVM_IMXRT1060, LVM_IMXRT1064, LVM_IMXRT1170, LVM_LPC55, LVM_IMXRT500, LVM_IMXRT600, LVM_IMXRT700) */
	
    // ALGORITHM_PSA
    /* PSA */
    LVM_UINT16                  PSA_HistorySize;         /* PSA History size in ms: 200 to 5000 */
    LVM_UINT16                  PSA_MaxBands;            /* Maximum number of bands: 6 to 64 */
    LVM_UINT16                  PSA_SpectrumUpdateRate;  /* Spectrum update rate : 10 to 25*/
    LVM_PSA_Mode_en             PSA_Included;            /* Controls the instance memory allocation for PSA: ON/OFF */
    // end ALGORITHM_PSA */

    LVM_UINT32                  BuildStructAlignCheck;   /* Structure aligned check */
} LVM_InstParams_t;


/* Headroom management parameter structure */
typedef struct
{
    LVM_Headroom_Mode_en        Headroom_OperatingMode; /* Headroom Control On/Off */
    LVM_HeadroomBandDef_t       *pHeadroomDefinition;   /* Pointer to headroom bands definition */
    LVM_UINT16                  NHeadroomBands;         /* Number of headroom bands */
} LVM_HeadroomParams_t;


/****************************************************************************************/
/*                                                                                      */
/*  Function Prototypes                                                                 */
/*                                                                                      */
/****************************************************************************************/


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_GetVersionInfo                                          */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used to retrieve information about:                                */
/*                               - the library's version                                */
/*                               - the build information                                */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  pVersion                Pointer to an empty version info structure                  */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_GetVersionInfo(LVM_VersionInfo_st  *pVersion);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_GetMemoryTable                                          */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used for memory allocation and free. It can be called in           */
/*  two ways:                                                                           */
/*                                                                                      */
/*      hInstance = NULL                Returns the memory requirements                 */
/*      hInstance = Instance handle     Returns the memory requirements and             */
/*                                      allocated base addresses for the instance       */
/*                                                                                      */
/*  When this function is called for memory allocation (hInstance=NULL) the memory      */
/*  base address pointers are NULL on return.                                           */
/*                                                                                      */
/*  When the function is called for free (hInstance = Instance Handle) the memory       */
/*  table returns the allocated memory and base addresses used during initialisation.   */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance Handle                                             */
/*  pMemoryTable            Pointer to an empty memory definition table                 */
/*  pInstParams             Pointer to the instance parameters                          */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_GetMemoryTable(LVM_Handle_t         hInstance,
                                       LVM_MemTab_t         *pMemoryTable,
                                       LVM_InstParams_t     *pInstParams);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_GetInstanceHandle                                       */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used to create a bundle instance. It returns the created instance  */
/*  handle through phInstance. All parameters are set to their default, inactive state. */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  phInstance              pointer to the instance handle                              */
/*  pMemoryTable            Pointer to the memory definition table                      */
/*  pInstParams             Pointer to the instance parameters                          */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1. This function must not be interrupted by the LVM_Process function                */
/*  2. This function must sent a valid parameter structure with BuildStructAlignCheck   */
/*     set to LVM_BUILD_STRUCT_CHECK_VALUE                                              */
/*  3. Instance parameter.MaxBlockSize is forced to MIN_INTERNAL_BLOCKSIZE if < to      */
/*     MIN_INTERNAL_BLOCKSIZE															*/
/*  4. Instance parameter .MaxBlockSize is internally forced to a multiple of			*/
/*     MIN_INTERNAL_BLOCKSIZE															*/
/*																						*/
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_GetInstanceHandle(LVM_Handle_t        *phInstance,
                                          LVM_MemTab_t        *pMemoryTable,
                                          LVM_InstParams_t    *pInstParams);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_ClearAudioBuffers                                       */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used to clear the internal audio buffers of the bundle.            */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance handle                                             */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1. This function may be interrupted by the LVM_Process function                     */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_ClearAudioBuffers(LVM_Handle_t  hInstance);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                 LVM_GetControlParameters                                   */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  Request the module parameters. The current parameter set is returned                */
/*  via the parameter pointer.                                                          */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance                Instance handle                                            */
/*  pParams                  Pointer to a valid empty parameter structure               */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*  2.  This function must sent a valid empty structure with BuildStructAlignCheck set  */
/*      to LVM_BUILD_STRUCT_CHECK_VALUE                                                 */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_GetControlParameters(LVM_Handle_t           hInstance,
                                             LVM_ControlParams_t    *pParams);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_SetControlParameters                                    */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  Sets or changes the LifeVibes module parameters.                                    */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance handle                                             */
/*  pParams                 Pointer to a valid parameter structure                      */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*  2.  This function must sent a valid parameter structure with BuildStructAlignCheck  */
/*      set to LVM_BUILD_STRUCT_CHECK_VALUE                                             */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_SetControlParameters(LVM_Handle_t           hInstance,
                                             LVM_ControlParams_t    *pParams);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_Process                                                 */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  Process function for the LifeVibes module.                                          */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance handle                                             */
/*  pInData                 Double pointer to the input data stream                     */
/*  pOutData                Double pointer to the output data stream                    */
/*  NumSamples              Number of samples per channel in the input buffer           */
/*  AudioTime               Audio Time of the current input data in milli-seconds       */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES: see header file note 1 and 2                                                  */
/*   1. The input and output buffers must be 32-bit aligned                             */
/*   2. If Crossover Disable, pOutData[0] MUST be initialize as a non-null pointer      */
/*   3. If Crossover Enable, pOutData[0] & pOutData[1] MUST be initialize as            */
/*      a non-null pointer                                                              */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_Process(LVM_Handle_t                hInstance,
                                const LVM_INT16             **pInData,
                                LVM_INT16                   **pOutData,
                                LVM_UINT16                  NumSamples,
                                LVM_UINT32                  AudioTime);


/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_GetAVLGain                                              */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used to retrieve the AVL last generated gain in Q16.15             */
/*  linear values.                                                                      */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance Handle                                             */
/*  pAVL_Gain               Pointer to the gain                                         */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
// ALGORITHM_AVL
LVM_ReturnStatus_en LVM_GetAVLGain( LVM_Handle_t    hInstance,
                                    LVM_INT32       *pAVL_Gain);
// end  ALGORITHM_AVL */

/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_SetHeadroomParams                                       */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used to set the automatic headroom management parameters.          */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance Handle                                             */
/*  pHeadroomParams         Pointer to headroom parameter structure                     */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
// ALGORITHM_EQNB
LVM_ReturnStatus_en LVM_SetHeadroomParams(  LVM_Handle_t            hInstance,
                                            LVM_HeadroomParams_t    *pHeadroomParams);

/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_GetHeadroomParams                                       */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/*  This function is used to get the automatic headroom management parameters.          */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance Handle                                             */
/*  pHeadroomParams         Pointer to headroom parameter structure (output)            */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1.  This function may be interrupted by the LVM_Process function                    */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_GetHeadroomParams(  LVM_Handle_t            hInstance,
                                            LVM_HeadroomParams_t    *pHeadroomParams);
// end ALGORITHM_EQNB

/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_GetSpectrum                                             */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/* This function is used to retrieve Spectral information at a given Audio time         */
/* for display usage                                                                    */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance Handle                                             */
/*  pCurrentPeaks           Pointer to location where currents peaks are to be saved    */
/*  pPastPeaks              Pointer to location where past peaks are to be saved        */
/*  pCentreFreqs            Pointer to location where centre frequency of each band is  */
/*                          to be saved                                                 */
/*  AudioTime               Audio time at which the spectral information is needed      */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1. This function may be interrupted by the LVM_Process function                     */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
// ALGORITHM_PSA
LVM_ReturnStatus_en LVM_GetSpectrum( LVM_Handle_t            hInstance,
                                     LVM_INT8                *pCurrentPeaks,
                                     LVM_INT8                *pPastPeaks,
                                     LVM_UINT16              *pCentreFreqs,
                                     LVM_UINT32              AudioTime);
// end  ALGORITHM_PSA */

/****************************************************************************************/
/*                                                                                      */
/* FUNCTION:                LVM_SetVolumeNoSmoothing                                    */
/*                                                                                      */
/* DESCRIPTION:                                                                         */
/* This function is used to set output volume without any smoothing                     */
/*                                                                                      */
/* PARAMETERS:                                                                          */
/*  hInstance               Instance Handle                                             */
/*  pParams                 Control Parameters, only volume value is used here          */
/*                                                                                      */
/* RETURNS: See LVM_ReturnStatus_en descritpion                                         */
/*                                                                                      */
/* NOTES:                                                                               */
/*  1. This function may be interrupted by the LVM_Process function                     */
/*                                                                                      */
/****************************************************************************************/
#ifdef __DLL_EXPORT
__declspec(dllexport)
#endif /* __DLL_EXPORT */
LVM_ReturnStatus_en LVM_SetVolumeNoSmoothing( LVM_Handle_t           hInstance,
                                              LVM_ControlParams_t    *pParams);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif      /* __LVM_H__ */

