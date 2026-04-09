/*
 * Copyright 2020 - 2022, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <porting.h>
#include <string.h>
#include <errno/errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/slist.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/a2dp.h>
#include <bluetooth/a2dp_codec_sbc.h>
#include <bluetooth/sdp.h>
#include <app_avrcp.h>
#include "clock_config.h"
#include "board.h"
#include "fsl_adapter_audio.h"
#include "fsl_codec_common.h"
#include "fsl_codec_adapter.h"
#include "fsl_debug_console.h"
#include "app_connect.h"
#include "app_handsfree.h"
#include "bluetooth/avrcp.h"


#include "EM_timer.h"

#include "fsl_sema42.h"
#include "GlobalDef.h"
#include "WorkStateManager.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "SubFunc.h"

#if UsingQAR87Board == 1
#include "system_status.h"

#endif

#if 1

#define A2DP_CLASS_OF_DEVICE (0x200404U)
#define APP_A2DP_STREAMER_SYNC_TASK_PRIORITY (5U)
/*If codec is cs42448, it doesn't support DAC CHANNAL, the macro A2DP_CODEC_DAC_VOLUME is meaningless.*/
#define A2DP_CODEC_DAC_VOLUME (100U) /* Range: 0 ~ 100 */
#define A2DP_CODEC_HP_VOLUME  (100U)  /* Range: 0 ~ 100 */
#define SDP_CLIENT_USER_BUF_LEN 512U
NET_BUF_POOL_FIXED_DEFINE(app_sdp_client_pool, CONFIG_BT_MAX_CONN,SDP_CLIENT_USER_BUF_LEN, CONFIG_NET_BUF_USER_DATA_SIZE, NULL);

extern uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx);

struct bt_a2dp *default_a2dp_snk;
struct bt_a2dp_endpoint *default_a2dp_endpoint_snk;
static uint8_t g_audioStart;
static uint8_t g_audioInit = 0;

static QueueHandle_t audio_sem;
BT_A2DP_SBC_SINK_ENDPOINT(sbcEndpoint);
//extern hal_audio_config_t audioTxConfig;
extern codec_config_t boardCodecConfig;
AT_NONCACHEABLE_SECTION_ALIGN(static HAL_AUDIO_HANDLE_DEFINE(audio_tx_handle), 4);
static codec_handle_t codec_handle;


static uint8_t app_sdp_a2dp_source_user(struct bt_conn *conn, struct bt_sdp_client_result *result);

static struct bt_sdp_discover_params discov_a2dp_source =
{
		.uuid = BT_UUID_DECLARE_16(BT_SDP_AUDIO_SOURCE_SVCLASS),
		.func = app_sdp_a2dp_source_user,
		.pool = &app_sdp_client_pool,
};


static uint8_t app_sdp_a2dp_source_user(struct bt_conn *conn,
			   struct bt_sdp_client_result *result)
{
    uint16_t param;
    int res;
    PRINTF("A2DP src sdp success callback\r\n");
    if ((result) && (result->resp_buf))
    {
        res = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_L2CAP, &param);
        if (res < 0)
        {
            return BT_SDP_DISCOVER_UUID_CONTINUE;
        }
        if (param == BT_UUID_AVDTP_VAL)
        {
            PRINTF ("SRC_A2DP Service found. Connecting ...\n");
            default_a2dp_snk = bt_a2dp_connect(conn);
            if (NULL == default_a2dp_snk)
            {
                PRINTF ("fail to connect a2dp\r\n");
            }
            return BT_SDP_DISCOVER_UUID_STOP;
        }
        return BT_SDP_DISCOVER_UUID_CONTINUE;
    }
    else
    {
        return BT_SDP_DISCOVER_UUID_CONTINUE;
    }
}

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
		BT_SDP_NEW_SERVICE,
		BT_SDP_LIST(
				BT_SDP_ATTR_SVCLASS_ID_LIST,
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), //35 03
				BT_SDP_DATA_ELEM_LIST(
						{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
					BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) //11 0B
						},
				)
		),
		BT_SDP_LIST(
				BT_SDP_ATTR_PROTO_DESC_LIST,
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),//35 10
				BT_SDP_DATA_ELEM_LIST(
						{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),// 35 06
					BT_SDP_DATA_ELEM_LIST(
							{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
						BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) // 01 00
							},
							{
									BT_SDP_TYPE_SIZE(BT_SDP_UINT16), //09
									BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) // 00 19
							},
					)
						},
						{
								BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),// 35 06
								BT_SDP_DATA_ELEM_LIST(
										{
									BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
											BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) // 00 19
										},
										{
												BT_SDP_TYPE_SIZE(BT_SDP_UINT16), //09
												BT_SDP_ARRAY_16(0X0100u) //AVDTP version: 01 00
										},
								)
						},
				)
		),
		BT_SDP_LIST(
				BT_SDP_ATTR_PROFILE_DESC_LIST,
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), //35 08
				BT_SDP_DATA_ELEM_LIST(
						{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), //35 06
					BT_SDP_DATA_ELEM_LIST(
							{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), //19
						BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) //11 0d
							},
							{
									BT_SDP_TYPE_SIZE(BT_SDP_UINT16), //09
									BT_SDP_ARRAY_16(0x0103U) //01 03
							},
					)
						},
				)
		),
		BT_SDP_SERVICE_NAME("A2DPSink"),
		BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);
