/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "qar87_config.h"

#include "fsl_debug_console.h"
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
#include <bluetooth/hfp_hf.h>
#include "FreeRTOS.h"
#include "task.h"

#include "app_handsfree.h"

#include "fsl_gpio.h"
#include "glf70583.h"
#include "pmic_support.h"
#include "board.h"
#include "pin_mux_dev.h"
#include "pmic_pca9422.h"

#if DMIC_TO_NOVATEK
#include "fsl_device_registers.h"
#include "fsl_dmic.h"
#include "fsl_dmic_dma.h"
#include "fsl_i2c.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PCA9420_LAST_REG (PCA9420_MODECFG_3_3)

#if DMIC_TO_NOVATEK
#define FIFO_DEPTH           (15U)
#define RECORD_BUFFER_SIZE   (128)
#define PLAYBACK_BUFFER_SIZE (128 * 2U)
#define BUFFER_NUM           (2U)

#define DMAREQ_DMIC0                    16U
#define DEMO_I2S_MASTER_CLOCK_FREQUENCY CLOCK_GetMclkClkFreq()
#define DEMO_I2S_TX                     I2S5 //(I2S3)
#define DEMO_I2S_CLOCK_DIVIDER                                                                                 \
	(24576000U / 48000U / 16U / 2) /* I2S source clock 24.576MHZ, sample rate 48KHZ, bits width 16, 2 channel, \
								  so bitclock should be 48KHZ * 16 = 768KHZ, divider should be 24.576MHZ / 768KHZ */
#define DEMO_DMA               (DMA0)
#define DEMO_DMIC_RX_CHANNEL   DMAREQ_DMIC0
#define DEMO_DMIC_RX_CHANNEL_1 17
#define DEMO_I2S_TX_CHANNEL    (11)//(7)
#define DEMO_I2S_TX_MODE       kI2S_MasterSlaveNormalSlave

#define DEMO_DMIC_CHANNEL          kDMIC_Channel0
#define DEMO_DMIC_CHANNEL_1        kDMIC_Channel1
#define DEMO_DMIC_CHANNEL_1_ENABLE DMIC_CHANEN_EN_CH1(1)
#define DEMO_DMIC_CHANNEL_ENABLE   DMIC_CHANEN_EN_CH0(1)
#define DEMO_AUDIO_BIT_WIDTH       (16)
#define DEMO_AUDIO_SAMPLE_RATE     (48000)
#define DEMO_AUDIO_PROTOCOL        kCODEC_BusI2S
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if !PIN_CONFIG_DEV_BOARD
extern void BOARD_InitHardware(void);
#else
extern void dev_BOARD_InitHardware(void);
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if DMIC_TO_NOVATEK
static i2s_config_t tx_config;
static uint8_t s_buffer[PLAYBACK_BUFFER_SIZE * BUFFER_NUM];
static uint32_t volatile s_writeIndex = 0U;
static uint32_t volatile s_emptyBlock = BUFFER_NUM;

static dmic_dma_handle_t s_leftDmicDmaHandle;
static dma_handle_t s_leftDmicRxDmaHandle;

static dmic_dma_handle_t s_rightDmicDmaHandle;
static dma_handle_t s_rightDmicRxDmaHandle;

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

/*******************************************************************************
 * Code
 ******************************************************************************/
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

#if DMIC_TO_NOVATEK
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

int main(void)
{
#if !PIN_CONFIG_DEV_BOARD
    BOARD_InitHardware();
#else
    dev_BOARD_InitHardware();

    PRINTF("-------------- PCA9422 BOARD_InitHardware OK---------------\r\n");

	/* Define the init structure for the input switch pin */
	gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
	gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
	/* Init input switch GPIO. */
	EnableIRQ(GPIO_INTA_IRQn);
#if PMIC_GLF70583_ENABLE
    /* Init GPIO */
    GPIO_PortInit(GPIO, PWR_SW1_PORT);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);
#endif

    BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);

