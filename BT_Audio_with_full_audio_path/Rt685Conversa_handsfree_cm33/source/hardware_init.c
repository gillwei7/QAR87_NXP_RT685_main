/*
 * Copyright 2020, 2024  NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*${header:start}*/
#include "clock_config.h"
#include "board.h"
#include "pin_mux.h"
#include "fsl_dma.h"
#include "fsl_wm8904.h"
#include "fsl_adapter_audio.h"
#include "fsl_codec_common.h"
#include "fsl_codec_adapter.h"
#include "fsl_debug_console.h"
#include "fsl_adapter_uart.h"
#include "controller_hci_uart.h"
#include "fsl_power.h"
#include "fsl_inputmux.h"
#include "fsl_pint.h"

#if UsingQAR87Board == 1
#include "hal_amp.h"
#include "hal_pmic.h"

#endif

#if CONFIG_BT_HOST_USB_ENABLE==1
#include "usb_host_config.h"
#include "usb_phy.h"
#include "usb_host.h"
#endif

#if (((defined(CONFIG_BT_SMP)) && (CONFIG_BT_SMP)))
#include "ksdk_mbedtls.h"
#endif /* CONFIG_BT_SMP */
/*${header:end}*/

#include "GlobalDef.h"

#include "fsl_inputmux.h"
#include "fsl_mu.h"
#include "fsl_sema42.h"
#include "fsl_power.h"

#include "dsp_support.h"

#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "AudioProcess.h"
#include "SubFunc.h"


#if EnableUsbComAndAudio==1
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_audio.h"
#include "usb_device_descriptor.h"
#include "audio_unified.h"

#include "AudioIoCfg_I2s.h"

#include "UsbApp.h"
#include "Sweep.h"


#endif

/*${macro:start}*/
#if defined(__GIC_PRIO_BITS)
#define USB_HOST_INTERRUPT_PRIORITY (25U)
#elif defined(__NVIC_PRIO_BITS) && (__NVIC_PRIO_BITS >= 3)
#define USB_HOST_INTERRUPT_PRIORITY (6U)
#else
#define USB_HOST_INTERRUPT_PRIORITY (3U)
#endif

/* I2S */
#define DEMO_MICBUF_TX_INSTANCE (2U)
#define DEMO_MICBUF_RX_INSTANCE (1U)
#define DEMO_SPKBUF_TX_INSTANCE (3U)
#if UsingQAR87Board == 1
#define DEMO_SPKBUF_RX_INSTANCE (4U)
#else
#define DEMO_SPKBUF_RX_INSTANCE (5U)
#endif

/* DMA */
#define EXAMPLE_DMA_INSTANCE      (0U)
#define EXAMPLE_MICBUF_TX_CHANNEL (5U)
#define EXAMPLE_MICBUF_RX_CHANNEL (2U)
#define EXAMPLE_SPKBUF_TX_CHANNEL (7U)
#if UsingQAR87Board == 1
#define EXAMPLE_SPKBUF_RX_CHANNEL (8U)
#else
#define EXAMPLE_SPKBUF_RX_CHANNEL (10U)
#endif

/* demo audio data channel */
#define DEMO_MICBUF_TX_CHANNEL (kHAL_AudioMono)
#define DEMO_MICBUF_RX_CHANNEL (kHAL_AudioMonoRight)
#define DEMO_SPKBUF_TX_CHANNEL (kHAL_AudioMonoLeft)
#define DEMO_SPKBUF_RX_CHANNEL (kHAL_AudioMono)
/*${macro:end}*/

#if EnableUsbComAndAudio==1
extern usb_device_composite_struct_t g_composite;
#endif

#if 1
/*${variable:start}*/
wm8904_config_t wm8904Config = {
    .i2cConfig          = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE},
    .recordSource       = kWM8904_RecordSourceLineInput,
    .recordChannelLeft  = kWM8904_RecordChannelLeft2,
    .recordChannelRight = kWM8904_RecordChannelRight2,
    .playSource         = kWM8904_PlaySourceDAC,
    .slaveAddress       = WM8904_I2C_ADDRESS,
    .protocol           = kWM8904_ProtocolI2S,
    .format             = {.sampleRate = kWM8904_SampleRate48kHz, .bitWidth = kWM8904_BitWidth16},
    .master             = true,
};
codec_config_t boardCodecConfig = {.codecDevType = kCODEC_WM8904, .codecDevConfig = &wm8904Config};

/*
 * AUDIO PLL setting: Frequency = Fref * (MULT + NUM / DENOM)
 *                              = 24 * (22 + 5040/27000)
 *                              = 532.48MHz
 */
