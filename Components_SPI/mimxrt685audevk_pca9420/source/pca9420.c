/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_pca9420.h"
#include "app.h"
#include "fsl_power.h"
#include "pmic_support.h"
#include "pmic_pca9422.h"
#include "ktd202x_leds.h"
#include "elan_ewd608.h"
#include "bq256xx_charger.h"
#include "glf70583.h"
#include "aw88166.h"
#include "glf70302.h"
#include "aw93305.h"

#include "fsl_pint.h"
#include "fsl_dma.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "music.h"

#include "fsl_dmic.h"
#include "fsl_dmic_dma.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PCA9420_LAST_REG (PCA9420_MODECFG_3_3)

#define GAUGE_ENABLE 1
#define LED_ENABLE 1
#define PMIC_PCA9422_ENABLE 1
#define TOUCH_ENABLE 1
#if TOUCH_ENABLE
#define TOUCH_AW93305_ENABLE 0
#endif
#define CHARGER_ENABLE 1
#define PMIC_GLF70583_ENABLE 1
#define AMP_ENABLE 0
#define DMIC_ENABLE 1


#if DMIC_ENABLE
#ifndef DEMO_DMIC_NUMS
#define DEMO_DMIC_NUMS 2U
#endif
#define FIFO_DEPTH           (15U)
#define RECORD_BUFFER_SIZE   (128)
#define PLAYBACK_BUFFER_SIZE (128 * 2U)
#define BUFFER_NUM           (2U)
#endif


#define NOVATEK_UBOOT_PORT  GPIO2_PORT // GPIO2_PORT 通常在 board.h 中定義為 2U
#define NOVATEK_UBOOT_PIN   15U        // Pin 15
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
static pca9420_handle_t pca9420Handle;
static volatile bool pca9420IntFlag;
static uint8_t buffer[PCA9420_LAST_REG + 1];

volatile bool g_touch_int_flag = false;
volatile bool g_charger_int_flag = false;
volatile bool g_gauge_int_flag = false;

static dma_handle_t s_DmaTxHandle;
static i2s_config_t s_TxConfig;
static i2s_dma_handle_t s_TxHandle;
static i2s_transfer_t s_TxTransfer;

#if DMIC_ENABLE
static i2s_config_t tx_config;
static uint8_t s_buffer[PLAYBACK_BUFFER_SIZE * BUFFER_NUM];
static uint32_t volatile s_writeIndex = 0U;
static uint32_t volatile s_emptyBlock = BUFFER_NUM;

static dmic_dma_handle_t s_leftDmicDmaHandle;
static dma_handle_t s_leftDmicRxDmaHandle;

#if DEMO_DMIC_NUMS == 2U
static dmic_dma_handle_t s_rightDmicDmaHandle;
static dma_handle_t s_rightDmicRxDmaHandle;
#endif

static dma_handle_t s_i2sTxDmaHandle;
static i2s_dma_handle_t s_i2sTxHandle;

SDK_ALIGN(dma_descriptor_t s_leftDmaDescriptorPingpong[2], 16);

static dmic_transfer_t s_leftReceiveXfer[2U] = {
    /* transfer configurations for channel0 */
    {
        .data                   = s_buffer,
        .dataWidth              = sizeof(uint16_t),
        .dataSize               = RECORD_BUFFER_SIZE,
        .dataAddrInterleaveSize = kDMA_AddressInterleave2xWidth,
        .linkTransfer           = &s_leftReceiveXfer[1],
    },

    {
        .data                   = &s_buffer[PLAYBACK_BUFFER_SIZE],
        .dataWidth              = sizeof(uint16_t),
        .dataSize               = RECORD_BUFFER_SIZE,
        .dataAddrInterleaveSize = kDMA_AddressInterleave2xWidth,
        .linkTransfer           = &s_leftReceiveXfer[0],
    },
};

#if DEMO_DMIC_NUMS == 2U
SDK_ALIGN(dma_descriptor_t s_rightDmaDescriptorPingpong[2], 16);

