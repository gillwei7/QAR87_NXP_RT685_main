/*
 * Copyright 2020, 2024 NXP
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
#include "glf70583.h"
#include "pmic_support.h"
#include "pmic_pca9422.h"
#include "usb_host_config.h"
#include "usb_phy.h"
#include "usb_host.h"
#if (((defined(CONFIG_BT_SMP)) && (CONFIG_BT_SMP)))
#include "ksdk_mbedtls.h"
#endif /* CONFIG_BT_SMP */
#include "aw88166.h"
#include "qar87_config.h"
#include "app_a2dp_sink.h"
/*${header:end}*/

/*${macro:start}*/
#if defined(__GIC_PRIO_BITS)
#define USB_HOST_INTERRUPT_PRIORITY (25U)
#elif defined(__NVIC_PRIO_BITS) && (__NVIC_PRIO_BITS >= 3)
#define USB_HOST_INTERRUPT_PRIORITY (6U)
#else
#define USB_HOST_INTERRUPT_PRIORITY (3U)
#endif
/*${macro:end}*/

/*${variable:start}*/

#define DEMO_AUDIO_INSTANCE (1)//B36932 (3)

/* DMA */
#define EXAMPLE_DMA_INSTANCE (0)
#define EXAMPLE_TX_CHANNEL   (3)//B36932(7)

/* demo audio data channel */
#define DEMO_AUDIO_DATA_CHANNEL (kHAL_AudioStereo)
/* demo audio bit width */
#define DEMO_AUDIO_BIT_WIDTH (kHAL_AudioWordWidth16bits)
/* demo audio sample frequency */
#define DEMO_AUDIO_SAMPLING_RATE (kHAL_AudioSampleRate44100Hz)

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

hal_audio_dma_config_t audioTxDmaConfig = {
    .instance             = EXAMPLE_DMA_INSTANCE,
    .channel              = EXAMPLE_TX_CHANNEL,
    .enablePreemption     = false,
    .enablePreemptAbility = false,
    .priority             = kHAL_AudioDmaChannelPriorityDefault,
    .dmaMuxConfig         = NULL,
    .dmaChannelMuxConfig  = NULL,
};

hal_audio_config_t audioTxConfig = {
    .dmaConfig         = &audioTxDmaConfig,
    .ipConfig          = NULL,
    .srcClock_Hz       = 0,
    .sampleRate_Hz     = (uint32_t)DEMO_AUDIO_SAMPLING_RATE,
    .fifoWatermark     = 0,
    .masterSlave       = kHAL_AudioMaster,//B36932 kHAL_AudioSlave,
    .bclkPolarity      = kHAL_AudioSampleOnRisingEdge,
    .frameSyncWidth    = kHAL_AudioFrameSyncWidthHalfFrame,
    .frameSyncPolarity = kHAL_AudioBeginAtFallingEdge,
    .lineChannels      = DEMO_AUDIO_DATA_CHANNEL,
    .dataFormat        = kHAL_AudioDataFormatI2sClassic,
    .bitWidth          = (uint8_t)DEMO_AUDIO_BIT_WIDTH,
    .instance          = DEMO_AUDIO_INSTANCE,
};

/*
 * AUDIO PLL setting: Frequency = Fref * (MULT + NUM / DENOM)
 *                              = 24 * (20 + 5040/13125)
 *                              = 489.216MHz
 */
/*setting for 44.1Khz*/
const clock_audio_pll_config_t audioPllConfig = {
    .audio_pll_src  = kCLOCK_AudioPllXtalIn, /* OSC clock */
    .numerator      = 5040,                  /* Numerator of the Audio PLL fractional loop divider is null */
    .denominator    = 13125,                 /* Denominator of the Audio PLL fractional loop divider is null */
    .audio_pll_mult = kCLOCK_AudioPllMult20  /* Divide by 20 */
};

/*
 * AUDIO PLL setting: Frequency = Fref * (MULT + NUM / DENOM)
 *                              = 24 * (22 + 5040/27000)
 *                              = 532.48MHz
 */
