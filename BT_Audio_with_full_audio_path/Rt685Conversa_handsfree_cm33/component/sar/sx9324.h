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


typedef enum {
    SAR_EVENT_NONE = 0,     // 無事件或無效中斷
    SAR_EVENT_APPROACH,     // 接近（near）
    SAR_EVENT_DEPART,       // 遠離（leave）
    SAR_EVENT_BODY          // 人體靠近（human body detected）
} SAR_EVENT_t;


/* API Prototypes */
bool sx9324_init();
SAR_EVENT_t sx9324_process();
void sx9324_readrawdata(uint8_t channel, SX9324_ChannelData_t *data);
void sx9324_manualcalibration();

#endif /* SAR_SX9324_H_ */
