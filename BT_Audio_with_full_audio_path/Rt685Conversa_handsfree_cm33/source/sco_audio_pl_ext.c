/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#if 1	//folding
#include "sco_audio_pl.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_adapter_audio.h"
#include "fsl_codec_common.h"
#include "fsl_codec_adapter.h"
#include "fsl_debug_console.h"
#include "ringtone.h"
#include <sys/atomic.h>


/*******************************************************************************
 * Definitions
 ******************************************************************************/


#include "GlobalDef.h"

#include "SubFunc.h"

#if EnableConversa==1

#include "fsl_mu.h"
#include "fsl_sema42.h"

#include "AudioIoCfg_I2s.h"
#include "AudioIoCfg_Pdm.h"

#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "AudioProcess.h"
#include "MainAudioFlow.h"
#include "Sweep.h"
#include "WorkStateManager.h"

#endif

#if UsingQAR87Board == 1
#include "hal_amp.h"

#endif

/* Sco loop back, data from sco input to sco output */
#define SCO_SAI_LOOPBACK (0)
/* Codec loop back data from mic to speaker */
#define CODEC_SAI_LOOPBACK (0)

#define OVER_SAMPLE_RATE        (256U)

#define BUFFER_SIZE      (1024U)
#define BUFFER_NUMBER    (4)
#define AUDIO_DUMMY_SIZE (64U)
#define HFP_STREAMER_TASK_PRIORITY (6U)

#if defined(CODEC_WM8962_ENABLE) || defined(CODEC_WM8904_ENABLE)
#define HFP_CODEC_DAC_VOLUME (75U) /* Range: 0 ~ 100 */
#elif defined(CODEC_WM8960_ENABLE)
#define HFP_CODEC_DAC_VOLUME  (100U) /* Range: 0 ~ 100 */
#else
#define HFP_CODEC_DAC_VOLUME  (100U) /* Range: 0 ~ 100 */
#endif

#define HFP_CODEC_HP_VOLUME  (70U)  /* Range: 0 ~ 100 */

/* --------------------------------------------- External Global Variables */

extern codec_config_t boardCodecScoConfig;
extern codec_config_t boardCodecScoConfig1;

extern hal_audio_config_t txSpeakerConfig;
extern hal_audio_config_t rxMicConfig;
extern hal_audio_config_t txMicConfig;
extern hal_audio_config_t rxSpeakerConfig;

/* --------------------------------------------- Exported Global Variables */

/* --------------------------------------------- Static Global Variables */

extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_speaker_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_mic_handle), 4);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_mic_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_speaker_handle), 4);
static codec_handle_t codec_handle;
static uint8_t codec_inited = 0;

//AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t MicBuffer    [BUFFER_NUMBER * BUFFER_SIZE], 4);
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t RxAudioBufFromBt[BUFFER_NUMBER * BUFFER_SIZE], 4);
//AT_NONCACHEABLE_SECTION(uint32_t g_AudioTxDummyBuffer[AUDIO_DUMMY_SIZE / 4U]);

OSA_SEMAPHORE_HANDLE_DEFINE(xSemaphoreScoAudio);
#if EnableVitBeforeTheCall==1
	OSA_SEMAPHORE_HANDLE_DEFINE(xSemaphoreVitAudio);
#endif

//static volatile uint8_t RingToneIsInitialized=0;
static volatile uint8_t NowInHfpTelCall=0;
static volatile uint8_t NowInHfpAppCall=0;
static volatile uint8_t WasInRingTone=0;
static volatile uint8_t saiEnable= 0;
static uint32_t txMic_index = 0U, rxMic_index = 0U;
atomic_t emptyMicBlock = BUFFER_NUMBER;
static uint32_t txSpeaker_index = 0U, rxSpeaker_index = 0U;
atomic_t emptySpeakerBlock = BUFFER_NUMBER;
static uint32_t rxSpeaker_test = 0U, rxMic_test = 0U;
static volatile uint8_t s_ringTone = 0;
static uint32_t cpy_index = 0U, tx_index = 0U;
static volatile uint8_t sco_audio_setup = 0;
static SCO_AUDIO_EP_INFO s_ep_info;
static API_RESULT audio_setup_pl_ext(uint8_t isRing, SCO_AUDIO_EP_INFO *ep_info);
API_RESULT sco_audio_start_pl_ext(void);

/* --------------------------------------------- Functions */
/* overwrite sco_audio_init_pl_ext of sco_audio_pl.c.
 * The follow functions can be overwritten too
 * if the actual example need implement them to
 * use different audio data.
 * sco_audio_init_pl_ext, sco_audio_shutdown_pl_ext,
 * sco_audio_setup_pl_ext, sco_audio_start_pl_ext,
 * sco_audio_stop_pl_ext, sco_audio_write_pl_ext.
 */
static uint32_t count = 0;
static U32 I2SOutputMuteCnt=0;
U32 PdmInputMuteCnt=0;
#endif

#if EnableConversa==1
int AOD_BTDnBuf;
int AOD_BTUpBuf;
int BTAudioBitWidth;
int BTAudioFs;
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t OneBlockTxBufToBT    [BUFFER_SIZE], 4);
S16 TmpDbgSigalBuf[BUFFER_SIZE/2];
void SCO_AudioFlow_SemaphorePost(void)
{
	#if EnableOnlyMicSpk_NoBT==1
		return;
	#endif

	#if EnableVitBeforeTheCall==1
		if((NowInHfpTelCall)||(NowInHfpAppCall)||(s_ringTone))
		{
			OSA_SemaphorePost(xSemaphoreScoAudio);
		}else
		{
			OSA_SemaphorePost(xSemaphoreVitAudio);
		}
	#else
		OSA_SemaphorePost(xSemaphoreScoAudio);
	#endif
}
#endif

static void rxMicCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
	if (kStatus_HAL_AudioError == completionStatus)
	{
		/* Handle the error. */
	}
	else
	{
    	DbgPin5Up();
		rxMic_test++;
		OSA_SemaphorePost(xSemaphoreScoAudio);
    	DbgPin5Dn();
	}
}

#if SCO_SAI_LOOPBACK
static void txMicCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    if (kStatus_HAL_AudioError == completionStatus)
    {
        /* Handle the error. */
    }
    else
    {
#if SCO_SAI_LOOPBACK
        (void)atomic_inc(&emptySpeakerBlock);
#else
        (void)atomic_inc(&emptyMicBlock);
#endif
        rxMic_test++;
        OSA_SemaphorePost(xSemaphoreScoAudio);
    }
}
#else
static void txMicCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    AllowAudioInterfaceReInit_Fc25=0;
    static volatile uint8_t s_8978ConsumerActualData = 0;
    hal_audio_transfer_t xfer;
	if (s_ringTone == 1U)
    {
    	//when ring tone is playing, all zeros should go to BT up streaming (mic tx)
		s_8978ConsumerActualData = 0;
		#if 0
			xfer.dataSize            = AUDIO_DUMMY_SIZE;
			xfer.data                = (uint8_t *)&g_AudioTxDummyBuffer[0];
		#else
			memset(OneBlockTxBufToBT,0,sizeof(OneBlockTxBufToBT));
			xfer.dataSize            = BUFFER_SIZE;
			xfer.data                = (uint8_t *)OneBlockTxBufToBT;
		#endif
        HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);
    }
    else
    {
    	DbgPin5Up();
		#if EnableConversa==1
			//take audio out from cir buffer and set xfer to start the transfer
        	OSA_SR_ALLOC();
    		OSA_ENTER_CRITICAL();
    		AOD_BTUpBuf=CirAudioBuf_SpaceOccupiedInSamples_S16(&BTUpAudioBuf_S16);
			if(AOD_BTUpBuf>=(BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8)))
			{
				//there is at least 1 frame of audio available, take it out, do xfer
				CirAudioBuf_ReadSamples_S16(&BTUpAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), (S16 *)OneBlockTxBufToBT);
			}else
			{
				//not enough audio samples (1 block of BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8) samples) --- send all zeros
				//this should not happen when audio PLL sync is doing well
				memset(OneBlockTxBufToBT,0,sizeof(OneBlockTxBufToBT));
			}
			OSA_EXIT_CRITICAL();

			//start a new transfer
			xfer.data            = OneBlockTxBufToBT;
			xfer.dataSize        = BUFFER_SIZE;
			HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);

			//audio PLL adjusting based on AOD of the cir buffer
			#if EnableAudioPllAdjustingToSyncBetweenBtFsAndLocalFs==1
				CheckI2SInputBufAodAndAdjustAudioPll(AOD_BTDnBuf);
			#endif

			//blink blue LED
			static int BluLedBlinkCnt=0;

			if(BluLedBlinkCnt++%16 < 8)	//64*16=1.024s
			{
				LedOff_B();
			}else
			{
				LedOn_B();
			}
		#else
			if (s_8978ConsumerActualData)
			{
				s_8978ConsumerActualData = 0;
				(void)atomic_inc(&emptyMicBlock);
				OSA_SemaphorePost(xSemaphoreScoAudio);

				if (atomic_get(&emptyMicBlock) < (BUFFER_NUMBER))
				{
					//there is at least 1 block of audio available to tx
					s_8978ConsumerActualData = 1;
					xfer.data                = MicBuffer + txMic_index * BUFFER_SIZE;
					xfer.dataSize            = BUFFER_SIZE;

					if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer))
					{
						txMic_index++;
					}
					if (txMic_index == BUFFER_NUMBER)
					{
						txMic_index = 0U;
					}
				}
				else
				{
					//there is no audio data available
					printf("mic dummy\r\n");
					s_8978ConsumerActualData = 0;
					xfer.dataSize            = AUDIO_DUMMY_SIZE;
					xfer.data                = (uint8_t *)&g_AudioTxDummyBuffer[0];
					HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);
				}
			}
			else
			{
				//there are at least 3 block of audio available to tx
				if (atomic_get(&emptyMicBlock) < (BUFFER_NUMBER - 2))
				{
					s_8978ConsumerActualData = 1;
					xfer.data                = MicBuffer + txMic_index * BUFFER_SIZE;
					xfer.dataSize            = BUFFER_SIZE;

					if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer))
					{
						txMic_index++;
					}
					if (txMic_index == BUFFER_NUMBER)
					{
						txMic_index = 0U;
					}
				}
				else
				{
					//available audio data has less than 3 blocks
					printf("mic dummy\r\n");
					s_8978ConsumerActualData = 0;
					xfer.dataSize            = AUDIO_DUMMY_SIZE;
					xfer.data                = (uint8_t *)&g_AudioTxDummyBuffer[0];
					HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);
				}
			}
		#endif
		DbgPin5Dn();
    }
    AllowAudioInterfaceReInit_Fc25=1;
}
#endif

