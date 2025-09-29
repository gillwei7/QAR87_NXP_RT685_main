/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "qar87_config.h"

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
#if !PIN_CONFIG_DEV_BOARD
extern codec_config_t boardCodecScoConfig;
extern codec_config_t boardCodecScoConfig1;
#endif

extern hal_audio_config_t txSpeakerConfig;
extern hal_audio_config_t rxMicConfig;
extern hal_audio_config_t txMicConfig;
extern hal_audio_config_t rxSpeakerConfig;

/* --------------------------------------------- Exported Global Variables */

/* --------------------------------------------- Static Global Variables */

extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_speaker_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_mic_handle), 4);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_mic_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_speaker_handle), 4);
static codec_handle_t codec_handle;
#if !DEV_AUDIO_DEBUG_GPIO
static uint8_t codec_inited = 0;
#endif

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t MicBuffer    [BUFFER_NUMBER * BUFFER_SIZE], 4);
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t SpeakerBuffer[BUFFER_NUMBER * BUFFER_SIZE], 4);
AT_NONCACHEABLE_SECTION(uint32_t g_AudioTxDummyBuffer[AUDIO_DUMMY_SIZE / 4U]);

OSA_SEMAPHORE_HANDLE_DEFINE(xSemaphoreScoAudio);

#if EnableConversa==1
int AOD_BTDnBuf;
int AOD_BTUpBuf;
int BTAudioBitWidth;
int BTAudioFs;
AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t OneBlockTxBufToBT    [BUFFER_SIZE], 4);
S16 TmpDbgSigalBuf[BUFFER_SIZE/2];
void SCO_AudioFlow_SemaphorePost(void)
{
	OSA_SemaphorePost(xSemaphoreScoAudio);
}
#endif

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
    static volatile uint8_t s_8978ConsumerActualData = 0;
    hal_audio_transfer_t xfer;
    if (s_ringTone == 1U)
    {
    	//when ring tone is playing, all zeros should go to BT up streaming (mic tx)
        s_8978ConsumerActualData = 0;
        xfer.dataSize            = AUDIO_DUMMY_SIZE;
        xfer.data                = (uint8_t *)&g_AudioTxDummyBuffer[0];
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
}
#endif

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
			memcpy((uint8_t *)&SpeakerBuffer[BUFFER_SIZE * (cpy_index % BUFFER_NUMBER)],
				   (uint8_t *)&music[cpy_index * BUFFER_SIZE], sizeof(uint8_t) * BUFFER_SIZE);
			(void)atomic_dec(&emptySpeakerBlock);
			cpy_index++;
		}

		if (atomic_get(&emptySpeakerBlock) < BUFFER_NUMBER)
		{
			//emptySpeakerBlock < BUFFER_NUMB
			//means occupied space > 0, means there is at least one block of audio to send
			/*  xfer structure */
			xfer.data     = (uint8_t *)&SpeakerBuffer[BUFFER_SIZE * (tx_index % BUFFER_NUMBER)];
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
				xfer.data            = SpeakerBuffer + txSpeaker_index * BUFFER_SIZE;
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
				xfer.data            = SpeakerBuffer + txSpeaker_index * BUFFER_SIZE;
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

static void rxSpeakerCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
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

				xfer.data     = SpeakerBuffer + rxSpeaker_index * BUFFER_SIZE;
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
				if(AOD_BTDnBuf<(BTDnAudioBuf_S16.LengthInSamples-BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8)))
				{
					//there is at least 1 free frame space in the cir buffer
					#if 0
						GenerateSinWav_S16_SingleCh(&TmpDbgSigalBuf[0*128], 128);
						GenerateSinWav_S16_SingleCh(&TmpDbgSigalBuf[1*128], 128);
						GenerateSinWav_S16_SingleCh(&TmpDbgSigalBuf[2*128], 128);
						GenerateSinWav_S16_SingleCh(&TmpDbgSigalBuf[3*128], 128);
						CirAudioBuf_WriteSamples_S16(&BTDnAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), TmpDbgSigalBuf);
					#else
						//CirAudioBuf_WriteSamples_S16(&BTDnAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), (S16 *)(SpeakerBuffer + (1-rxSpeaker_index) * BUFFER_SIZE));		//buffer A or B select, select the other one by 1-rxSpeaker_index
						CirAudioBuf_WriteSamples_S16(&BTDnAudioBuf_S16, BUFFER_SIZE/(kHAL_AudioWordWidth16bits/8), (S16 *)(SpeakerBuffer + rxSpeaker_index * BUFFER_SIZE));
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
}

