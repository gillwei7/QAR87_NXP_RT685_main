/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "GlobalDef.h"
#if EnableUsbComAndAudio==1

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_msc.h"
#include "usb_device_cdc_acm.h"

#include "usb_device_audio.h"
#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"

#include "UsbApp.h"

#include "fsl_device_registers.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"

#include "fsl_usart.h"
#include "fsl_dmic.h"

#include <stdio.h>
#include <stdlib.h>
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
#if defined(FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U)
#include "usb_phy.h"
#endif

#include "pin_mux.h"
#include "fsl_i2c.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"

#include "fsl_inputmux.h"
#include "fsl_mu.h"
#include "fsl_sema42.h"
#include "fsl_power.h"
#include <stdbool.h>
#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
#include "fsl_sctimer.h"
#endif



#include "dsp_support.h"
#include "SubFunc.h"
#include "CircularBufManagement.h"
#include "CircularBuf.h"



/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define BOARD_I2S_DEMO_I2C_BASEADDR (I2C4)
#define DEMO_I2C_MASTER_CLOCK_FREQUENCY CLOCK_GetMclkClkFreq()
#define DEMO_I2S_TX (I2S3)
#define DEMO_DMA (DMA0)
#define DEMO_I2S_TX_CHANNEL (7)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_InitHardware(void);
void USB_DeviceClockInit(void);
void USB_DeviceIsrEnable(void);
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle);
#endif

usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);
extern usb_status_t USB_DeviceMscDiskInit(usb_device_composite_struct_t *deviceComposite);

#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0)) && \
    (defined(USB_DEVICE_MSC_USE_WRITE_TASK) && (USB_DEVICE_MSC_USE_WRITE_TASK > 0))
extern void USB_DeviceMscInitQueue(void);
extern void USB_DeviceMscWriteTask(void);
#endif

extern void Init_Board_Audio(void);
extern void USB_AudioSpeakerResetTask(void);
//extern void Board_DMIC_DMA_Init(void);
/*******************************************************************************
 * Variables
 ******************************************************************************/
extern U16 UsbUpStreamingIsStarted;
extern U16 UsbDnStreamingIsStarted;

extern U32 AudioIoFrameCnt;
//U32 AudioIoFrameCntForMuteMicInputAtStartingUp=300;		//mute mic input for 300ms
U32 AudioIoFrameCntForMuteMicInputAtStartingUp=1;		//--- with other init time longer than 300ms, this value can be decreased

float UsbDnStreamGainPrevValueL=0.0f;
float UsbDnStreamGainPrevValueR=0.0f;
float UsbDnStreamGainL=1.0f;
float UsbDnStreamGainR=1.0f;


#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
static uint32_t eventCounterU = 0;
static uint32_t captureRegisterNumber;
static sctimer_config_t sctimerInfo;
#endif
/* Composite device structure. */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)

usb_device_composite_struct_t g_composite;
usb_device_composite_struct_t *PtrUsbDevComposite;

extern usb_device_class_struct_t g_UsbDeviceCdcVcomConfig;
extern usb_device_class_struct_t g_UsbDeviceAudioClassRecorder;
extern usb_device_class_struct_t g_UsbDeviceAudioClassSpeaker;
extern usb_device_class_struct_t g_mscDiskClass;
extern usb_device_class_struct_t g_UsbDeviceAudioClassMidi;

#if (defined(USB_DEVICE_CONFIG_USE_TASK) && (USB_DEVICE_CONFIG_USE_TASK > 0)) && \
    (defined(USB_DEVICE_MSC_USE_WRITE_TASK) && (USB_DEVICE_MSC_USE_WRITE_TASK > 0))
extern usb_msc_buffer_struct_t *currentTrasfer;
#endif