#if EnableConversa==0
static void txSpeakerCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
	static volatile uint8_t s_consumerActualData = 0;
	hal_audio_transfer_t xfer;
	if (s_ringTone == 1U)
	{
		if ((atomic_get(&emptySpeakerBlock) > 0U) && (cpy_index < MUSIC_LEN / BUFFER_SIZE))
		{
			//Spk tx buffer has at least 1 block of free space, AND there is at least one block of ring-tone signal data available
			/* Fill in the buffers. */
			memcpy((uint8_t *)&RxAudioBufFromBt[BUFFER_SIZE * (cpy_index % BUFFER_NUMBER)],
				   (uint8_t *)&music[cpy_index * BUFFER_SIZE], sizeof(uint8_t) * BUFFER_SIZE);
			(void)atomic_dec(&emptySpeakerBlock);
			cpy_index++;
		}

		if (atomic_get(&emptySpeakerBlock) < BUFFER_NUMBER)
		{
			//emptySpeakerBlock < BUFFER_NUMB
			//means occupied space > 0, means there is at least one block of audio to send
			/*  xfer structure */
			xfer.data     = (uint8_t *)&RxAudioBufFromBt[BUFFER_SIZE * (tx_index % BUFFER_NUMBER)];
			xfer.dataSize = BUFFER_SIZE;
			/* Wait for available queue. */
			if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer))
			{
				tx_index++;
			}
			(void)atomic_inc(&emptySpeakerBlock);
		}else
		{
			//emptySpeakerBlock >= BUFFER_NUMB
			//there isn't any data to send, all the block of spaces are free, do nothing
		}
	}
	else
	{
		DbgPin6Up();
		if (s_consumerActualData)
		{
			s_consumerActualData = 0;
			(void)atomic_inc(&emptySpeakerBlock);
			OSA_SemaphorePost(xSemaphoreScoAudio);

			if (atomic_get(&emptySpeakerBlock) < (BUFFER_NUMBER))
			{
				//there is at least 1 block of audio available to tx
				s_consumerActualData = 1;
				xfer.data            = RxAudioBufFromBt + txSpeaker_index * BUFFER_SIZE;
				xfer.dataSize        = BUFFER_SIZE;

				if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer))
				{
					txSpeaker_index++;
				}
				if (txSpeaker_index == BUFFER_NUMBER)
				{
					txSpeaker_index = 0U;
				}
			}
			else
			{
				//there is no audio data available
				s_consumerActualData = 0;
				xfer.dataSize        = AUDIO_DUMMY_SIZE;
				xfer.data            = (uint8_t *)&g_AudioTxDummyBuffer[0];
				HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer);
			}
		}
		else
		{
			if (atomic_get(&emptySpeakerBlock) < (BUFFER_NUMBER - 2))
			{
				//there are at least 3 block of audio available to tx
				s_consumerActualData = 1;
				xfer.data            = RxAudioBufFromBt + txSpeaker_index * BUFFER_SIZE;
				xfer.dataSize        = BUFFER_SIZE;
				if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer))
				{
					txSpeaker_index++;
				}
				if (txSpeaker_index == BUFFER_NUMBER)
				{
					txSpeaker_index = 0U;
				}
			}
			else
			{
				//available audio data has less than 3 blocks
				s_consumerActualData = 0;
				xfer.dataSize        = AUDIO_DUMMY_SIZE;
				xfer.data            = (uint8_t *)&g_AudioTxDummyBuffer[0];
				HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer);
			}
		}
		DbgPin6Dn();
	}
}
#endif

static void rxSpeakerCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    AllowAudioInterfaceReInit_Fc25=0;
    if (kStatus_HAL_AudioError == completionStatus)
    {
        /* Handle the error. */
    }
    else
    {
    	DbgPin6Up();
		#if EnableConversa==1
			if (atomic_get(&emptySpeakerBlock) < 2U)
			{
				hal_audio_transfer_t xfer;

				xfer.data     = RxAudioBufFromBt + rxSpeaker_index * BUFFER_SIZE;
				xfer.dataSize = BUFFER_SIZE;

				(void)atomic_dec(&emptySpeakerBlock);
				if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_speaker_handle[0], &xfer))
				{
					rxSpeaker_index++;
				}
				if (rxSpeaker_index == 2)
				{
					rxSpeaker_index = 0U;
				}

	        	OSA_SR_ALLOC();
				OSA_ENTER_CRITICAL();
				//take audio out from block rxSpeaker_index and put to cir buffer
				AOD_BTDnBuf=CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16);
				//if(AOD_BTDnBuf<(BTDnAudioBuf_S16.LengthInSamples-BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8)))
				if((BTDnAudioBuf_S16.LengthInSamples - AOD_BTDnBuf) >= BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8))
				{
					//there is at least 1 free frame space in the cir buffer
					#if 0
						GenerateSinWavFromTable_S16_SingleCh(&TmpDbgSigalBuf[0*128], 128);
						GenerateSinWavFromTable_S16_SingleCh(&TmpDbgSigalBuf[1*128], 128);
						GenerateSinWavFromTable_S16_SingleCh(&TmpDbgSigalBuf[2*128], 128);
						GenerateSinWavFromTable_S16_SingleCh(&TmpDbgSigalBuf[3*128], 128);
						CirAudioBuf_WriteSamples_S16(&BTDnAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), TmpDbgSigalBuf);
					#else
						//CirAudioBuf_WriteSamples_S16(&BTDnAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), (S16 *)(RxAudioBufFromBt + (1-rxSpeaker_index) * BUFFER_SIZE));		//buffer A or B select, select the other one by 1-rxSpeaker_index
						CirAudioBuf_WriteSamples_S16(&BTDnAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), (S16 *)(RxAudioBufFromBt + rxSpeaker_index * BUFFER_SIZE));
					#endif
					(void)atomic_inc(&emptySpeakerBlock);
				}else
				{
					//no more free space --- abandon the current received audio frame
					//this should not happen when audio PLL sync is doing well
					(void)atomic_inc(&emptySpeakerBlock);
				}
				OSA_EXIT_CRITICAL();
			}
		#else
        	OSA_SemaphorePost(xSemaphoreScoAudio);
		#endif
        rxSpeaker_test++;
    	DbgPin6Dn();
    }
    AllowAudioInterfaceReInit_Fc25=1;
}

