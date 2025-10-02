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

    int16_t current = ((int16_t)msb << 8) | lsb;
    PRINTF("[GAUGE]IBAT raw: MSB=0x%02X, LSB=0x%02X → current=%d mA\r\n", msb, lsb, current);

    *ma = current;
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


bool glf70302_read_battery(BatteryInfo *info) {


    memset(info, 0, sizeof(BatteryInfo));  // 全部初始化為 0

    bool success = true;

    if (!glf70302_read_soc(&info->soc)) {
        info->soc = 0;
        success = false;
    }

    if (!glf70302_read_voltage(&info->voltage)) {
        info->voltage = 0;
        success = false;
    }

    if (!glf70302_read_current(&info->current)) {
        info->current = 0;
        success = false;
    }

    if (!glf70302_read_temperature(&info->temperature)) {
        info->temperature = 0;
        success = false;
    }

    PRINTF("[GAUGE] 目前電量: %d%%\r\n", info->soc);
    PRINTF("[GAUGE] 電池電壓: %d mV\r\n", info->voltage);

//	char buffer[32];
//	sprintf(buffer, "%d", info->current);  // 使用標準 C 的 sprintf 處理符號
//	PRINTF("電池電流: %s mA\r\n", buffer);

    PRINTF("[GAUGE] 電池電流: %d mA\r\n", info->current);
    PRINTF("[GAUGE] 電池溫度: %d°C\r\n", info->temperature);

    return success;

}