static void Deinit_Board_Audio(void)
{
	#if EnableConversa==1
#if !DEV_AUDIO_DEBUG_GPIO
		if (codec_inited == 0)
		{
			return ;
		}
#endif
//		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);

		//close all I2S and related DMA --- if need to just close the wanted, just call one of the 3 grouped functions
		CloseI2sDma((I2S_Type *)DEMO_I2S1Rx0);
		CloseI2sDma((I2S_Type *)DEMO_I2S3Tx0);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2S1Rx0);
			CloseI2sAndI2sIntr((I2S_Type *)DEMO_I2S3Tx0);
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
		//close PDM all channels
		BOARD_DeInit_DMA_PDM(0xff);

		HAL_AudioTxDeinit((hal_audio_handle_t)&tx_mic_handle[0]);
		HAL_AudioRxDeinit((hal_audio_handle_t)&rx_speaker_handle[0]);

		(void)BOARD_SwitchAudioFreq(0U);
#if !DEV_AUDIO_DEBUG_GPIO
		codec_inited = 0;
#endif
		PRINTF("Deinit_Board_Audio is done \r\n");
	#else
		if (codec_inited == 0)
		{
			return ;
		}
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		HAL_AudioTxDeinit((hal_audio_handle_t)&tx_speaker_handle[0]);
		HAL_AudioRxDeinit((hal_audio_handle_t)&rx_mic_handle[0]);
		HAL_AudioTxDeinit((hal_audio_handle_t)&tx_mic_handle[0]);
		HAL_AudioRxDeinit((hal_audio_handle_t)&rx_speaker_handle[0]);
		(void)BOARD_SwitchAudioFreq(0U);
		codec_inited = 0;
	#endif
}

