/*
 * glf70583.h
 *
 *  Created on: 2025年8月29日
 *      Author: 11301026
 */
#if UsingQAR87Board == 1
#ifndef PMIC_GLF70583_GLF70583_H_
#define PMIC_GLF70583_GLF70583_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

#define GLF70583_A_I2C_ADDR          0x25
#define GLF70583_B_I2C_ADDR          0x26

status_t  glf70583_i2c_write(uint8_t addr,uint8_t reg, uint8_t cmd);
status_t  glf70583_i2c_read(uint8_t addr,uint8_t reg, uint8_t *buf, uint8_t len);

#endif /* PMIC_GLF70583_GLF70583_H_ */
#endif //#if UsingQAR87Board == 1