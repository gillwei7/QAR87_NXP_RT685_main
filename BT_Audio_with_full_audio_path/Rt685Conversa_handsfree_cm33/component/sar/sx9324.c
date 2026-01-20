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

#if 0
	    {0x00, 0x00},              // 新增
	    {0x02, 0x00},              // 新增
	    {0x03, 0x00},              // 新增

	    {0x05, 0x00},              // 更新：原 0x70
	    {0x06, 0x00},
	    {0x07, 0x00},
	    {0x08, 0x00},

	    {0x10, 0x0A},              // 更新：原 0x16

	    {0x14, 0x00},
	    {0x15, 0x00},

	    {0x20, 0x20},              // 更新：原 0x00
	    {0x21, 0x10},              // 仍沿用原值（新表未覆寫）
	    {0x22, 0x00},              // 仍沿用原值
	    {0x23, 0x00},

	    {0x24, 0x47},              // 更新：原 0x44
	    {0x25, 0x00},              // 仍沿用原值
	    {0x26, 0x00},              // 更新：原 0x01
	    {0x27, 0x47},              // 更新：原 0x44

	    {0x28, 0x2D},              // 更新：原 0x04
	    {0x29, 0x1B},              // 更新：原 0x10
	    {0x2A, 0x1F},              // 更新：原 0x1B
	    {0x2B, 0x3D},              // 更新：原 0x00
	    {0x2C, 0x12},
	    {0x2D, 0x08},

	    {0x30, 0x0B},              // 更新：原 0x09
	    {0x31, 0x0B},              // 更新：原 0x09
	    {0x32, 0x30},              // 更新：原 0x08
	    {0x33, 0x20},
	    {0x34, 0x20},
	    {0x35, 0x00},
	    {0x36, 0x20},              // 更新：原 0x1B
	    {0x37, 0xC0},              // 更新：原 0x1B

	    {0x40, 0x00},
	    {0x41, 0x00},
	    {0x42, 0x00},
	    {0x43, 0x00},
	    {0x44, 0x00},
	    {0x45, 0x05},
	    {0x46, 0x00},
	    {0x47, 0x00},
	    {0x48, 0x00},
	    {0x49, 0x00},              // 更新：原 0x80
	    {0x4A, 0x40},              // 更新：原 0x00
	    {0x4B, 0x31},              // 更新：原 0x00
	    {0x4C, 0x00},
	    {0x4D, 0x00},
	    {0x4E, 0x00},              // 更新：原 0x80
	    {0x4F, 0x00},              // 更新：原 0x0C
	    {0x50, 0x00},
	    {0x51, 0x00},
	    {0x52, 0x00},
	    {0x53, 0x00},              // 更新：原 0xF0
	    {0x54, 0x00},              // 更新：原 0xF0

	    {0x11, 0x21},              // 更新：原 0x24（保留最後寫入）
#endif

#if 0
	{SX932x_IRQ_ENABLE_REG, 0x70},
	{0x06, 0x00}, {0x07, 0x00}, {0x08, 0x00},

	/* General */
	{SX932x_CTRL0_REG, 0x16},
	{0x14, 0x00}, {0x15, 0x00},

	/* AFE Control */
	{0x20, 0x00}, {0x21, 0x10}, {0x22, 0x00}, {0x23, 0x00},
	{0x24, 0x44}, {0x25, 0x00}, {0x26, 0x01}, {0x27, 0x44},

	/* Phase Config (您的設定 - 正確) */
	{0x28, 0x04}, // PH0: Scan CS0
	{0x29, 0x10}, // PH1: Scan CS1
	{0x2A, 0x1B}, // PH2: GND
	{0x2B, 0x1B}, // PH3: GND

	/* Gain */
	{0x2C, 0x12}, {0x2D, 0x08},

	/* Prox Thresholds */
	{0x30, 0x09}, {0x31, 0x09}, {0x32, 0x08}, {0x33, 0x20},
	{0x34, 0x0C}, {0x35, 0x00}, {0x36, 0x1B}, {0x37, 0x1B},

	/* Advanced SAR Settings */
	{0x40, 0x00}, {0x41, 0x00}, {0x42, 0x00}, {0x43, 0x00},
	{0x44, 0x00}, {0x45, 0x05}, {0x46, 0x00}, {0x47, 0x00},
	{0x48, 0x00}, {0x49, 0x80},

	/* [修改點] Body Threshold: 建議設一點門檻避免雜訊 */
	{0x4A, 0x00}, // Body Threshold PH0/1 (Ex: 0x10)

	{0x4B, 0x00}, {0x4C, 0x00},

	/* [修改點] SAR Enable: 必須設為 0x40 才能開啟人體判別 */
	{0x4D, 0x40}, // SAREN = 1 (Enable SAR Engine)

	{0x4E, 0x80}, // SAR Slope (Default)
	{0x4F, 0x0C}, // SAR Offset (Default)

	{0x50, 0x00}, {0x51, 0x00}, {0x52, 0x00}, {0x53, 0xF0},
	{0x54, 0xF0},

	/* [修改點] Enable Phases: PH0 & PH1 */
	{SX932x_CTRL1_REG, 0x21}
