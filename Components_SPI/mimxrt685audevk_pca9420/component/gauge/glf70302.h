/*
 * glf70302.h
 *
 *  Created on: 2025年9月15日
 *      Author: 11301026
 */

#ifndef GAUGE_GLF70302_H_
#define GAUGE_GLF70302_H_


#include <stdint.h>
#include <stdbool.h>

// I2C address
#define GLF70302_I2C_ADDR 0x55

// Register addresses
#define REG_OCV_LSB      0x00
#define REG_OCV_MSB      0x01
#define REG_CYCLE_LSB    0x06
#define REG_CYCLE_MSB    0x07
#define REG_TEMP         0x08
#define REG_SOC          0x09
#define REG_DIETEMP      0x0D
#define REG_MEMCOMM      0x10
#define REG_SEALSTAT     0x11
#define REG_ADDR0        0x12
#define REG_ADDR1        0x13
#define REG_DATA0        0x14
#define REG_DATA1        0x15
#define REG_DATA2        0x16
#define REG_DATA3        0x17
#define REG_SET          0x18
#define REG_MEMSET       0x19
#define REG_SOCHOST      0x1B
#define REG_TEMPHOST     0x21
#define REG_VBATHOST     0x22
#define REG_IBATHOST     0x24
#define REG_IBAT_LSB     0x3A
#define REG_IBAT_MSB     0x3B
#define REG_VBAT_LSB     0x3C
#define REG_VBAT_MSB     0x3D
#define REG_FLAG         0x4E

// Function prototypes
bool glf70302_init(void);
bool glf70302_read_register(uint8_t reg, uint8_t *value);
bool glf70302_write_register(uint8_t reg, uint8_t value);

bool glf70302_read_voltage(uint16_t *mv);
bool glf70302_read_current(int16_t *ma);
bool glf70302_read_temperature(int8_t *temp);
bool glf70302_read_soc(uint8_t *soc);

bool glf70302_set_soc_host(uint8_t soc);
bool glf70302_enable_host_soc(void);


#endif /* GAUGE_GLF70302_H_ */