/*setting for multiple of 8Khz,such as 48Khz/16Khz/32KHz*/
const clock_audio_pll_config_t audioPllConfig_48K = {
    .audio_pll_src  = kCLOCK_AudioPllXtalIn, /* OSC clock */
    .numerator 		= 5280,                         /* Numerator of the Audio PLL fractional loop divider is null */
    .denominator 	= 10000,                      /* Denominator of the Audio PLL fractional loop divider is null */
    .audio_pll_mult = kCLOCK_AudioPllMult22  /* Divide by 22 */
};
//should be used together with CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 18U); /* Set AUDIOPLLCLKDIV divider to value 18 */
//and CLOCK_InitAudioPfd(kCLOCK_Pfd0, 22);


/*
 * AUDIO PLL setting: Frequency = Fref * (MULT + NUM / DENOM)
 *                              = 24 * (20 + 5040/13125)
 *                              = 489.216MHz
 */
/*setting for 44.1Khz*/
const clock_audio_pll_config_t audioPllConfig_44p1K = {
    .audio_pll_src  = kCLOCK_AudioPllXtalIn, /* OSC clock */
    .numerator      = 10270,                  /* Numerator of the Audio PLL fractional loop divider is null */
    .denominator    = 18750,                 /* Denominator of the Audio PLL fractional loop divider is null */
    .audio_pll_mult = kCLOCK_AudioPllMult19  /* Divide by 20 */
};
//should be used together with CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 17U); /* Set AUDIOPLLCLKDIV divider to value 17 */
//and CLOCK_InitAudioPfd(kCLOCK_Pfd0, 22);

//boardCodecScoConfig and wm8904ScoConfig is for audio after ring tone
wm8904_config_t wm8904ScoConfig = {
    .i2cConfig          = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE, .codecI2CSourceClock = 25010526},
    .recordSource       = kWM8904_RecordSourceLineInput,
    .recordChannelLeft  = kWM8904_RecordChannelLeft2,
    .recordChannelRight = kWM8904_RecordChannelRight2,
    .playSource         = kWM8904_PlaySourceDAC,
    .slaveAddress       = WM8904_I2C_ADDRESS,
    .protocol           = kWM8904_ProtocolI2S,
		.format             = {.sampleRate = kWM8904_SampleRate16kHz, .bitWidth = kWM8904_BitWidth32},
    .mclk_HZ            = 24576000,

#if Rt685I2SToAmpIsI2SMaster==1
    .master             = false,
#else
    .master             = true,
#endif
};
codec_config_t boardCodecScoConfig = {.codecDevType = kCODEC_WM8904, .codecDevConfig = &wm8904ScoConfig};



hal_audio_dma_config_t txMicDmaConfig = {
    .instance             = EXAMPLE_DMA_INSTANCE,
    .channel              = EXAMPLE_MICBUF_TX_CHANNEL,
    .enablePreemption     = false,
    .enablePreemptAbility = false,
    .priority             = kHAL_AudioDmaChannelPriorityDefault,
    .dmaMuxConfig         = NULL,
    .dmaChannelMuxConfig  = NULL,
};

hal_audio_config_t txMicConfig = {
    .dmaConfig     = &txMicDmaConfig,
    .ipConfig      = NULL,
    .srcClock_Hz   = 24576000,
    .sampleRate_Hz = (uint32_t)kHAL_AudioSampleRate8KHz,
#if (defined(WIFI_88W8987_BOARD_AW_CM358_USD) || defined(WIFI_IW416_BOARD_MURATA_1ZM_USD))
    .f rameLength = 22, /* Here is 22 because the bt module will generate 22 bits clock after one clock WS. */
#elif (defined(WIFI_IW416_BOARD_AW_AM510_USD) || defined(WIFI_IW416_BOARD_AW_AM457_USD) || \
       defined(WIFI_IW416_BOARD_MURATA_1XK_USD)|| defined(WIFI_IW612_BOARD_MURATA_2EL_USD))
    .frameLength = 256, /* Here is 256 because the bt module will generate 256 bits clock after one clock WS. */
#else
#endif
    .fifoWatermark     = 0,
    .masterSlave       = kHAL_AudioSlave,
    .bclkPolarity      = kHAL_AudioSampleOnFallingEdge,
    .frameSyncWidth    = kHAL_AudioFrameSyncWidthOneBitClk,
    .frameSyncPolarity = kHAL_AudioBeginAtRisingEdge,
    .lineChannels      = DEMO_MICBUF_TX_CHANNEL,
    .dataFormat        = kHAL_AudioDataFormatDspModeA,
    .bitWidth          = (uint8_t)kHAL_AudioWordWidth16bits,
    .instance          = DEMO_MICBUF_TX_INSTANCE,
};

hal_audio_dma_config_t rxSpeakerDmaConfig = {
    .instance             = EXAMPLE_DMA_INSTANCE,
    .channel              = EXAMPLE_SPKBUF_RX_CHANNEL,
    .enablePreemption     = false,
    .enablePreemptAbility = false,
    .priority             = kHAL_AudioDmaChannelPriorityDefault,
    .dmaMuxConfig         = NULL,
    .dmaChannelMuxConfig  = NULL,
};