/*setting for multiple of 8Khz,such as 48Khz/16Khz/32KHz*/
const clock_audio_pll_config_t audioPllConfig1 = {
    .audio_pll_src  = kCLOCK_AudioPllXtalIn, /* OSC clock */
    .numerator      = 5040,                  /* Numerator of the Audio PLL fractional loop divider is null */
    .denominator    = 27000,                 /* Denominator of the Audio PLL fractional loop divider is null */
    .audio_pll_mult = kCLOCK_AudioPllMult22  /* Divide by 22 */
};
/*${variable:end}*/

/*${function:start}*/

uint32_t BOARD_SwitchAudioFreq(uint32_t sampleRate)
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
            CLOCK_InitAudioPll(&audioPllConfig);
        }
        else if (0U == sampleRate % 8000U)
        {
            CLOCK_InitAudioPll(&audioPllConfig1);
        }
        else
        {
            /* no action */
        }

        CLOCK_InitAudioPfd(kCLOCK_Pfd0, 26);         /* Enable Audio PLL clock */
        CLOCK_SetClkDiv(kCLOCK_DivAudioPllClk, 15U); /* Set AUDIOPLLCLKDIV divider to value 15 */

        /* attach main clock to I3C (500MHz / 20 = 25MHz). */
        CLOCK_AttachClk(kMAIN_CLK_to_I3C_CLK);
        CLOCK_SetClkDiv(kCLOCK_DivI3cClk, 20);

        /* attach AUDIO PLL clock to FLEXCOMM1 (I2S1) */
        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM1);
        /* attach AUDIO PLL clock to FLEXCOMM3 (I2S3) */
        CLOCK_AttachClk(kAUDIO_PLL_to_FLEXCOMM3);

        /* attach AUDIO PLL clock to MCLK (AudioPll * (18 / 26) / 15 / 1 = 24.576MHz / 22.5792MHz) */
        CLOCK_AttachClk(kAUDIO_PLL_to_MCLK_CLK);
        CLOCK_SetClkDiv(kCLOCK_DivMclkClk, 1);
        SYSCTL1->MCLKPINDIR = SYSCTL1_MCLKPINDIR_MCLKPINDIR_MASK;

        /* Set shared signal set 0: SCK, WS from Flexcomm1 */
        SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(1) | SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(1);
        /* Set flexcomm3 SCK, WS from shared signal set 0 */
        SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) | SYSCTL1_FCCTRLSEL_WSINSEL(1);

        wm8904Config.i2cConfig.codecI2CSourceClock = CLOCK_GetI3cClkFreq();
        wm8904Config.mclk_HZ                       = CLOCK_GetMclkClkFreq();
#if 0
        switch (sampleRate)
        {
            case 8000:
                wm8904Config.format.sampleRate = kWM8904_SampleRate8kHz;
                break;
            case 11025:
                wm8904Config.format.sampleRate = kWM8904_SampleRate11025Hz;
                break;
            case 12000:
                wm8904Config.format.sampleRate = kWM8904_SampleRate12kHz;
                break;
            case 16000:
                wm8904Config.format.sampleRate = kWM8904_SampleRate16kHz;
                break;
            case 22050:
                wm8904Config.format.sampleRate = kWM8904_SampleRate22050Hz;
                break;
            case 24000:
                wm8904Config.format.sampleRate = kWM8904_SampleRate24kHz;
                break;
            case 32000:
                wm8904Config.format.sampleRate = kWM8904_SampleRate32kHz;
                break;
            case 44100:
                wm8904Config.format.sampleRate = kWM8904_SampleRate44100Hz;
                break;
            case 48000:
                wm8904Config.format.sampleRate = kWM8904_SampleRate48kHz;
                break;
            default:
                /* codec does not support this sample rate. */
                break;
        }
#endif
    }

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