#endif


void a2dp_sink_register_service()
{
	bt_sdp_register_service(&a2dp_sink_rec);

}

void app_audio_streamer_task_signal(void)
{
    if (0U != __get_IPSR())
    {
        portBASE_TYPE task_to_wake = pdFALSE;

        if (pdTRUE == xSemaphoreGiveFromISR(audio_sem, &task_to_wake))
        {
            portYIELD_FROM_ISR((task_to_wake));
        }
    }
    else
    {
        xSemaphoreGive(audio_sem);
    }
}

#if 0
void AudioTask(void *handle)
{
    audio_sem = xSemaphoreCreateCounting(0xFF, 0U);
    if (NULL == audio_sem)
    {
        vTaskDelete(NULL);
    }

    while (1U)
    {
        if (pdFALSE == xSemaphoreTake(audio_sem, portMAX_DELAY))
        {
            continue;
        }

        //vTaskDelay (100);	//???
        //bt_a2dp_snk_media_sync(default_a2dp_endpoint_snk, NULL, 0U);
    }
}
#endif


uint8_t app_get_snk_a2dp_status()
{
	return g_audioStart;
}

static void tx_callback(hal_audio_handle_t handle, hal_audio_status_t completionStatus, void *callbackParam)
{
    //app_audio_streamer_task_signal();
}

int CurrentStreamSbc_sampleRate_Hz;
int CurrentStreamSbc_lineChannels;
int CurrentStreamSbc_srcClock_Hz;
int CurrentStreamSbc_BitWidth;

static void configure_a2dp_codec(void)
{
    PRINTF("configure_a2dp_codec %dHz %d %d\r\n", CurrentStreamSbc_sampleRate_Hz, CurrentStreamSbc_BitWidth, CurrentStreamSbc_srcClock_Hz);

    //HAL_AudioTxInit((hal_audio_handle_t)&audio_tx_handle[0], &audioTxConfig);
    //HAL_AudioTxInstallCallback((hal_audio_handle_t)&audio_tx_handle[0], tx_callback, NULL);

	#ifdef A2DP_SINK_AUDIOxxx
    	CurrentStreamSbc_srcClock_Hz = BOARD_SwitchAudioFreq(CurrentStreamSbc_sampleRate_Hz);
		if (CODEC_Init(&codec_handle, &boardCodecConfig) != kStatus_Success)
		{
			PRINTF("codec init failed!\r\n");
		}
		g_audioInit =1;
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		CODEC_SetFormat(&codec_handle, CurrentStreamSbc_srcClock_Hz, CurrentStreamSbc_sampleRate_Hz, CurrentStreamSbc_BitWidth);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, A2DP_CODEC_DAC_VOLUME);
		CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, A2DP_CODEC_HP_VOLUME);
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
	#endif

	BtA2dpFs_ProvidedFromBtStack=CurrentStreamSbc_sampleRate_Hz;