hal_audio_config_t rxSpeakerConfig = {
    .dmaConfig     = &rxSpeakerDmaConfig,
    .ipConfig      = NULL,
    .srcClock_Hz   = 24576000,
    .sampleRate_Hz = (uint32_t)kHAL_AudioSampleRate8KHz,
#if (defined(WIFI_88W8987_BOARD_AW_CM358_USD) || defined(WIFI_IW416_BOARD_MURATA_1ZM_USD))
    .f rameLength = 22, /* Here is 22 because the bt module will generate 22 bits clock after one clock WS. */
#elif (defined(WIFI_IW416_BOARD_AW_AM510_USD) || defined(WIFI_IW416_BOARD_AW_AM457_USD) || \
       defined(WIFI_IW416_BOARD_MURATA_1XK_USD) || defined(WIFI_IW612_BOARD_MURATA_2EL_USD))
    .frameLength = 256, /* Here is 256 because the bt module will generate 256 bits clock after one clock WS. */
#else
#endif
    .fifoWatermark     = 0,
    .masterSlave       = kHAL_AudioSlave,
    .bclkPolarity      = kHAL_AudioSampleOnFallingEdge,
    .frameSyncWidth    = kHAL_AudioFrameSyncWidthOneBitClk,
    .frameSyncPolarity = kHAL_AudioBeginAtRisingEdge,
    .lineChannels      = DEMO_SPKBUF_RX_CHANNEL,
    .dataFormat        = kHAL_AudioDataFormatDspModeA,
    .bitWidth          = (uint8_t)kHAL_AudioWordWidth16bits,
    .instance          = DEMO_SPKBUF_RX_INSTANCE,
};
/*${variable:end}*/
#endif

uint8_t domainId;
uint8_t APP_GetMCoreDomainID(void)
{
    return 1U;
}

#if 0
/*${function:start}*/
void BOARD_SwitchAudioFrameLen(uint32_t sampleRate)
{
  if(8000 == sampleRate)
  {
#if (defined(WIFI_88W8987_BOARD_AW_CM358_USD) || defined(WIFI_88W8987_BOARD_MURATA_1ZM_USD))
      rxSpeakerConfig.frameLength = 22;
      txMicConfig.frameLength = 22;
#endif
  }
  else if(16000 == sampleRate)
  {
#if (defined(WIFI_88W8987_BOARD_AW_CM358_USD) || defined(WIFI_88W8987_BOARD_MURATA_1ZM_USD))
      rxSpeakerConfig.frameLength = 128;
      txMicConfig.frameLength = 128;
#endif
  }
}
#endif

void InitAudioPLLForAllAudioPeripherals(void)
{
    CLOCK_DeinitAudioPll();
	CLOCK_InitAudioPll(&audioPllConfig_48K);
	CLOCK_InitAudioPfd(kCLOCK_Pfd0, 22);         /* Enable Audio PLL clock */
	CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 18U); /* Set AUDIOPLLCLKDIV divider to value 18 */
}

void InitBaseAudioClkForPdm(void)
{
	//enalbe PDM clk
	/* DMIC source from audio pll, divider 8, 24.576M/8=3.072MHZ */
	CLOCK_AttachClk(kAUDIO_PLL_to_DMIC_CLK);
	//no matter BT side is 16KHz or 8KHz, DMIC is always 16KHz
	CLOCK_SetClkDiv(kCLOCK_DivDmicClk, 8);		//PDM clk is: 24.576/8 =3.072MHz --- OSR to be 48, PDM stream after CIC is: 3072k/48=64K --> then half down to 32KHz --> then half down to 16KHz (don't use 2Fs)
}
#if 0
void InitBaseAudioClkForFc5Fc6(void)
{
}
void InitBaseAudioClkForFc1Fc3(void)
{
    /* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
    CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
    /* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
    CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
    /* attach AUDIO PLL clock to FLEXCOMM1 (I2S2) */
    CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM2);
    /* attach AUDIO PLL clock to FLEXCOMM3 (I2S5) */
    CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM5);

    /* attach AUDIO PLL clock to MCLK (AudioPll * (18 / 26) / 15 / 1 = 24.576MHz / 22.5792MHz) */
    CLOCK_AttachClk(kAUDIO_PLL_to_MCLK_CLK);
    CLOCK_SetClkDiv(kCLOCK_DivMclkClk, 1);
    SYSCTL1->MCLKPINDIR = SYSCTL1_MCLKPINDIR_MCLKPINDIR_MASK;

    /* Set shared signal set 0: SCK, WS from Flexcomm1 */
    SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) | SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);
    /* Set flexcomm3 SCK, WS from shared signal set 0 */
    SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) | SYSCTL1_FCCTRLSEL_WSINSEL(1);

    /* Set shared signal set 1: SCK, WS from Flexcomm5 */
    SYSCTL1->SHAREDCTRLSET[1] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(5) | SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(5);
    /* Set flexcomm2 SCK, WS from shared signal set 1 */
    SYSCTL1->FCCTRLSEL[2] = SYSCTL1_FCCTRLSEL_SCKINSEL(2) | SYSCTL1_FCCTRLSEL_WSINSEL(2);
}
#endif


