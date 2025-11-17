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

#if UseEventToControlBtHfp==1
	EventGroupHandle_t EvtGrpHdl_StateMangerTaskToBtStack;
#endif
extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_speaker_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_mic_handle), 4);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_mic_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_speaker_handle), 4);
static codec_handle_t codec_handle;
uint8_t codec_inited = 0;

//AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t MicBuffer    [BUFFER_NUMBER * BUFFER_SIZE], 4);
AT_NONCACHEABLE_SECTION_ALIGN(uint8_t RxAudioBufFromBt[BUFFER_NUMBER * BUFFER_SIZE], 4);
//AT_NONCACHEABLE_SECTION(uint32_t g_AudioTxDummyBuffer[AUDIO_DUMMY_SIZE / 4U]);

OSA_SEMAPHORE_HANDLE_DEFINE(xSemaphoreScoAudio);
OSA_SEMAPHORE_HANDLE_DEFINE(xSemaphoreDmaAudioDataReady);

//static volatile uint8_t RingToneIsInitialized=0;
static volatile uint8_t NowInHfpTelCall=0;
static volatile uint8_t NowInHfpAppCall=0;
static volatile uint8_t WasInRingTone=0;
static volatile uint8_t saiEnable= 0;
static uint32_t txMic_index = 0U, rxMic_index = 0U;
atomic_t emptyMicBlock = BUFFER_NUMBER;
static uint32_t txSpeaker_index = 0U, rxSpeaker_index = 0U;
atomic_t emptySpeakerBlock = BUFFER_NUMBER;
//static uint32_t rxSpeaker_test = 0U, rxMic_test = 0U;
volatile uint8_t NowInIncomingCallRingTone = 0;
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
#endif

int BtHfpAudioVolume;
int BtHfpAudioFs;
int BtHfpAudioBitWidth;

int AOD_BTDnBuf;
int AOD_BTUpBuf;

AT_NONCACHEABLE_SECTION_ALIGN(static uint8_t OneBlockTxBufToBT    [BUFFER_SIZE], 4);
S16 TmpDbgSigalBuf[BUFFER_SIZE/2];
void SCO_AudioFlow_SemaphorePost(void)
{
	if((NowInHfpTelCall)||(NowInHfpAppCall)||(NowInIncomingCallRingTone))
	{
		OSA_SemaphorePost(xSemaphoreScoAudio);
	}else
	{
		OSA_SemaphorePost(xSemaphoreDmaAudioDataReady);
	}
}

static void txMicCallback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    AllowAudioInterfaceReInit_Fc25=0;
    static volatile uint8_t s_8978ConsumerActualData = 0;
    hal_audio_transfer_t xfer;
	if (NowInIncomingCallRingTone == 1U)
    {
    	//when ring tone is playing, all zeros should go to BT up streaming (mic tx)
		s_8978ConsumerActualData = 0;
		memset(OneBlockTxBufToBT,0,sizeof(OneBlockTxBufToBT));
		xfer.dataSize            = BUFFER_SIZE;
		xfer.data                = (uint8_t *)OneBlockTxBufToBT;
        HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&tx_mic_handle[0], &xfer);
    }
    else
    {
    	DbgPin5Up();
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

		/*
		//blink blue LED
		static int BluLedBlinkCnt=0;
		if(BluLedBlinkCnt++%16 < 8)	//64*16=1.024s
		{
			LedOff_B();
		}else
		{
			LedOn_B();
		}
		*/
		DbgPin5Dn();
    }
    AllowAudioInterfaceReInit_Fc25=1;
}

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
        //rxSpeaker_test++;
    	DbgPin6Dn();
    }
    AllowAudioInterfaceReInit_Fc25=1;
}

extern void ClearBTUpDnAudioBufDataArea(void);

void DeinitHfpVariables(void)
{
	NowInHfpTelCall=0;
	NowInHfpAppCall=0;
	NowInIncomingCallRingTone=0;
	WasInRingTone=0;

	CirAudioBuf_ClearAllSamples_S16(&BTUpAudioBuf_S16);
	CirAudioBuf_ClearAllSamples_S16(&BTDnAudioBuf_S16);

	//this is necessary, or there is extra sound in the beginning of the next call
	memset(RxAudioBufFromBt, 0, sizeof(RxAudioBufFromBt));
}