/* USB device class information */
static usb_device_class_config_struct_t g_CompositeClassConfig[4] = {{
                                                                         USB_DeviceCdcVcomCallback,
                                                                         (class_handle_t)NULL,
                                                                         &g_UsbDeviceCdcVcomConfig,
                                                                     },
                                                                     {
                                                                         USB_DeviceAudioCompositeCallback,
                                                                         (class_handle_t)NULL,
                                                                         &g_UsbDeviceAudioClassRecorder,
                                                                     },
                                                                     {
                                                                         USB_DeviceAudioCompositeCallback,
                                                                         (class_handle_t)NULL,
                                                                         &g_UsbDeviceAudioClassSpeaker,
                                                                     },
#ifdef EnableMscInTheDemo
																,
                                                                {
                                                                    USB_DeviceMscCallback,
                                                                    (class_handle_t)NULL,
                                                                    &g_mscDiskClass,
                                                                }
#endif
};

/* USB device class configuraion information */
static usb_device_class_config_list_struct_t g_UsbDeviceCompositeConfigList =
{
    g_CompositeClassConfig,
    USB_DeviceCallback,
#ifdef EnableMscInTheDemo
	4
#else
	3
#endif
};

/*******************************************************************************
 * Code
 ******************************************************************************/
// global volatile counter used for sleep/wait


#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
uint32_t AODOfUacDnBuf = 0;		//amount of audio data in the UAC dn buffer
__attribute__((section("CodeQuickAccess")))
void SCTIMER_SOF_TOGGLE_HANDLER()
{
	return;	//in this hfp demo, we do i2s sync to BT, not to UAC --- UAC audio upstreaming will be adjusted by USB_MicUpStreamDataRateControl_AdjustPacketLength
			//UAC dn, will not be used, and if used, will not be synchronized

    uint32_t currentSctCap = 0, pllCountPeriod = 0, pll_change = 0;
    static int32_t pllCount = 0, pllDiff = 0;
    static int32_t err, abs_err;

    //LedPin1Up();LedPin1Up();LedPin1Up();LedPin1Up();LedPin1Up();
    if (SCTIMER_GetStatusFlags(SCT0) & (1 << eventCounterU))
    {
        /* Clear interrupt flag.*/
        SCTIMER_ClearStatusFlags(SCT0, (1 << eventCounterU));
    }

    //don't open the next line in real project runnint --- open the next line is only to show the effect of USB audio data rate sync
    //return;		//return from here means disable the CTimer adjusting Audio PLL --- this is to show the effect of using CTimer

    if (g_composite.audioUnified.speakerIntervalCount != AUDIO_PLL_ADJUST_INTERVAL)
    {
        g_composite.audioUnified.speakerIntervalCount++;
        //LedPin1Dn();
        return;
    }

    g_composite.audioUnified.speakerIntervalCount = 1;
    currentSctCap                                 = SCT0->CAP[0];
    pllCountPeriod                                = currentSctCap - g_composite.audioUnified.audioPllTicksPrev;
    g_composite.audioUnified.audioPllTicksPrev    = currentSctCap;
    pllCount                                      = pllCountPeriod;
    if (g_composite.audioUnified.attach)
    {
        if (abs(pllCount - AUDIO_PLL_USB1_SOF_INTERVAL_COUNT) < (AUDIO_PLL_USB1_SOF_INTERVAL_COUNT >> 7))
        {
            pllDiff = pllCount - g_composite.audioUnified.audioPllTicksEma;
            g_composite.audioUnified.audioPllTickEmaFrac += (pllDiff % 8);
            g_composite.audioUnified.audioPllTicksEma +=
                (pllDiff / 8) + g_composite.audioUnified.audioPllTickEmaFrac / 8;
            g_composite.audioUnified.audioPllTickEmaFrac = (g_composite.audioUnified.audioPllTickEmaFrac % 8);

            err     = g_composite.audioUnified.audioPllTicksEma - AUDIO_PLL_USB1_SOF_INTERVAL_COUNT;
            abs_err = abs(err);
            if (abs_err >= g_composite.audioUnified.audioPllStep)
            {
                if (err > 0)
                {
                    g_composite.audioUnified.curAudioPllFrac -= abs_err / g_composite.audioUnified.audioPllStep;
                }
                else
                {
                    g_composite.audioUnified.curAudioPllFrac += abs_err / g_composite.audioUnified.audioPllStep;
                }
                pll_change = 1;
            }
            /*
            if (g_composite.audioUnified.startPlayHalfFull)
            {
                if (AODOfUacDnBuf > UacDnBuf_LengthInSamples *3/4)
                {
                    g_composite.audioUnified.curAudioPllFrac++;
                    pll_change = 1;
                    PRINTF("+\n");
                }
                else if (AODOfUacDnBuf <= UacDnBuf_LengthInSamples *1/4)
                {
                    g_composite.audioUnified.curAudioPllFrac--;
                    pll_change = 1;
                    PRINTF("-\n");
                }
            }
            */
            if (pll_change)
            {
                CLKCTL1->AUDIOPLL0NUM = g_composite.audioUnified.curAudioPllFrac;
            }

			#if EnableMonitorUsbAudioUpStreamLengthAdjusting==1
				VarBlockSharedByDspAndMcu.MonitorInfoArray1[10]=err;
				VarBlockSharedByDspAndMcu.MonitorInfoArray1[11]=g_composite.audioUnified.curAudioPllFrac;
			#endif
        }
    }

    //LedPin1Dn();
}
void SCTIMER_CaptureInit(void)
{
	return;	//in this hfp demo, we do i2s sync to BT, not to UAC --- UAC audio upstreaming will be adjusted by USB_MicUpStreamDataRateControl_AdjustPacketLength
			//UAC dn, will not be used, and if used, will not be synchronized

    INPUTMUX->SCT0_IN_SEL[eventCounterU] = 0xFU; /* 0xFU for USB1.*/
    SCTIMER_GetDefaultConfig(&sctimerInfo);

    /* Switch to 16-bit mode */
    sctimerInfo.clockMode   = kSCTIMER_Input_ClockMode;
    sctimerInfo.clockSelect = kSCTIMER_Clock_On_Rise_Input_7;

    /* Initialize SCTimer module */
    SCTIMER_Init(SCT0, &sctimerInfo);

    if (SCTIMER_SetupCaptureAction(SCT0, kSCTIMER_Counter_U, &captureRegisterNumber, eventCounterU) == kStatus_Fail)
    {
        usb_echo("SCT Setup Capture failed!\r\n");
    }
    SCT0->EV[0].STATE = 0x1;
    SCT0->EV[0].CTRL  = (0x01 << 10) | (0x2 << 12);

    /* Enable interrupt flag for event associated with out 4, we use the interrupt to update dutycycle */
    SCTIMER_EnableInterrupts(SCT0, (1 << eventCounterU));

    /* Receive notification when event is triggered */
    SCTIMER_SetCallback(SCT0, SCTIMER_SOF_TOGGLE_HANDLER, eventCounterU);

    /* Enable at the NVIC */
	NVIC_SetPriority(SCT0_IRQn, (USB_DEVICE_INTERRUPT_PRIORITY - 1U));
    EnableIRQ(SCT0_IRQn);

    /* Start the L counter */
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);
}
#endif
__attribute__((section("CodeQuickAccess")))
void USB_IRQHandler(void)
{
	//DbgPin6Up();DbgPin5Up();
    USB_DeviceLpcIp3511IsrFunction(g_composite.deviceHandle);
	//DbgPin6Dn();DbgPin6Dn();
}