void SetFcClkSharing(int ClkSrcId, int ClkDstId, int InternalSharingGroupIdx)		//InternalSharingGroupIdx can only be 0 or 1
{
	if(InternalSharingGroupIdx>1)
		InternalSharingGroupIdx=1;

    SYSCTL1->SHAREDCTRLSET[InternalSharingGroupIdx] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(ClkSrcId) | SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(ClkSrcId);
    SYSCTL1->FCCTRLSEL[ClkDstId] = SYSCTL1_FCCTRLSEL_SCKINSEL(InternalSharingGroupIdx+1) | SYSCTL1_FCCTRLSEL_WSINSEL(InternalSharingGroupIdx+1);
}

uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate, int I2SClkShareCfgIdx)
{
    CLOCK_DeinitAudioPll();

    if (0U == sampleRate)
    {
        /* Disable MCLK output */
        SYSCTL1->MCLKPINDIR &= ~SYSCTL1_MCLKPINDIR_MCLKPINDIR_MASK;
    }
    else
    {

        if (44100U == sampleRate)
        {
			CLOCK_InitAudioPll(&audioPllConfig_44p1K);
			CLOCK_InitAudioPfd(kCLOCK_Pfd0, 22);         /* Enable Audio PLL clock */
			CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 17U); /* Set AUDIOPLLCLKDIV divider to value 17 */
        }
        else if (0U == sampleRate % 8000U)
        {
			CLOCK_InitAudioPll(&audioPllConfig_48K);
			CLOCK_InitAudioPfd(kCLOCK_Pfd0, 22);         /* Enable Audio PLL clock */
			CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 18U); /* Set AUDIOPLLCLKDIV divider to value 18 */
        }
        else
        {
        	//should not happen here
            // no action
        	return 0;
        }		


			#if EnableAudioPllAdjustingToSyncBetweenBtFsAndLocalFs==1
				AUDIOPLL0NUM_StartingUpValue = CLKCTL1->AUDIOPLL0NUM;
				AUDIOPLL0NUM_AdjustingValue=0;
			#endif

        /* attach main clock to I3C (500MHz / 20 = 25MHz). */
        CLOCK_AttachClk(kMAIN_CLK_to_I3C_CLK);
        CLOCK_SetClkDiv(kCLOCK_DivI3cClk, 20);


		switch(I2SClkShareCfgIdx)
		{
			case BtPcmFc5Fc2_CodecFc1Fc3:
				//this is for EVK board
		        /* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
		        /* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
		        /* attach AUDIO PLL clock to FLEXCOMM2 (I2S2) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM2);
		        /* attach AUDIO PLL clock to FLEXCOMM5 (I2S5) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM5);
		        //zzz to add NVT audio PLL later
		        //CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM6);
			break;
			case BtPcmFc2Fc4_AmpFc1Fc3:
				//this is for QR87 big board
				//BT HFP PCM (FC2, FC4) and Smart Amp (FC1, FC3) for both HFP and A2DP
		        /* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
		        /* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);
		        /* attach AUDIO PLL clock to FLEXCOMM2 (I2S2) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM2);
				/* attach AUDIO PLL clock to FLEXCOMM4 (I2S4) */
        		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM4);
		        //zzz to add NVT audio PLL later
		        //CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM6);
			break;
			case AmpFc1Fc3:
				//this is for QR87 big board
				//smart amplifier
		        /* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
		        /* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);

		        // I2S for NT98532 connection
		        /* attach AUDIO PLL clock to FLEXCOMM5 (I2S5) */
		        //CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM5);
		        /* attach AUDIO PLL clock to FLEXCOMM6 (I2S6) */
        		//CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM6);

			break;
			case NvtFc5Fc6_AmpFc1Fc3:
				//this is for QR87 big board
				//smart amplifier
		        /* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
		        /* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);

		        // I2S for NT98532 connection
		        /* attach AUDIO PLL clock to FLEXCOMM5 (I2S5) */
		        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM5);
		        /* attach AUDIO PLL clock to FLEXCOMM6 (I2S6) */
        		CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM6);


			break;
			default:
				//this is for ... reserved
				//...
			break;
		}

        // attach AUDIO PLL clock to MCLK , 24.576MHz or 22.5792MHz
        CLOCK_AttachClk(kAUDIO_PLL_to_MCLK_CLK);
        CLOCK_SetClkDiv(kCLOCK_DivMclkClk, 1);
        SYSCTL1->MCLKPINDIR = SYSCTL1_MCLKPINDIR_MCLKPINDIR_MASK;

		#if 0
			/* Set shared signal set 0: SCK, WS from Flexcomm1 */
			SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) | SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);
			/* Set flexcomm3 SCK, WS from shared signal set 0 */
			SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) | SYSCTL1_FCCTRLSEL_WSINSEL(1);

			/* Set shared signal set 1: SCK, WS from Flexcomm5 */
			SYSCTL1->SHAREDCTRLSET[1] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(5) | SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(5);
			/* Set flexcomm2 SCK, WS from shared signal set 1 */
			SYSCTL1->FCCTRLSEL[2] = SYSCTL1_FCCTRLSEL_SCKINSEL(2) | SYSCTL1_FCCTRLSEL_WSINSEL(2);
		#else
			switch(I2SClkShareCfgIdx)
			{
				case BtPcmFc5Fc2_CodecFc1Fc3:
					//this is for EVK board --- with BT connected
					SetFcClkSharing(FcIdx_RxFrBt,  FcIdx_TxToBt,  0);	//share clk of FcIdx_RxFrNvt to FcIdx_TxToNvt, using internal share group 0
					SetFcClkSharing(FcIdx_RxFrAmp, FcIdx_TxToAmp, 1);	//share clk of FcIdx_RxFrNvt to FcIdx_TxToNvt, using internal share group 1
				break;
				case BtPcmFc2Fc4_AmpFc1Fc3:
					//this is for QR87 big board --- with BT connected
					SetFcClkSharing(FcIdx_TxToBt, FcIdx_RxFrBt,  0);	//share clk of FcIdx_RxFrBt  to FcIdx_TxToBt,  using internal share group 0
						SetFcClkSharing(FcIdx_TxToAmp, FcIdx_RxFrAmp, 1);	//share clk of FcIdx_TxToAmp to FcIdx_RxFrAmp, using internal share group 1
				break;
				case AmpFc1Fc3:
					//this is for QR87 big board --- A2DP Amp only
					//SetFcClkSharing(FcIdx_TxToNvt,FcIdx_RxFrNvt , 0);	//share clk of FcIdx_RxFrNvt to FcIdx_TxToNvt, using internal share group 0
						SetFcClkSharing(FcIdx_TxToAmp, FcIdx_RxFrAmp, 1);	//share clk of FcIdx_TxToAmp to FcIdx_RxFrAmp, using internal share group 1
				break;
				case NvtFc5Fc6_AmpFc1Fc3:
					//this is for QR87 big board --- with NVT connected
					SetFcClkSharing(FcIdx_TxToNvt,FcIdx_RxFrNvt , 0);	//share clk of FcIdx_RxFrNvt to FcIdx_TxToNvt, using internal share group 0
						SetFcClkSharing(FcIdx_TxToAmp, FcIdx_RxFrAmp, 1);	//share clk of FcIdx_TxToAmp to FcIdx_RxFrAmp, using internal share group 1
				break;
			}
		#endif

        switch (sampleRate)
        {
            case 8000:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate8kHz;
                break;
            case 11025:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate11025Hz;
                break;
            case 12000:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate12kHz;
                break;
            case 16000:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate16kHz;
                break;
            case 22050:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate22050Hz;
                break;
            case 24000:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate24kHz;
                break;
            case 32000:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate32kHz;
                break;
            case 44100:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate44100Hz;
                break;
            case 48000:
                wm8904ScoConfig.format.sampleRate = kWM8904_SampleRate48kHz;
                break;
            default:
                /* codec does not support this sample rate. */
                break;
        }

        wm8904ScoConfig.i2cConfig.codecI2CSourceClock  = CLOCK_GetI3cClkFreq();
        wm8904ScoConfig.mclk_HZ                        = CLOCK_GetMclkClkFreq();
		#if 0
			wm8904ScoConfig1.format.sampleRate             = wm8904ScoConfig.format.sampleRate;
			wm8904ScoConfig1.i2cConfig.codecI2CSourceClock = wm8904ScoConfig.i2cConfig.codecI2CSourceClock;
			wm8904ScoConfig1.mclk_HZ                       = wm8904ScoConfig.mclk_HZ;
		#endif
    }
    //BOARD_SwitchAudioFrameLen(sampleRate);
    return CLOCK_GetMclkClkFreq();
}

