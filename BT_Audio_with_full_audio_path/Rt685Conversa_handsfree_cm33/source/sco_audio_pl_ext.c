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
#include "i2c_component_handler.h"

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


extern hal_audio_config_t txMicConfig;
extern hal_audio_config_t rxSpeakerConfig;

/* --------------------------------------------- Exported Global Variables */

/* --------------------------------------------- Static Global Variables */


EventGroupHandle_t EvtGrpHdl_StateMangerTaskToBtStack;
//EventGroupHandle_t EvtGrpHdl_AudioDmaIntrToAudioTask;

extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_speaker_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_mic_handle), 4);

AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(tx_mic_handle), 4);
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(rx_speaker_handle), 4);
static codec_handle_t codec_handle;

AT_NONCACHEABLE_SECTION_ALIGN(uint8_t RxAudioBufFromBt[BUFFER_NUMBER * BUFFER_SIZE], 4);

OSA_SEMAPHORE_HANDLE_DEFINE(xSemaphoreDmaAudioDataReady);

//static volatile uint8_t RingToneIsInitialized=0;
static volatile uint8_t NowInHfpTelCall=0;
static volatile uint8_t NowInHfpAppCall=0;
static volatile uint8_t WasInInComingRingTone=0;
static uint32_t txMic_index = 0U, rxMic_index = 0U;
atomic_t emptyMicBlock = BUFFER_NUMBER;
static uint32_t txSpeaker_index = 0U, rxSpeaker_index = 0U;
atomic_t emptySpeakerBlock = BUFFER_NUMBER;

volatile uint8_t NowInIncomingCallRingTone = 0;

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
	OSA_SemaphorePost(xSemaphoreDmaAudioDataReady);
	//xEventGroupSetBits(EvtGrpHdl_AudioDmaIntrToAudioTask,DmaAudioIntrRequest_AllAudioDataIsReady);
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
//    	DbgPin5Up();
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
			CheckPcmRxFrBtBufAodAndAdjustAudioPll(AOD_BTDnBuf);
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
//		DbgPin5Dn();
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
//    	DbgPin6Up();
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
			if (rxSpeaker_index >= 2)
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
					GenerateSinWavFromTable_S16_SingleCh(1,&TmpDbgSigalBuf[0*128], 128);
					GenerateSinWavFromTable_S16_SingleCh(1,&TmpDbgSigalBuf[1*128], 128);
					GenerateSinWavFromTable_S16_SingleCh(1,&TmpDbgSigalBuf[2*128], 128);
					GenerateSinWavFromTable_S16_SingleCh(1,&TmpDbgSigalBuf[3*128], 128);
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
//    	DbgPin6Dn();
    }
    AllowAudioInterfaceReInit_Fc25=1;
}

extern void ClearBTUpDnAudioBufDataArea(void);

void DeinitHfpVariables(void)
{
	NowInHfpTelCall=0;
	NowInHfpAppCall=0;
	NowInIncomingCallRingTone=0;
	WasInInComingRingTone=0;

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
{
	int r;

	if(AmpState==AmpState_ConfiguredAndActive)
		return;

	//to do...... initial or start smart amplifier //B36932
	//r = initial codec or start codec
	if(fs==48000)
	{
		amp_post_event(AMP_EVT_MUSIC);
//		hal_amp_aw88166_left_start ("Music");
//		hal_amp_aw88166_right_start("Music");
	}else
	if(fs==16000)
	{
		amp_post_event(AMP_EVT_RECEIVER);
//		hal_amp_aw88166_left_start ("Receiver");
//		hal_amp_aw88166_right_start("Receiver");
	}
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

//	DbgPin8Up();
	((wm8904_config_t *)boardCodecScoConfig.codecDevConfig)->mclk_HZ=Mfreq;

	r=CODEC_Init(&codec_handle, &boardCodecScoConfig);

	if (r!= kStatus_Success)
	{
		PRINTF("InitAndStartCodec is failed, %d \r\n",r);
		return;
	}else
	{
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		//CODEC_SetFormat(&codec_handle, CLOCK_GetMclkClkFreq(), txSpeakerConfig.sampleRate_Hz, txSpeakerConfig.bitWidth);
		CODEC_SetFormat(&codec_handle, CLOCK_GetMclkClkFreq(), fs, bits);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, HFP_CODEC_DAC_VOLUME);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, HFP_CODEC_HP_VOLUME);
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
	}