void USB_DeviceClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
    /* enable USB IP clock */
    CLOCK_SetClkDiv(kCLOCK_DivPfc1Clk, 5);
    CLOCK_AttachClk(kXTALIN_CLK_to_USB_CLK);
    CLOCK_SetClkDiv(kCLOCK_DivUsbHsFclk, 1);
    CLOCK_EnableUsbhsDeviceClock();
    RESET_PeripheralReset(kUSBHS_PHY_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSBHS_DEVICE_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSBHS_HOST_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kUSBHS_SRAM_RST_SHIFT_RSTn);
    /*Make sure USDHC ram buffer has power up*/
    POWER_DisablePD(kPDRUNCFG_APD_USBHS_SRAM);
    POWER_DisablePD(kPDRUNCFG_PPD_USBHS_SRAM);
    POWER_ApplyPD();

    //CLOCK_EnableUsbhsPhyClock();
    /* save usb ip clock freq*/
    uint32_t usbClockFreq = g_xtalFreq / 1;
    /* enable usb ram clock */
    CLOCK_EnableClock(kCLOCK_UsbhsSram);
    /* enable USB PHY PLL clock, the phy bus clock (480MHz) source is same with USB IP */
    CLOCK_EnableUsbHs0PhyPllClock(kXTALIN_CLK_to_USB_CLK, usbClockFreq);