#if UsingQAR87Board == 1

void DeInitCodec(void)
{
	int r;

	if(AmpState==AmpState_UnConfigured)
		return;

	//to do...... deinit codec //B36932
	//r = deinit codec
	//Not sure if "stop" same with Deinit //B36932
	hal_amp_aw88166_left_stop();
	hal_amp_aw88166_right_stop();
	
	//if(r!=kStatus_Success)
	//{
	//	PRINTF("DeInitCodec is failed, %d \r\n",r);
	//	return;
	//}
	AmpState=AmpState_UnConfigured;
}
void InitAndStartCodec(int fs, int bits, int Mfreq)
//InitAndStartAmp
{
	int r;

	if(AmpState==AmpState_ConfiguredAndActive)
		return;

	//to do...... initial or start smart amplifier //B36932
	//r = initial codec or start codec
	if(fs==48000)
	{
		hal_amp_aw88166_left_start ("Music");
		hal_amp_aw88166_right_start("Music");
	}else
	if(fs==16000)
	{
		hal_amp_aw88166_left_start ("Receiver");
		hal_amp_aw88166_right_start("Receiver");
	}
	codec_inited = 1;

	AmpState=AmpState_ConfiguredAndActive;
}

#else
void DeInitCodec(void)
{
	int r;

	if(AmpState==AmpState_UnConfigured)
		return;

	r=CODEC_Deinit(&codec_handle);
	if(r!=kStatus_Success)
	{
		PRINTF("DeInitCodec is failed, %d \r\n",r);
		return;
	}
	AmpState=AmpState_UnConfigured;
}
void InitAndStartCodec(int fs, int bits, int Mfreq)
{
	int r;

	if(AmpState==AmpState_ConfiguredAndActive)
		return;

	DbgPin8Up();
	((wm8904_config_t *)boardCodecScoConfig.codecDevConfig)->mclk_HZ=Mfreq;

	r=CODEC_Init(&codec_handle, &boardCodecScoConfig);

	if (r!= kStatus_Success)
	{
		PRINTF("InitAndStartCodec is failed, %d \r\n",r);
		return;
	}else
	{
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		//CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);
		CODEC_SetFormat(&codec_handle, txSpeakerConfig.srcClock_Hz, fs, bits);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
		codec_inited = 1;
	}
	DbgPin8Dn();
	AmpState=AmpState_ConfiguredAndActive;
}
#endif

void InitHfpAudioIntfToBT(int MclkFreq, int Fs)
{
	//this part keeps the same as the original demo setup
	txMicConfig.srcClock_Hz   = MclkFreq;
	txMicConfig.sampleRate_Hz = Fs;
	HAL_AudioTxInit((hal_audio_handle_t)&tx_mic_handle[0], &txMicConfig);
	HAL_AudioTxInstallCallback((hal_audio_handle_t)&tx_mic_handle[0], txMicCallback, NULL);

	//this part keeps the same as the original demo setup
	rxSpeakerConfig.srcClock_Hz   = MclkFreq;
	rxSpeakerConfig.sampleRate_Hz = Fs;
	HAL_AudioRxInit((hal_audio_handle_t)&rx_speaker_handle[0], &rxSpeakerConfig);
	HAL_AudioRxInstallCallback((hal_audio_handle_t)&rx_speaker_handle[0], rxSpeakerCallback, NULL);
}

extern void Deinit_GeneralAudio(int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec);
void Deinit_Board_Audio(void)
{
#if 1
	Deinit_GeneralAudio(1,0,1,1);	//int ToDeinitAmpI2S, int ToDeinitNvtI2S, int ToDeinitPdm, int ToDeinitCodec
	ClearAudioCirBuf(1,1,0);		//int ToClrBtCir, int ToClrUacCir,  int ToClrSbcCir
	DeinitHfpVariables();
	return;
#else



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
	NowInIncomingCallRingTone=0;
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
#endif
	PRINTF("Deinit_Board_Audio is done \r\n");
}