static dmic_transfer_t s_rightReceiveXfer[2U] = {
    /* transfer configurations for channel0 */
    {
        .data                   = &s_buffer[2],
        .dataWidth              = sizeof(uint16_t),
        .dataSize               = RECORD_BUFFER_SIZE,
        .dataAddrInterleaveSize = kDMA_AddressInterleave2xWidth,
        .linkTransfer           = &s_rightReceiveXfer[1],
    },

    {
        .data                   = &s_buffer[PLAYBACK_BUFFER_SIZE + 2U],
        .dataWidth              = sizeof(uint16_t),
        .dataSize               = RECORD_BUFFER_SIZE,
        .dataAddrInterleaveSize = kDMA_AddressInterleave2xWidth,
        .linkTransfer           = &s_rightReceiveXfer[0],
    },
};
#endif
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/
#if AMP_ENABLE
static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
    /* Enqueue the same original buffer all over again */
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_TxTransferSendDMA(base, handle, *transfer);
}

static void StopSoundPlayback(void)
{
    PRINTF("[I2S1]Stopping sound playback\r\n");

    close_aw88166_pa(AW_DEV_0);
    close_aw88166_pa(AW_DEV_1);

    //I2S_TransferAbortDMA(DEMO_I2S_TX_toAmp, &s_TxHandle);
    I2S_TransferAbortDMA(DEMO_I2S_TX_toNova, &s_TxHandle);


}

static void StartSoundPlayback(void)
{
    PRINTF("[I2S1]Setup looping playback of sine wave\r\n");

    s_TxTransfer.data     = &g_Music[0];
    s_TxTransfer.dataSize = sizeof(g_Music);

    //I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX_toAmp, &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
    I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX_toNova, &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
    /* need to queue two transmit buffers so when the first one
     * finishes transfer, the other immediatelly starts */
    //I2S_TxTransferSendDMA(DEMO_I2S_TX_toAmp, &s_TxHandle, s_TxTransfer);
    I2S_TxTransferSendDMA(DEMO_I2S_TX_toNova, &s_TxHandle, s_TxTransfer);

    start_aw88166_pa(AW_DEV_0, "Music");
    start_aw88166_pa(AW_DEV_1, "Music");

}
#endif

#if DMIC_ENABLE
void dmic_Callback(DMIC_Type *base, dmic_dma_handle_t *handle, status_t status, void *userData)
{
    if (s_emptyBlock)
    {
        s_emptyBlock--;
    }
}

void i2s_Callback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{
    if (s_emptyBlock < BUFFER_NUM)
    {
        s_emptyBlock++;
    }
}
#endif

void GPIO_INTA_DriverIRQHandler(void)
{

	uint32_t status_1 = GPIO_PortGetInterruptStatus(GPIO, GPIO1_PORT, kGPIO_InterruptA);
	uint32_t status_2 = GPIO_PortGetInterruptStatus(GPIO, GPIO2_PORT, kGPIO_InterruptA);


    if (status_1 & (1 << TOUCH_INT_PIN)) { //Touch
        // 關閉中斷，避免重入
        GPIO_PinDisableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

        // 觸發中斷，清除中斷旗標
        GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

        // 執行對應處理
        g_touch_int_flag = true;
        //PRINTF("[Debug] TOUCH_GPIO_INTA_IRQHandler \r\n");
    }
    if (status_1 & (1 << CHARG_INT_PIN)) { //Charger

    	GPIO_PinDisableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
        g_charger_int_flag = true;
        //PRINTF("[Debug] PIO0_29_INTA_IRQHandler \r\n");
    }
    if (status_2 & (1 << GAUGE_INT_PIN)) { //Gauge

    	GPIO_PinDisableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
        g_gauge_int_flag = true;
        //PRINTF("[Debug] GAUGE_INT_PIN_INTA_IRQHandler \r\n");
    }

    SDK_ISR_EXIT_BARRIER;
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

void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status)
{
    PRINTF("\f\r\nPINT Pin Interrupt %d event detected.\r\n", pintr);
    uint8_t pin_state;
    if(pintr==0)
    {
    	pin_state = GPIO_PinRead(GPIO,POWER_KEY_PORT,POWER_KEY_PIN);

    }
    else if(pintr==1)
    {
    	pin_state = GPIO_PinRead(GPIO,FUN_KEY1_N_PORT,FUN_KEY1_N_PIN);
    }
    PRINTF(" pin_state:%d \r\n",pin_state);
}

