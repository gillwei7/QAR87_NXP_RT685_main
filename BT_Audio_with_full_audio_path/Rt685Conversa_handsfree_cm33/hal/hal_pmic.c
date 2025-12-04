/*
 * hal_pmic.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#include "hal_pmic.h"

static uint8_t config_status = 1;

void hal_pmic_pca9422_init(void)
{

    /* Init PCA9422 PMIC. */
    BOARD_InitPmic();
    /* Apply PMIC mode and voltage settings */
    BOARD_Init_PMICConfigure();

}

void hal_pmic_pca9422_power_down(void)
{
    PRINTF("[PCA9422] PCA9422 power down \r\n");
	/* pca9422 power down  process */
    uint8_t value;
    value = 0x08;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}

void hal_pmic_pca9422_enter_ship_mode(void)
{
	PRINTF("[PCA9422] PCA9422 enter ship mode \r\n");
	/* pca9422 ship mode process */
	uint8_t value;
    value = 0x10;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x09, 1, &value, 1);
    value = 0x00;
    BOARD_PMIC_I2C_Send(PCA9422_DEFAULT_I2C_ADDR, 0x0A, 1, &value, 1);
}

void hal_pmic_glf70583_init (uint8_t glf70583_a_i2c_addr, uint8_t glf70583_b_i2c_addr)
{
    /*
    PMIC-1(0x25):
        SW1: NT98532/ eMMC/ Camera/ C4 Flash
        SW2: NT98532
        SW3: NT98532
        SW4: Camera (for 小板開)
        LDO1: NT98532/ eMMC
        LDO2: Camera_雜訊_2.8V

    PMIC-2(0x26):
        SW1: IW611
        SW2: NT98532
        SW3: C4 (for 小板開)
        SW4: IW611
        LDO1: C4 (for 小板開)
        LDO2: C4 (for 小板開)
     */

    uint8_t top_stat = 0;
    glf70583_i2c_read(glf70583_a_i2c_addr,0x00,&top_stat,1);
//    PRINTF("[GLF70583]top_stat:%X \n",top_stat);

    /** Solution: The manufacturer did not set LDO1 to LOAD SWITCH **/
    glf70583_i2c_write(glf70583_a_i2c_addr, 0xF5, 0xC6); // unlocked chip
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x24, 0xB8);
    hal_loop_delay_ms(10);
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x24, 0xB9);

//    PRINTF("[GLF70583] Start to Set GLF70583_A \n");

    /** GLF70583_A Setting **/
    // BUCK1 1v8
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x1C, 0x0C);
    // BUCK2 1v35
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x1E, 0xF4);
    // BUCK3 0v95
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x20, 0x5A);
    // BUCK4 1v05
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x23, 0x1E);
    // LDO2 2v8
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x25, 0x90);

//    PRINTF("[GLF70583] Start to Set GLF70583_B \n");

    /** GLF70583_B Setting **/
    // BUCK1 3v3
    glf70583_i2c_write(glf70583_b_i2c_addr, 0xF5, 0xC6); // unlocked chip
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x1C, 0xA2); // set to 3V3
    // BUCK2 1v35
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x1E, 0xF4);
    // BUCK3 0v85
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x20, 0x46);
    // BUCK4 1v8
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x23, 0xB4);

    // BUCK1 Delay 4ms
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x66, 0x0C);
    // BUCK2 Delay 2ms
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x67, 0x08);
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x67, 0x08);
    // BUCK3 Delay 0ms
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x68, 0x00);
    // LDO1 Delay 5ms
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x6A, 0x12);

#if UsingQAR87BoardHwVersion == 0 // Dev Board

#if 0 //for test only 0x25->SW1 output
    // 0x25->SW1 on、Others off
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x26, 0x80);
    // 0x26->All off
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x26, 0x00);

#else
    // 0x25->BUCK4 off、Others on
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x26, 0xEC);
    // 0x26->BUCK1、2、4 ON、Others off
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x26, 0xD0);
    //glf70583_i2c_write(glf70583_b_i2c_addr, 0x26, 0x40);//BUCK2 ON、Others off
#endif
#endif

#if UsingQAR87BoardHwVersion == 1 // Actual Board
    //TODO: C4 (0x26:SW3、LDO1、LDO2) power sequence
    // 0x25 -> Enable All
    glf70583_i2c_write(glf70583_a_i2c_addr, 0x26, 0xFC);
    // 0x26 -> Enable: BUCK1, 2, 3 and 4
    glf70583_i2c_write(glf70583_b_i2c_addr, 0x26, 0xF0);
#endif

//    PRINTF("[GLF70583] Enable GLF70583 \n");
    GPIO_PinWrite(GPIO, NXP_532_PWR_PMIC1_PORT, NXP_532_PWR_PMIC1_PIN, 1); //Enable GLF70583
}

void hal_pmic_glf70583_actual_board_init (void)
{
	config_status = (uint8_t)GPIO_PinRead(GPIO, HW_CONFIG_03_PORT, HW_CONFIG_03_PIN);
	PRINTF("[HW] config_status: %d \r\n", config_status);

	if (config_status == 0) {
		PRINTF("[PMIC] GLF70583 correct \r\n");
		hal_pmic_glf70583_init(GLF70583_A_I2C_ADDR, GLF70583_B_I2C_ADDR);
	} else if (config_status == 1) {
		PRINTF("[PMIC] GLF70583 reverse \r\n");
		hal_pmic_glf70583_init(GLF70583_B_I2C_ADDR, GLF70583_A_I2C_ADDR);
	}
}

void BOARD_InitPMICs(void)
{
    hal_i3c_init();

#if PMIC_PCA9422_ENABLE
    hal_pmic_pca9422_init();
#endif

#if PMIC_GLF70583_ENABLE
    hal_pmic_glf70583_actual_board_init();
#endif
    PRINTF("BOARD_InitPMICs OK \n");
}
#endif