static void Scan_I2C_Devices(I3C_Type *base)
{
    uint8_t dummyData = 0x00;
    status_t result;

    PRINTF("[I2C]Scanning I2C addresses...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) // I2C valid 7-bit address range
    {
        result = BOARD_I3C_Send(base, addr, 0x00, 0, &dummyData, 0);

        if (result == kStatus_Success)
        {
        	PRINTF("[I2C]Device found at 0x%02X\n", addr);
        }
    }

    PRINTF("[I2C]Scan complete.\n");
}

void BOARD_InitPMICs(void)
{
	/* Define the init structure for the input switch pin */
	gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
	gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
	/* Init input switch GPIO. */
	EnableIRQ(GPIO_INTA_IRQn);

	//PMIC_GLF70583_ENABLE
    /* Init GPIO */
    GPIO_PortInit(GPIO, PWR_SW1_PORT);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);


    BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);

    //PMIC_PCA9422_ENABLE
   /* Init PCA9422 PMIC. */
	BOARD_InitPmic();
	//PRINTF("-------------- PCA9422 BOARD_InitPmic OK--------------\r\n");
	/* Apply PMIC mode and voltage settings */
	BOARD_Init_PMICConfigure();
	//PRINTF("-------------- PCA9422 BOARD_Init_PMICConfigure OK--------------\r\n");

	//PMIC_GLF70583_ENABLE
	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
	//PRINTF("[GLF70583]top_stat:%X \n",top_stat);

	//Solution: The manufacturer did not set it to LOAD SWITCH
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0xF5, 0xC6);
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB8);
	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB9);

	// BUCK1 Delay 4ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x66, 0x0C);
	// BUCK2 Delay 2ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x67, 0x08);
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x67, 0x08);
	// BUCK3 Delay 0ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x68, 0x00);
	// LDO1 Delay 5ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x6A, 0x12);
	// 0x25->BUCK4?BLDO2 off
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x26, 0xE8);
	// 0x26->BUCK2 ON?BOthers off
	//glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xD0);

	//uint8_t ch = GETCHAR();
	//PRINTF("GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583
	// PMIC_GLF70583_ENABLE
}

void BOARD_InitHardware(void)
{
    DMA_Type *dmaBases[] = DMA_BASE_PTRS;

    /* Attach main clock to I3C, 500MHz / 20 = 25Hz. */
    CLOCK_AttachClk(kMAIN_CLK_to_I3C_CLK);
    CLOCK_SetClkDiv(kCLOCK_DivI3cClk, 20);
	
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    //BOARD_InitDebugConsole();
    //BOARD_I3C_ReleaseBus();
    //BOARD_InitI3CPins();

    DMA_Init(dmaBases[EXAMPLE_DMA_INSTANCE]);

    CLOCK_EnableClock(kCLOCK_InputMux);

    /* Define the init structure for the reset pin*/
    //gpio_pin_config_t reset_config = {
    //    kGPIO_DigitalOutput,
    //    1,
    //};

    /* Init output reset pin. */
    //GPIO_PortInit(GPIO, 2);
    //GPIO_PinInit(GPIO, 2, 12, &reset_config);

    /* Attach AUX0_PLL clock to flexspi with divider 4*/
    BOARD_SetFlexspiClock(2, 8);
    /* attach FRG0 clock to FLEXCOMM4 */
//    CLOCK_SetFRGClock(BOARD_BT_UART_FRG_CLK);
//    CLOCK_AttachClk(BOARD_BT_UART_CLK_ATTACH);

    BOARD_InitPMICs();

    BOARD_InitDebugConsole();
#if (((defined(CONFIG_BT_SMP)) && (CONFIG_BT_SMP)))
    CRYPTO_InitHardware();
#endif /* CONFIG_BT_SMP */

    // Debug pin
    gpio_pin_config_t GPIO_Output_config =
    {
        kGPIO_DigitalOutput,
        1,
    };
//    GPIO_PortInit(GPIO, 0);
    GPIO_PinInit(GPIO, DbgPin5Port, DbgPin5, &GPIO_Output_config);
    DbgPin5Dn();
    a2dp_delay_ms(1);
    for(int i=0;i<5;i++)
    {
        DbgPin5Up();a2dp_delay_ms(1);
        DbgPin5Dn();a2dp_delay_ms(1);
    }

    gpio_pin_config_t amp_config = {
                    kGPIO_DigitalOutput,
                    1,
                };
            GPIO_PinInit(GPIO, AMP_RESET_PORT, AMP_RESET_PIN, &amp_config);
        PRINTF("[AMP][AW88166]\r\n");
        init_aw88166();
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
    config->clockSrc        = BOARD_BT_UART_CLK_FREQ;
    config->defaultBaudrate = BOARD_BT_UART_BAUDRATE;
    config->runningBaudrate = BOARD_BT_UART_BAUDRATE;
    config->instance        = BOARD_BT_UART_INSTANCE;
    config->enableRxRTS     = 1u;
    config->enableTxCTS     = 1u;
    return 0;
}
#else
#endif

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

/*${function:end}*/