static void i2c_release_bus_delay(void)
{
    uint32_t i = 0;
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
}


void BOARD_I3C_ReleaseBus(void)
{
    uint8_t i = 0;

    GPIO_PortInit(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT);
    GPIO_PortInit(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT);

    BOARD_InitI3CPinsAsGPIO();

    /* Drive SDA low first to simulate a start */
    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 0U);
    i2c_release_bus_delay();

    /* Send 9 pulses on SCL */
    for (i = 0; i < 9; i++)
    {
        GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                      BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 0U);
        i2c_release_bus_delay();

        GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                      BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 1U);
        i2c_release_bus_delay();

        GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                      BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 1U);
        i2c_release_bus_delay();
        i2c_release_bus_delay();
    }

    /* Send stop */
    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 1U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 1U);
    i2c_release_bus_delay();
}

extern void UsbAppInit(void);
void BOARD_InitHardware(void)
{
    DMA_Type *dmaBases[] = DMA_BASE_PTRS;
    /* Define the init structure for the reset pin*/
    gpio_pin_config_t reset_config = {
        kGPIO_DigitalOutput,
        1,
    };

#if UsingQAR87Board == 1
    gpio_pin_config_t IW611_PMIC_config = {
        kGPIO_DigitalOutput,
        1,
    };
    /* Attach main clock to I3C, 500MHz / 20 = 25Hz. */
    CLOCK_AttachClk(kMAIN_CLK_to_I3C_CLK);
    CLOCK_SetClkDiv(kCLOCK_DivI3cClk, 20);

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
	
    /* Attach AUX0_PLL clock to flexspi with divider 4*/
    //BOARD_SetFlexspiClock(2, 8);
    /* attach FRG0 clock to FLEXCOMM4 */
    CLOCK_SetFRGClock(BOARD_BT_UART_FRG_CLK);
    CLOCK_AttachClk(BOARD_BT_UART_CLK_ATTACH);

    BOARD_InitDebugConsole();

    /* Connect trigger sources to PINT */
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, kPINT_PinInt0, POWERKEY_PINT_PIN_INT0_SRC);
    INPUTMUX_AttachSignal(INPUTMUX, kPINT_PinInt1, FUNKEY_PINT_PIN_INT1_SRC);
    /* Turnoff clock to inputmux to save power. Clock is only needed to make changes */
    INPUTMUX_Deinit(INPUTMUX);