/*Initialize sco audio interface and codec.*/
static void Init_Board_Sco_Audio(uint32_t samplingRate, UCHAR bitWidth)
{
    uint32_t src_clk_hz;

#if 1
    AudioPortIsActive_Pdm=0;
    AudioPortIsActive_I2SToAmp=0;
    AmpState==AmpState_UnConfigured;
    InitAudioInterface_HfpCall(0, samplingRate, bitWidth);
    return;
#else

	//configure PDM, Fc1,Fc3, CODEC, and configure Fc2 Fc5
	if (samplingRate > 0U)
	{
		PRINTF("Init Audio SCO SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

		/* Enable clock */
		//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
		#if UsingQAR87Board == 1
			src_clk_hz = BOARD_SwitchAudioFreq(16000,BtPcmFc2Fc4_AmpFc1Fc3);
		#else
			src_clk_hz = BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
		#endif
		//enalbe PDM clk
		/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
		CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
		//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
		CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)

		BtHfpAudioBitWidth=bitWidth;
		if(BtHfpAudioBitWidth!=16)
		{
			PRINTF("Init_Board_Sco_Audio error --- BT bit width is NOT 16 \r\n");
		}
		BtHfpAudioFs=samplingRate;
		if((BtHfpAudioFs!=8000)&&(BtHfpAudioFs!=16000))
		{
			PRINTF("Init_Board_Sco_Audio error --- BT bit Fs is NOT 8kHz or 16kHz \r\n");
		}
		VarBlockSharedByDspAndMcu.BtHfpFs=BtHfpAudioFs;

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

#if UsingQAR87Board == 1
		//initial codec/amplifier
		//B36932, to do....

		//initial audio buffer and dmic and I2S

		InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

		//PDM, fc3, fc1 and chained DMA configuring
		//we open pdm ports here
		Init_MicDmaCfgCh(0xff,AudioFrameSizeInSamplePerCh_16KHz,32);	//mic0,1,2,3,4,5
		BOARD_Init_DMA_PDM(0xff);
		BOARD_Init_DMIC(0xff,0,16000); //0: no skip general Dmic init. If not the first mic init, then should skip.
		ConfigDmicChainedDma(0xff);

		BOARD_Init_DMA_I2S_Fc1();
		BOARD_Init_DMA_I2S_Fc3();
		BOARD_Init_I2S_Fc1(16000,16);
		BOARD_Init_I2S_Fc3(16000,16);
		ClearDmaBuf_I2S1Rx0();
		ClearDmaBuf_I2S3Tx0();
		ConfigI2S1ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,16);
		ConfigI2S3ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,16);
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
		codec_inited = 1;
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

			InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

			//PDM, fc3, fc1 and chained DMA configuring
			//we open pdm ports here
			Init_MicDmaCfgCh(0xff,AudioFrameSizeInSamplePerCh_16KHz,32);	//mic0,1,2,3,4,5
			BOARD_Init_DMA_PDM(0xff);
			BOARD_Init_DMIC(0xff,0,16000); //0: no skip general Dmic init. If not the first mic init, then should skip.
			ConfigDmicChainedDma(0xff);


			BOARD_Init_DMA_I2S_Fc1();
			BOARD_Init_DMA_I2S_Fc3();
				BOARD_Init_I2S_Fc1(16000,32);
				BOARD_Init_I2S_Fc3(16000,32);
					ClearDmaBuf_I2S1Rx0();
					ClearDmaBuf_I2S3Tx0();
						ConfigI2S1ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,32);
						ConfigI2S3ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,32);
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
#endif

}
static void Init_Board_RingTone_Audio(uint32_t samplingRate, UCHAR bitWidth)	//never be called
{
    uint32_t src_clk_hz;
	//configure Fc1,Fc3, CODEC
	if (samplingRate > 0U)
	{
		PRINTF("Init Audio RingTone SAI and CODEC samplingRate :%d  bitWidth:%d \r\n", samplingRate, bitWidth);

		/* Enable clock */
		//no matter BT side is 16KHz or 8KHz, CODEC is always 16KHz
		#if UsingQAR87Board == 1
			src_clk_hz = BOARD_SwitchAudioFreq(16000,BtPcmFc2Fc4_AmpFc1Fc3);
		#else
			src_clk_hz = BOARD_SwitchAudioFreq(16000,BtPcmFc5Fc2_CodecFc1Fc3);
		#endif
		
		BtHfpAudioBitWidth=bitWidth;
		if(BtHfpAudioBitWidth!=16)
		{
			PRINTF("Init_Board_Sco_Audio error --- BT bit width is NOT 16 \r\n");
		}
		BtHfpAudioFs=samplingRate;
		if((BtHfpAudioFs!=8000)&&(BtHfpAudioFs!=16000))
		{
			PRINTF("Init_Board_Sco_Audio error --- BT bit Fs is NOT 8kHz or 16kHz \r\n");
		}
		VarBlockSharedByDspAndMcu.BtHfpFs=BtHfpAudioFs;

		//DbgPin8Up();

#if UsingQAR87Board == 1
		//initial codec/amplifier
		//B36932, to do....

		//initial audio buffer and dmic and I2S
		InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

		//fc3, fc1 and chained DMA configuring
		BOARD_Init_DMA_I2S_Fc1();
		BOARD_Init_DMA_I2S_Fc3();
			BOARD_Init_I2S_Fc1(16000,32);
			BOARD_Init_I2S_Fc3(16000,32);
				ClearDmaBuf_I2S1Rx0();
				ClearDmaBuf_I2S3Tx0();
					ConfigI2S1ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,32);
					ConfigI2S3ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,32);
						EnableI2S1Rx0DmaChannel();
						EnableI2S3Tx0DmaChannel();
		DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3);
		hal_amp_aw88166_left_start("Receiver");
		hal_amp_aw88166_right_start("Receiver");
		codec_inited = 1;
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

			InitAudioCircularBuf(1,1,0);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir

			//fc3, fc1 and chained DMA configuring
			BOARD_Init_DMA_I2S_Fc1();
			BOARD_Init_DMA_I2S_Fc3();
				BOARD_Init_I2S_Fc1(16000,32);
				BOARD_Init_I2S_Fc3(16000,32);
					ClearDmaBuf_I2S1Rx0();
					ClearDmaBuf_I2S3Tx0();
						ConfigI2S1ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,32);
						ConfigI2S3ChainedDma(AudioFrameSizeInSamplePerCh_16KHz,32);
							EnableI2S1Rx0DmaChannel();
							EnableI2S3Tx0DmaChannel();
			DmaTxRxIsExpected=(AudioI2sPortsBitMapFlag_Fc1|AudioI2sPortsBitMapFlag_Fc3);
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

    I2SOutputMuteCntAmp=25;		//this is to mute I2S output for several frames after a call is started --- because from DSP, could have bad audio data coming (from Conversa/SRC)
    saiEnable = 1;
    return API_SUCCESS;
}

