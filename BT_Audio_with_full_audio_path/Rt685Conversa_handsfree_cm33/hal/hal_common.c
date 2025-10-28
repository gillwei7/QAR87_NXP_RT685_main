/*
 * hal_common.c
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#include "hal_common.h"

/**
 * @description: Delay N us
 * @paran:
 * @return {*}
 * @author: lmx
 * @param {u32} val：delay time，unit：us
 */
void hal_delay_us(uint32_t val)
{
}

/**
 * @description: Delay N ms
 * @paran:
 * @return {*}
 * @author: lmx
 * @param {u32} val：delay time，unit：ms
 */
void hal_delay_ms(uint32_t val)
{
}

void hal_gpio_port_init(void)
{
	GPIO_PortInit(GPIO, GPIO0_PORT);
	GPIO_PortInit(GPIO, GPIO1_PORT);
	GPIO_PortInit(GPIO, GPIO2_PORT);

}

void Scan_I2C_Devices(I3C_Type *base)
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