extern void ClearBTUpDnAudioBufDataArea(void);
void Deinit_Board_Audio(void)
{
	#if EnableConversa==1
	#if UsingQAR87Board == 1
		//deinit code (amplifier)
		if (codec_inited == 0)
		{
			return ;
		}
		hal_amp_aw88166_left_stop();
		hal_amp_aw88166_right_stop();
		//to do .... codec mute
	#else
		if (codec_inited == 0)
		{
			return ;
		}
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
	#endif
		//close all I2S and related DMA --- if need to just close the wanted, just call one of the 3 grouped functions
		CloseI2sDma((I2S_Type *)DEMO_I2SRxFrAmp);
		CloseI2sDma((I2S_Type *)DEMO_I2STxToAmp);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2SRxFrAmp);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2STxToAmp);
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
		//close PDM all channels
		BOARD_DeInit_DMA_PDM(0xff);

		HAL_AudioTxDeinit((hal_audio_handle_t)&tx_mic_handle[0]);
		HAL_AudioRxDeinit((hal_audio_handle_t)&rx_speaker_handle[0]);

		(void)BOARD_SwitchAudioFreq(0U,0);
		codec_inited = 0;
		NowInHfpTelCall=0;
		NowInHfpAppCall=0;
		s_ringTone=0;
	    RequestToGetOutofHfp=1;


		CirAudioBuf_ClearAllSamples_S16(&BTUpAudioBuf_S16);
		CirAudioBuf_ClearAllSamples_S16(&BTDnAudioBuf_S16);
		//ClearBTUpDnAudioBufDataArea();

		//this is necessary, or there is extra sound in the beginning of the next call
        memset(RxAudioBufFromBt, 0, sizeof(RxAudioBufFromBt));
		#if 0
			ClearDmaBuf_I2S3Tx0();
			ClearDmaBuf_I2S1Rx0();
			memset(VarBlockSharedByDspAndMcu.PdmInAudioBuf, 0, sizeof(VarBlockSharedByDspAndMcu.PdmInAudioBuf));
			memset(VarBlockSharedByDspAndMcu.UacUpAudioBuf, 0, sizeof(VarBlockSharedByDspAndMcu.UacUpAudioBuf));
			memset(VarBlockSharedByDspAndMcu.I2SLineInBufL, 0, sizeof(VarBlockSharedByDspAndMcu.I2SLineInBufL));
			memset(VarBlockSharedByDspAndMcu.I2SLineInBufR, 0, sizeof(VarBlockSharedByDspAndMcu.I2SLineInBufR));
			memset(VarBlockSharedByDspAndMcu.I2SLineOtBufL, 0, sizeof(VarBlockSharedByDspAndMcu.I2SLineOtBufL));
			memset(VarBlockSharedByDspAndMcu.I2SLineOtBufR, 0, sizeof(VarBlockSharedByDspAndMcu.I2SLineOtBufR));
			memset(VarBlockSharedByDspAndMcu.BTRxInAudio, 0, sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio));
			memset(VarBlockSharedByDspAndMcu.BTTxOtAudio, 0, sizeof(VarBlockSharedByDspAndMcu.BTTxOtAudio));
		#endif

		PRINTF("Deinit_Board_Audio is done \r\n");

	#else
	#if UsingQAR87Board == 1
		if (codec_inited == 0)
		{
			return ;
		}
		//to do .... codec mute
	#else
		if (codec_inited == 0)
		{
			return ;
		}
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
	#endif
		HAL_AudioTxDeinit((hal_audio_handle_t)&tx_speaker_handle[0]);
		HAL_AudioRxDeinit((hal_audio_handle_t)&rx_mic_handle[0]);
		HAL_AudioTxDeinit((hal_audio_handle_t)&tx_mic_handle[0]);
		HAL_AudioRxDeinit((hal_audio_handle_t)&rx_speaker_handle[0]);
		(void)BOARD_SwitchAudioFreq(0U,0);
		codec_inited = 0;
	#endif
}

