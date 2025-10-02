/*
 * bq256xx_charger.c
 *
 *  Created on: 2025年8月28日
 *      Author: 11301026
 */

#include "bq256xx_charger.h"
#include "fsl_debug_console.h"

status_t  bq256xx_write_reg(uint8_t reg, uint8_t cmd)
{
	status_t ret;
	ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR,
						BQ256XX_I2C_ADDR,
						reg,
						1,
						&cmd,
						1);
    //PRINTF("[Debug] bq256xx_write_reg BOARD_I3C_Send ret:%d \r\n",ret);

    return ret;
}


status_t  bq256xx_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{

	status_t ret;
    ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR,
    						BQ256XX_I2C_ADDR,
							reg,
							1,
                            buf,
							len);
    //PRINTF("[Debug] BQ256XX_Read BOARD_I3C_Receive ret:%d \r\n",ret);

    return ret;

}


status_t bq256xx_update_bits(uint8_t reg, uint8_t mask, uint8_t val)
{
    status_t ret;
    uint8_t tmp;

    // Step 1: 讀取原始值
    ret = bq256xx_read_reg(reg, &tmp, 1);
    if (ret != 0) {
        PRINTF("[Error] Failed to read reg 0x%02X\r\n", reg);
        return ret;
    }

    // Step 2: 清除 mask bits，並套用新值
    tmp = (tmp & ~mask) | (val & mask);

    // Step 3: 寫入更新後的值
    ret = bq256xx_write_reg(reg, tmp);
    if (ret != 0) {
        PRINTF("[Error] Failed to write reg 0x%02X\r\n", reg);
    }

    return ret;
}


status_t bq256xx_init(const bq256xx_cfg_t *cfg)
{
    status_t ret;

    ret = bq256xx_set_vindpm(cfg->vindpm_uv);
    if (ret != kStatus_Success) return ret;

    ret = bq256xx_set_iindpm(cfg->iindpm_ua);
    if (ret != kStatus_Success) return ret;

    ret = bq256xx_set_ichg(cfg->ichg_ua);
    if (ret != kStatus_Success) return ret;

    ret = bq256xx_set_vbatreg(cfg->vbatreg_uv);
    if (ret != kStatus_Success) return ret;

    ret = bq256xx_set_iprechg(cfg->iprechg_ua);
    if (ret != kStatus_Success) return ret;

    ret = bq256xx_set_iterm(cfg->iterm_ua);
    if (ret != kStatus_Success) return ret;

    ret = bq256xx_set_wdt(cfg->wdt_ms);
    if (ret != kStatus_Success) return ret;

    // Disable JEITA VSET
    ret = bq256xx_update_bits(BQ256XX_REG_CHG_CTRL3, BQ256XX_JEITA_VSET_DIS, BQ256XX_JEITA_VSET_DIS);
    return ret;
}


status_t bq256xx_set_vindpm(uint32_t uv)
{
    if (uv < BQ256XX_VINDPM_MIN_uV || uv > BQ256XX_VINDPM_MAX_uV)
        return kStatus_Fail;

    uint8_t val = (uv - BQ256XX_VINDPM_OFFSET_uV) / BQ256XX_VINDPM_STEP_uV;

    return bq256xx_update_bits(BQ256XX_REG_CHG_CTRL2, BQ256XX_VINDPM_MASK, val);
}

status_t bq256xx_set_iindpm(uint32_t ua)
{
    if (ua < BQ256XX_IINDPM_MIN_uA || ua > BQ256XX_IINDPM_MAX_uA)
        return kStatus_Fail;

    uint8_t val = (ua - BQ256XX_IINDPM_OFFSET_uA) / BQ256XX_IINDPM_STEP_uA;
    return bq256xx_update_bits(BQ256XX_REG_INPUT_LIMIT, BQ256XX_IINDPM_MASK, val);
}

status_t bq256xx_set_ichg(uint32_t ua)
{
    if (ua > BQ256XX_ICHG_MAX_uA)
        return kStatus_Fail;

    uint8_t val = ua / BQ25618_ICHG_STEP_uA;
    return bq256xx_update_bits(BQ256XX_REG_CHG_CURRENT, BQ256XX_ICHG_MASK, val);
}

status_t bq256xx_set_vbatreg(uint32_t uv)
{
    if (uv < BQ2560X_VBATREG_MIN_uV || uv > BQ2560X_VBATREG_MAX_uV)
        return kStatus_Fail;

    uint8_t val = ((uv - BQ2560X_VBATREG_MIN_uV) / BQ2560X_VBATREG_STEP_uV) << 3;
    return bq256xx_update_bits(BQ256XX_REG_VBAT_LIMIT, BQ256XX_VBATREG_MASK, val);
}

status_t bq256xx_set_iprechg(uint32_t ua)
{
    if (ua < BQ256XX_IPRECHG_MIN_uA || ua > BQ256XX_IPRECHG_MAX_uA)
        return kStatus_Fail;

    uint8_t val = (ua - BQ25618_IPRECHG_MIN_uA) / BQ25618_IPRECHG_MIN_uA;
    val <<= 4;  // 放到 bits[7:4]

    return bq256xx_update_bits(BQ256XX_REG_PRE_TERM_CURR, BQ256XX_IPRE_MASK, val);
}

status_t bq256xx_set_iterm(uint32_t ua)
{
    if (ua < BQ25618_ITERM_MIN_uA || ua > BQ256XX_ITERM_MAX_uA)
        return kStatus_Fail;

    uint8_t val = ((ua - BQ25618_ITERM_MIN_uA) / BQ25618_ITERM_MIN_uA) ;//<< 4;
    return bq256xx_update_bits(BQ256XX_REG_PRE_TERM_CURR, BQ256XX_ITERM_MASK, val);
}

status_t bq256xx_set_wdt(uint32_t ms)
{
    uint8_t val;

    switch (ms) {
        case 0: val = BQ256XX_WDT_DISABLE; break;
        case 40000: val = BQ256XX_WDT_40S; break;
        case 80000: val = BQ256XX_WDT_80S; break;
        case 1600000: val = BQ256XX_WDT_160S; break;
        default: return kStatus_Fail;
    }

    return bq256xx_update_bits(BQ256XX_REG_CHG_CTRL1, BQ256XX_WDT_MASK, val);
}


status_t bq256xx_chip_reset(void)
{
    return bq256xx_update_bits(BQ256XX_REG_PART_INFO, BQ256XX_REG_RST, BQ256XX_REG_RST);
}


status_t bq256xx_poll_status(bq256xx_status_t *stat)
{
    uint8_t val;

    // REG08: CHG_STAT0
    if (bq256xx_read_reg(BQ256XX_REG_CHG_STAT0, &val,1) != kStatus_Success)
        return kStatus_Fail;
    stat->vbus_stat = (val >> 5) & 0x07;  // bits[7:5]
    stat->chg_stat  = (val >> 3) & 0x03;  // bits[4:3]
    stat->power_good = (val & (1 << 2)) ? true : false;

    // REG09: CHG_STAT1
    if (bq256xx_read_reg(BQ256XX_REG_CHG_STAT1, &val,1) != kStatus_Success)
        return kStatus_Fail;
    stat->fault_stat = val;  // 可進一步解析 WDT/BAT/CHRG/NTC fault

    if (bq256xx_read_reg(BQ256XX_REG_CHG_STAT2, &val, 1) != kStatus_Success)
        return kStatus_Fail;
    bool vbus_st = (val & (1 << 7)) ? true : false;
    stat->vbus_good = vbus_st;

    return kStatus_Success;
}