/*! @brief Main function */
int main(void)
{

#if DMIC_ENABLE
    dmic_channel_config_t dmic_channel_cfg;
    i2s_transfer_t i2sTxTransfer;
    static uint32_t logCounter = 0;
#endif

    /* Init board hardware. */
    BOARD_InitHardware();
    PRINTF("-------------- PCA9422 BOARD_InitHardware OK--------------\r\n");

    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
    gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
    /* Init input switch GPIO. */
    EnableIRQ(GPIO_INTA_IRQn);

    GPIO_PortInit(GPIO, GPIO0_PORT);
    GPIO_PortInit(GPIO, GPIO1_PORT);
    GPIO_PortInit(GPIO, GPIO2_PORT);

    gpio_pin_config_t novatek_config = {
        kGPIO_DigitalOutput, // 設定為數位輸出
        1                    // 初始狀態設為 LOW (拉低)
    };
    GPIO_PinInit(GPIO, NOVATEK_UBOOT_PORT, NOVATEK_UBOOT_PIN, &novatek_config);
    PRINTF("Set Novatek Pin (PIO2_15) to OUTPUT LOW.\r\n");

    /* Initialize PINT */
    PINT_Init(EXAMPLE_PINT_BASE);
    PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt0, kPINT_PinIntEnableBothEdges, pint_intr_callback);
    PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt0);
    PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt1, kPINT_PinIntEnableBothEdges, pint_intr_callback);
    PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt1);

#if TOUCH_ENABLE
    GPIO_PinInit(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &sw_config);
    /* Enable GPIO pin interrupt */
    GPIO_SetPinInterruptConfig(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
#endif

#if PMIC_GLF70583_ENABLE
    /* Init GPIO */
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);
    GPIO_PinInit(GPIO, RESET553_N_PORT, RESET553_N_PIN, &output_int_config);
#endif

#if CHARGER_ENABLE
    GPIO_PinInit(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, &sw_config);
    /* Enable GPIO pin interrupt */
    GPIO_SetPinInterruptConfig(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
#endif


#if AMP_ENABLE
    gpio_pin_config_t amp_config = {
            kGPIO_DigitalOutput,
            1,
        };
	GPIO_PinInit(GPIO, AMP_RESET_PORT, AMP_RESET_PIN, &amp_config);
#endif

    BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);

#if PMIC_PCA9422_ENABLE
    /* Init PCA9422 GPIO */
    gpio_pin_config_t PCA9422_output_config = {kGPIO_DigitalOutput, 1,};
    GPIO_PinInit(GPIO, DVS_CTR0_PORT, DVS_CTR0_PIN, &PCA9422_output_config);
    GPIO_PinInit(GPIO, DVS_CTR1_PORT, DVS_CTR1_PIN, &PCA9422_output_config);
    GPIO_PinInit(GPIO, DVS_CTR2_PORT, DVS_CTR2_PIN, &PCA9422_output_config);
    GPIO_PinInit(GPIO, SLEEP_MODE0_PORT, SLEEP_MODE0_PIN, &PCA9422_output_config);
    GPIO_PinInit(GPIO, STBY_MODE0_PORT, STBY_MODE0_PIN, &PCA9422_output_config);
    GPIO_PinInit(GPIO, PCA9422_INTB_PORT, PCA9422_INTB_PIN, &sw_config);

   /* Init PCA9422 PMIC. */
	BOARD_InitPmic();
	PRINTF("-------------- PCA9422 BOARD_InitPmic OK--------------\r\n");
	/* Apply PMIC mode and voltage settings */
	BOARD_Init_PMICConfigure();
	PRINTF("-------------- PCA9422 BOARD_Init_PMICConfigure OK--------------\r\n");
#endif

