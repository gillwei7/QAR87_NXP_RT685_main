/*
 * sx9324.h
 *
 *  Created on: 2026年1月8日
 *      Author: 11301026
 */

#ifndef SAR_SX9324_H_
#define SAR_SX9324_H_

#include <stdint.h>
#include <stdbool.h>

/* NXP RT685 SDK Includes */
#include "fsl_common.h"
#include "board.h"

/* I2C Address (7-bit standard address for Flexcomm I2C) */
#define SX9324_I2C_ADDR         0x28

/* Register Map (Same as before) */
#define SX932x_IRQSTAT_REG      0x00
#define SX932x_STAT0_REG        0x01
#define SX932x_STAT1_REG        0x02
#define SX932x_STAT2_REG        0x03
#define SX932x_IRQ_ENABLE_REG   0x05
#define SX932x_CTRL0_REG        0x10
#define SX932x_CTRL1_REG        0x11
#define SX932x_SOFTRESET_REG    0x9F
#define SX932x_WHOAMI_REG       0xFA

/* Values */
#define SX932x_SOFTRESET        0xDE
#define SX932x_WHOAMI_VALUE     0x22

/* IRQ Flags */
#define SX932x_IRQ_TOUCH        0x40
#define SX932x_IRQ_RELEASE      0x20
#define SX932x_IRQ_CONV         0x08

/* Data Structures */
typedef struct {
    int32_t useful;
    int32_t average;
    int32_t diff;
    uint16_t offset;
} SX9324_ChannelData_t;

typedef struct {
    uint8_t prox_stat;
    uint8_t body_stat;
    uint8_t irq_stat;
} SX9324_Status_t;

/* Driver Handle for RT685 */
typedef struct {
	I3C_Type    *i3c_base;    // Flexcomm Base (e.g., I2C0, I2C1...)
    GPIO_Type *gpio_base;   // Global GPIO Base (usually GPIO)
    uint32_t   nirq_port;   // GPIO Port (0 or 1 on RT685)
    uint32_t   nirq_pin;    // GPIO Pin
    SX9324_Status_t status;
} SX9324_Handle_t;

/* API Prototypes */
bool sx9324_init(SX9324_Handle_t *dev, I3C_Type *i3c_base);
void sx9324_process(SX9324_Handle_t *dev);
void sx9324_readrawdata(SX9324_Handle_t *dev, uint8_t channel, SX9324_ChannelData_t *data);
void sx9324_manualcalibration(SX9324_Handle_t *dev);

#endif /* SAR_SX9324_H_ */