/*Initialize sco audio interface and codec.*/
static void Init_Board_Sco_Audio(uint32_t samplingRate, UCHAR bitWidth)
{
    uint32_t src_clk_hz;
	#if EnableConversa==1
		if (samplingRate > 0U)
		{
		    // Do not print anything before Audio PLL and Debug console reset
			///PRINTF("Init Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

			/* Enable clock */
	    	//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
			src_clk_hz = BOARD_SwitchAudioFreq(16000);

			//enalbe PDM clk
			/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
			CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
	    	//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
			CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

			BTAudioBitWidth=bitWidth;
			PRINTF("Init_Board_Sco_Audio sampling rate:%d,bitwidth:%d\r\n",samplingRate,bitWidth);
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
#if !DEV_AUDIO_DEBUG_GPIO
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
#endif
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
				PRINTF("Init_Board_Sco_Audio is successful and finished \r\n");
#if !DEV_AUDIO_DEBUG_GPIO
			}
#endif
		}
	#else
		if (samplingRate > 0U)
		{
			PRINTF("Init Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

			/* Enable clock */
			src_clk_hz = BOARD_SwitchAudioFreq(samplingRate);

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
static void Init_Board_RingTone_Audio(uint32_t samplingRate, UCHAR bitWidth)
{
    uint32_t src_clk_hz;

    if (samplingRate > 0U)
    {
        // Do not print anything before Audio PLL and Debug console reset
		//PRINTF("Init RingTone Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

        /* Enable clock */
        src_clk_hz = BOARD_SwitchAudioFreq(samplingRate);

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
		PRINTF("Init_Board_RingTone_Audio finished \r\n");
#if !DEV_AUDIO_DEBUG_GPIO
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
}

static API_RESULT audio_setup_pl_ext(uint8_t isRing, SCO_AUDIO_EP_INFO *ep_info)
{
    txMic_index     = 0U;
    rxMic_index     = 0U;
    txSpeaker_index = 0U, rxSpeaker_index = 0U;
    (void)atomic_set(&emptySpeakerBlock, BUFFER_NUMBER);
    (void)atomic_set(&emptyMicBlock, BUFFER_NUMBER);
    if (isRing)
    {
        Init_Board_RingTone_Audio(ep_info->sampl_freq, ep_info->sample_len);
    }
    else
    {
        Init_Board_Sco_Audio(ep_info->sampl_freq, ep_info->sample_len);
    }
    saiEnable = 1;
    return API_SUCCESS;
}

static uint32_t count = 0;

extern U32 AudioIoFrameCnt;
extern volatile S32 *I2SDmaOtCh01Ptr;
extern int UsbAudioUpstreamLengthAdjusted;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;
extern void USB_MicUpStreamDataRateControl_AdjustPacketLength(int AodInCirBuf);
extern void USB_DeviceCdcVcomTask(void);
volatile U32 MU_U32InfoFromDsp;
__attribute__((section("CodeQuickAccess")))
void APP_MU_IRQHandler(void)
{
	int i;
    uint32_t flag = 0;
	S16 TmpAudioS16Buf[AudioFrameSizeInSamplePerCh];

    flag = MU_GetStatusFlags(APP_MU);
    if ((flag & kMU_Rx0FullFlag) == kMU_Rx0FullFlag)
    {
    	MU_U32InfoFromDsp = MU_ReceiveMsgNonBlocking(APP_MU, CHN_MU_REG_NUM);

		DbgPin7Up();
    	if(MU_U32InfoFromDsp==EvtFlag_DspProcessingIsFiinished)
    	{
    		OSA_SR_ALLOC();


			#if 1	//folding --- write BT cir buffer
				if(VarBlockSharedByDspAndMcu.BtFs==8000)
				{
					#if 0
						OSA_ENTER_CRITICAL();
						//take audio samples out from BT Dn buffer --- put to BTRxInAudio, and then DSP side will take in and process
						if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/2))
						{
							//there are enough audio sample from BT down streaming
							CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh/2, TmpAudioS16Buf);
							for(i=0;i<AudioFrameSizeInSamplePerCh/2;i++)
								VarBlockSharedByDspAndMcu.BTRxInAudio[i]=(TmpAudioS16Buf[i]<<16);
						}else
						{
							//not enough audio samples from BT down streaming
							//this should not happen when audio PLL sync is doing well
							PRINTF("BT Dn CirBuf is E \r\n");
							memset(VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio)/2);
						}
						OSA_EXIT_CRITICAL();
					#endif

					#if 1
						OSA_ENTER_CRITICAL();
						//put audio samples into from BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
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
							PRINTF("BT Up CirBuf is F \r\n");
						}
						OSA_EXIT_CRITICAL();
					#endif
				}else
				if(VarBlockSharedByDspAndMcu.BtFs==16000)
				{
					#if 0
						OSA_ENTER_CRITICAL();
						//take audio samples out from BT Dn buffer --- put to BTRxInAudio, and then DSP side will take in and process
						if(CirAudioBuf_SpaceOccupiedInSamples_S16(&BTDnAudioBuf_S16)>=(AudioFrameSizeInSamplePerCh/1))
						{
							//there are enough audio sample from BT down streaming
							CirAudioBuf_ReadSamples_S16(&BTDnAudioBuf_S16, AudioFrameSizeInSamplePerCh/1, TmpAudioS16Buf);
							for(i=0;i<AudioFrameSizeInSamplePerCh/1;i++)
								VarBlockSharedByDspAndMcu.BTRxInAudio[i]=(TmpAudioS16Buf[i]<<16);
						}else
						{
							//not enough audio samples from BT down streaming
							//this should not happen when audio PLL sync is doing well
							PRINTF("BT Dn CirBuf is E \r\n");
							memset(VarBlockSharedByDspAndMcu.BTRxInAudio,0,sizeof(VarBlockSharedByDspAndMcu.BTRxInAudio));
						}
						OSA_EXIT_CRITICAL();
					#endif

					#if 1
						OSA_ENTER_CRITICAL();
						//put audio samples into from BT Up buffer --- BTTxOtAudio has the processed audio of the previous frame
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
							PRINTF("BT Up CirBuf is F \r\n");
						}
						OSA_EXIT_CRITICAL();
					#endif
				}else
				{
					//should never come here
					PRINTF("MCU: audio flow error --- BT fs is not 8Khz or 16KHz \r\n");
				}
			#endif



    		OSA_ENTER_CRITICAL();
				//put audio data from DSP side to UacUp cir buffer
				int AOFSOfUacUpBuf,AODOfUacUpBuf;	//amount of free space, amount of data
				AOFSOfUacUpBuf=CirUacUpAudioBuf_SpaceAvailableInSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh);
				AODOfUacUpBuf=UacUpAudioBuf_MCh.LengthInSamples-AOFSOfUacUpBuf;
				if(UsbUpStreamingIsStarted)
				{
					if (AOFSOfUacUpBuf >= (AudioFrameSizeInSamplePerCh))
					{
						CirUacUpAudioBuf_WriteSamples_MultiCh((T_CirUacUpAudioBuf_MCh *)&UacUpAudioBuf_MCh, AudioFrameSizeInSamplePerCh, (T_MCh32BitUacUpAudioSample *)VarBlockSharedByDspAndMcu.UacUpAudioBuf);
					}
					else
					{
						//PRINTF("F\r\n");
					}
				}

				//adjust mic upstreaming tx length
				//call adjust mic upstreaming length with AOFS just after writing the cir buffer
				if(UsbUpStreamingIsStarted)
				{
					USB_MicUpStreamDataRateControl_AdjustPacketLength(AODOfUacUpBuf+AudioFrameSizeInSamplePerCh);	//calling adjust just before writing the circular buffer
				}
			OSA_EXIT_CRITICAL();

			#if 1
				//copy I2S output buffer (line out to DAC) from shared memory to DMA buffer, I2S output buffer are just processed and written by DSP
				for(int i=0;i<AudioFrameSizeInSamplePerCh;i++)
				{
					*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufL[i];	//stream out the audio of the previous processed audio block
					*I2SDmaOtCh01Ptr++=VarBlockSharedByDspAndMcu.I2SLineOtBufR[i];	//stream out the audio of the previous processed audio block
				}
			#endif

			#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
				if(UsbUpStreamingIsStarted)
					VarBlockSharedByDspAndMcu.MonitorInfoArray1[8]=AODOfUacUpBuf+AudioFrameSizeInSamplePerCh;
			#endif

    	}
		DbgPin7Dn();
    }
    SDK_ISR_EXIT_BARRIER;
}

