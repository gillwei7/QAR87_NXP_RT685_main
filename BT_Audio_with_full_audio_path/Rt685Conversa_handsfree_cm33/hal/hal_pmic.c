/*
 * hal_pmic.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_pmic.h"

void hal_pmic_pca9422_init(void)
{
    /* Init PCA9422 PMIC. */
    BOARD_InitPmic();
    /* Apply PMIC mode and voltage settings */
    BOARD_Init_PMICConfigure();

}

void hal_pmic_glf70583_init(void)
{
    uint8_t top_stat = 0;
    glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
//    PRINTF("[GLF70583]top_stat:%X \n",top_stat);

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

    GPIO_PinWrite(GPIO, NXP_532_PWR_PMIC1_PORT, NXP_532_PWR_PMIC1_PIN, 1); //Enable GLF70583
}

void BOARD_InitPMICs(void)
{
    hal_i3c_init();

#if PMIC_PCA9422_ENABLE
    hal_pmic_pca9422_init();
#endif

#if PMIC_GLF70583_ENABLE
    hal_pmic_glf70583_init();
    hal_loop_delay_ms(10);
    hal_soc_enable();
#endif
    PRINTF("BOARD_InitPMICs OK \n");
}
#endif