#else
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    BOARD_I3C_ReleaseBus();
    BOARD_InitI3CPins();

    /* Init output reset pin. */
    GPIO_PortInit(GPIO, 2);
    GPIO_PinInit(GPIO, 2, 12, &reset_config);

    /* Attach AUX0_PLL clock to flexspi with divider 4*/
    BOARD_SetFlexspiClock(2, 8);
    /* attach FRG0 clock to FLEXCOMM4 */
    CLOCK_SetFRGClock(BOARD_BT_UART_FRG_CLK);
    CLOCK_AttachClk(BOARD_BT_UART_CLK_ATTACH);

	PRINTF("RT685 MCU: start\r\n");
#endif

	PRINTF("\r\n");
	PRINTF("RT685 MCU: ----IW611 BT HFP A2DP with Conversa--- \r\n");
	PRINTF("RT685 MCU: ------------ McuVer 1.7.0 ------------ \r\n");
	PRINTF("RT685 MCU: ----IW611 BT HFP A2DP with Conversa--- \r\n");

	PRINTF("\r\n");
	PRINTF("RT685 MCU: size of shared memory structure is %d \r\n", sizeof(VarBlockSharedByDspAndMcu));

	KIN1_InitCycleCounter(); /* enable DWT hardware */
	KIN1_ResetCycleCounter(); /* reset cycle counter */
	KIN1_EnableCycleCounter(); /* start counting */

	PRINTF("RT685 MCU: Test get cycle counter without Jlink\r\n");
	TestGetCycCnt();		//can be closed after debug

#if UsingQAR87Board == 1
	#if UsingDbgPins == 1
		//InitDbgPin(); //should be called after gpio init
	#endif
	InitSineToneGen1();
	InitSineToneGen2();
	InitBtnEvt(); //need more editing for Quanta board
#else
	InitDbgPin();
	OpeningBlink(3);
	InitSineToneGen1();
	InitSineToneGen2();
	InitBtnEvt();