extern U32 AudioIoFrameCnt;
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;
extern void USB_DeviceCdcVcomTask(void);

extern void hfp_AnswerCall(void);	//looks like this function can not be directly called from APP_MU_IRQHandler
extern void hfp_RejectCall(void);	//looks like this function can not be directly called from APP_MU_IRQHandler
int NeedToCall_hfp_AnswerCall=0;
int NeedToCall_hfp_RejectCall=0;

volatile U32 MU_U32InfoFromDsp;
__attribute__((section("CodeQuickAccess")))
void APP_MU_IRQHandler(void)
{
	int i;
    uint32_t flag = 0;

    flag = MU_GetStatusFlags(APP_MU);
    if ((flag & kMU_Rx0FullFlag) == kMU_Rx0FullFlag)
    {
    	MU_U32InfoFromDsp = MU_ReceiveMsgNonBlocking(APP_MU, CHN_MU_REG_NUM);
    	DbgPin5Up();
		switch(MU_U32InfoFromDsp)
		{
			case MuEvtDspToMcu_AudioProcIsFinished_HfpCall:
				McuMainAudioFlowFinalize_HfpCall();
				break;
			case MuEvtDspToMcu_AudioProcIsFinished_HomeVitStandBy:
				McuMainAudioFlowFinalize_HomeVitStandBy();
				break;
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
		DbgPin5Dn();
    }
//	AllowAudioInterfaceReInit_PdmI2S=1;
    SDK_ISR_EXIT_BARRIER;
}
extern void connect_paired_device(uint8_t device_index);
extern void app_discover(void);
extern int bt_br_set_connectable(bool enable);
extern int bt_br_set_discoverable(bool enable);
__attribute__((section("CodeQuickAccess")))
void ButtonEventProcess(void)
{
#if 1
	//button evnet process
	if(BtnEvtVarGroup[0].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		//on board user button 1 is short pressed : to connect BT
		//app_discover();
    	bt_br_set_connectable(false);
        if (bt_br_set_connectable(true))
        {
            PRINTF("BR/EDR set/reset connectable failed\n");
            return;
        }
        if (bt_br_set_discoverable(true))
        {
            PRINTF("BR/EDR set discoverable failed\n");
            return;
        }
        PRINTF("BR/EDR set connectable and discoverable done\n");

		BtnEvtVarGroup[0].BtnEvt1=0;
	}
	if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		//on board user button 2 is short pressed : to connect BT
		//connect_paired_device(1);
    	bt_br_set_connectable(false);
        if (bt_br_set_connectable(true))
        {
            PRINTF("BR/EDR set/reset connectable failed\n");
            return;
        }
        if (bt_br_set_discoverable(true))
        {
            PRINTF("BR/EDR set discoverable failed\n");
            return;
        }
        PRINTF("BR/EDR set connectable and discoverable done\n");

		BtnEvtVarGroup[1].BtnEvt1=0;
	}

#else
	//button evnet process
	if(BtnEvtVarGroup[0].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		//on board user button 1 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[0]=1;

		BtnEvtVarGroup[0].BtnEvt1=0;
	}
	if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_SING_PRESS)
	{
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
#endif
}