__attribute__((section("CodeQuickAccess")))
void ButtonEventProcess(void)
{
	//button evnet process
	if(BtnEvtVarGroup[0].BtnEvt1==1)
	{
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]++;
		if(VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]>1)
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]=0;

		//on board user button 1 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[0]=1;

		BtnEvtVarGroup[0].BtnEvt1=0;
	}
	if(BtnEvtVarGroup[1].BtnEvt1==1)
	{
		VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]++;
		if(VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]>1)
			VarBlockSharedByDspAndMcu.MonitorInfoArray1[0]=0;

		//on board user button 2 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[1]=1;

		BtnEvtVarGroup[1].BtnEvt1=0;
	}
}
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
            continue;
        }
        count++;

		#if EnableConversa==1
   			ProcessAudio_AfterAudioInputBufIsReady();
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
				xfer.data     = SpeakerBuffer + rxSpeaker_index * BUFFER_SIZE;
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
				xfer.data     = SpeakerBuffer + txSpeaker_index * BUFFER_SIZE;
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
				//Spk tx audio data moving is done in fc3Tx or fc1Rx or PDM callback --- calling ProcessAudio_AfterAudioInputBufIsReady
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
					//Spk tx audio data moving is done in fc3Tx or fc1Rx or PDM callback --- calling ProcessAudio_AfterAudioInputBufIsReady
				#else
				if (atomic_get(&emptySpeakerBlock) > 0U)
				{
					xfer.data     = SpeakerBuffer + rxSpeaker_index * BUFFER_SIZE;
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

		USB_DeviceCdcVcomTask();	//can be placed to other place if here is not good (when other new task is needed and created)
#if !DEV_AUDIO_DEBUG_GPIO
		ButtonEventProcess();		//this button process can be placed to other place if here is not good (when other new task is needed and created)
#endif
		//DbgPin7Dn();
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
    sco_audio_setup = 1;
    if (s_ringTone == 0U)
    {
        audio_setup_pl_ext(false, ep_info);
    }
    memcpy(&s_ep_info, ep_info, sizeof(SCO_AUDIO_EP_INFO));
    return API_SUCCESS;
}