#if PMIC_GLF70583_ENABLE

	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
	PRINTF("[GLF70583]top_stat:%X \n",top_stat);

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
	// 0x25->BUCK4、LDO2 off
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x26, 0xE8);
	// 0x26->BUCK1、2、4 ON、Others off
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xD0);
	//glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);//BUCK2 ON、Others off

	//uint8_t ch = GETCHAR();
	PRINTF("GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	PRINTF("GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1); \n");
	GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1);

#endif

#if AMP_ENABLE

	PRINTF("[AMP][AW88166] \r\n");

	init_aw88166();

	PRINTF("[I2S]Configure I2S \r\n");

    I2S_TxGetDefaultConfig(&s_TxConfig);

    // I2S 32bits
    s_TxConfig.dataLength  = 32;
    s_TxConfig.frameLength = 64;
    s_TxConfig.divider     = 8;//(24576000U / 16000U / 32U / 2);//DEMO_I2S_CLOCK_DIVIDER;
    s_TxConfig.masterSlave = DEMO_I2S_TX_MODE;

    //I2S_TxInit(DEMO_I2S_TX_toAmp, &s_TxConfig);
    I2S_TxInit(DEMO_I2S_TX_toNova, &s_TxConfig);

    DMA_Init(DEMO_DMA);

    //DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp);
    //DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp, kDMA_ChannelPriority3);
    //DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL_toAmp);

    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toNova);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL_toNova, kDMA_ChannelPriority3);
    DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL_toNova);

    StartSoundPlayback();

#endif

#if DMIC_ENABLE

PRINTF("DMIC-I2S5 Audio Pass-through DMA Example\r\n");

    DMA_Init(DEMO_DMA);

    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL);
    DMA_EnableChannel(DEMO_DMA, DEMO_DMIC_RX_CHANNEL);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL, kDMA_ChannelPriority3);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_DMIC_RX_CHANNEL, kDMA_ChannelPriority2);
    DMA_CreateHandle(&s_i2sTxDmaHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL);
    DMA_CreateHandle(&s_leftDmicRxDmaHandle, DEMO_DMA, DEMO_DMIC_RX_CHANNEL);

#if DEMO_DMIC_NUMS == 2U
    DMA_EnableChannel(DEMO_DMA, DEMO_DMIC_RX_CHANNEL_1);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_DMIC_RX_CHANNEL_1, kDMA_ChannelPriority2);
    DMA_CreateHandle(&s_rightDmicRxDmaHandle, DEMO_DMA, DEMO_DMIC_RX_CHANNEL_1);
#endif

    memset(&dmic_channel_cfg, 0U, sizeof(dmic_channel_config_t));

    dmic_channel_cfg.divhfclk            = kDMIC_PdmDiv1;
    dmic_channel_cfg.osr                 = 32U;
    dmic_channel_cfg.gainshft            = 4U;
    dmic_channel_cfg.preac2coef          = kDMIC_CompValueZero;
    dmic_channel_cfg.preac4coef          = kDMIC_CompValueZero;
    dmic_channel_cfg.dc_cut_level        = kDMIC_DcCut155;
    dmic_channel_cfg.post_dc_gain_reduce = 1U;
    dmic_channel_cfg.saturate16bit       = 1U;
    dmic_channel_cfg.sample_rate         = kDMIC_PhyFullSpeed;
    DMIC_Init(DMIC0);
#if !(defined(FSL_FEATURE_DMIC_HAS_NO_IOCFG) && FSL_FEATURE_DMIC_HAS_NO_IOCFG)
    DMIC_SetIOCFG(DMIC0, kDMIC_PdmDual);
#endif
    DMIC_Use2fs(DMIC0, true);
    DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL, true);
#if defined(BOARD_DMIC_CHANNEL_STEREO_SIDE_SWAP) && (BOARD_DMIC_CHANNEL_STEREO_SIDE_SWAP)
    DMIC_ConfigChannel(DMIC0, DEMO_DMIC_CHANNEL, kDMIC_Right, &dmic_channel_cfg);
#else
    DMIC_ConfigChannel(DMIC0, DEMO_DMIC_CHANNEL, kDMIC_Left, &dmic_channel_cfg);
#endif
    DMIC_FifoChannel(DMIC0, DEMO_DMIC_CHANNEL, FIFO_DEPTH, true, true);

