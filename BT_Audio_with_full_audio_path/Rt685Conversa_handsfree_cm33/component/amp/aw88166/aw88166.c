/*
 * aw88166.c
 *
 *  Created on: Oct 17, 2024
 *      Author: 11301026
 */

#include "fsl_debug_console.h"
#include "fsl_i3c.h"
#include "board.h"
#include "aw88166.h"
//#include "aw_params_20241105.h"
//#include "aw_params.h"
//B36932 #include "aw_params_music44.1k16bit_bck32fs.h"
//#include "aw_params_music_48k_32bit_20251112.h" //B36932
//#include "aw_params_music_48k_16bit.h"
#include "aw_params_music_48k16b_receiver_16k16b.h"




static status_t aw_dev0_i2c_write_func(uint16_t dev_addr, uint8_t reg_addr,
                                uint8_t *pdata, uint16_t len)
{
	status_t ret = 0;
#if UsingQAR87Board == 1
	ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR,
	                          (uint8_t)dev_addr,
	                          (uint32_t)reg_addr,
	                          1,
							  pdata,
	                          len);
	//PRINTF("[Debug][aw_dev0_i2c_write_func] BOARD_I3C_Send ret:%d \r\n",ret);
#endif
    return ret;
}


static status_t aw_dev0_i2c_read_func(uint16_t dev_addr, uint8_t reg_addr,
                               uint8_t *pdata, uint16_t len)
{
	status_t ret = 0;
#if UsingQAR87Board == 1
	ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR,
            (uint8_t)dev_addr,
            (uint32_t)reg_addr,
            1,
            pdata,
            len);
	//PRINTF("[Debug][aw_dev0_i2c_read_func] BOARD_I3C_Receive ret:%d \r\n",ret);
#endif
    return   ret;

}

static void aw_dev0_reset_gpio_ctl(bool State)
{
#if UsingQAR87Board == 1
    //GPIO_PinWrite(GPIO, AMP_RESET_PORT, AMP_RESET_PIN, State ? 1U : 0U);
    GPIO_PinWrite(GPIO, GPIO_AMP_RESET_R_PORT, GPIO_AMP_RESET_R_PIN, 1U);
#endif
}

extern int aw883xx_pid_2066_dev_init(void *aw883xx);

struct aw_init_info init_info[] = {
		{
			.dev = AW_DEV_0,
			.i2c_addr =AW88166_DEV0_I2C_ADDR,
			.re_min = 2000,        /*awinic:If the re_min(ohms) and re_max(ohms) are not set, the default value is used*/
			.re_max = 39000,
			.mix_chip_count = AW_DEV0_MIX_CHIP_NUM,
			.prof_info = g_dev0_prof_info,
			.i2c_read_func = aw_dev0_i2c_read_func,
			.i2c_write_func = aw_dev0_i2c_write_func,
			.reset_gpio_ctl = aw_dev0_reset_gpio_ctl,
			.dev_init_ops = {aw883xx_pid_2066_dev_init},
		},

		{
			.dev = AW_DEV_1,
			.i2c_addr =AW88166_DEV1_I2C_ADDR,
			.re_min = 2000,
			.re_max = 39000,
			.mix_chip_count = AW_DEV1_MIX_CHIP_NUM,
			.prof_info = g_dev1_prof_info,
			.i2c_read_func = aw_dev0_i2c_read_func,
			.i2c_write_func = aw_dev0_i2c_write_func,
			.reset_gpio_ctl = aw_dev0_reset_gpio_ctl,
			.dev_init_ops = {aw883xx_pid_2066_dev_init},
		},


	};


void init_aw88166(void){

	int ret ;

	PRINTF("[AMP][init_aw88166] \r\n");

	ret = aw883xx_smartpa_init((void *)&init_info[AW_DEV_0]);
	ret+= aw883xx_smartpa_init((void *)&init_info[AW_DEV_1]);

	if(ret==0)
		PRINTF("[AMP][init_aw88166]aw883xx_smartpa_init OK \r\n");
	else
		PRINTF("[AMP][init_aw88166]aw883xx_smartpa_init error:%d \r\n",ret);
}

void start_aw88166_pa(aw_dev_index_t dev, char *prof_name){

	//aw883xx_set_profile_byname(AW_DEV_0, "Music");
	//aw883xx_ctrl_state (AW_DEV_0, AW_START);
	PRINTF("[AW88166] start_aw88166_pa: %d, %s\r\n", dev, prof_name);

	aw883xx_set_profile_byname(dev, prof_name);  //EX: (AW_DEV_0, "Music")
	aw883xx_ctrl_state (dev, AW_START);

}

void close_aw88166_pa(aw_dev_index_t dev){

	//aw883xx_ctrl_state (AW_DEV_0, AW_STOP);
	aw883xx_ctrl_state (dev, AW_STOP);
}