#if 0
	if(CurrentStreamSbc_sampleRate_Hz==16000)
		AudioPortI2SAndPdmCfg_SetAmpI2SFs(AudioPortI2SAndPdmCfg,Fs_16000);
	if(CurrentStreamSbc_sampleRate_Hz==44100)
		AudioPortI2SAndPdmCfg_SetAmpI2SFs(AudioPortI2SAndPdmCfg,Fs_44100);
	if(CurrentStreamSbc_sampleRate_Hz==48000)
		AudioPortI2SAndPdmCfg_SetAmpI2SFs(AudioPortI2SAndPdmCfg,Fs_48000);

	//if(CurrentStreamSbc_BitWidth==16)
	//	AudioPortI2SAndPdmCfg_SetAmpI2SBit(AudioPortI2SAndPdmCfg,BitWidth_16);
	//if(CurrentStreamSbc_BitWidth==32)
		AudioPortI2SAndPdmCfg_SetAmpI2SBit(AudioPortI2SAndPdmCfg,BitWidth_32);			//always 32 bit for streaming AMP

	AudioPortI2SAndPdmCfg_SetNvtI2SBit(AudioPortI2SAndPdmCfg,BitWidth_I2SIsDisabled);
	AudioPortI2SAndPdmCfg_SetPdmFs(AudioPortI2SAndPdmCfg, Fs_16000);
	AudioPortI2SAndPdmCfg_SetPdmEnableBit(AudioPortI2SAndPdmCfg,0b000011);