/*Initialize sco audio interface and codec.*/
static void Init_Board_Sco_Audio(uint32_t samplingRate, UCHAR bitWidth)
{
    uint32_t src_clk_hz;
	#if EnableConversa==1
    	//configure PDM, Fc1,Fc3, CODEC, and configure Fc2 Fc5
		if (samplingRate > 0U)
		{
			PRINTF("Init Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

			/* Enable clock */
	    	//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
			src_clk_hz = BOARD_SwitchAudioFreq(16000,0);

			//enalbe PDM clk
			/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
			CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
	    	//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
			CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

			BTAudioBitWidth=bitWidth;
			if(BTAudioBitWidth!=16)
			{
				PRINTF("Init_Board_Sco_Audio error --- BT bit width is NOT 16 \r\n");
			}
			BTAudioFs=samplingRate;
			if((BTAudioFs!=8000)&&(BTAudioFs!=16000))
			{
				PRINTF("Init_Board_Sco_Audio error --- BT bit Fs is NOT 8kHz or 16kHz \r\n");
			}
			VarBlockSharedByDspAndMcu.BtFs=BTAudioFs;

			//this part keeps the same as the original demo setup
			txMicConfig.srcClock_Hz   = src_clk_hz;
			txMicConfig.sampleRate_Hz = samplingRate;
			HAL_AudioTxInit((hal_audio_handle_t)&tx_mic_handle[0], &txMicConfig);
			HAL_AudioTxInstallCallback((hal_audio_handle_t)&tx_mic_handle[0], txMicCallback, NULL);

			//this part keeps the same as the original demo setup
			rxSpeakerConfig.srcClock_Hz   = src_clk_hz;
			rxSpeakerConfig.sampleRate_Hz = samplingRate;
			HAL_AudioRxInit((hal_audio_handle_t)&rx_speaker_handle[0], &rxSpeakerConfig);
			HAL_AudioRxInstallCallback((hal_audio_handle_t)&rx_speaker_handle[0], rxSpeakerCallback, NULL);

	    	DbgPin8Up();
	#if UsingQAR87Board == 1
			//initial codec/amplifier
			//B36932, to do....

	    	codec_inited = 1;
			//initial audio buffer and dmic and I2S

			InitAudioCircularBuf();

			//PDM, fc3, fc1 and chained DMA configuring
			//we open pdm ports here
			Init_MicDmaCfgCh(0xff);	//mic0,1,2,3,4,5
			BOARD_Init_DMA_PDM(0xff);
			BOARD_Init_DMIC(0xff,0); //0: no skip general Dmic init. If not the first mic init, then should skip.
			ConfigDmicChainedDma(0xff);

			BOARD_Init_DMA_I2S_Fc1();
			BOARD_Init_DMA_I2S_Fc3();
			BOARD_Init_I2S_Fc1();
			BOARD_Init_I2S_Fc3();
			ClearDmaBuf_I2S1Rx0();
			ClearDmaBuf_I2S3Tx0();
			ConfigI2S1ChainedDma();
			ConfigI2S3ChainedDma();
			EnableI2S1Rx0DmaChannel();
			EnableI2S3Tx0DmaChannel();
			DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|
					//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
					#if EnableMic01==1
						AudioPdmPortsBitMapFlag_Mic01
					#endif
					#if EnableMic23==1
						|AudioPdmPortsBitMapFlag_Mic23
					#endif
					#if EnableMic45==1
						|AudioPdmPortsBitMapFlag_Mic45
					#endif
					#if EnableMic67==1
						|AudioPdmPortsBitMapFlag_Mic67
					#endif
			);
			hal_amp_aw88166_left_start("Receiver");
			hal_amp_aw88166_right_start("Receiver");
			PRINTF("Init_Board_Sco_Audio is successful and finished \r\n");

	
	#else
			/* Codec */
			if (CODEC_Init(&codec_handle, &boardCodecScoConfig) != kStatus_Success)
			{
		    	DbgPin8Dn();
				PRINTF("Init_Board_Sco_Audio is failed --- CODEC init failure \r\n");
			}else
			{
		    	DbgPin8Dn();
				CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
				//CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);
				CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, 16000, 32);
				CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
				CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
				CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
				codec_inited = 1;

				InitAudioCircularBuf();

				//PDM, fc3, fc1 and chained DMA configuring
				//we open pdm ports here
				Init_MicDmaCfgCh(0xff);	//mic0,1,2,3,4,5
				BOARD_Init_DMA_PDM(0xff);
				BOARD_Init_DMIC(0xff,0); //0: no skip general Dmic init. If not the first mic init, then should skip.
				ConfigDmicChainedDma(0xff);


				BOARD_Init_DMA_I2S_Fc1();
				BOARD_Init_DMA_I2S_Fc3();
					BOARD_Init_I2S_Fc1();
					BOARD_Init_I2S_Fc3();
						ClearDmaBuf_I2S1Rx0();
						ClearDmaBuf_I2S3Tx0();
							ConfigI2S1ChainedDma();
							ConfigI2S3ChainedDma();
								EnableI2S1Rx0DmaChannel();
								EnableI2S3Tx0DmaChannel();
				DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|
						//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
						#if EnableMic01==1
							AudioPdmPortsBitMapFlag_Mic01
						#endif
						#if EnableMic23==1
							|AudioPdmPortsBitMapFlag_Mic23
						#endif
						#if EnableMic45==1
							|AudioPdmPortsBitMapFlag_Mic45
						#endif
						#if EnableMic67==1
							|AudioPdmPortsBitMapFlag_Mic67
						#endif
				);

				PRINTF("Init_Board_Sco_Audio is successful and finished \r\n");
			}
	#endif
		}
	#else
		if (samplingRate > 0U)
		{
			PRINTF("Init Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

			/* Enable clock */
			src_clk_hz = BOARD_SwitchAudioFreq(samplingRate,0);

			/* Audio streamer */
			#if (defined FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER && FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) || (defined FSL_FEATURE_PCC_HAS_SAI_DIVIDER && FSL_FEATURE_PCC_HAS_SAI_DIVIDER)
				txSpeakerConfig.srcClock_Hz = OVER_SAMPLE_RATE * samplingRate;
			#else
				txSpeakerConfig.srcClock_Hz = src_clk_hz;
			#endif
			txSpeakerConfig.sampleRate_Hz     = samplingRate;
			txSpeakerConfig.frameSyncWidth    = kHAL_AudioFrameSyncWidthHalfFrame,
			txSpeakerConfig.frameSyncPolarity = kHAL_AudioBeginAtRisingEdge,
			txSpeakerConfig.lineChannels      = kHAL_AudioMonoLeft;
			txSpeakerConfig.dataFormat        = kHAL_AudioDataFormatDspModeB;
			HAL_AudioTxInit((hal_audio_handle_t)&tx_speaker_handle[0], &txSpeakerConfig);
			HAL_AudioTxInstallCallback((hal_audio_handle_t)&tx_speaker_handle[0], txSpeakerCallback, NULL);

			#if (defined FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER && FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) || (defined FSL_FEATURE_PCC_HAS_SAI_DIVIDER && FSL_FEATURE_PCC_HAS_SAI_DIVIDER)
				rxMicConfig.srcClock_Hz = OVER_SAMPLE_RATE * samplingRate;
			#else
				rxMicConfig.srcClock_Hz = src_clk_hz;
			#endif
			rxMicConfig.sampleRate_Hz = samplingRate;
			HAL_AudioRxInit((hal_audio_handle_t)&rx_mic_handle[0], &rxMicConfig);
			HAL_AudioRxInstallCallback((hal_audio_handle_t)&rx_mic_handle[0], rxMicCallback, NULL);

			txMicConfig.srcClock_Hz   = src_clk_hz;
			txMicConfig.sampleRate_Hz = samplingRate;
			HAL_AudioTxInit((hal_audio_handle_t)&tx_mic_handle[0], &txMicConfig);
			HAL_AudioTxInstallCallback((hal_audio_handle_t)&tx_mic_handle[0], txMicCallback, NULL);

			rxSpeakerConfig.srcClock_Hz   = src_clk_hz;
			rxSpeakerConfig.sampleRate_Hz = samplingRate;
			HAL_AudioRxInit((hal_audio_handle_t)&rx_speaker_handle[0], &rxSpeakerConfig);
			HAL_AudioRxInstallCallback((hal_audio_handle_t)&rx_speaker_handle[0], rxSpeakerCallback, NULL);

			/* Codec */
	    	DbgPin8Up();
			if (CODEC_Init(&codec_handle, &boardCodecScoConfig) != kStatus_Success)
			{
				PRINTF("codec init failed!\r\n");
			}
	    	DbgPin8Dn();
			CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
			CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);
			CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
			CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
			CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
			codec_inited = 1;
		}
	#endif
}
static void Init_Board_RingTone_Audio(uint32_t samplingRate, UCHAR bitWidth)	//never be called
{
    uint32_t src_clk_hz;
	#if EnableConversa==1
		//configure Fc1,Fc3, CODEC
		if (samplingRate > 0U)
		{
			PRINTF("Init Audio RingTone SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

			/* Enable clock */
			//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
			src_clk_hz = BOARD_SwitchAudioFreq(16000,0);

			BTAudioBitWidth=bitWidth;
			if(BTAudioBitWidth!=16)
			{
				PRINTF("Init_Board_Sco_Audio error --- BT bit width is NOT 16 \r\n");
			}
			BTAudioFs=samplingRate;
			if((BTAudioFs!=8000)&&(BTAudioFs!=16000))
			{
				PRINTF("Init_Board_Sco_Audio error --- BT bit Fs is NOT 8kHz or 16kHz \r\n");
			}
			VarBlockSharedByDspAndMcu.BtFs=BTAudioFs;

			DbgPin8Up();

	#if UsingQAR87Board == 1
			//initial codec/amplifier
			//B36932, to do....
			
			codec_inited = 1;
			//initial audio buffer and dmic and I2S	
			InitAudioCircularBuf();

			//fc3, fc1 and chained DMA configuring
			BOARD_Init_DMA_I2S_Fc1();
			BOARD_Init_DMA_I2S_Fc3();
				BOARD_Init_I2S_Fc1();
				BOARD_Init_I2S_Fc3();
					ClearDmaBuf_I2S1Rx0();
					ClearDmaBuf_I2S3Tx0();
						ConfigI2S1ChainedDma();
						ConfigI2S3ChainedDma();
							EnableI2S1Rx0DmaChannel();
							EnableI2S3Tx0DmaChannel();
			DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3);
			hal_amp_aw88166_left_start("Music");
			hal_amp_aw88166_right_start("Music");

			PRINTF("Init_Board_RingTone_Audio is successful and finished \r\n");
			
	#else	
			/* Codec */		
			if (CODEC_Init(&codec_handle, &boardCodecScoConfig) != kStatus_Success)
			{
				DbgPin8Dn();
				PRINTF("Init_Board_Sco_Audio is failed --- CODEC init failure \r\n");
			}else
			{
				DbgPin8Dn();
				CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
				//CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);
				CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, 16000, 32);
				CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
				CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
				CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
				codec_inited = 1;

				InitAudioCircularBuf();

				//fc3, fc1 and chained DMA configuring
				BOARD_Init_DMA_I2S_Fc1();
				BOARD_Init_DMA_I2S_Fc3();
					BOARD_Init_I2S_Fc1();
					BOARD_Init_I2S_Fc3();
						ClearDmaBuf_I2S1Rx0();
						ClearDmaBuf_I2S3Tx0();
							ConfigI2S1ChainedDma();
							ConfigI2S3ChainedDma();
								EnableI2S1Rx0DmaChannel();
								EnableI2S3Tx0DmaChannel();
				DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3);
				PRINTF("Init_Board_RingTone_Audio is successful and finished \r\n");
			}
	#endif			
		}
	#else
		if (samplingRate > 0U)
		{
			PRINTF("Init RingTone Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

			/* Enable clock */
			src_clk_hz = BOARD_SwitchAudioFreq(samplingRate,0);

			/* Audio streamer */
			txSpeakerConfig.sampleRate_Hz     = samplingRate;
			txSpeakerConfig.frameSyncWidth    = kHAL_AudioFrameSyncWidthHalfFrame,
			txSpeakerConfig.frameSyncPolarity = kHAL_AudioBeginAtFallingEdge,
			txSpeakerConfig.lineChannels      = kHAL_AudioStereo;
			txSpeakerConfig.dataFormat        = kHAL_AudioDataFormatI2sClassic;
	#if (defined FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER && FSL_FEATURE_SAI_HAS_MCLKDIV_REGISTER) || \
		(defined FSL_FEATURE_PCC_HAS_SAI_DIVIDER && FSL_FEATURE_PCC_HAS_SAI_DIVIDER)
			txSpeakerConfig.srcClock_Hz = OVER_SAMPLE_RATE * samplingRate;
	#else
			txSpeakerConfig.srcClock_Hz = src_clk_hz;
	#endif

			HAL_AudioTxInit((hal_audio_handle_t)&tx_speaker_handle[0], &txSpeakerConfig);
			HAL_AudioTxInstallCallback((hal_audio_handle_t)&tx_speaker_handle[0], txSpeakerCallback, NULL);

	#if UsingQAR87Board == 1
			//initial codec/amplifier
			//B36932, to do....
			hal_amp_aw88166_left_start("Music");
			hal_amp_aw88166_right_start("Music");

			codec_inited = 1;
	#else		
			if (CODEC_Init(&codec_handle, &boardCodecScoConfig1) != kStatus_Success)
			{
				PRINTF("Init_Board_RingTone_Audio is failed --- CODEC init failure \r\n");
			}else
			{
				CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
				CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);
				CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
				CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
				CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
				codec_inited = 1;
				PRINTF("Init_Board_RingTone_Audio is successful and finished \r\n");
			}
	#endif		
		}
	#endif
}

static API_RESULT audio_setup_pl_ext(uint8_t isRing, SCO_AUDIO_EP_INFO *ep_info)
{
    txMic_index     = 0U;
    rxMic_index     = 0U;
    txSpeaker_index = 0U, rxSpeaker_index = 0U;
    (void)atomic_set(&emptySpeakerBlock, BUFFER_NUMBER);
    (void)atomic_set(&emptyMicBlock, BUFFER_NUMBER);

    //in fact, always call Init_Board_Sco_Audio.
    if (isRing)
    {
    	//never comes here
        Init_Board_RingTone_Audio(ep_info->sampl_freq, ep_info->sample_len);
    	//RingToneIsInitialized=1;
    }
    else
    {
        Init_Board_Sco_Audio(ep_info->sampl_freq, ep_info->sample_len);
    	//RingToneIsInitialized=1;
    }

    I2SOutputMuteCnt=25;		//this is to mute I2S output for several frames after a call is started --- because from DSP, could have bad audio data coming (from Conversa/SRC)
    saiEnable = 1;
    return API_SUCCESS;
}

extern U32 AudioIoFrameCnt;
extern volatile S32 *I2SDmaOtCh01Ptr;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;
extern void USB_DeviceCdcVcomTask(void);

