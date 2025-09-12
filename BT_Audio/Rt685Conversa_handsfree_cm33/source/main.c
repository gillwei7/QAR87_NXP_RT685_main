/*
 * Copyright 2021 NXP
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

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PCA9420_LAST_REG (PCA9420_MODECFG_3_3)

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

int main(void)
{
#if !PIN_CONFIG_DEV_BOARD
    BOARD_InitHardware();
#else
    dev_BOARD_InitHardware();
#endif
    PRINTF("-------------- PCA9422 BOARD_InitHardware OK--------------\r\n");

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

            #endif
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
}