///*
void InitAndStartPdm(void)
{
	#if 0
		//init basic clk and PDM
		InitAudioPLLForAllAudioPeripherals();
		InitBaseAudioClkForPdm();

		DMA_Init(DMA0);
		//we open pdm ports here
		Init_MicDmaCfgCh(0xff,AudioFrameSizeInSamplePerCh_16KHz,32);	//mic0,1,2,3,4,5
		BOARD_Init_DMA_PDM(0xff);
		BOARD_Init_DMIC(0xff,0,16000); //0: no skip general Dmic init. If not the first mic init, then should skip.
		ConfigDmicChainedDma(0xff);
		VarBlockSharedByDspAndMcu.BtHfpFs=8000;	//dsp may check this value, need to set it to either 8000 or 16000

		InitAudioCircularBuf(1,1,1);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
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
	#else
		InitAudioInterface_HomeVitStandby(0);
	#endif
}
//*/
__attribute__((section("CodeQuickAccess")))
void VitStandBy_Task(void *handle)
{
    OSA_SR_ALLOC();

	SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
	PRINTF("RT685 MCU: enter task VitStandBy_Task \r\n");
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);

    while (1)
    {
        OSA_SemaphoreWait(xSemaphoreDmaAudioDataReady, osaWaitForever_c);
#if 1
        DbgPin5Up();
		switch(DeviceWorkStateCur)
		{
			#if 0
				case WorkState_HfpCall:
					if(saiEnable == 0)
					{
						DbgPin8Up();
						Deinit_Board_Audio();
						DbgPin8Dn();
						//DbgPin7Dn();

						InitAndStartPdm();
						continue;
					}
					ProcessAudio_AfterAudioInputBufIsReady_HfpCall();
					break;
			#endif
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
		DbgPin5Dn();

		AudioIoFrameCnt++;

		AllowAudioInterfaceReInit_PdmI2S=1;

		#if EnableUsbComAndAudio==1
			USB_DeviceCdcVcomTask();	//can be placed to other place if here is not good (when other new task is needed and created)
		#endif
		//ButtonEventProcess();			//moved to WorkStateManager
#endif
    }
}


