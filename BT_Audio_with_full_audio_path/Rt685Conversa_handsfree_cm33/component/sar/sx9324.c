/*
 * sx9324.c
 *
 *  Created on: 2026年1月8日
 *      Author: 11301026
 */

#include "sx9324.h"
#include "fsl_debug_console.h"

/* Delay Helper (RT685 is Cortex-M33, usually running fast) */
extern void SDK_DelayAtLeastUs(uint32_t delay_us, uint32_t coreClock_Hz);
#define SX9324_DELAY_MS(ms) SDK_DelayAtLeastUs((ms) * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk))

/* Register Config Struct */
typedef struct {
    uint8_t reg;
    uint8_t val;
} SX9324_RegCfg_t;

/* Default Registers (Same as previous) */
static const SX9324_RegCfg_t sx9324_default_regs[] = {
    {0x10, 0x05},
    {0x11, 0x21},
    {0x14, 0x00},
    {0x15, 0x00},
    {0x20, 0x20},
    {0x23, 0x00},
    {0x24, 0x47},
    {0x26, 0x00},
    {0x27, 0x47},
    {0x28, 0x39},
    {0x29, 0x1B},
    {0x2A, 0x1F},
    {0x2B, 0x3D},
    {0x2C, 0x12},
    {0x2D, 0x08},
    {0x30, 0x0A},
    {0x31, 0x08},
    {0x32, 0x20},
    {0x33, 0x00},
    {0x34, 0x61},
    {0x35, 0x40},
    {0x36, 0x14},
    {0x37, 0x00},
    {0x40, 0x00},
    {0x41, 0x00},
    {0x42, 0x00},
    {0x43, 0x00},
    {0x44, 0x00},
    {0x45, 0x05},
    {0x46, 0x00},
    {0x47, 0x00},
    {0x48, 0x00},
    {0x49, 0x00},
    {0x4A, 0x00},
    {0x4B, 0x00},
    {0x4C, 0x00},
    {0x4D, 0x00},
    {0x4E, 0x80},
    {0x4F, 0x0C},
    {0x50, 0x14},
    {0x51, 0x70},
    {0x52, 0x20},
    {0x53, 0x00},
    {0x54, 0x00},
    {0x02, 0x00},
    {0x03, 0x00},
    {0x05, 0x60},
    {0x06, 0x00},
    {0x07, 0x80},
    {0x08, 0x01}
};

/*
 * Flexcomm I2C Wrapper
 */
static status_t sx9324_writereg(uint8_t reg, uint8_t val) {

    return BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR, SX9324_I2C_ADDR, (uint32_t)reg, 1, &val, 1);
}

static status_t sx9324_readregs(uint8_t reg, uint8_t *buffer, size_t len) {

    return BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR, SX9324_I2C_ADDR, (uint32_t)reg, 1, buffer, (uint8_t)len);
}

/* API Implementation */

bool sx9324_init() {
    uint8_t whoami = 0;

    // 1. Soft Reset
    if(sx9324_writereg(SX932x_SOFTRESET_REG, SX932x_SOFTRESET)!= kStatus_Success)
        return false;
    SX9324_DELAY_MS(100);

    // 2. Check Connection
    if (sx9324_readregs(SX932x_WHOAMI_REG, &whoami, 1) != kStatus_Success) {
        return false;
    }

    PRINTF("[SAR]ID(0x%02X): 0x%02X\r\n",SX932x_WHOAMI_REG ,whoami);

    // 3. Write Config
    uint32_t num_regs = sizeof(sx9324_default_regs) / sizeof(SX9324_RegCfg_t);
    for (uint32_t i = 0; i < num_regs; i++) {
    	if(sx9324_writereg(sx9324_default_regs[i].reg, sx9324_default_regs[i].val)!=kStatus_Success)
    		 return false;
    }
    SX9324_DELAY_MS(50);

    // 4. Calibration
    sx9324_manualcalibration();
    SX9324_DELAY_MS(100);

    // 5. Clear Initial IRQ
    uint8_t dummy;
    if(sx9324_readregs(SX932x_IRQSTAT_REG, &dummy, 1)!=kStatus_Success)
		 return false;

    return true;
}

void sx9324_manualcalibration() {
    sx9324_writereg(SX932x_STAT2_REG, 0x0F);
}

SAR_EVENT_t sx9324_process() {

    uint8_t irq_stat=0 ,prox_stat =0 ,body_stat= 0;

    sx9324_readregs(SX932x_IRQSTAT_REG, &irq_stat, 1);
    sx9324_readregs(SX932x_STAT0_REG, &prox_stat, 1);
    sx9324_readregs(SX932x_STAT1_REG, &body_stat, 1);

#if 0
    PRINTF("Reg 0x00 : 0x%02X\r\n",irq_src);
    PRINTF("Reg 0x01 : 0x%02X\r\n",prox_stat);
    PRINTF("Reg 0x02 : 0x%02X\r\n",body_stat);
#endif

	if ((irq_stat & SX932x_IRQ_TOUCH)){

		if(prox_stat !=0 && body_stat!=0)
			{
				PRINTF("[SAR] HUMAN BODY Detected! \r\n");
				return SAR_EVENT_BODY;
			}
		else if (prox_stat !=0)
			{
				PRINTF("[SAR] Detecte near \r\n");
				return SAR_EVENT_APPROACH;
			}
	}
	else if ((irq_stat & SX932x_IRQ_RELEASE))
			{
				PRINTF("[SAR] Detecte leave \r\n");
				return SAR_EVENT_DEPART;
			}
	else{
				return SAR_EVENT_NONE;
	}

}

void sx9324_readrawdata(uint8_t channel, SX9324_ChannelData_t *data) {
    uint8_t buf[2];

    sx9324_writereg(0x60, channel); // CPSRD

    sx9324_readregs(0x61, buf, 2);
    data->useful = (int32_t)((buf[0] << 8) | buf[1]);

    sx9324_readregs(0x63, buf, 2);
    data->average = (int32_t)((buf[0] << 8) | buf[1]);

    sx9324_readregs(0x65, buf, 2);
    data->diff = (int32_t)((buf[0] << 8) | buf[1]);

    sx9324_readregs(0x67, buf, 2);
    data->offset = (uint16_t)((buf[0] << 8) | buf[1]);

    if (data->useful > 32767) data->useful -= 65536;
    if (data->average > 32767) data->average -= 65536;
    if (data->diff > 32767) data->diff -= 65536;
}