extern void hfp_AnswerCall(void);	//looks like this function can not be directly called from APP_MU_IRQHandler
extern void hfp_RejectCall(void);	//looks like this function can not be directly called from APP_MU_IRQHandler
int NeedToCall_hfp_AnswerCall=0;
int NeedToCall_hfp_RejectCall=0;

int LocalToneGainControlCnt=0;
float LocalToneGainCurrent=0.0f;
float LocalToneGainTarget=0.0f;
volatile U32 MU_U32InfoFromDsp;
__attribute__((section("CodeQuickAccess")))
void APP_MU_IRQHandler(void)
{
	int i;
    uint32_t flag = 0;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];
	S32 TmpRingToneS32_SingleCh[AudioFrameSizeInSamplePerCh];

    flag = MU_GetStatusFlags(APP_MU);
    if ((flag & kMU_Rx0FullFlag) == kMU_Rx0FullFlag)
    {
    	MU_U32InfoFromDsp = MU_ReceiveMsgNonBlocking(APP_MU, CHN_MU_REG_NUM);

		DbgPin7Up();
    	if(MU_U32InfoFromDsp==MuEvtDspToMcu_AudioProcIsFinished_HfpCall)
    	{
    		OSA_SR_ALLOC();

			#if 1	//folding --- write conversa Tx output audio to BT up streaming cir buffer
				if(VarBlockSharedByDspAndMcu.BtFs==8000)
				{
					#if 1
						OSA_ENTER_CRITICAL();
						//put audio samples into BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
						if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/2))
						{
							//there are enough free space from BT up streaming
							for(i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
								TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.BTTxOtAudio[i]>>16);
							CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh/2, TmpAudioS16Buf);
						}else
						{
							//not enough free space for BT UP streaming --- abandon the current frame of BTTxOtAudio
							//this should not happen when audio PLL sync is doing well
							#if EnableBtCirBufUnderflowOverFlowPrint==1
								PRINTF("BT Up CirBuf is F \r\n");
							#endif
						}
						OSA_EXIT_CRITICAL();
					#endif
				}else
				if(VarBlockSharedByDspAndMcu.BtFs==16000)
				{
					#if 1
						OSA_ENTER_CRITICAL();
						//put audio samples into BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
						if(CirAudioBuf_SpaceAvailableInSamples_S16(&BTUpAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/1))
						{
							//there are enough free space from BT up streaming
							for(i=0;i<AudioFrameSizeInSamplePerCh/1;i++)
								TmpAudioS16Buf[i]=(VarBlockSharedByDspAndMcu.BTTxOtAudio[i]>>16);
							CirAudioBuf_WriteSamples_S16(&BTUpAudioBuf_S16, AudioFrameSizeInSamplePerCh/1, TmpAudioS16Buf);
						}else
						{
							//not enough free space for BT UP streaming --- abandon the current frame of BTTxOtAudio
							//this should not happen when audio PLL sync is doing well
							#if EnableBtCirBufUnderflowOverFlowPrint==1
								PRINTF("BT Up CirBuf is F \r\n");
							#endif
						}
						OSA_EXIT_CRITICAL();
					#endif
				}else
				{
					//should never come here
					PRINTF("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
				}
			#endif

			#if EnableUsbComAndAudio==1//folding --- write conversa Tx output audio to UAC up streaming buffer
				OSA_ENTER_CRITICAL();
					//put audio data from DSP side to UacUp cir buffer
					int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
					AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(&UacUpAudioBuf_MCh);
					AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
					if(UsbUpStreamingIsStarted)
					{
						if (AOFSOfUacUpBuf >= (AudioFrameSizeInSamplePerCh))
						{
							CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, AudioFrameSizeInSamplePerCh, (T_MCh32BitUacUpAudioSample *)VarBlockSharedByDspAndMcu.UacUpAudioBuf);
						}
						else
						{
							#if EnableUacCirBufUnderflowOverFlowPrint==1
								PRINTF("UacUp F\r\n");
							#endif
						}
					}

					//adjust mic upstreaming tx length
					//call adjust mic upstreaming length with AOFS just after writing the cir buffer
					if(UsbUpStreamingIsStarted)
					{
						USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+AudioFrameSizeInSamplePerCh);	//calling adjust just before writing the circular buffer
					}
				OSA_EXIT_CRITICAL();

				#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
					if(UsbUpStreamingIsStarted)
						VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=AODOfUacUpBuf+AudioFrameSizeInSamplePerCh;
				#endif
			#endif

			#if 1	//folding --- write conversa Rx output audio to I2S
				if(s_ringTone)
				{
					#if 0
						//generate tone from sin calculation --- much more mips are needed, but can be flexible setting different f
						GenerateSineToneSingleFreq_S32_LRMixed(&SineToneGenerator1, TmpRingToneS32_SingleCh, AudioFrameSizeInSamplePerCh , 1);
					#else
						//generate tone from sin wav table --- much smaller mips, but freq is fixed at 320Hz
						GenerateSinWavFromTable_S32_SingleCh(TmpRingToneS32_SingleCh, AudioFrameSizeInSamplePerCh);
					#endif

					LocalToneGainControlCnt++;

					//16KHz Fs
					//0.8s tone sound, 1.2s silence
					if((LocalToneGainControlCnt%400)<180)
					{
						LocalToneGainTarget=0.7f;
					}else
					{
						LocalToneGainTarget=0.0f;
					}

					for(int j=0;j<16;j++)
					{
						//loop 16 times, each time processes 8 samples --- this is to avoid volume change click noise
						LocalToneGainCurrent=0.02f*LocalToneGainTarget + 0.98f*LocalToneGainCurrent;
						//copy I2S output buffer (line out to DAC) from shared memory to DMA buffer, I2S output buffer are just processed and written by DSP
						for(int i=0;i<AudioFrameSizeInSamplePerCh/16;i++)
						{
							*I2SDmaOtCh01Ptr++=TmpRingToneS32_SingleCh[j*8+i]*LocalToneGainCurrent;
							*I2SDmaOtCh01Ptr++=TmpRingToneS32_SingleCh[j*8+i]*LocalToneGainCurrent;
						}
					}
				}else
				{
					LocalToneGainCurrent=0.0f;
					LocalToneGainTarget=0.0f;
					//copy I2S output buffer (line out to DAC) from shared memory to DMA buffer, I2S output buffer are just processed and written by DSP
					if(I2SOutputMuteCnt)
					{
						I2SOutputMuteCnt--;
						#if 1
							memset((void *)I2SDmaOtCh01Ptr,0,sizeof(U32)*2*AudioFrameSizeInSamplePerCh);
						#else
							//for debug watching audio wave form
							for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
							{
								*I2SDmaOtCh01Ptr++=0;
								*I2SDmaOtCh01Ptr++=0x100000*i;
							}
						#endif
					}else
					{
						for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
						{
							*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufL[i];	//stream out the audio of conversa Rx output --- L
							*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufR[i];	//stream out the audio of conversa output
							//*I2SDmaOtCh01Ptr++=0x100000*i;
						}
					}
				}

			#endif

			#if 1	//folding --- check button long press to answer or reject an incoming call
				//if((NowInHfpTelCall)||(s_ringTone))					//in App call, button press to answer reject/stop has no effect (after calling the answer reject/stop BT function)
																	//so, intended to check button press only in HfpCall or RingTone --- but later realized that: read the comments below
				if((NowInHfpTelCall)||(NowInHfpAppCall)||(s_ringTone))	//Note!!! when calling out, not able to identify if it is App call or Hfp call. So, no matter what call, check the button long press (answer, reject/stop)
																	//check the beyerdynamic speaker, it is the same!!!
																	//so, we keep the logic of checking button in App call or Hfp call as it is.
				{
					if(codec_inited)  //this means: in ring tone or in the call
					//if(RingToneIsInitialized)
					{
						//check if button 1 is pressed --- answer the call
						if(BtnEvtVarGroup[0].BtnEvt1==BTN_EVT_LONG_PRESS_2)
						{
							if(!NowInHfpTelCall)
								NeedToCall_hfp_AnswerCall=1;
							//hfp_AnswerCall();
							BtnEvtVarGroup[0].BtnEvt1=0;
						}
						//check if button 2 is pressed --- reject the call
						if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_LONG_PRESS_2)
						{
							NeedToCall_hfp_RejectCall=1;
							//hfp_RejectCall();
							BtnEvtVarGroup[1].BtnEvt1=0;
						}
					}
				}
			#endif
    	} else
    	if(MU_U32InfoFromDsp==MuEvtDspToMcu_AudioProcIsFinished_HomeVitStandBy)
    	{
    		OSA_SR_ALLOC();

			#if EnableUsbComAndAudio==1	//folding --- write conversa Tx output audio to UAC up streaming buffer
				OSA_ENTER_CRITICAL();
					//put audio data from DSP side to UacUp cir buffer
					int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
					AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh(&UacUpAudioBuf_MCh);
					AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
					if(UsbUpStreamingIsStarted)
					{
						if (AOFSOfUacUpBuf >= (AudioFrameSizeInSamplePerCh))
						{
							CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, AudioFrameSizeInSamplePerCh, (T_MCh32BitUacUpAudioSample *)VarBlockSharedByDspAndMcu.UacUpAudioBuf);
						}
						else
						{
							#if EnableUacCirBufUnderflowOverFlowPrint==1
								PRINTF("UacUp F\r\n");
							#endif
						}
					}

					//adjust mic upstreaming tx length
					//call adjust mic upstreaming length with AOFS just after writing the cir buffer
					if(UsbUpStreamingIsStarted)
					{
						USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+AudioFrameSizeInSamplePerCh);	//calling adjust just before writing the circular buffer
					}
				OSA_EXIT_CRITICAL();

				#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
					if(UsbUpStreamingIsStarted)
						VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=AODOfUacUpBuf+AudioFrameSizeInSamplePerCh;
				#endif
			#endif
    	}else
    	{
			switch(MU_U32InfoFromDsp)
			{
				case MuEvtDspToMcu_AudioProcIsFinished_AudioIoDbg:
					McuMainAudioFlowFinalize_AudioIoDbg();
					break;
				case MuEvtDspToMcu_AudioProcIsFinished_VideoRecording:
					McuMainAudioFlowFinalize_VideoRecording();
					break;
				case MuEvtDspToMcu_AudioProcIsFinished_MediaPlayer:
					McuMainAudioFlowFinalize_MediaPlayer();
					break;
				case MuEvtDspToMcu_AudioProcIsFinished_MusicPlayer:
					McuMainAudioFlowFinalize_MusicPlayer();
					break;
				case MuEvtDspToMcu_AudioProcIsFinished_Translation:
					McuMainAudioFlowFinalize_Translation();
					break;
				case MuEvtDspToMcu_AudioProcIsFinished_AiConversation:
					McuMainAudioFlowFinalize_AiConversation();
					break;
				case MuEvtDspToMcu_AudioProcIsFinished_VideoAi:
					McuMainAudioFlowFinalize_VideoAi();
					break;
			}
    	}
		DbgPin7Dn();
    }
	AllowAudioInterfaceReInit_PdmI2S=1;
    SDK_ISR_EXIT_BARRIER;
}

