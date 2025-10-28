/*
 * glf70583.c
 *
 *  Created on: 2025年8月29日
 *      Author: 11301026
 */
#if UsingQAR87Board == 1
#include "glf70583.h"
#include "fsl_debug_console.h"

status_t  glf70583_i2c_write(uint8_t addr,uint8_t reg, uint8_t cmd)
{
	status_t ret;
	ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR,
						addr,
						reg,
						1,
						&cmd,
						1);
    //PRINTF("[Debug] glf70583_i2c_write BOARD_I3C_Send ret:%d \r\n",ret);

    return ret;
}

status_t  glf70583_i2c_read(uint8_t addr,uint8_t reg, uint8_t *buf, uint8_t len)
{
	status_t ret;
    ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR,
    						addr,
							reg,
							1,
                            buf,
							len);
    //PRINTF("[Debug] glf70583_i2c_read BOARD_I3C_Send ret:%d \r\n",ret);

    return ret;
}
#endif //#if UsingQAR87Board == 1