#if DEMO_DMIC_NUMS == 2U
    DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_1, true);
#if defined(BOARD_DMIC_CHANNEL_STEREO_SIDE_SWAP) && (BOARD_DMIC_CHANNEL_STEREO_SIDE_SWAP)
    DMIC_ConfigChannel(DMIC0, DEMO_DMIC_CHANNEL_1, kDMIC_Left, &dmic_channel_cfg);
#else
    DMIC_ConfigChannel(DMIC0, DEMO_DMIC_CHANNEL_1, kDMIC_Right, &dmic_channel_cfg);
#endif
    DMIC_FifoChannel(DMIC0, DEMO_DMIC_CHANNEL_1, FIFO_DEPTH, true, true);
#endif

    DMIC_EnableChannnel(DMIC0, DEMO_DMIC_CHANNEL_ENABLE
#if DEMO_DMIC_NUMS == 2U
                             | DEMO_DMIC_CHANNEL_1_ENABLE
#endif
    );
    PRINTF("Configure I2S\r\n");

    I2S_TxGetDefaultConfig(&tx_config);
    tx_config.oneChannel = false; // 明確設定為 false，代表是雙聲道（立體聲）
    tx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;
    tx_config.masterSlave = DEMO_I2S_TX_MODE;
    I2S_TxInit(DEMO_I2S_TX, &tx_config); // DEMO_I2S_TX 現在是 I2S1
    I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX, &s_i2sTxHandle, &s_i2sTxDmaHandle, i2s_Callback, NULL);

#if DEMO_DMIC_NUMS == 2U
    DMIC_TransferCreateHandleDMA(DMIC0, &s_leftDmicDmaHandle, NULL, NULL, &s_leftDmicRxDmaHandle);
    DMIC_InstallDMADescriptorMemory(&s_leftDmicDmaHandle, s_leftDmaDescriptorPingpong, 2U);

    DMIC_TransferCreateHandleDMA(DMIC0, &s_rightDmicDmaHandle, dmic_Callback, NULL, &s_rightDmicRxDmaHandle);
    DMIC_InstallDMADescriptorMemory(&s_rightDmicDmaHandle, s_rightDmaDescriptorPingpong, 2U);

    status_t dmic_dma_status_left;
    status_t dmic_dma_status_right;

    dmic_dma_status_left = DMIC_TransferReceiveDMA(DMIC0, &s_leftDmicDmaHandle, s_leftReceiveXfer, DEMO_DMIC_CHANNEL);
	dmic_dma_status_right = DMIC_TransferReceiveDMA(DMIC0, &s_rightDmicDmaHandle, s_rightReceiveXfer, DEMO_DMIC_CHANNEL_1);

	if (dmic_dma_status_left != kStatus_Success || dmic_dma_status_right != kStatus_Success)
	{
		PRINTF("FATAL ERROR: Failed to start DMIC DMA transfer!\r\n");
		while(1); // 停在這裡
	}

//    DMIC_TransferReceiveDMA(DMIC0, &s_leftDmicDmaHandle, s_leftReceiveXfer, DEMO_DMIC_CHANNEL);
//    DMIC_TransferReceiveDMA(DMIC0, &s_rightDmicDmaHandle, s_rightReceiveXfer, DEMO_DMIC_CHANNEL_1);
#else
    DMIC_TransferCreateHandleDMA(DMIC0, &s_leftDmicDmaHandle, dmic_Callback, NULL, &s_leftDmicRxDmaHandle);
    DMIC_InstallDMADescriptorMemory(&s_leftDmicDmaHandle, s_leftDmaDescriptorPingpong, 2U);
    DMIC_TransferReceiveDMA(DMIC0, &s_leftDmicDmaHandle, s_leftReceiveXfer, DEMO_DMIC_CHANNEL);