//	DbgPin8Dn();
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
            PRINTF_M("BR/EDR set/reset connectable failed\n");
            return;
        }
        if (bt_br_set_discoverable(true))
        {
        	PRINTF_M("BR/EDR set discoverable failed\n");
            return;
        }
        PRINTF_M("BR/EDR set connectable and discoverable done\n");

		BtnEvtVarGroup[0].BtnEvt1=0;
	}
	if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		//on board user button 2 is short pressed : to connect BT
		//connect_paired_device(1);
    	bt_br_set_connectable(false);
        if (bt_br_set_connectable(true))
        {
        	PRINTF_M("BR/EDR set/reset connectable failed\n");
            return;
        }
        if (bt_br_set_discoverable(true))
        {
        	PRINTF_M("BR/EDR set discoverable failed\n");
            return;
        }
        PRINTF_M("BR/EDR set connectable and discoverable done\n");

		BtnEvtVarGroup[1].BtnEvt1=0;
	}

#else
	//button evnet process
	if(BtnEvtVarGroup[0].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		//on board user button 1 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdIdx_Btn1]=1;

		BtnEvtVarGroup[0].BtnEvt1=0;
	}
	if(BtnEvtVarGroup[1].BtnEvt1==BTN_EVT_SING_PRESS)
	{
		//on board user button 2 is short pressed
		VarBlockSharedByDspAndMcu.U32ControlPara[ControlParaIdIdx_Btn2]=1;

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
__attribute__((section("CodeQuickAccess")))
void AudioFlow_Task(void *handle)
{
    OSA_SR_ALLOC();

	PRINTF_M("RT685 MCU: enter task AudioFlow_Task \r\n");

    while (1)
    {
        OSA_SemaphoreWait(xSemaphoreDmaAudioDataReady, osaWaitForever_c);

    	//xEventGroupWaitBits(EvtGrpHdl_AudioDmaIntrToAudioTask, DmaAudioIntrRequest_AllAudioDataIsReady, pdTRUE, pdFALSE, portMAX_DELAY);
    	//xEventGroupClearBits(EvtGrpHdl_AudioDmaIntrToAudioTask,DmaAudioIntrRequest_AllAudioDataIsReady);

        DbgPin5Up();
		switch(DeviceWorkStateCur)
		{
			case WorkState_HfpCall:
				ProcessAudio_AfterAudioInputBufIsReady_HfpCall();
				break;
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

		if(DeviceWorkStateCur!=WorkState_HfpCall)
			GraduallySetAudioPllBackToDefault();

		AudioIoFrameCnt++;

		AllowAudioInterfaceReInit_PdmI2S=1;

		#if EnableUsbComAndAudio==1
			USB_DeviceCdcVcomTask();	//can be placed to other place if here is not good (when other new task is needed and created)
		#endif
    }
}


static uint32_t taskCreated = 0;
void StartAudioTask(void)
{
	BaseType_t result = 0;


	AmpState=AmpState_UnConfigured;

	VarBlockSharedByDspAndMcu.I2SFs_Nvt=NvtI2SFs_48KHz;
	VarBlockSharedByDspAndMcu.I2SFs_Amp=16000;
	VarBlockSharedByDspAndMcu.PdmFs=16000;
	VarBlockSharedByDspAndMcu.UacUpFs=AUDIO_IN_SAMPLING_RATE_KHZ*1000;
	VarBlockSharedByDspAndMcu.UacDnFs=AUDIO_OUT_SAMPLING_RATE_KHZ*1000;

	InitAudioCircularBuf(1,1,1);	//int ToInitBtCir, int ToInitUacCir,  int ToInitSbcCir
	//InitAndStartPdm();		//if use this , it init dma again, cause BT firmware downloading fail
	
	#if UsingQAR87Board == 1
		//Initial Smart Amplifier
	    //B36932 Quanta don't do it here, hal_amp_aw88166_power_on();
	    //B36932 Quanta don't do it here, hal_amp_aw88166_init();
	#endif

	if (taskCreated == 0)
	{

		EvtGrpHdl_StateMangerTaskToBtStack=xEventGroupCreate();

		//EvtGrpHdl_AudioDmaIntrToAudioTask=xEventGroupCreate();
		OSA_SemaphoreCreate(xSemaphoreDmaAudioDataReady, 0);

		result = xTaskCreate(AudioFlow_Task, "VitStandBy", 1024*10, NULL, HFP_STREAMER_TASK_PRIORITY, NULL);
		assert(pdPASS == result);

		taskCreated = 1U;
	}
}

void sco_audio_init_pl_ext(void)
{
	PRINTF_M("sco_audio_init_pl_ext\r\n");
    return;
}
void sco_audio_shutdown_pl_ext(void)
{
	PRINTF_M("sco_audio_shutdown_pl_ext\r\n");
	return;
}

API_RESULT sco_audio_setup_pl_ext(SCO_AUDIO_EP_INFO *ep_info)
{
	PRINTF_M("RT685 MCU: sco_audio_setup_pl_ext, ringtone=%d \r\n", NowInIncomingCallRingTone);
	
    txMic_index     = 0U;
    rxMic_index     = 0U;
    txSpeaker_index = 0U, rxSpeaker_index = 0U;
    (void)atomic_set(&emptySpeakerBlock, BUFFER_NUMBER);
    (void)atomic_set(&emptyMicBlock, BUFFER_NUMBER);

	BtHfpAudioFs      =ep_info->sampl_freq;
	BtHfpAudioBitWidth=ep_info->sample_len;
    WasInInComingRingTone=NowInIncomingCallRingTone;
    RequestToGetIntoHfp=1;

    return API_SUCCESS;
}


void sco_audio_start(void)
{
	hal_audio_transfer_t xfer;
	BaseType_t result = 0;

	PRINTF_M("RT685 MCU: sco_audio_start_pl_ext \r\n");

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

	//start dmic immediately, then in dmic intr, fc1,fc3 will be started
	ImmediatelyStartDmicDmaChannels(0xff);	//mic0,1,2,3, after calling this, dmic dma intr occurs one frame later!
	PRINTF_M("sco_audio_start_pl_ext, SCO audio ports are all started \r\n");

	if (NowInIncomingCallRingTone == 0U)
	{
		if(WasInInComingRingTone)
		{
			NowInHfpTelCall=1;
			PRINTF_M("NowInHfpTelCall \r\n");
		}
		else
		{
			NowInHfpAppCall=1;	//Note:!!! when calling out TEL, still arrives here, can not identify if it is an HFP call out
							//Note!!! when calling out, not able to identify if it is App call or Hfp call. So, not matter what call, check the button long press (answer, reject/stop)
							//check the beyerdynamic speaker, it is the same!!!
							//so, we keep the logic of checking button in App call or Hfp call as it is.

			PRINTF_M("Now either InHfpAppCall or InHfpTelCall \r\n");
		}
	}
	else
	{
		NowInHfpTelCall=0;
		NowInHfpAppCall=0;
		PRINTF_M("NowInRingTone \r\n");
	}
	return;
}
API_RESULT sco_audio_start_pl_ext(void)
{
	PRINTF_M("RT685 MCU: sco_audio_start_pl_ext \r\n");

	BtHfpRequest=HfpRequest_AudioStart;
	//block till workstate is WorkState_HfpCall and after HfpRequest_AudioStart is done (audio interface is running)
	xEventGroupWaitBits(EvtGrpHdl_StateMangerTaskToBtStack, HfpRequest_AudioStart, pdTRUE, pdFALSE, portMAX_DELAY);
	xEventGroupClearBits(EvtGrpHdl_StateMangerTaskToBtStack,HfpRequest_AudioStart);

	return API_SUCCESS;
}

API_RESULT sco_audio_stop_pl_ext(void)
{
	PRINTF_M("RT685 MCU: sco_audio_stop_pl_ext \r\n");

	WasInInComingRingTone=0;
	RequestToGetOutofHfp=1;
	return API_SUCCESS;
}

#ifdef HCI_SCO
void sco_audio_spkr_play_pl_ext(UCHAR *m_data, UINT16 m_datalen)
{
    /* Write to Codec */
}
#endif /* HCI_SCO */
API_RESULT sco_audio_set_speaker_volume_ext(UCHAR volume)
{
	PRINTF_M("RT685 MCU: sco_audio_set_speaker_volume_ext: %d \r\n", volume);

	if(DeviceWorkStateCur!=WorkState_HfpCall)
	{
		return API_FAILURE;
	}

	BtHfpRequest=HfpRequest_SetCodecAmpVolume;
	BtHfpAudioVolume=volume;
	//block till workstate is WorkState_HfpCall and after HfpRequest_AudioStart is done (audio interface is running)
	xEventGroupWaitBits(EvtGrpHdl_StateMangerTaskToBtStack, HfpRequest_SetCodecAmpVolume, pdTRUE, pdFALSE, portMAX_DELAY);
	xEventGroupClearBits(EvtGrpHdl_StateMangerTaskToBtStack,HfpRequest_SetCodecAmpVolume);

	return API_SUCCESS;
}
void sco_audio_play_ringtone_pl_ext(void)
{
if (NowInIncomingCallRingTone == 0)
{
	BtHfpAudioFs=16000;
	BtHfpAudioBitWidth=16;
	NowInIncomingCallRingTone = 1;
	RequestToGetIntoHfp=1;
}

PRINTF_M("sco_audio_play_ringtone_pl_ext, NowInIncomingCallRingTone=%d\r\n",NowInIncomingCallRingTone);
return;
}
void sco_audio_play_ringtone_exit_pl_ext(void)
{
	PRINTF_M("sco_audio_play_ringtone_exit_pl_ext\r\n");
    if (NowInIncomingCallRingTone == 1)
    {
        sco_audio_start_pl_ext();
        NowInIncomingCallRingTone = 0U;
    }else
    {
    	if(WasInInComingRingTone)
    	{
    		RequestToGetOutofHfp=1;
    		PRINTF_M("sco_audio_play_ringtone_exit_pl_ext RequestToGetOutofHfp=1\r\n");
    	}
    }
}