#if PMIC_PCA9422_ENABLE
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
	// 0x26->BUCK2 ON、Others off
	//glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xD0);

	//uint8_t ch = GETCHAR();
	PRINTF("GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

#endif // PMIC_GLF70583_ENABLE

#if DMIC_TO_NOVATEK
    dmic_channel_config_t dmic_channel_cfg;
    i2s_transfer_t i2sTxTransfer;
    DMA_Init(DEMO_DMA);

    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL);
    DMA_EnableChannel(DEMO_DMA, DEMO_DMIC_RX_CHANNEL);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL, kDMA_ChannelPriority3);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_DMIC_RX_CHANNEL, kDMA_ChannelPriority2);
    DMA_CreateHandle(&s_i2sTxDmaHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL);
    DMA_CreateHandle(&s_leftDmicRxDmaHandle, DEMO_DMA, DEMO_DMIC_RX_CHANNEL);

    DMA_EnableChannel(DEMO_DMA, DEMO_DMIC_RX_CHANNEL_1);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_DMIC_RX_CHANNEL_1, kDMA_ChannelPriority2);
    DMA_CreateHandle(&s_rightDmicRxDmaHandle, DEMO_DMA, DEMO_DMIC_RX_CHANNEL_1);

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

    DMIC_Use2fs(DMIC0, true);
    DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL, true);
    DMIC_ConfigChannel(DMIC0, DEMO_DMIC_CHANNEL, kDMIC_Left, &dmic_channel_cfg);
    DMIC_FifoChannel(DMIC0, DEMO_DMIC_CHANNEL, FIFO_DEPTH, true, true);
    DMIC_EnableChannelDma(DMIC0, DEMO_DMIC_CHANNEL_1, true);
    DMIC_ConfigChannel(DMIC0, DEMO_DMIC_CHANNEL_1, kDMIC_Right, &dmic_channel_cfg);
    DMIC_FifoChannel(DMIC0, DEMO_DMIC_CHANNEL_1, FIFO_DEPTH, true, true);


    DMIC_EnableChannnel(DMIC0, DEMO_DMIC_CHANNEL_ENABLE
                             | DEMO_DMIC_CHANNEL_1_ENABLE
    );
    PRINTF("Configure I2S\r\n");

    /*
     * masterSlave = kI2S_MasterSlaveNormalMaster;
     * mode = kI2S_ModeI2sClassic;
     * rightLow = false;
     * leftJust = false;
     * pdmData = false;
     * sckPol = false;
     * wsPol = false;
     * divider = 1;
     * oneChannel = false;
     * dataLength = 16;
     * frameLength = 32;
     * position = 0;
     * fifoLevel = 4;
     */
    I2S_TxGetDefaultConfig(&tx_config);
    tx_config.divider     = DEMO_I2S_CLOCK_DIVIDER;
    tx_config.masterSlave = DEMO_I2S_TX_MODE;
    I2S_TxInit(DEMO_I2S_TX, &tx_config);
    I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX, &s_i2sTxHandle, &s_i2sTxDmaHandle, i2s_Callback, NULL);

    DMIC_TransferCreateHandleDMA(DMIC0, &s_leftDmicDmaHandle, NULL, NULL, &s_leftDmicRxDmaHandle);
    DMIC_InstallDMADescriptorMemory(&s_leftDmicDmaHandle, s_leftDmaDescriptorPingpong, 2U);

    DMIC_TransferCreateHandleDMA(DMIC0, &s_rightDmicDmaHandle, dmic_Callback, NULL, &s_rightDmicRxDmaHandle);
    DMIC_InstallDMADescriptorMemory(&s_rightDmicDmaHandle, s_rightDmaDescriptorPingpong, 2U);

    DMIC_TransferReceiveDMA(DMIC0, &s_leftDmicDmaHandle, s_leftReceiveXfer, DEMO_DMIC_CHANNEL);
    DMIC_TransferReceiveDMA(DMIC0, &s_rightDmicDmaHandle, s_rightReceiveXfer, DEMO_DMIC_CHANNEL_1);
#endif

#endif // PIN_CONFIG_DEV_BOARD
#if !DMIC_TO_NOVATEK
    if (xTaskCreate(peripheral_hfp_hf_task, "peripheral_hfp_hf_task", configMINIMAL_STACK_SIZE * 8, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        PRINTF("pherial hrs task creation failed!\r\n");
        while (1)
            ;
    }

    vTaskStartScheduler();
    for (;;)
        ;
#else
    while (1)
        {
            if (s_emptyBlock < BUFFER_NUM)
            {
                i2sTxTransfer.data     = s_buffer + s_writeIndex * PLAYBACK_BUFFER_SIZE;
                i2sTxTransfer.dataSize = PLAYBACK_BUFFER_SIZE;
                if (I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_i2sTxHandle, i2sTxTransfer) == kStatus_Success)
                {
                    if (++s_writeIndex >= BUFFER_NUM)
                    {
                        s_writeIndex = 0U;
                        for(int i =0;i<PLAYBACK_BUFFER_SIZE;i++){
                            PRINTF(" %02X",s_buffer[i]);
                        }
                        PRINTF("\r\n");
                    }
                }
            }
        }
#endif
}