#endif

    PRINTF("Starting audio loopback...\r\n");
    /* ========================[ 新增的除錯程式碼 ]======================== */
	PRINTF("--- Starting DMIC Register Status Check ---\r\n");
	// 延遲一段時間，讓 FIFO 有機會填充
	for (volatile int i = 0; i < 1000000; i++)
	{
		__NOP();
	}

	// 讀取並印出暫存器狀態 10 次
	for (int i = 0; i < 10; i++)
	{
		uint32_t chanen = DMIC0->CHANEN;
		// 根據您的 app.h，您使用的是 Channel 0 和 Channel 2
		uint32_t fifo_status_ch0 = DMIC0->CHANNEL[0].FIFO_STATUS;
		uint32_t fifo_status_ch2 = DMIC0->CHANNEL[2].FIFO_STATUS;

		PRINTF("Loop %d: CHANEN=0x%08X, CH0_FIFO_STATUS=0x%08X, CH2_FIFO_STATUS=0x%08X\r\n",
			   i, chanen, fifo_status_ch0, fifo_status_ch2);

		// 再次延遲
		for (volatile int j = 0; j < 500000; j++)
		{
			__NOP();
		}
	}
	PRINTF("--- DMIC Register Status Check Finished ---\r\n");
	/* ========================[ 除錯程式碼結束 ]======================== */
	while (1)
	{
		if (s_emptyBlock < BUFFER_NUM)
		{
			i2sTxTransfer.data     = s_buffer + s_writeIndex * PLAYBACK_BUFFER_SIZE;
			i2sTxTransfer.dataSize = PLAYBACK_BUFFER_SIZE;
			if (I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_i2sTxHandle, i2sTxTransfer) == kStatus_Success)
			{
				/* 2. 增加日誌邏輯 */
				logCounter++;
				if (logCounter >= 500)
				{
					PRINTF("I2S: Successfully sent 500 audio blocks. Current write index: %d\r\n", s_writeIndex);
					logCounter = 0; // 計數器歸零
				}

				if (++s_writeIndex >= BUFFER_NUM)
				{
					s_writeIndex = 0U;
				}
			}
		}
	}


#endif
#if LED_ENABLE
	/* LED Init */
	ktd202x_probe();

	ktd202x_ch1_led_on(1); //blue
	ktd202x_ch2_led_on(1); //red
	ktd202x_ch3_led_on(1);//green
	/*
	ktd202x_ch3_led_breathe(PERIOD_CODE_1P5S,
	                        RISE_CODE_600MS,
	                        FALL_CODE_600MS,
	                        ON_PERCENT_60,
	                        RAMP_SCALE_2X_SLOW,
	                        false,
	                        LED_CURRENT_CH3 );

	ktd202x_ch2_led_breathe(PERIOD_CODE_1P5S,
							RISE_CODE_600MS,
							FALL_CODE_600MS,
	                        ON_PERCENT_60,
							RAMP_SCALE_2X_SLOW,
	                        true,
							LED_CURRENT_CH2);
	*/
#endif

#if CHARGER_ENABLE

	status_t bq_ret;
	uint8_t ch_st,ID;

	bq256xx_read_reg(BQ256XX_REG_PART_INFO,&ID,1);
	PRINTF("bq256xx ID:(0x%02X)\n",ID);

    // 設定充電器參數
    bq256xx_cfg_t charger_cfg = {
            .vindpm_uv = 4450000,
            .iindpm_ua = 2000000,
            .ichg_ua = 530000,
            .vbatreg_uv = 4005000,
            .iprechg_ua = 60000,
            .iterm_ua = 20000,
//        .vindpm_uv = 4500000,
//        .iindpm_ua = 2400000,
//        .ichg_ua = 2040000,
//        .vbatreg_uv = 4208000,
//        .iprechg_ua = 180000,
//        .iterm_ua = 180000,
        .wdt_ms = 0  // 禁用 Watchdog
    };

    bq_ret = bq256xx_init(&charger_cfg);
    if ( bq_ret!= kStatus_Success) {
        PRINTF("bq256xx init failed!,ret:%d \n",bq_ret);
        return -1;
    }
    else{
        PRINTF("bq256xx initialized.OK \n");
    }

    //bq256xx_write_reg(0x00, 0x53); // TS_IGNORE + IINDPM = 2000mA
    //bq256xx_write_reg(0x02, 0xA8); // ICHG = 800mA->0xA8
    bq256xx_write_reg(0x03, 0x31); // IPRECHG = 60mA, ITERM = 20mA