__attribute__((section("CodeQuickAccess")))
void ButtonEventProcess(void)
{
	//button evnet process
	if(BtnEvtVarGroup[0].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]++;
		if(VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]>1)
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]=0;

		//on board user button 1 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[0]=1;

		BtnEvtVarGroup[0].BtnEvt1=0;
	}
	if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]++;
		if(VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]>1)
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]=0;

		//on board user button 2 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[1]=1;

		BtnEvtVarGroup[1].BtnEvt1=0;
	}

	if(NeedToCall_hfp_AnswerCall)
	{
		hfp_AnswerCall();
		NeedToCall_hfp_AnswerCall=0;
	}
	if(NeedToCall_hfp_RejectCall)
	{
		hfp_RejectCall();
		NeedToCall_hfp_RejectCall=0;
	}
}

#if EnableVitBeforeTheCall==1
///*
void InitAndStartPdm(void)
{
	//init basic clk and PDM
	InitAudioPLLForAllAudioPeripherals();
	InitBaseAudioClkForPdm();

	DMA_Init(DMA0);
	//we open pdm ports here
	Init_MicDmaCfgCh(0xff);	//mic0,1,2,3,4,5
	BOARD_Init_DMA_PDM(0xff);
	BOARD_Init_DMIC(0xff,0); //0: no skip general Dmic init. If not the first mic init, then should skip.
	ConfigDmicChainedDma(0xff);
	VarBlockSharedByDspAndMcu.BtFs=8000;	//dsp may check this value, need to set it to either 8000 or 16000

	InitAudioCircularBuf();
	PdmInputMuteCnt=12;			//96ms

	//note: DmaTxRxIsExpected is not or ed with AudioI2sPortsBitMapFlag_Fc1 AudioI2sPortsBitMapFlag_Fc3, so after start PDM, fc1 fc3 will not be started
	DmaTxRxIsExpected=(
			//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
					#if EnableMic01==1
						AudioPdmPortsBitMapFlag_Mic01
					#endif
					#if EnableMic23==1
						|AudioPdmPortsBitMapFlag_Mic23
					#endif
					#if EnableMic45==1
						|AudioPdmPortsBitMapFlag_Mic45
					#endif
					#if EnableMic67==1
						|AudioPdmPortsBitMapFlag_Mic67
					#endif
			);

	//start dmic immediately --- but no need to start fc1 fc3 in the PDM callback
	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!
	PRINTF("For VitStandby, PDM ports are configured and started. \r\n");
}
//*/
__attribute__((section("CodeQuickAccess")))
void VitStandBy_Task(void *handle)
{
    OSA_SR_ALLOC();

	SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	PRINTF("RT685 MCU: enter task VitStandBy_Task \r\n");
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

    while (1)
    {
        OSA_SemaphoreWait(xSemaphoreVitAudio, osaWaitForever_c);

		#if 0
			switch(DeviceWorkStateCur)
			{
			case WorkState_HomeVitStandby:
				ProcessAudio_AfterAudioInputBufIsReady_HomeVitStandBy();
				break;
			case WorkState_AudioIoDbg:
				ProcessAudio_AfterAudioInputBufIsReady_AudioIoDbg();
				break;
			case WorkState_VideoRecording:
				ProcessAudio_AfterAudioInputBufIsReady_VideoRecording();
				break;
			case WorkState_MediaPlayer:
				ProcessAudio_AfterAudioInputBufIsReady_MediaPlayer();
				break;
			case WorkState_MusicPlayer:
				ProcessAudio_AfterAudioInputBufIsReady_MusicPlayer();
				break;
			case WorkState_Translation:
				ProcessAudio_AfterAudioInputBufIsReady_Translation();
				break;
			case WorkState_AiConversation:
				ProcessAudio_AfterAudioInputBufIsReady_AiConversation();
				break;
			case WorkState_VideoAi:
				ProcessAudio_AfterAudioInputBufIsReady_VideoAi();
				break;
			}
		#else
			//temp --- only homevit process for now
			switch(DeviceWorkStateCur)
			{
			case WorkState_AudioIoDbg:
				ProcessAudio_AfterAudioInputBufIsReady_AudioIoDbg();
				break;
			default:
				ProcessAudio_AfterAudioInputBufIsReady_HomeVitStandBy();
				break;
			}
		#endif

		#if EnableUsbComAndAudio==1
			USB_DeviceCdcVcomTask();	//can be placed to other place if here is not good (when other new task is needed and created)
		#endif
		ButtonEventProcess();
    }
}
#endif

__attribute__((section("CodeQuickAccess")))
void SCO_Edma_Task(void *handle)
{
    hal_audio_transfer_t xfer;
    OSA_SR_ALLOC();

	#if EnableConversa==1
		SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
		PRINTF("RT685 MCU: enter task SCO_Edma_Task \r\n");
		SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
	#endif

    while (1)
    {
        OSA_SemaphoreWait(xSemaphoreScoAudio, osaWaitForever_c);

    	//DbgPin7Up();

        if(saiEnable == 0)
        {
        	DbgPin8Up();
            Deinit_Board_Audio();
        	DbgPin8Dn();
        	//DbgPin7Dn();

			#if EnableVitBeforeTheCall==1
				InitAndStartPdm();
			#endif
            continue;
        }
        count++;

		#if EnableConversa==1
   			ProcessAudio_AfterAudioInputBufIsReady_InCall();
   		#endif

		#ifdef SCO_DEBUG_MSG
			if (count % 300 == 0)
			{
				PRINTF("@(%d  %d)", emptyMicBlock, emptySpeakerBlock);
				PRINTF("#( %d %d)", rxSpeaker_test, rxMic_test);
			}
		#endif

		#if SCO_SAI_LOOPBACK
			if (atomic_get(&emptySpeakerBlock) > 0)
			{
				xfer.data     = RxAudioBufFromBt + rxSpeaker_index * BUFFER_SIZE;
				xfer.dataSize = BUFFER_SIZE;

				if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_speaker_handle[0], &xfer))
				{
					rxSpeaker_index++;
				}
				if (rxSpeaker_index == BUFFER_NUMBER)
				{
					rxSpeaker_index = 0U;
				}
			}
			if (atomic_get(&emptySpeakerBlock) < BUFFER_NUMBER)
			{
				xfer.data     = RxAudioBufFromBt + txSpeaker_index * BUFFER_SIZE;
				xfer.dataSize = BUFFER_SIZE;

				if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer))
				{
					txSpeaker_index++;
				}
				if (txSpeaker_index == BUFFER_NUMBER)
				{
					txSpeaker_index = 0U;
				}
			}
		#else
			#if EnableConversa==1
				//Spk tx audio data moving is done in fc3Tx or fc1Rx or PDM callback --- calling ProcessAudio_AfterAudioInputBufIsReady_InCall
			#else
				if (atomic_get(&emptyMicBlock) > 0U)
				{
					xfer.data     = MicBuffer + rxMic_index * BUFFER_SIZE;
					xfer.dataSize = BUFFER_SIZE;

					(void)atomic_dec(&emptyMicBlock);
					if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_mic_handle[0], &xfer))
					{
						rxMic_index++;
					}
					if (rxMic_index == BUFFER_NUMBER)
					{
						rxMic_index = 0U;
					}
				}
			#endif

			#if CODEC_SAI_LOOPBACK
				if (atomic_get(&emptyMicBlock) < BUFFER_NUMBER)
				{
					xfer.data     = MicBuffer + txMic_index * BUFFER_SIZE;
					xfer.dataSize = BUFFER_SIZE;

					if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer))
					{
						txMic_index++;
					}
					if (txMic_index == BUFFER_NUMBER)
					{
						txMic_index = 0U;
					}
				}
			#else
				#if EnableConversa==1
					//Spk tx audio data moving is done in fc3Tx or fc1Rx or PDM callback --- calling ProcessAudio_AfterAudioInputBufIsReady_InCall
				#else
				if (atomic_get(&emptySpeakerBlock) > 0U)
				{
					xfer.data     = RxAudioBufFromBt + rxSpeaker_index * BUFFER_SIZE;
					xfer.dataSize = BUFFER_SIZE;

					(void)atomic_dec(&emptySpeakerBlock);
					if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_speaker_handle[0], &xfer))
					{
						rxSpeaker_index++;
					}
					if (rxSpeaker_index == BUFFER_NUMBER)
					{
						rxSpeaker_index = 0U;
					}
				}
				#endif
			#endif
		#endif

		#if EnableUsbComAndAudio==1
			USB_DeviceCdcVcomTask();	//can be placed to other place if here is not good (when other new task is needed and created)
		#endif
		#if UsingQAR87Board == 1
			//ButtonEventProcess..... //B36932
		#else
			ButtonEventProcess();		//this button process can be placed to other place if here is not good (when other new task is needed and created)
			//DbgPin7Dn();
		#endif
    }
}
void sco_audio_init_pl_ext(void)
{
    return;
}
void sco_audio_shutdown_pl_ext(void)
{
	return;
}

