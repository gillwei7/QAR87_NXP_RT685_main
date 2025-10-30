/*
 * aw93305.c
 *
 *  Created on: 2025年9月15日
 *      Author: 11301026
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "fsl_debug_console.h"
#include "board.h"
#include "aw_type.h"
#include "aw_cap_config.h"
#include "aw_cap.h"
#include "aw933xx.h"
#include "aw93305.h"

#include "FreeRTOS.h"
#include "task.h"


static void aw_delay(AW_U32 ms)
{
	SDK_DelayAtLeastUs(ms * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
	//vTaskDelay(pdMS_TO_TICKS(ms));
}

void (*aw933xx_irq_cb)(void);

void AW93305_EXTI_Callback(void)
{
	AWLOGD("interrupt");
	aw933xx_irq_process_impl();
//	if (aw933xx_irq_cb != NULL) {
//			aw933xx_irq_cb();
//		}

}

static void aw_irq_cb_set(void (*cb)(void))
{
	aw933xx_irq_cb = cb;
}

static AW_S32 aw_i2c_write(AW_U16 reg_addr, AW_U32 reg_data)
{
    AW_S32 status = 0;
    AW_U8 cnt = 0;
    AW_U8 data[4] = {0};
    data[0] = (reg_data >> OFFSET_BIT_24) & 0xff;
    data[1] = (reg_data >> OFFSET_BIT_16) & 0xff;
    data[2] = (reg_data >> OFFSET_BIT_8) & 0xff;
    data[3] = reg_data & 0xff;

    while (cnt < AW_I2C_RETRIES) {
        status = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR, AW_CAP_CHIP_I2C_ADDR,
                                reg_addr, 2, data, 4);
        if (status != kStatus_Success) {
            cnt++;
            AWLOGE("write %04X fail, status: %d, try times: %d\n",
                   reg_addr, status, cnt);
            //HAL_Delay(AW_I2C_RETRIES_DELAY);
            aw_delay(AW_I2C_RETRIES_DELAY);
        } else {
            break;
        }
    }
    return status;
}

static AW_S32 aw_i2c_read(AW_U16 reg_addr, AW_U32 *reg_data)
{
    AW_S32 status = 0;
    AW_U8 cnt = 0;
    AW_U8 data[4] = {0};

    while (cnt < AW_I2C_RETRIES) {
        status = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR, AW_CAP_CHIP_I2C_ADDR,
                                   reg_addr, 2, data, 4);
        if (status != kStatus_Success) {
            cnt++;
            AWLOGE("read %04X fail, status: %d, try times: %d\n",
                   reg_addr, status, cnt);
            //HAL_Delay(AW_I2C_RETRIES_DELAY);
            aw_delay(AW_I2C_RETRIES_DELAY);
        } else {
            *reg_data = data[3] |
                        (data[2] << OFFSET_BIT_8) |
                        (data[1] << OFFSET_BIT_16) |
                        (data[0] << OFFSET_BIT_24);
            break;
        }
    }
    return status;
}

static AW_S32 aw_i2c_write_seq(AW_U16 addr, AW_U8 *data, AW_U32 len)
{

    AW_S32 status = 0;
    AW_U8 cnt = 0;

    while (cnt < AW_I2C_RETRIES) {
        status = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR, AW_CAP_CHIP_I2C_ADDR,
                                addr, 2, data, len);
        if (status != kStatus_Success) {
            cnt++;
            AWLOGE("write %04X fail, status: %d, try times: %d\n",
                   addr, status, cnt);
            //SDK_DelayAtLeastUs(AW_I2C_RETRIES_DELAY * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
        } else {
            break;
        }
    }
    return status;

}

static AW_S32 aw_i2c_read_seq(AW_U16 addr, AW_U8 *data, AW_U32 len)
{

    AW_S32 status = 0;
    AW_U8 cnt = 0;

    while (cnt < AW_I2C_RETRIES) {
        status = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR, AW_CAP_CHIP_I2C_ADDR,
                                   addr, 2, data, len);
        if (status != kStatus_Success) {
            cnt++;
            AWLOGE("read %04X fail, status: %d, try times: %d\n",
                   addr, status, cnt);
            //SDK_DelayAtLeastUs(AW_I2C_RETRIES_DELAY * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
        } else {
            break;
        }
    }
    return status;

}

static const struct aw_i2c_func_t aw_i2c_func = {
	.i2c_read = aw_i2c_read,
	.i2c_write = aw_i2c_write,
	.i2c_read_seq = aw_i2c_read_seq,
	.i2c_write_seq = aw_i2c_write_seq,
};

static const struct aw_func_t aw_func = {
	.i2c_func = &aw_i2c_func,
	.delay = aw_delay,
	.set_irq_cb = aw_irq_cb_set,
};

void awinic_single_enter(void)
{
	aw933xx_init(&aw_func);
}