#endif

#if 1
    {0x05, 0x70},
    {0x06, 0x00}, {0x07, 0x00}, {0x08, 0x00},
    {0x10, 0x0A},
    {0x14, 0x00}, {0x15, 0x00},
    {0x20, 0x20}, {0x21, 0x10}, {0x22, 0x00}, {0x23, 0x00},
    {0x24, 0x44}, {0x25, 0x00}, {0x26, 0x01}, {0x27, 0x44},
    {0x28, 0x04}, {0x29, 0x10}, {0x2A, 0x1B}, {0x2B, 0x1B},
    {0x2C, 0x12}, {0x2D, 0x08},
    {0x30, 0x09}, {0x31, 0x09}, {0x32, 0x08}, {0x33, 0x20},
    {0x34, 0x0C}, {0x35, 0x00}, {0x36, 0x1B}, {0x37, 0x1B},
    {0x40, 0x00}, {0x41, 0x00}, {0x42, 0x00}, {0x43, 0x00},
    {0x44, 0x00}, {0x45, 0x05}, {0x46, 0x00}, {0x47, 0x00},
    {0x48, 0x00}, {0x49, 0x80}, {0x4A, 0x00}, {0x4B, 0x00},
    {0x4C, 0x00}, {0x4D, 0x00}, {0x4E, 0x80}, {0x4F, 0x0C},
    {0x50, 0x00}, {0x51, 0x00}, {0x52, 0x00}, {0x53, 0xF0},
    {0x54, 0xF0},
    {0x11, 0x21}
#endif

#if 0
    {0x10,0x0A}, {0x11,0x21}, {0x14,0x00}, {0x15,0x00},
    {0x20,0x20}, {0x23,0x00}, {0x24,0x47},
    {0x26,0x00}, {0x27,0x47}, {0x28,0x2D}, {0x29,0x1B},
    {0x2A,0x1F}, {0x2B,0x3D}, {0x2C,0x12}, {0x2D,0x08},
    {0x30,0x0B}, {0x31,0x0B}, {0x32,0x20}, {0x33,0x20}, {0x34,0x0C},
    {0x35,0x00}, {0x36,0x20}, {0x37,0xC0},
    {0x40,0x00}, {0x41,0x00}, {0x42,0x00}, {0x43,0x00}, {0x44,0x00},
    {0x45,0x05}, {0x46,0x00}, {0x47,0x00}, {0x48,0x00}, {0x49,0x00},
    {0x4A,0x40}, {0x4B,0x31}, {0x4C,0x00}, {0x4D,0x00}, {0x4E,0x00}, {0x4F,0x00},
    {0x50,0x00}, {0x51,0x00}, {0x52,0x00}, {0x53,0x00}, {0x54,0x00},
    {0x02,0x00}, {0x03,0x00}, {0x05,0x00}, {0x06,0x00}, {0x07,0x00}, {0x08,0x00}, {0x00,0x00},
#endif
};

/*
 * Flexcomm I2C Wrapper
 */
static status_t sx9324_writereg(SX9324_Handle_t *dev, uint8_t reg, uint8_t val) {

    return BOARD_I3C_Send(dev->i3c_base, SX9324_I2C_ADDR, (uint32_t)reg, 1, &val, 1);
}

static status_t sx9324_readregs(SX9324_Handle_t *dev, uint8_t reg, uint8_t *buffer, size_t len) {

    return BOARD_I3C_Receive(dev->i3c_base, SX9324_I2C_ADDR, (uint32_t)reg, 1, buffer, (uint8_t)len);
}

/* API Implementation */

bool sx9324_init(SX9324_Handle_t *dev, I3C_Type *i3c_base, uint32_t port, uint32_t pin) {
    dev->i3c_base = i3c_base;
    dev->gpio_base = GPIO; // RT685 Global GPIO Base
    dev->nirq_port = port;
    dev->nirq_pin  = pin;

    uint8_t whoami = 0;

    // 1. Soft Reset
    if(sx9324_writereg(dev, SX932x_SOFTRESET_REG, SX932x_SOFTRESET)!= kStatus_Success)
        return false;
    SX9324_DELAY_MS(100);

    // 2. Check Connection
    if (sx9324_readregs(dev, SX932x_WHOAMI_REG, &whoami, 1) != kStatus_Success) {
        return false;
    }

    PRINTF("[SAR]ID(0x%02X): 0x%02X\r\n",SX932x_WHOAMI_REG ,whoami);

    // 3. Write Config
    uint32_t num_regs = sizeof(sx9324_default_regs) / sizeof(SX9324_RegCfg_t);
    for (uint32_t i = 0; i < num_regs; i++) {
    	if(sx9324_writereg(dev, sx9324_default_regs[i].reg, sx9324_default_regs[i].val)!=kStatus_Success)
    		 return false;
    }
    SX9324_DELAY_MS(50);

    // 4. Calibration
    sx9324_manualcalibration(dev);
    SX9324_DELAY_MS(100);

    // 5. Clear Initial IRQ
    uint8_t dummy;
    if(sx9324_readregs(dev, SX932x_IRQSTAT_REG, &dummy, 1)!=kStatus_Success)
		 return false;


    uint8_t stat2;
    if(sx9324_readregs(dev, SX932x_STAT2_REG, &stat2, 1)!=kStatus_Success)
		 return false;
    PRINTF("[SAR] Calib Status (STAT2): 0x%02X\r\n", stat2);

    PRINTF("[SAR] sx9324 init ok \r\n");
    return true;
}