API_RESULT sco_audio_setup_pl_ext(SCO_AUDIO_EP_INFO *ep_info)
{
	SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	PRINTF("RT685 MCU: sco_audio_setup_pl_ext, ringtone=%d \r\n", s_ringTone);
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

    sco_audio_setup = 1;
    if (s_ringTone == 0U)
    {
        audio_setup_pl_ext(false, ep_info);
    }else
    {
        //audio_setup_pl_ext(true, ep_info);
        audio_setup_pl_ext(false, ep_info);
        WasInRingTone=1;
    }
    memcpy(&s_ep_info, ep_info, sizeof(SCO_AUDIO_EP_INFO));
    RequestToGetIntoHfp=1;
    return API_SUCCESS;
}

static uint32_t taskCreated = 0;
void StartAudioTask(void)
{
	BaseType_t result = 0;

	#if EnableVitBeforeTheCall==1
		InitAndStartPdm();		//if use this , it init dma again, cause BT firmware downloading fail
	#endif

	if (taskCreated == 0)
	{
		OSA_SemaphoreCreate(xSemaphoreScoAudio, 0);
		result = xTaskCreate(SCO_Edma_Task,   "SCO_Edma",   1024, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
		assert(pdPASS == result);

		#if EnableVitBeforeTheCall==1
			OSA_SemaphoreCreate(xSemaphoreVitAudio, 0);
			result = xTaskCreate(VitStandBy_Task, "VitStandBy", 1024, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
			assert(pdPASS == result);
		#endif

		taskCreated = 1U;
	}
}
#if EnableOnlyMicSpk_NoBT==1
void StartMicSpkTest(void)
{
	PRINTF("Init fc1, fc3, fc5, fc6 and PDM, all work at 16KHz 32bit. \r\n");

	/* Enable clock */
	#if EnableOnlyMicSpk_NoBT==0
		//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
		BOARD_SwitchAudioFreq(16000,0);
	#else
		BOARD_SwitchAudioFreq(Fs_I2SToAmp_MicSpkTest,0);
	#endif

	//enalbe PDM clk
	/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
	CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
	//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
	CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

	DbgPin8Up();
#if UsingQAR87Board == 1
	//Quanta changes CODEC_Init to AMP_Init
	//to do .... //B36932

	codec_inited = 1;
	InitAudioCircularBuf();

	//PDM, fc3, fc1 and chained DMA configuring
	//we open pdm ports here
	Init_MicDmaCfgCh(0xff);	//mic0,1,2,3,4,5
	BOARD_Init_DMA_PDM(0xff);
	BOARD_Init_DMIC(0xff,0); //0: no skip general Dmic init. If not the first mic init, then should skip.
	ConfigDmicChainedDma(0xff);


	BOARD_Init_DMA_I2S_Fc1();
	BOARD_Init_DMA_I2S_Fc3();
		BOARD_Init_I2S_Fc1();
		BOARD_Init_I2S_Fc3();
			ClearDmaBuf_I2S1Rx0();
			ClearDmaBuf_I2S3Tx0();
				ConfigI2S1ChainedDma();
				ConfigI2S3ChainedDma();
					EnableI2S1Rx0DmaChannel();
					EnableI2S3Tx0DmaChannel();
	DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|
			//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
					#if EnableMic01==1
						AudioPdmPortsBitMapFlag_Mic01
					#endif
					#if EnableMic23==1
						|AudioPdmPortsBitMapFlag_Mic23
					#endif
					#if EnableMic45==1
						|AudioPdmPortsBitMapFlag_Mic45
					#endif
					#if EnableMic67==1
						|AudioPdmPortsBitMapFlag_Mic67
					#endif
					);
	
#else
	
	if (CODEC_Init(&codec_handle, &boardCodecScoConfig) != kStatus_Success)
	{
    	DbgPin8Dn();
		PRINTF("Init_Board_Sco_Audio is failed --- CODEC init failure \r\n");
	}else
	{
    	DbgPin8Dn();

    	//Quanta changes the following CODEC operations to AMP operations
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		//CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);

		#if EnableOnlyMicSpk_NoBT==0
			//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
			CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, 16000, 32);
		#else
			CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, Fs_I2SToAmp_MicSpkTest, 32);
		#endif

		CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
		codec_inited = 1;

		InitAudioCircularBuf();

		//PDM, fc3, fc1 and chained DMA configuring
		//we open pdm ports here
		Init_MicDmaCfgCh(0xff);	//mic0,1,2,3,4,5
		BOARD_Init_DMA_PDM(0xff);
		BOARD_Init_DMIC(0xff,0); //0: no skip general Dmic init. If not the first mic init, then should skip.
		ConfigDmicChainedDma(0xff);


		BOARD_Init_DMA_I2S_Fc1();
		BOARD_Init_DMA_I2S_Fc3();
			BOARD_Init_I2S_Fc1();
			BOARD_Init_I2S_Fc3();
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
					ConfigI2S1ChainedDma();
					ConfigI2S3ChainedDma();
						EnableI2S1Rx0DmaChannel();
						EnableI2S3Tx0DmaChannel();
		DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3|
				//AudioPdmPortsBitMapFlag_Mic01|AudioPdmPortsBitMapFlag_Mic23|AudioPdmPortsBitMapFlag_Mic45|AudioPdmPortsBitMapFlag_Mic67
							#if EnableMic01==1
								AudioPdmPortsBitMapFlag_Mic01
							#endif
							#if EnableMic23==1
								|AudioPdmPortsBitMapFlag_Mic23
							#endif
							#if EnableMic45==1
								|AudioPdmPortsBitMapFlag_Mic45
							#endif
							#if EnableMic67==1
								|AudioPdmPortsBitMapFlag_Mic67
							#endif
							);
	}
#endif
	//start dmic immediately, then in dmic intr, fc1,fc3 will be started
	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!
	while(1)
	{
		if(DmaTxRxIsDone==DmaTxRxIsExpected)
		{
			ProcessAudio_AfterAudioInputBufIsReady_TestMicSpk();
		}
		#if EnableUsbComAndAudio==1
			USB_DeviceCdcVcomTask();	//can be placed to other place if here is not good (when other new task is needed and created)
		#endif
	}
}
#endif