#endif

	#if EnableUsbComAndAudio==1
		#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
			/* attach AUDIO PLL clock to SCTimer input7. */
			CLOCK_AttachClk(kAUDIO_PLL_to_SCT_CLK);
			CLOCK_SetClkDiv(kCLOCK_DivSctClk, 1);

			g_composite.audioUnified.curAudioPllFrac = CLKCTL1->AUDIOPLL0NUM;		//starting value is 5040
			g_composite.audioUnified.curAudioPllFrac_starting = CLKCTL1->AUDIOPLL0NUM;
		#endif
		UsbAppInit();
	#endif

	//boot DSP and handshake with DSP
	#if 1	//folding
			//need to boot DSP
			PRINTF("RT685 MCU: Booting DSP\r\n");
			/* Clear MUA reset before run DSP core */
			RESET_PeripheralReset(kMU_RST_SHIFT_RSTn);

			//INPUTMUX_Init(INPUTMUX);
			//INPUTMUX_AttachSignal(INPUTMUX, 1U, kINPUTMUX_MuBToDspInterrupt);
			//INPUTMUX_Deinit(INPUTMUX);

			/* Clear SEMA42 reset */
			RESET_PeripheralReset(kSEMA_RST_SHIFT_RSTn);

			/* Clear MUA reset */
			RESET_PeripheralReset(kMU_RST_SHIFT_RSTn);

			/* SEMA42 init */
			SEMA42_Init(APP_SEMA42);
			/* Reset the sema42 gate */
			SEMA42_ResetAllGates(APP_SEMA42);
			domainId = APP_GetMCoreDomainID();

			MU_Init(APP_MU);
			EnableIRQ(MU_A_IRQn);

			/* Enable transmit and receive interrupt */
			MU_EnableInterrupts(APP_MU, kMU_Rx0FullInterruptEnable);

			PRINTF("RT685 MCU: Waiting for DSP handshake\r\n");
			BOARD_DSP_Init();

			/* Wait DSP core is Boot Up */
			while (BOOT_FLAG != MU_GetFlags(APP_MU))
			{
				delay_ms(1);
			};

			memset((void *)&VarBlockSharedByDspAndMcu, 0, sizeof(VarBlockSharedByDspAndMcu));

		//init opus file ptrs
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 0]=(unsigned int)&OpusFileBeg1;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 1]=(unsigned int)&OpusFileEnd1;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 2]=(unsigned int)&OpusFileBeg2;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 3]=(unsigned int)&OpusFileEnd2;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 4]=(unsigned int)&OpusFileBeg3;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 5]=(unsigned int)&OpusFileEnd3;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 6]=(unsigned int)&OpusFileBeg4;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 7]=(unsigned int)&OpusFileEnd4;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 8]=(unsigned int)&OpusFileBeg5;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[ 9]=(unsigned int)&OpusFileEnd5;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[10]=(unsigned int)&OpusFileBeg6;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[11]=(unsigned int)&OpusFileEnd6;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[12]=(unsigned int)&OpusFileBeg7;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[13]=(unsigned int)&OpusFileEnd7;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[14]=(unsigned int)&OpusFileBeg8;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[15]=(unsigned int)&OpusFileEnd8;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[16]=(unsigned int)&OpusFileBeg9;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[17]=(unsigned int)&OpusFileEnd9;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[18]=(unsigned int)&OpusFileBeg10;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[19]=(unsigned int)&OpusFileEnd10;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[20]=(unsigned int)&OpusFileBeg11;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[21]=(unsigned int)&OpusFileEnd11;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[22]=(unsigned int)&OpusFileBeg12;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[23]=(unsigned int)&OpusFileEnd12;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[24]=(unsigned int)&OpusFileBeg13;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[25]=(unsigned int)&OpusFileEnd13;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[26]=(unsigned int)&OpusFileBeg14;
		VarBlockSharedByDspAndMcu.FileAddrTable_Opus[27]=(unsigned int)&OpusFileEnd14;

		VarBlockSharedByDspAndMcu.FileAddrTable_Sbc[0]=(unsigned int)&SbcFileBeg1;
		VarBlockSharedByDspAndMcu.FileAddrTable_Sbc[1]=(unsigned int)&SbcFileEnd1;
		VarBlockSharedByDspAndMcu.FileAddrTable_Sbc[2]=(unsigned int)&SbcFileBeg2;
		VarBlockSharedByDspAndMcu.FileAddrTable_Sbc[3]=(unsigned int)&SbcFileEnd2;
		VarBlockSharedByDspAndMcu.FileAddrTable_Sbc[4]=(unsigned int)&SbcFileBeg3;
		VarBlockSharedByDspAndMcu.FileAddrTable_Sbc[5]=(unsigned int)&SbcFileEnd3;


		VarBlockSharedByDspAndMcu.Cm33Hifi1HandShakeCheck=0x1234abcd;		//a flag for DSP side to check
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, (U32)&VarBlockSharedByDspAndMcu);

			DelayMsByReadingCycCnt(20);		//wait a while to let DSP priting finish

		SEMA42_Lock(APP_SEMA42, SEMA42_GATE0, domainId);
			BOARD_InitDebugConsole();		//not sure --- conflict with DSP init debug console --- earlier prints can not be displayed
			PRINTF("RT685 MCU: DSP handshake is received\r\n");
		SEMA42_Unlock(APP_SEMA42, SEMA42_GATE0);
#endif

    DMA_Init(dmaBases[EXAMPLE_DMA_INSTANCE]);

#if (((defined(CONFIG_BT_SMP)) && (CONFIG_BT_SMP)))
    CRYPTO_InitHardware();
