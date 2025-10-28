/*
 * hal_pmic.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#include "hal_pmic.h"


void BOARD_InitPMICs(void)
{
//    PRINTF("-------------- PCA9422 BOARD_InitHardware OK---------------\r\n");

	/* Define the init structure for the input switch pin */
	gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
	gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
	/* Init input switch GPIO. */
	EnableIRQ(GPIO_INTA_IRQn);

#if PMIC_GLF70583_ENABLE
    /* Init GPIO */
//    GPIO_PortInit(GPIO, PWR_SW1_PORT);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);
#endif

    BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);

#if PMIC_PCA9422_ENABLE
   /* Init PCA9422 PMIC. */
	BOARD_InitPmic();
//	PRINTF("-------------- PCA9422 BOARD_InitPmic OK--------------\r\n");
	/* Apply PMIC mode and voltage settings */
	BOARD_Init_PMICConfigure();
//	PRINTF("-------------- PCA9422 BOARD_Init_PMICConfigure OK--------------\r\n");
#endif

#if PMIC_GLF70583_ENABLE

	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
//	PRINTF("[GLF70583]top_stat:%X \n",top_stat);

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
//	PRINTF("GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
//	PRINTF("GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1); \n");
	//gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
	GPIO_PinInit(GPIO, RESET553_N_PORT, RESET553_N_PIN, &output_int_config);
	GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1); //NT98532 Reset Pin
//	PRINTF("BOARD_InitPMICs OK \n");

#endif // PMIC_GLF70583_ENABLE
}