#endif

	g_audioInit =1;
    //RequestToGetIntoA2dpPlay=1;	//should request to start once the sbc cir buffer is half full
}
void sbc_configured(struct bt_a2dp_endpoint_configure_result *configResult)
{
	if (configResult->err == 0)
	{
		default_a2dp_endpoint_snk = &sbcEndpoint;

        // SINK Audio
			CurrentStreamSbc_sampleRate_Hz  = bt_a2dp_sbc_get_sampling_frequency((struct bt_a2dp_codec_sbc_params *)&configResult->config.media_config->codec_ie[0]);
			CurrentStreamSbc_lineChannels = (hal_audio_channel_t)bt_a2dp_sbc_get_channel_num((struct bt_a2dp_codec_sbc_params *)&configResult->config.media_config->codec_ie[0]);

			#ifdef A2DP_SINK_AUDIOxxx
test
				//#if AmpIsAlwaysIn48Or16KHz==1
				#if 1
					if(C urrentStreamSbc_sampleRate_Hz==44100)
						#if UsingQAR87Board == 1
test						CurrentStreamSbc_srcClock_Hz = BOARD_SwitchAudioFreq(48000, BtPcmFc2Fc4_AmpFc1Fc3);
						#else
							CurrentStreamSbc_srcClock_Hz = BOARD_SwitchAudioFreq(48000, BtPcmFc5Fc2_CodecFc1Fc3);
						#endif
					else
						#if UsingQAR87Board == 1
test						CurrentStreamSbc_srcClock_Hz = BOARD_SwitchAudioFreq(CurrentStreamSbc_sampleRate_Hz, BtPcmFc2Fc4_AmpFc1Fc3);
						#else
test						CurrentStreamSbc_srcClock_Hz = BOARD_SwitchAudioFreq(CurrentStreamSbc_sampleRate_Hz, BtPcmFc5Fc2_CodecFc1Fc3);
						#endif
				#endif
			#endif
			CurrentStreamSbc_BitWidth=16;

			PRINTF("sbc_configured %dHz %d %d\r\n", CurrentStreamSbc_sampleRate_Hz, CurrentStreamSbc_BitWidth, CurrentStreamSbc_srcClock_Hz);
		#ifdef A2DP_SINK_AUDIOxxx

		//        HAL_AudioTxInit((hal_audio_handle_t)&audio_tx_handle[0], &audioTxConfig);
		//        HAL_AudioTxInstallCallback((hal_audio_handle_t)&audio_tx_handle[0], tx_callback, NULL);

			if (CODEC_Init(&codec_handle, &boardCodecConfig) != kStatus_Success)
			{
				PRINTF("codec init failed!\r\n");
			}
			CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
			CODEC_SetFormat(&codec_handle, CurrentStreamSbc_srcClock_Hz, CurrentStreamSbc_sampleRate_Hz, CurrentStreamSbc_BitWidth);
			CODEC_SetVolume(&codec_handle, kCODEC_VolumeDAC, A2DP_CODEC_DAC_VOLUME);
			CODEC_SetVolume(&codec_handle, kCODEC_VolumeHeadphoneLeft | kCODEC_VolumeHeadphoneRight, A2DP_CODEC_HP_VOLUME);
			CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, false);
		#endif

        //g_audioInit = 1;

		//cmd_init_ct();//B36932 move to app_dcc.c line 126
		//ClearAudioCirBuf(0,0,1);			//???

		/*AVRCP Profile level connection*/
		//avrcp_control_connect(conn_rider_phone);//B36932 move to app_handsfree.c line 177
        PRINTF("sbc_configured done, Fs: %d\r\n",CurrentStreamSbc_sampleRate_Hz);	//seems to be always 44100Hz Fs
	}
	else
	{
		PRINTF("sbc_configured return (err %d)\n", configResult->err);
	}
}
void sbc_deinit()
{
	g_audioInit = 0;
	g_audioStart = 0;
	#ifdef A2DP_SINK_AUDIOxxx
		CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
		HAL_AudioTxDeinit((hal_audio_handle_t)&audio_tx_handle[0]);
		(void)BOARD_SwitchAudioFreq(0U);
	#endif
    RequestToGetOutofA2dpPlay=1;
    set_music_status(STATUS_OFF);

	PRINTF("sbc_deinit done\r\n");
}
void sbc_deconfigured(int err)
{
	if (err == 0)
	{
		g_audioStart = 0;
		/* Stop Audio Player */
		//PRINTF("a2dp deconfigure\r\n");
		// SINK Audio
		#ifdef A2DP_SINK_AUDIOxxx
			CODEC_SetMute(&codec_handle, kCODEC_PlayChannelHeadphoneRight | kCODEC_PlayChannelHeadphoneLeft, true);
			HAL_AudioTxDeinit((hal_audio_handle_t)&audio_tx_handle[0]);
			(void)BOARD_SwitchAudioFreq(0U);
		#endif
        g_audioInit=0;
		RequestToGetOutofA2dpPlay=1;
	    set_music_status(STATUS_OFF);
    	PRINTF("sbc_deconfigured done\r\n");
	}
	else
	{
		PRINTF("sbc_deconfigured callback return (err %d)\n", err);
	}
}
void sbc_start_play(int err)
{

	if (err == 0)
	{
		g_audioStart = 1;
		PRINTF("sbc_start_play done\r\n");
	}
	else
	{
		PRINTF("sbc_start_play Callback return (err %d)\n", err);
	}
}
void sbc_stop_play(int err)
{
	if (err == 0)
	{
		g_audioStart = 0;
		/* Stop Audio Player */
		// SINK Audio

		#ifdef A2DP_SINK_AUDIO
			//HAL_AudioTransferAbortSend((hal_audio_handle_t)&audio_tx_handle[0]);
		#endif
	    RequestToGetOutofA2dpPlay=1;
	    set_music_status(STATUS_OFF);
		PRINTF("sbc_stop_play done\r\n");
	}
	else
	{
		PRINTF("sbc_stop_play Callback return (err %d)\n", err);
	}
}
void sbc_streamer_data(uint8_t *data, uint32_t length)
{
	if ((data != NULL) && (length != 0U))
	{
		//hal_audio_transfer_t xfer;

		if(0 == g_audioStart)
		{
			/*return;*/
		}

		//data_send_source(data, length);

		// SINK Audio
		#ifdef A2DP_SINK_AUDIO
			//xfer.dataSize       = length;
			//xfer.data           = data;


//			DbgPin6Up();DbgPin5Up();
			SEMA42_Lock(APP_SEMA42, SEMA42_GATE1, domainId);
				int FreeAod;
				FreeAod=CirAudioBuf_SpaceAvailableInSamples_S8((volatile T_CircularAudioBuf_S8 *)&VarBlockSharedByDspAndMcu.CirBuf_SbcRaw);
				//FreeAod=10000;
				/*
				PRINTF("BT: %x, %x, %d, %d\n",
								(U32)(
										((T_CircularAudioBuf_S8 *)(&VarBlockSharedByDspAndMcu.CirBuf_SbcRaw))->PtrRd
									 ),
								(U32)(
										((T_CircularAudioBuf_S8 *)(&VarBlockSharedByDspAndMcu.CirBuf_SbcRaw))->PtrWr
									 ),
								l,
								length
							);
				*/

				//write a2dp sbc stream BIN to Cirbuffer
				if(FreeAod>=length)
				{
					//has space to fill
					CirAudioBuf_WriteSamples_S8((volatile T_CircularAudioBuf_S8 *)&VarBlockSharedByDspAndMcu.CirBuf_SbcRaw, length, data);
				}else
				{
					//cir buffer doesn't have enough space --- this should not happen
//					PRINTF("sbc_streamer_data not enough space to put Sbc Raw data, in the cir buffer \r\n");
					//PRINTF("f\r\n");
				}

//				PRINTF("BT Sbc buf: %d\r\n", CirBuf_SbcRaw_LengthInBytes-FreeAod);
			SEMA42_Unlock(APP_SEMA42, SEMA42_GATE1);
//			DbgPin6Dn();DbgPin5Dn();
			// Get into MusicPlayer State Only when current state is HomeVitStandby
			if(DeviceWorkStateCur==WorkState_HomeVitStandby || DeviceWorkStateCur==WorkState_Menu || DeviceWorkStateCur==WorkState_About)
			{
				//request to get into WorkState_MusicPlayer after SBC buffer is half full
				//if(CirAudioBuf_GetUsagePercentage_S8((T_CircularAudioBuf_S8 *)&VarBlockSharedByDspAndMcu.CirBuf_SbcRaw)>60)
				if(FreeAod<CirBuf_SbcRaw_LengthInBytes*1/2)
				{
					// Get into
					//this is 3/4 full
					RequestToGetIntoA2dpPlay=1;
				    set_music_status(STATUS_ON);

					//VarBlockSharedByDspAndMcu.NeedToStartPlaySbc=1;
					//VarBlockSharedByDspAndMcu.PlaySbcFileIdx=0xffff;		//0xffff stands for a2dp sbc stream
				}
			}


			if (g_audioInit && call_status())
			{
				#if 0
					if (kStatus_HAL_AudioSuccess != HAL_AudioTransferSendNonBlocking((hal_audio_handle_t)&audio_tx_handle[0], &xfer))
					{
						PRINTF("prime fail\r\n");
					}
					app_audio_streamer_task_signal();
					//PRINTF("sbc_streamer_data %d\r\n", length);		//length is 1536 samples: 48*32=1536
				#endif
			}else if (!g_audioInit && call_status())
			{
				configure_a2dp_codec();
			}
		#endif
	}
}

