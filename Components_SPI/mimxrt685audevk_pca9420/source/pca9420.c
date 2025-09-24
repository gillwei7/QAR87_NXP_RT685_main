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

#include "fsl_dma.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "music.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PCA9420_LAST_REG (PCA9420_MODECFG_3_3)

#define GAUGE_ENABLE 0
#define LED_ENABLE 1
#define PMIC_PCA9422_ENABLE 1
#define TOUCH_ENABLE 1
#if TOUCH_ENABLE
#define TOUCH_AW93305_ENABLE 0
#endif
#define CHARGER_ENABLE 1
#define PMIC_GLF70583_ENABLE 1
#define AMP_ENABLE 1
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

static dma_handle_t s_DmaTxHandle;
static i2s_config_t s_TxConfig;
static i2s_dma_handle_t s_TxHandle;
static i2s_transfer_t s_TxTransfer;

/*******************************************************************************
 * Code
 ******************************************************************************/
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

    I2S_TransferAbortDMA(DEMO_I2S_TX, &s_TxHandle);


}

static void StartSoundPlayback(void)
{
    PRINTF("[I2S1]Setup looping playback of sine wave\r\n");

    s_TxTransfer.data     = &g_Music[0];
    s_TxTransfer.dataSize = sizeof(g_Music);

    I2S_TxTransferCreateHandleDMA(DEMO_I2S_TX, &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
    /* need to queue two transmit buffers so when the first one
     * finishes transfer, the other immediatelly starts */
    I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_TxHandle, s_TxTransfer);
    I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_TxHandle, s_TxTransfer);

    start_aw88166_pa(AW_DEV_0, "Music");
    start_aw88166_pa(AW_DEV_1, "Music");

}

void GPIO_INTA_DriverIRQHandler(void)
{

	uint32_t status = GPIO_PortGetInterruptStatus(GPIO, 1, kGPIO_InterruptA);


    if (status & (1 << TOUCH_INT_PIN)) { //Touch
        // 關閉中斷，避免重入
        GPIO_PinDisableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

        // 觸發中斷，清除中斷旗標
        GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

        // 執行對應處理
        g_touch_int_flag = true;
        //PRINTF("[Debug] TOUCH_GPIO_INTA_IRQHandler \r\n");
    }
    if (status & (1 << CHARG_INT_PIN)) { //Charger

    	GPIO_PinDisableInterrupt(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, CHARG_INT_PORT, CHARG_INT_PIN, kGPIO_InterruptA);
        g_charger_int_flag = true;
        //PRINTF("[Debug] PIO0_29_INTA_IRQHandler \r\n");
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


/*! @brief Main function */
int main(void)
{

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
	// 0x26->BUCK2 ON、Others off
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);

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

    I2S_TxInit(DEMO_I2S_TX, &s_TxConfig);

    DMA_Init(DEMO_DMA);

    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL, kDMA_ChannelPriority3);
    DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL);

    StartSoundPlayback();

#endif

#if LED_ENABLE
	/* LED Init */
	ktd202x_probe();

	ktd202x_ch1_led_on(1); //blue
	ktd202x_ch2_led_on(1); //red
	ktd202x_ch3_led_on(1);//green
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

    uint8_t soc = 0;
    uint16_t voltage = 0;
    int16_t current = 0;
    int8_t temperature = 0;

    glf70302_set_soc_host(75);
    glf70302_enable_host_soc();

    glf70302_read_soc(&soc);
    PRINTF("目前電量: %d%%\r\n", soc);

    glf70302_read_voltage(&voltage);
    PRINTF("電池電壓: %dmV\r\n", voltage);

    glf70302_read_current(&current);
    PRINTF("電池電流: %dmA\r\n", current);

    glf70302_read_temperature(&temperature);
    PRINTF("電池溫度: %d°C\r\n", temperature);

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

