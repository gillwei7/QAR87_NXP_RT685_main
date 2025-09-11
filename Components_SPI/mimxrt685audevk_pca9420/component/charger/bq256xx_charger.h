/*
 * bq256xx_charger.h
 *
 *  Created on: 2025年8月28日
 *      Author: 11301026
 */

#ifndef CHARGER_BQ256XX_CHARGER_H_
#define CHARGER_BQ256XX_CHARGER_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* BQ256xx I2C slave address (7-bit) */
#define BQ256XX_I2C_ADDR          0x6A

/*Input Voltage Limit (VINDPM)*/
#define BQ256XX_VINDPM_MIN_uV     3900000
#define BQ256XX_VINDPM_MAX_uV     5400000
#define BQ256XX_VINDPM_STEP_uV    100000
#define BQ256XX_VINDPM_OFFSET_uV  3900000
/*Input Current Limit (IINDPM)*/
#define BQ256XX_IINDPM_MIN_uA     100000
#define BQ256XX_IINDPM_MAX_uA     3200000
#define BQ256XX_IINDPM_STEP_uA    100000
#define BQ256XX_IINDPM_OFFSET_uA  100000
/*Charge Current (ICHG)*/
#define BQ256XX_ICHG_STEP_uA      60000
#define BQ256XX_ICHG_MAX_uA       3000000
/*Battery Regulation Voltage (VBATREG)*/
#define BQ2560X_VBATREG_MIN_uV    3856000
#define BQ2560X_VBATREG_MAX_uV    4624000
#define BQ2560X_VBATREG_STEP_uV   32000
/*Precharge / Termination Current*/
#define BQ256XX_IPRECHG_MIN_uA    60000
#define BQ256XX_IPRECHG_MAX_uA    780000
#define BQ256XX_ITERM_MIN_uA      60000
#define BQ256XX_ITERM_MAX_uA      780000


/* Register Map */
#define BQ256XX_REG_INPUT_LIMIT   0x00
#define BQ256XX_REG_PUMP_CONTROL  0x01
#define BQ256XX_REG_CHG_CURRENT   0x02
#define BQ256XX_REG_PRE_TERM_CURR 0x03
#define BQ256XX_REG_VBAT_LIMIT    0x04
#define BQ256XX_REG_CHG_CTRL1     0x05
#define BQ256XX_REG_CHG_CTRL2     0x06
#define BQ256XX_REG_CHG_CTRL3     0x07
#define BQ256XX_REG_CHG_STAT0     0x08
#define BQ256XX_REG_CHG_STAT1     0x09
#define BQ256XX_REG_CHG_STAT2     0x0A
#define BQ256XX_REG_PART_INFO     0x0B

/* --- Bit Masks --- */
/* REG05 (CHG_CTRL1) */
#define BQ256XX_WDT_MASK          (0x3 << 4)  // bits[5:4]
#define BQ256XX_WDT_DISABLE       (0x0 << 4)
#define BQ256XX_WDT_40S           (0x1 << 4)
#define BQ256XX_WDT_80S           (0x2 << 4)
#define BQ256XX_WDT_1600S         (0x3 << 4)

/* REG06 (CHG_CTRL2) */
#define BQ256XX_VINDPM_MASK       0x0F        // bits[3:0]

/* REG00 (INPUT_LIMIT) */
#define BQ256XX_IINDPM_MASK       0x1F        // bits[4:0]

/* REG02 (CHG_CURRENT) */
#define BQ256XX_ICHG_MASK         0x7F        // bits[6:0]

/* REG03 (PRECHG/TERM) */
#define BQ256XX_IPRE_MASK         0x0F        // bits[3:0]
#define BQ256XX_ITERM_MASK        0xF0        // bits[7:4]

/* REG04 (VBAT LIMIT) */
#define BQ256XX_VBATREG_MASK      0xF8        // bits[7:3]

/* REG07 (CHG_CTRL3) */
#define BQ256XX_JEITA_VSET_DIS    (1 << 4)

/* REG0B (PART_INFO) */
#define BQ256XX_REG_RST           (1 << 7)

/* Charger configuration */
typedef struct {
    uint32_t vindpm_uv;     // Input voltage limit (uV)
    uint32_t iindpm_ua;     // Input current limit (uA)
    uint32_t ichg_ua;       // Fast charge current (uA)
    uint32_t vbatreg_uv;    // Battery regulation voltage (uV)
    uint32_t iprechg_ua;    // Pre-charge current (uA)
    uint32_t iterm_ua;      // Termination current (uA)
    uint32_t wdt_ms;        // Watchdog timeout (0=disable, else 40s/80s/1600s)
} bq256xx_cfg_t;

/* Charger runtime status */
typedef struct {
    bool power_good;
    bool vbus_good;
    uint8_t vbus_stat;
    uint8_t chg_stat;
    uint8_t fault_stat;
} bq256xx_status_t;

/* Low-level I2C hooks: must be implemented on MCU side */
status_t  bq256xx_write_reg(uint8_t reg, uint8_t cmd);
status_t  bq256xx_read_reg(uint8_t reg, uint8_t *buf, uint8_t len);
status_t  bq256xx_update_bits(uint8_t reg, uint8_t mask, uint8_t val);

/* High-level API */
status_t bq256xx_init(const bq256xx_cfg_t *cfg);
status_t bq256xx_set_vindpm(uint32_t uv);
status_t bq256xx_set_iindpm(uint32_t ua);
status_t bq256xx_set_ichg(uint32_t ua);
status_t bq256xx_set_vbatreg(uint32_t uv);
status_t bq256xx_set_iprechg(uint32_t ua);
status_t bq256xx_set_iterm(uint32_t ua);
status_t bq256xx_set_wdt(uint32_t ms);

status_t bq256xx_poll_status(bq256xx_status_t *stat);
status_t bq256xx_chip_reset(void);

#endif /* CHARGER_BQ256XX_CHARGER_H_ */