void sx9324_manualcalibration(SX9324_Handle_t *dev) {
    sx9324_writereg(dev, SX932x_STAT2_REG, 0x0F);
}

void sx9324_process(SX9324_Handle_t *dev) {


    uint8_t whoami = 0;
//    status_t result = sx9324_readregs(dev, SX932x_WHOAMI_REG, &whoami, 1);

//    if (result != kStatus_Success) {
//        PRINTF("I2C Read Failed! Error Code: %d\r\n", result);
//    } else {
//        // 正常應該印出 0x21 或 0x22，如果印出 0x00 代表 I2C 線路有問題
//        PRINTF("I2C Alive. ID: 0x%02X\r\n", whoami);
//    }

//    PRINTF("GPIO_PinRead -> PIO2_14: %d \r\n",GPIO_PinRead(dev->gpio_base, dev->nirq_port, dev->nirq_pin));

#if 1
    uint8_t irq_src=0 ,prox_stat =0 ,body_stat= 0;
    sx9324_readregs(dev, SX932x_IRQSTAT_REG, &irq_src, 1);
    PRINTF("Reg 0x00 : 0x%02X\r\n",irq_src);
    sx9324_readregs(dev, SX932x_STAT0_REG, &prox_stat, 1);
    PRINTF("Reg 0x01 : 0x%02X\r\n",prox_stat);
    sx9324_readregs(dev, SX932x_STAT1_REG, &body_stat, 1);
    PRINTF("Reg 0x02 : 0x%02X\r\n",body_stat);
#endif

	dev->status.irq_stat = irq_src;
	dev->status.prox_stat = prox_stat;
	dev->status.body_stat = body_stat;

	if ((dev->status.irq_stat & SX932x_IRQ_TOUCH)){

		//SX9324_ReadRegs(dev, SX932x_STAT0_REG, &dev->status.prox_stat, 1);
		//SX9324_ReadRegs(dev, SX932x_STAT1_REG, &dev->status.body_stat, 1);

		if(dev->status.prox_stat !=0 && dev->status.body_stat!=0)
			{
				PRINTF("[SAR] HUMAN BODY Detected! \r\n");
			}
		else if (dev->status.prox_stat !=0)
			{
				PRINTF("[SAR] Detecte near \r\n");
			}
	}
	else if ((irq_src & SX932x_IRQ_RELEASE)){

				PRINTF("[SAR] Detecte leave \r\n");
	}
    PRINTF("\n");

#if 0
    // RT685 GPIO Read: GPIO_PinRead(Base, Port, Pin)
    if (GPIO_PinRead(dev->gpio_base, dev->nirq_port, dev->nirq_pin) == 0) {

        uint8_t irq_src = 0;

        if(SX9324_ReadRegs(dev, SX932x_IRQSTAT_REG, &irq_src, 1) == kStatus_Success) {
            dev->status.irq_stat = irq_src;
            PRINTF("Reg 0x00 : 0x%02X\r\n",irq_src);

            if ((irq_src & SX932x_IRQ_TOUCH) || (irq_src & SX932x_IRQ_RELEASE) || (irq_src & SX932x_IRQ_CONV)) {
                SX9324_ReadRegs(dev, SX932x_STAT0_REG, &dev->status.prox_stat, 1);
                PRINTF("Reg 0x01 prox_stat: 0x%02X\r\n",dev->status.prox_stat);
                SX9324_ReadRegs(dev, SX932x_STAT1_REG, &dev->status.body_stat, 1);
                PRINTF("Reg 0x02 body_stat: 0x%02X\r\n",dev->status.body_stat);
            }
            else{
            	dev->status.prox_stat = 0;
            	dev->status.body_stat = 0;
            }
        }
    }
#endif
}

void sx9324_readrawdata(SX9324_Handle_t *dev, uint8_t channel, SX9324_ChannelData_t *data) {
    uint8_t buf[2];

    sx9324_writereg(dev, 0x60, channel); // CPSRD

    sx9324_readregs(dev, 0x61, buf, 2);
    data->useful = (int32_t)((buf[0] << 8) | buf[1]);

    sx9324_readregs(dev, 0x63, buf, 2);
    data->average = (int32_t)((buf[0] << 8) | buf[1]);

    sx9324_readregs(dev, 0x65, buf, 2);
    data->diff = (int32_t)((buf[0] << 8) | buf[1]);

    sx9324_readregs(dev, 0x67, buf, 2);
    data->offset = (uint16_t)((buf[0] << 8) | buf[1]);

    if (data->useful > 32767) data->useful -= 65536;
    if (data->average > 32767) data->average -= 65536;
    if (data->diff > 32767) data->diff -= 65536;
}