#if defined(FSL_FEATURE_USBHSD_USB_RAM) && (FSL_FEATURE_USBHSD_USB_RAM)
    for (int i = 0; i < FSL_FEATURE_USBHSD_USB_RAM; i++)
    {
        ((uint8_t *)FSL_FEATURE_USBHSD_USB_RAM_BASE_ADDRESS)[i] = 0x00U;
    }
#endif
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL_SYS_CLK_HZ, &phyConfig);

    /* the following code should run after phy initialization and should wait some microseconds to make sure utmi clock
     * valid */
    /* enable usb1 host clock */
    CLOCK_EnableClock(kCLOCK_UsbhsHost);
    /*  Wait until host_needclk de-asserts */
    while (SYSCTL0->USBCLKSTAT & SYSCTL0_USBCLKSTAT_HOST_NEED_CLKST_MASK)
    {
        __ASM("nop");
    }
    /*According to reference mannual, device mode setting has to be set by access usb host register */
    USBHSH->PORTMODE |= USBHSH_PORTMODE_DEV_ENABLE_MASK;
    /* disable usb1 host clock */
    CLOCK_DisableClock(kCLOCK_UsbhsHost);
}
void USB_DeviceIsrEnable(void)
{
    uint8_t irqNumber;

    uint8_t usbDeviceIP3511Irq[] = USBHSD_IRQS;
    irqNumber                    = usbDeviceIP3511Irq[CONTROLLER_ID - kUSB_ControllerLpcIp3511Hs0];

    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    EnableIRQ((IRQn_Type)irqNumber);
}
#if USB_DEVICE_CONFIG_USE_TASK
void USB_DeviceTaskFn(void *deviceHandle)
{
    USB_DeviceLpcIp3511TaskFunction(deviceHandle);
}
#endif


__attribute__((section("CodeQuickAccess")))
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16   = (uint16_t *)param;
    uint8_t *temp8     = (uint8_t *)param;
    uint8_t count      = 0U;

    switch (event)
    {
		case kUSB_DeviceEventDetach:
				Evt_UsbUnPlug=1;
			break;
        case kUSB_DeviceEventBusReset:
        {
            for (count = 0U; count < USB_DEVICE_INTERFACE_COUNT; count++)
            {
                g_composite.currentInterfaceAlternateSetting[count] = 0U;
            }
            g_composite.attach               = 0U;
            g_composite.currentConfiguration = 0U;
            error                            = kStatus_USB_Success;
#if (defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)) || (defined(USB_DEVICE_CONFIG_LPCIP3511HS) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U))
            /* Get USB speed to configure the device, including max packet size and interval of the endpoints. */
            if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &g_composite.speed))
            {
                USB_DeviceSetSpeed(handle, g_composite.speed);
            }
#if (USB_DEVICE_CONFIG_AUDIO_CLASS_2_0)
	#if defined(USB_DEVICE_AUDIO_USE_SYNC_MODE) && (USB_DEVICE_AUDIO_USE_SYNC_MODE > 0U)
	#else
            if (USB_SPEED_HIGH == g_composite.speed)
            {
            	AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_1_TO_16_16);
            }else
            {
    			AUDIO_UPDATE_FEEDBACK_DATA(audioFeedBackBuffer, AUDIO_SAMPLING_RATE_1_TO_10_14);
            }
	#endif
#endif

#if (defined(USB_DEVICE_MSC_USE_WRITE_TASK) && (USB_DEVICE_MSC_USE_WRITE_TASK > 0))
            /*re-init the queue every time device is reset*/
            USB_DeviceMscInitQueue();
            currentTrasfer = NULL;
#endif
        }