/*
    uint8_t reg_val = 0;

    for (uint8_t reg = 0x00; reg <= 0x7; reg++)
    {

        if (bq256xx_read_reg(reg, &reg_val, 1) == kStatus_Success)
        {
            PRINTF("[Debug][bq256xx] REG%02X = 0x%02X\r\n", reg, reg_val);
        }
        else
        {
            PRINTF("[Debug][bq256xx] Failed to read REG%02X\r\n", reg);
        }

    }
*/
#endif

#if TOUCH_ENABLE

#if TOUCH_AW93305_ENABLE
    //    uint8_t data[4] = {0};  // IRQSRC 是 32-bit，需讀取 4 bytes
    //    status_t  ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR,
    //                               0x12,         // 預設 I2C 地址（CS2 浮接）
    //                               0xF080,       // IRQSRC 暫存器地址
    //                               2,            // subAddressSize = 2 bytes (16-bit register address)
    //                               data,
    //                               sizeof(data));
    //
    //       PRINTF("IRQSRC Read Result: %d, Data: %02X %02X %02X %02X\r\n", ret, data[0], data[1], data[2], data[3]);

    awinic_single_enter();
#else
	uint16_t fw_version = 0;
	int ret = elan_touch_get_fw_version(&fw_version);

    if (ret == 0) {
        PRINTF("Elan Touch FW Version: 0x%04X\n", fw_version);
    } else {
        PRINTF("Failed to get FW version, error code: %d\n", ret);
    }

    const uint8_t data_reg = 0xC0;
    uint8_t buf[EWD_FRAME_MAX_LEN];

#endif


#endif

#if GAUGE_ENABLE

    GPIO_PinInit(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, &sw_config);
    /* Enable GPIO pin interrupt */
    GPIO_SetPinInterruptConfig(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);

    //glf70302_set_soc_host(75);
    //glf70302_enable_host_soc();

    BatteryInfo battery;
	glf70302_read_battery(&battery);

#endif

    while(1)
    {
#if AMP_ENABLE
//    	uint8_t ch = GETCHAR();
//    	if(ch==0x11)
//    	{
//    		StopSoundPlayback();
//    	}
//    	else if (ch==0x22){
//    		StartSoundPlayback();
//    	}
#endif
#if GAUGE_ENABLE

    	if(g_gauge_int_flag)
    	{
    		g_gauge_int_flag = false;

    		glf70302_read_battery(&battery);

			GPIO_PinEnableInterrupt(GPIO, GAUGE_INT_PORT, GAUGE_INT_PIN, kGPIO_InterruptA);
    	}
#endif
#if CHARGER_ENABLE
    	if(g_charger_int_flag )
    	{
    		g_charger_int_flag = false;

			// 輪詢充電狀態
			bq256xx_status_t status;
			if (bq256xx_poll_status(&status) == kStatus_Success) {
				PRINTF("Power Good: %s\n", status.power_good ? "Yes" : "No");
				PRINTF("VBUS Status: 0x%02X\n", status.vbus_stat);
				PRINTF("Charge Status: 0x%02X\n", status.chg_stat);
				PRINTF("Fault Status: 0x%02X\n", status.fault_stat);
				PRINTF("VBUS Good: %s\n", status.vbus_good ? "Yes" : "No");
				PRINTF("\n");
			} else {
				PRINTF("Failed to read charger status.\n");
			}
			GPIO_PinEnableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
		}
#endif

#if TOUCH_ENABLE

    	if(g_touch_int_flag )
    	{
    		g_touch_int_flag = false;  // 清除旗標以避免重複處理
#if TOUCH_AW93305_ENABLE
    		AW93305_EXTI_Callback();
#else
    		int rc = hal_i2c_mem_read_impl(EKTF_I2C_ADDR_7BIT, data_reg, buf, EWD_FRAME_MAX_LEN);

            if (rc == 0) {
                elan_parse_and_report_data(buf, EWD_FRAME_MAX_LEN);
            }
#endif
            // 資料處理完畢後，重新啟用中斷
            GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
    	}
#endif
    }


}