API_RESULT sco_audio_start_pl_ext(void)
{
	#if EnableConversa==1
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
			xfer.data     = SpeakerBuffer + rxSpeaker_index * BUFFER_SIZE;
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
		xfer.dataSize = AUDIO_DUMMY_SIZE;
		xfer.data     = (uint8_t *)&g_AudioTxDummyBuffer[0];
		HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);

		//start dmic immediately, then in dmic intr, fc1,fc3 will be started
		ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!
		PRINTF("sco_audio_start_pl_ext, SCO audio ports are all started \r\n");

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
			xfer.data     = SpeakerBuffer + rxSpeaker_index * BUFFER_SIZE;
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
	sco_audio_setup = 0;
	saiEnable = 0;
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
		audio_setup_pl_ext(true, &ep_info);
		s_ringTone = 1;
	}

	memset(SpeakerBuffer, 0x0, BUFFER_NUMBER * BUFFER_SIZE);
	if ((atomic_get(&emptySpeakerBlock) > 0U) && (cpy_index < MUSIC_LEN / BUFFER_SIZE))
	{
		/* Fill in the buffers. */
		memcpy((uint8_t *)&SpeakerBuffer[BUFFER_SIZE * (cpy_index % BUFFER_NUMBER)],
			   (uint8_t *)&music[cpy_index * BUFFER_SIZE], sizeof(uint8_t) * BUFFER_SIZE);
		(void)atomic_dec(&emptySpeakerBlock);
		cpy_index++;
	}
	#if EnableConversa==0
		if (atomic_get(&emptySpeakerBlock) < BUFFER_NUMBER)
		{
			/*  xfer structure */
			xfer.data     = (uint8_t *)&SpeakerBuffer[BUFFER_SIZE * (tx_index % BUFFER_NUMBER)];
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
    if (kStatus_Success == CODEC_SetVolume(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, ((volume * 6U) + 9U)))
    {
        return API_SUCCESS;
    }

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
        memset(SpeakerBuffer, 0x0, BUFFER_NUMBER * BUFFER_SIZE);
        s_ringTone = 0U;
        if (sco_audio_setup == 1)
        {
        	PRINTF("sco_audio_play_ringtone_exit_pl_ext, ring tone playing is finished \r\n");
            audio_setup_pl_ext(false, &s_ep_info);
            sco_audio_start_pl_ext();
        }
    }
}