#endif /* CONFIG_BT_SMP */
}

#if defined(WIFI_88W8987_BOARD_AW_CM358_USD) || defined(WIFI_IW416_BOARD_MURATA_1XK_USD) || \
    defined(WIFI_88W8987_BOARD_MURATA_1ZM_USD) || defined(WIFI_IW612_BOARD_MURATA_2EL_USD)
int controller_hci_uart_get_configuration(controller_hci_uart_config_t *config)
{
    if (NULL == config)
    {
        return -1;
    }
    config->clockSrc        = BOARD_BT_UART_CLK_FREQ;
    config->defaultBaudrate = 115200u;
    config->runningBaudrate = BOARD_BT_UART_BAUDRATE;
    config->instance        = BOARD_BT_UART_INSTANCE;
    config->enableRxRTS     = 1u;
    config->enableTxCTS     = 1u;
    return 0;
}
#elif (defined(WIFI_IW416_BOARD_AW_AM510_USD) || defined(WIFI_IW416_BOARD_AW_AM457_USD))
int controller_hci_uart_get_configuration(controller_hci_uart_config_t *config)
{
    if (NULL == config)
    {
        return -1;
    }
    config->clockSrc = BOARD_BT_UART_CLK_FREQ;
    config->defaultBaudrate = BOARD_BT_UART_BAUDRATE;
    config->runningBaudrate = BOARD_BT_UART_BAUDRATE;
    config->instance = BOARD_BT_UART_INSTANCE;
    config->enableRxRTS = 1u;
    config->enableTxCTS = 1u;
    return 0;
}
#else
#endif

#if CONFIG_BT_HOST_USB_ENABLE==1
void USB_HostClockInit(void)
{
    uint8_t usbClockDiv = 1;
    uint32_t usbClockFreq;
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
    /* enable USB IP clock */
    CLOCK_SetClkDiv(kCLOCK_DivPfc1Clk, 5);
    CLOCK_AttachClk(kXTALIN_CLK_to_USB_CLK);
    CLOCK_SetClkDiv(kCLOCK_DivUsbHsFclk, usbClockDiv);
    CLOCK_EnableUsbhsHostClock();
    RESET_PeripheralReset(kUSBHS_PHY_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSBHS_DEVICE_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSBHS_HOST_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSBHS_SRAM_RST_SHIFT_RSTn);
    /*Make sure USDHC ram buffer has power up*/
    POWER_DisablePD(kPDRUNCFG_APD_USBHS_SRAM);
    POWER_DisablePD(kPDRUNCFG_PPD_USBHS_SRAM);
    POWER_ApplyPD();

    /* save usb ip clock freq*/
    usbClockFreq = g_xtalFreq / usbClockDiv;
    /* enable USB PHY PLL clock, the phy bus clock (480MHz) source is same with USB IP */
    CLOCK_EnableUsbHs0PhyPllClock(kXTALIN_CLK_to_USB_CLK, usbClockFreq);

#if ((defined FSL_FEATURE_USBHSH_USB_RAM) && (FSL_FEATURE_USBHSH_USB_RAM > 0U))

    for (int i = 0; i < (FSL_FEATURE_USBHSH_USB_RAM >> 2); i++)
    {
        ((uint32_t *)FSL_FEATURE_USBHSH_USB_RAM_BASE_ADDRESS)[i] = 0U;
    }
#endif
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL_SYS_CLK_HZ, &phyConfig);

    CLOCK_EnableClock(kCLOCK_UsbhsDevice);
    USBHSH->PORTMODE &= ~USBHSH_PORTMODE_DEV_ENABLE_MASK;
    while (SYSCTL0->USBCLKSTAT & SYSCTL0_USBCLKSTAT_DEV_NEED_CLKST_MASK)
    {
        __ASM("nop");
    }
    /* disable usb1 device clock */
    CLOCK_DisableClock(kCLOCK_UsbhsDevice);

    CLOCK_EnableClock(kCLOCK_UsbhsDevice);
    USBHSH->PORTMODE &= ~USBHSH_PORTMODE_DEV_ENABLE_MASK;
    while (SYSCTL0->USBCLKSTAT & SYSCTL0_USBCLKSTAT_DEV_NEED_CLKST_MASK)
    {
        __ASM("nop");
    }
    /* disable usb1 device clock */
    CLOCK_DisableClock(kCLOCK_UsbhsDevice);
}
void USB_HostIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbHOSTEhciIrq[] = USBHSH_IRQS;
    irqNumber                = usbHOSTEhciIrq[CONTROLLER_ID - kUSB_ControllerIp3516Hs0];
    /* USB_HOST_CONFIG_EHCI */

    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);

    EnableIRQ((IRQn_Type)irqNumber);
}
#endif

/*${function:end}*/