static void a2dp_snk_edgefast_a2dp_init(void)
{
	sbcEndpoint.control_cbs.configured = sbc_configured;
	sbcEndpoint.control_cbs.deconfigured = sbc_deconfigured;
	sbcEndpoint.control_cbs.start_play = sbc_start_play;
	sbcEndpoint.control_cbs.stop_play = sbc_stop_play;
	sbcEndpoint.control_cbs.sink_streamer_data = sbc_streamer_data;
	bt_a2dp_register_endpoint(&sbcEndpoint, BT_A2DP_AUDIO, BT_A2DP_SINK);
}
void sdp_discover_for_a2dp_source(void)
{
	int res;
	res = bt_sdp_discover(conn_rider_phone, &discov_a2dp_source);

	if (res)
	{
		PRINTF("A2DP-Sink discovery failed: result\r\n");
	}
	else
	{
		PRINTF("A2DP-Sink discovery started\r\n");
	}
}
void a2dp_sink_ready()
{
	int err = 0;


	API_RESULT retval;
	retval = API_SUCCESS;

	UCHAR new[1U] = {0x01};
	retval = BT_hci_vendor_specific_command(0x1A1,new,1);
	if(retval != API_SUCCESS)
	{
		PRINTF("A2DP Vendor command is not configured properly\n");
	}
	a2dp_snk_edgefast_a2dp_init();
}