API_RESULT sco_audio_start_pl_ext(void)
{
	#if EnableConversa==1
		hal_audio_transfer_t xfer;
		BaseType_t result = 0;

		SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
		PRINTF("RT685 MCU: sco_audio_start_pl_ext \r\n");
		SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

		/*
		if (s_ringTone == 1U)
		{
			return API_SUCCESS;
		}

		if (taskCreated == 0)
		{
			OSA_SemaphoreCreate(xSemaphoreScoAudio, 0);
			result = xTaskCreate(SCO_Edma_Task, "SCO_Edma", 1024, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
			assert(pdPASS == result);
			taskCreated = 1U;
			(void)result;
		}
		*/

		/*
		(void)atomic_set(&emptyMicBlock, 0);
		for (uint8_t index = 0; index < BUFFER_NUMBER; ++index)
		{
			xfer.data     = MicBuffer + rxMic_index * BUFFER_SIZE;
			xfer.dataSize = BUFFER_SIZE;

			if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_mic_handle[0], &xfer))
			{
				rxMic_index++;
			}
			if (rxMic_index == BUFFER_NUMBER)
			{
				rxMic_index = 0U;
			}
		}
		*/

		(void)atomic_set(&emptySpeakerBlock, 0);
		for (uint8_t index = 0; index < BUFFER_NUMBER; ++index)
		{
			xfer.data     = RxAudioBufFromBt + rxSpeaker_index * BUFFER_SIZE;
			xfer.dataSize = BUFFER_SIZE;

			if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_speaker_handle[0], &xfer))
			{
				rxSpeaker_index++;
			}
			if (rxSpeaker_index == BUFFER_NUMBER)
			{
				rxSpeaker_index = 0U;
			}
		}
		//BOARD_SCO_EnableSaiMclkOutput(true);

		/* play dummy data to codec */
		//xfer.dataSize = AUDIO_DUMMY_SIZE;
		//xfer.data     = (uint8_t *)&g_AudioTxDummyBuffer[0];
		//HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer);

		/* play dummy data to 8978 */
		//xfer.dataSize = AUDIO_DUMMY_SIZE;
		//xfer.data     = (uint8_t *)&g_AudioTxDummyBuffer[0];
		memset(OneBlockTxBufToBT,0,sizeof(OneBlockTxBufToBT));
		xfer.dataSize = BUFFER_SIZE;
		xfer.data     = (uint8_t *)OneBlockTxBufToBT;
		HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);

		CirAudioBuf_ClearAllSamples_S16(&BTUpAudioBuf_S16);
		CirAudioBuf_ClearAllSamples_S16(&BTDnAudioBuf_S16);
		//ClearBTUpDnAudioBufDataArea();

		//start dmic immediately, then in dmic intr, fc1,fc3 will be started
		ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!
		PRINTF("sco_audio_start_pl_ext, SCO audio ports are all started \r\n");

		if (s_ringTone == 0U)
		{
			if(WasInRingTone)
			{
				NowInHfpTelCall=1;
				PRINTF("NowInHfpTelCall \r\n");
			}
			else
			{
				NowInHfpAppCall=1;	//Note:!!! when calling out TEL, still arrives here, can not identify if it is an HFP call out
								//Note!!! when calling out, not able to identify if it is App call or Hfp call. So, not matter what call, check the button long press (answer, reject/stop)
								//check the beyerdynamic speaker, it is the same!!!
								//so, we keep the logic of checking button in App call or Hfp call as it is.

				PRINTF("NowInHfpAppCall \r\n");
			}
		}
		else
		{
			//NowInHfpTelCall=1;
			PRINTF("NowInRingTone \r\n");
		}

		return API_SUCCESS;
	#else
		static uint32_t taskCreated = 0;
		hal_audio_transfer_t xfer;
		BaseType_t result = 0;
		if (s_ringTone == 1U)
		{
			return API_SUCCESS;
		}
		if (taskCreated == 0)
		{
			OSA_SemaphoreCreate(xSemaphoreScoAudio, 0);
			result = xTaskCreate(SCO_Edma_Task, "SCO_Edma", 1024, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
			assert(pdPASS == result);
			taskCreated = 1U;
			(void)result;
		}

		(void)atomic_set(&emptyMicBlock, 0);
		for (uint8_t index = 0; index < BUFFER_NUMBER; ++index)
		{
			xfer.data     = MicBuffer + rxMic_index * BUFFER_SIZE;
			xfer.dataSize = BUFFER_SIZE;

			if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_mic_handle[0], &xfer))
			{
				rxMic_index++;
			}
			if (rxMic_index == BUFFER_NUMBER)
			{
				rxMic_index = 0U;
			}
		}

		(void)atomic_set(&emptySpeakerBlock, 0);
		for (uint8_t index = 0; index < BUFFER_NUMBER; ++index)
		{
			xfer.data     = RxAudioBufFromBt + rxSpeaker_index * BUFFER_SIZE;
			xfer.dataSize = BUFFER_SIZE;

			if (kStatus_HAL_AudioSuccess == HAL_AudioTransferReceiveNonBlocking((hal_audio_handle_t)&rx_speaker_handle[0], &xfer))
			{
				rxSpeaker_index++;
			}
			if (rxSpeaker_index == BUFFER_NUMBER)
			{
				rxSpeaker_index = 0U;
			}
		}
		//BOARD_SCO_EnableSaiMclkOutput(true);

		/* play dummy data to codec */
		xfer.dataSize = AUDIO_DUMMY_SIZE;
		xfer.data     = (uint8_t *)&g_AudioTxDummyBuffer[0];
		HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer);

		/* play dummy data to 8978 */
		xfer.dataSize = AUDIO_DUMMY_SIZE;
		xfer.data     = (uint8_t *)&g_AudioTxDummyBuffer[0];
		HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);

		return API_SUCCESS;
	#endif
}

API_RESULT sco_audio_stop_pl_ext(void)
{
	SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	PRINTF("RT685 MCU: sco_audio_stop_pl_ext \r\n");
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

	sco_audio_setup = 0;
	saiEnable = 0;
	//RingToneIsInitialized=0;
	WasInRingTone=0;
	return API_SUCCESS;
}

#ifdef HCI_SCO
void sco_audio_spkr_play_pl_ext(UCHAR *m_data, UINT16 m_datalen)
{
    /* Write to Codec */
}
#endif /* HCI_SCO */
API_RESULT platform_audio_play_ringtone()
{
	cpy_index = 0;
	tx_index  = 0U;
	hal_audio_transfer_t xfer;
	if (sco_audio_setup)
	{
		return API_SUCCESS;
	}
	(void)atomic_set(&emptySpeakerBlock, BUFFER_NUMBER);
	if (s_ringTone == 0)
	{
		if (sco_audio_setup == 1)
		{
			Deinit_Board_Audio();
		}

		SCO_AUDIO_EP_INFO ep_info;
		ep_info.sampl_freq = 16000U;
		ep_info.sample_len = 16U;
		//audio_setup_pl_ext(true, &ep_info);
		audio_setup_pl_ext(false, &ep_info);
		s_ringTone = 1;
	}

	#if EnableConversa==0
		memset(RxAudioBufFromBt, 0x0, BUFFER_NUMBER * BUFFER_SIZE);
		if ((atomic_get(&emptySpeakerBlock) > 0U) && (cpy_index < MUSIC_LEN / BUFFER_SIZE))
		{
			/* Fill in the buffers. */
			memcpy((uint8_t *)&RxAudioBufFromBt[BUFFER_SIZE * (cpy_index % BUFFER_NUMBER)],
				   (uint8_t *)&music[cpy_index * BUFFER_SIZE], sizeof(uint8_t) * BUFFER_SIZE);
			(void)atomic_dec(&emptySpeakerBlock);
			cpy_index++;
		}

		if (atomic_get(&emptySpeakerBlock) < BUFFER_NUMBER)
		{
			/*  xfer structure */
			xfer.data     = (uint8_t *)&RxAudioBufFromBt[BUFFER_SIZE * (tx_index % BUFFER_NUMBER)];
			xfer.dataSize = BUFFER_SIZE;
			/* Wait for available queue. */
			if (kStatus_HAL_AudioSuccess == HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_speaker_handle[0], &xfer))
			{
				tx_index++;
			}
			(void)atomic_inc(&emptySpeakerBlock);
		}
	#endif
	return API_SUCCESS;
}

API_RESULT sco_audio_set_speaker_volume(UCHAR volume)
{
    if (sco_audio_setup == 0)
    {
        return API_FAILURE;
    }
    /* HFP support 0- 15, codec support 0-100*/
	#if UsingQAR87Board == 1
	//Codec or Amplifier configuration
	//to do ....
	#else
    if (kStatus_Success == CODEC_SetVolume(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, ((volume * 6U) + 9U)))
    {
        return API_SUCCESS;
    }
	#endif
    return API_FAILURE;
}
void sco_audio_play_ringtone_pl_ext(void)
{
    platform_audio_play_ringtone();
	PRINTF("platform_audio_play_ringtone, ring tone playing is started \r\n");
}
void sco_audio_play_ringtone_exit_pl_ext(void)
{
    if (s_ringTone == 1)
    {
        Deinit_Board_Audio();
        //memset(RxAudioBufFromBt, 0x0, BUFFER_NUMBER * BUFFER_SIZE);
        s_ringTone = 0U;
        if (sco_audio_setup == 1)
        {
        	PRINTF("sco_audio_play_ringtone_exit_pl_ext, ring tone playing is finished \r\n");
            audio_setup_pl_ext(false, &s_ep_info);
            sco_audio_start_pl_ext();
        }
    }
}
