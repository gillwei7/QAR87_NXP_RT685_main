/*
 * glf70302.c
 *
 *  Created on: 2025年9月15日
 *      Author: 11301026
 */


#include "board.h"
#include "glf70302.h"
#include "fsl_debug_console.h"


bool glf70302_read_register(uint8_t reg, uint8_t *value) {
    status_t ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR,
                                     GLF70302_I2C_ADDR,
                                     reg,
                                     1,          // subaddressSize
                                     value,
                                     1);         // rxBuffSize
    //PRINTF("[Debug][glf70302_read_register] BOARD_I3C_Receive ret:%d \r\n",ret);
    return (ret == kStatus_Success);
}

bool glf70302_write_register(uint8_t reg, uint8_t value) {
    status_t ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR,
                                  GLF70302_I2C_ADDR,
                                  reg,
                                  1,          // subaddressSize
                                  &value,
                                  1);         // txBuffSize
    return (ret == kStatus_Success);
}


bool glf70302_read_voltage(uint16_t *mv) {
    uint8_t lsb, msb;
    if (!glf70302_read_register(REG_VBAT_LSB, &lsb)) return false;
    if (!glf70302_read_register(REG_VBAT_MSB, &msb)) return false;
    *mv = ((uint16_t)msb << 8) | lsb;
    return true;
}

bool glf70302_read_current(int16_t *ma) {
    uint8_t lsb, msb;
    if (!glf70302_read_register(REG_IBAT_LSB, &lsb)) return false;
    if (!glf70302_read_register(REG_IBAT_MSB, &msb)) return false;
    *ma = ((int16_t)msb << 8) | lsb;
    return true;
}

bool glf70302_read_temperature(int8_t *temp) {
    return glf70302_read_register(REG_TEMP, (uint8_t *)temp);
}

bool glf70302_read_soc(uint8_t *soc) {
    return glf70302_read_register(REG_SOC, soc);
}

bool glf70302_set_soc_host(uint8_t soc) {
    return glf70302_write_register(REG_SOCHOST, soc);
}

bool glf70302_enable_host_soc(void) {
    uint8_t reg;
    if (!glf70302_read_register(REG_SET, &reg)) return false;
    reg |= (1 << 3);  // set bit3 = HOSTSOCINIT
    return glf70302_write_register(REG_SET, reg);
}