#endif
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (0U == (*temp8))
            {
                g_composite.attach               = 0U;
                g_composite.currentConfiguration = 0U;
            }
            else if (USB_COMPOSITE_CONFIGURE_INDEX == (*temp8))
            {
                g_composite.attach = 1;
                USB_DeviceCdcVcomSetConfigure(g_composite.cdcVcom.cdcAcmHandle, *temp8);
				#ifdef EnableMscInTheDemo
					USB_DeviceMscDiskSetConfigure(g_composite.mscDisk.mscHandle, *temp8);
				#endif
                USB_DeviceAudioCompositeSetConfigure(g_composite.audioUnified.audioSpeakerHandle, *temp8);
                USB_DeviceAudioCompositeSetConfigure(g_composite.audioUnified.audioRecorderHandle, *temp8);
				
                g_composite.currentConfiguration = *temp8;
                error                            = kStatus_USB_Success;
            }
            else
            {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        case kUSB_DeviceEventSetInterface:

            if (g_composite.attach)
            {
                uint8_t interface                                       = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting                                = (uint8_t)(*temp16 & 0x00FFU);

                if (interface < USB_DEVICE_INTERFACE_COUNT)
                {
                    g_composite.currentInterfaceAlternateSetting[interface] = alternateSetting;
                }

                if (USB_AUDIO_RECORDER_STREAM_INTERFACE_INDEX == interface)
                {
                    error = USB_DeviceAudioRecorderSetInterface(g_composite.audioUnified.audioRecorderHandle, interface,
                                                                alternateSetting);
                }
                else if (USB_AUDIO_SPEAKER_STREAM_INTERFACE_INDEX == interface)
                {
                    error = USB_DeviceAudioSpeakerSetInterface(g_composite.audioUnified.audioSpeakerHandle, interface,
                                                               alternateSetting);
                }
                else
                {
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
            {
                *temp8 = g_composite.currentConfiguration;
                error  = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                if (interface < USB_DEVICE_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00U) | g_composite.currentInterfaceAlternateSetting[interface];
                    error   = kStatus_USB_Success;
                }
                else
                {
                    error = kStatus_USB_InvalidRequest;
                }
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                error = USB_DeviceGetConfigurationDescriptor(handle,
                                                             (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
#if (defined(USB_DEVICE_CONFIG_CV_TEST) && (USB_DEVICE_CONFIG_CV_TEST > 0U))
        case kUSB_DeviceEventGetDeviceQualifierDescriptor:
            if (param)
            {
                /* Get device descriptor request */
                error = USB_DeviceGetDeviceQualifierDescriptor(
                    handle, (usb_device_get_device_qualifier_descriptor_struct_t *)param);
            }
            break;
#endif
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        default:
            break;
    }

    return error;
}

void UsbAppInit(void)
{

    USB_DeviceClockInit();
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    g_composite.speed                            = USB_SPEED_FULL;
    g_composite.attach                           = 0U;

    g_composite.audioUnified.audioSpeakerHandle  = (class_handle_t)NULL;
    g_composite.audioUnified.audioRecorderHandle = (class_handle_t)NULL;

    g_composite.cdcVcom.cdcAcmHandle = (class_handle_t)NULL;
	#ifdef EnableMscInTheDemo
		g_composite.mscDisk.mscHandle 				 = (class_handle_t)NULL;
	#endif
    g_composite.deviceHandle         			= NULL;

	#ifdef EnableMscInTheDemo
		usb_echo("Please insert Disk\r\n");
		if (kStatus_USB_Success != USB_DeviceMscDiskStorageInit())
		{
			usb_echo("Disk init failed\r\n");
			return;
		}
	#endif

    if (kStatus_USB_Success !=
        USB_DeviceClassInit(CONTROLLER_ID, &g_UsbDeviceCompositeConfigList, &g_composite.deviceHandle))
    {
        usb_echo("USB device composite demo init failed\r\n");
        return;
    }
    else
    {
        //usb_echo("USB device composite demo\r\n");

        g_composite.cdcVcom.cdcAcmHandle             = g_UsbDeviceCompositeConfigList.config[0].classHandle;
        g_composite.audioUnified.audioRecorderHandle = g_UsbDeviceCompositeConfigList.config[1].classHandle;
        g_composite.audioUnified.audioSpeakerHandle  = g_UsbDeviceCompositeConfigList.config[2].classHandle;

		#ifdef EnableMscInTheDemo
			g_composite.mscDisk.mscHandle                = g_UsbDeviceCompositeConfigList.config[3].classHandle;
		#endif

        USB_DeviceAudioCompositeInit(&g_composite);
        USB_DeviceCdcVcomInit       (&g_composite);

		#ifdef EnableMscInTheDemo
			USB_DeviceMscDiskInit       (&g_composite);
		#endif
    }

    USB_DeviceIsrEnable();

    /*Add one delay here to make the DP pull down long enough to allow host to detect the previous disconnection.*/
    SDK_DelayAtLeastUs(5000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    USB_DeviceRun(g_composite.deviceHandle);

    usb_echo("RT685 MCU: USB composite device of VCOM and audio started\r\n");

}


#endif

















/*


09 02 40 01 05 01 00 c0 32
08 0b 00 03 01 00 20 02
09 04 00 00 00 01 01 20 00
09 24 01 00 02 04 6f 00 00
08 24 0a 10 01 07 00 00
11 24 02 01 01 02 00 10 02 03 00 00 00 00 00 00 00
12 24 06 02 01 0f 00 00 00 00 00 00 00 00 00 00 00 00
0c 24 03 03 01 01 00 02 10 00 00 00
11 24 02 04 01 01 00 10 02 03 00 00 00 00 00 00 00
12 24 06 05 04 0f 00 00 00 00 00 00 00 00 00 00 00 00
0c 24 03 06 01 03 00 05 10 00 00 00
09 04 01 00 00 01 02 20 00
09 04 01 01 01 01 02 20 00
10 24 01 03 00 01 01 00 00 00 02 03 00 00 00 00
06 24 02 01 02 10
07 05 82 0d c4 00 04
08 25 01 00 00 00 00 00
09 04 02 00 00 01 02 20 00
09 04 02 01 02 01 02 20 00
10 24 01 04 00 01 01 00 00 00 02 03 00 00 00 00
06 24 02 01 02 10
07 05 02 05 1c 00 01
08 25 01 00 00 00 00 00
07 05 83 11 04 00 04
08 0b 03 02 02 02 00 02
09 04 03 00 01 02 02 00 00
05 24 00 10 01
05 24 01 01 01
04 24 02 06
05 24 06 03 04
07 05 84 03 10 00
07 09 04 04 00 02
0a 00 00 00 07 05 85 02 00 02 00
07 05 05 02 00 02 00



09 02 48 01 05 01 00 80 32
08 0b 00 03 01 00 20 04
09 04 00 00 00 01 01 20 00
09 24 01 00 02 04 77 00 00
08 24 0a 10 01 07 00 00
11 24 02 01 01 02 00 10 08 01 00 00 00 00 00 00 00
12 24 06 02 01 0f 00 00 00 00 00 00 00 00 00 00 00 00
0c 24 03 03 01 01 00 02 10 00 00 00
08 24 0a 20 01 07 00 00
11 24 02 04 01 01 00 20 02 03 00 00 00 00 00 00 00
12 24 06 05 04 0f 00 00 00 00 00 00 00 00 00 00 00 00
0c 24 03 06 01 03 00 05 20 00 00 00
09 04 01 00 00 01 02 20 00
09 04 01 01 01 01 02 20 00
10 24 01 03 00 01 01 00 00 00 08 01 00 00 00 00
06 24 02 01 04 20
07 05 82 0d a0 00 02
08 25 01 00 00 00 00 00
09 04 02 00 00 01 02 20 00
09 04 02 01 02 01 02 20 00
10 24 01 04 00 01 01 00 00 00 02 03 00 00 00 00
06 24 02 01 04 20
07 05 02 05 68 00 02
08 25 01 00 00 00 00 00
07 05 83 11 04 00 02
08 0b 03 02 02 02 00 05
09 04 03 00 01 02 02 00 00
05 24 00 10 01
05 24 01 01 01
04 24 02 06
05 24 06 03 04
07 05 84 03 10 00
07 09 04 04 00 02
0a 00 00 00 07 05 85 02 00 02 00
07 05 05 02 00 02 00

*/