__attribute__((section("CodeQuickAccess")))
void SCO_Edma_Task(void *handle)
{
    hal_audio_transfer_t xfer;
    OSA_SR_ALLOC();

	SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
	PRINTF("RT685 MCU: enter task SCO_Edma_Task \r\n");
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);

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

			InitAndStartPdm();
            continue;
        }

		ProcessAudio_AfterAudioInputBufIsReady_HfpCall();
		AudioIoFrameCnt++;
		AllowAudioInterfaceReInit_PdmI2S=1;

		#ifdef SCO_DEBUG_MSG
			i f (count % 300 == 0)
			{
				PRINTF("@(%d  %d)", emptyMicBlock, emptySpeakerBlock);
				PRINTF("#( %d %d)", rxSpeaker_test, rxMic_test);
			}
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
	SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
	PRINTF("RT685 MCU: sco_audio_setup_pl_ext, ringtone=%d \r\n", NowInIncomingCallRingTone);
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);

    sco_audio_setup = 1;
    if (NowInIncomingCallRingTone == 0U)
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


	AmpState==AmpState_UnConfigured;

	VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
	VarBlockSharedByDspAndMcu.I2SFs_Amp=16000;
	VarBlockSharedByDspAndMcu.PdmFs=16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	InitAudioCircularBuf(1,1,1);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
	//InitAndStartPdm();		//if use this , it init dma again, cause BT firmware downloading fail
#if UsingQAR87Board == 1
	//Initial Smart Amplifier
    hal_amp_aw88166_power_on();
    hal_amp_aw88166_init();

#endif

	if (taskCreated == 0)
	{

		OSA_SemaphoreCreate(xSemaphoreScoAudio, 0);
		result = xTaskCreate(SCO_Edma_Task,   "SCO_Edma",   1024, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
		assert(pdPASS == result);
		OSA_SemaphoreCreate(xSemaphoreDmaAudioDataReady, 0);
		result = xTaskCreate(VitStandBy_Task, "VitStandBy", 1024, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
		assert(pdPASS == result);

		taskCreated = 1U;
	}
}

API_RESULT sco_audio_start_pl_ext(void)
{
	#if UseEventToControlBtHfp==1
		PRINTF_M("RT685 MCU: sco_audio_start_pl_ext \r\n");

		BtHfpRequest=HfpRequest_AudioStart;
		//block till workstate is WorkState_HfpCall and after HfpRequest_AudioStart is done (audio interface is running)
		xEventGroupWaitBits(EvtGrpHdl_StateMangerTaskToBtStack, HfpRequest_AudioStart, pdTRUE, pdFALSE, portMAX_DELAY);
		xEventGroupClearBits(EvtGrpHdl_StateMangerTaskToBtStack,HfpRequest_AudioStart);

		return API_SUCCESS;
	#endif
	hal_audio_transfer_t xfer;
	BaseType_t result = 0;

	SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
	PRINTF("RT685 MCU: sco_audio_start_pl_ext \r\n");
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);

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

	if (NowInIncomingCallRingTone == 0U)
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
}

API_RESULT sco_audio_stop_pl_ext(void)
{
	SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
	PRINTF("RT685 MCU: sco_audio_stop_pl_ext \r\n");
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);

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
	if (NowInIncomingCallRingTone == 0)
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
		NowInIncomingCallRingTone = 1;
	}

	return API_SUCCESS;
}

API_RESULT sco_audio_set_speaker_volume_ext(UCHAR volume)
{
	#if UseEventToControlBtHfp==1
		PRINTF_M("RT685 MCU: sco_audio_set_speaker_volume_ext \r\n");

		BtHfpRequest=HfpRequest_SetCodecAmpVolume;
		//block till workstate is WorkState_HfpCall and after HfpRequest_AudioStart is done (audio interface is running)
		xEventGroupWaitBits(EvtGrpHdl_StateMangerTaskToBtStack, HfpRequest_SetCodecAmpVolume, pdTRUE, pdFALSE, portMAX_DELAY);
		xEventGroupClearBits(EvtGrpHdl_StateMangerTaskToBtStack,HfpRequest_SetCodecAmpVolume);

		return API_SUCCESS;
	#endif
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
    if (NowInIncomingCallRingTone == 1)
    {
        Deinit_Board_Audio();
        //memset(RxAudioBufFromBt, 0x0, BUFFER_NUMBER * BUFFER_SIZE);
        NowInIncomingCallRingTone = 0U;
        if (sco_audio_setup == 1)
        {
        	PRINTF("sco_audio_play_ringtone_exit_pl_ext, ring tone playing is finished \r\n");
            audio_setup_pl_ext(false, &s_ep_info);
            sco_audio_start_pl_ext();
        }
    }
}
