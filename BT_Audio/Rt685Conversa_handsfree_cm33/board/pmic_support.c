/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "board.h"
#include "pmic_support.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(currVolt, targetVolt) \
    do                                                               \
    {                                                                \
        if ((uint32_t)(currVolt) > (uint32_t)(targetVolt))           \
        {                                                            \
            POWER_SetLvdFallingTripVoltage(kLvdFallingTripVol_720);  \
        }                                                            \
    } while (0)


/*! Invalid voltage level. */
#define POWER_INVALID_VOLT_LEVEL (0xFFFFFFFFU)

/*! Core frequency levels number. */
#define POWER_FREQ_LEVELS_NUM (5U)

#define MEGA (1000000U)

const uint32_t powerFreqLevel[POWER_FREQ_LEVELS_NUM] = {275U * MEGA, 230U * MEGA, 192U * MEGA, 100U * MEGA, 60U * MEGA};

/*******************************************************************************
 * Variables
 ******************************************************************************/

pca9422_handle_t pca9422Handle;
static pca9422_modecfg_t pca9422CurrModeCfg;
static pca9422_power_mode_t pca9422CurrMode;
static const uint8_t pca9422VoltLevel[] = {PCA9422_BUCK1_OUT_VAL(1100000), PCA9422_BUCK1_OUT_VAL(1000000),
                                           PCA9422_BUCK1_OUT_VAL(900000), PCA9422_BUCK1_OUT_VAL(800000),
                                           PCA9422_BUCK1_OUT_VAL(700000)};
static bool pmicVoltChangedForDeepSleep;
static uint8_t pmicVoltValueBeforeChange;

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t BOARD_CalcVoltLevel(uint32_t cm33_clk_freq, uint32_t dsp_clk_freq)
{
    uint32_t i;
    uint32_t volt;
    uint32_t freq = MAX(cm33_clk_freq, dsp_clk_freq);

    for (i = 0U; i < POWER_FREQ_LEVELS_NUM; i++)
    {
        if (freq > powerFreqLevel[i])
        {
            break;
        }
    }

    if (i == 0U) /* Frequency exceed max supported */
    {
        volt = POWER_INVALID_VOLT_LEVEL;
    }
    else
    {
        volt = pca9422VoltLevel[i - 1U];
    }

    return volt;
}

static void InitPCA9422Regulators(void)
{
    pca9422_regulator_config_t pca9422RegConfig;
    PCA9422_GetRegulatorDefaultConfig(&pca9422RegConfig);
    pca9422RegConfig.I2C_SendFunc    = BOARD_PMIC_I2C_Send;
    pca9422RegConfig.I2C_ReceiveFunc = BOARD_PMIC_I2C_Receive;

    PCA9422_InitRegulator(&pca9422Handle, &pca9422RegConfig);
}

static void InitPCA9422Charger(void)
{
    pca9422_charger_config_t pca9422ChgConfig;

    PCA9422_GetChargerDefaultConfig(&pca9422ChgConfig);

    pca9422ChgConfig.I2C_SendFunc    = BOARD_PMIC_I2C_Send;
    pca9422ChgConfig.I2C_ReceiveFunc = BOARD_PMIC_I2C_Receive;

    PCA9422_InitCharger(&pca9422Handle, &pca9422ChgConfig);
}

void BOARD_InitPmic(void)
{

    //BOARD_PMIC_I2C_Init();
    /* Initialize PCA9422 Regulators. */
    InitPCA9422Regulators();
    /* Initialize PCA9422 Charger Component. */
    InitPCA9422Charger();


}

bool BOARD_SetPmicVoltageForFreq(uint32_t cm33_clk_freq, uint32_t dsp_clk_freq)
{
    power_lvd_falling_trip_vol_val_t lvdVolt;
    uint32_t volt;
    bool ret;

    PCA9422_GetCurrentPowerMode(&pca9422Handle, &pca9422CurrMode);
    PCA9422_ReadPowerModeConfigs(&pca9422Handle, pca9422CurrMode, &pca9422CurrModeCfg);

    lvdVolt = POWER_GetLvdFallingTripVoltage();

    /* Enter FBB mode first */
    if (POWER_GetBodyBiasMode(kCfg_Run) != kPmu_Fbb)
    {
        POWER_EnterFbb();
    }

    volt = BOARD_CalcVoltLevel(cm33_clk_freq, dsp_clk_freq);
    ret  = volt != POWER_INVALID_VOLT_LEVEL;

    if (ret)
    {
        if (volt < PCA9422_BUCK1_OUT_VAL(800000))
        {
            POWER_DisableLVD();
        }
        else
        {
            if (volt < PCA9422_BUCK1_OUT_VAL(900000))
            {
                PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(lvdVolt, kLvdFallingTripVol_795);
            }
            else if (volt < PCA9422_BUCK1_OUT_VAL(1000000))
            {
                PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(lvdVolt, kLvdFallingTripVol_885);
            }
            else
            {
            }
        }

        /* Configure vddcore voltage value */
        pca9422CurrModeCfg.sw1OutVolt = PCA9422_BUCK1_OUT_VOLT(volt);
        PCA9422_WritePowerModeConfigs(&pca9422Handle, pca9422CurrMode, pca9422CurrModeCfg);

        if (volt >= PCA9422_BUCK1_OUT_VAL(800000))
        {
            POWER_RestoreLVD();
        }
    }

    return ret;
}

void BOARD_SetPmicVoltageBeforeDeepSleep(void)
{
    PCA9422_GetCurrentPowerMode(&pca9422Handle, &pca9422CurrMode);
    PCA9422_ReadPowerModeConfigs(&pca9422Handle, pca9422CurrMode, &pca9422CurrModeCfg);

    if (pca9422CurrModeCfg.sw1OutVolt <= PCA9422_BUCK1_OUT_VAL(700000))
    {
        pmicVoltValueBeforeChange   = pca9422CurrModeCfg.sw1OutVolt;
        pmicVoltChangedForDeepSleep = true;
        /* On resume from deep sleep with external PMIC, LVD is always used even if we have already disabled it.
         * Here we need to set up a safe threshold to avoid LVD reset and interrupt. */
        POWER_SetLvdFallingTripVoltage(kLvdFallingTripVol_720);
        pca9422CurrModeCfg.sw1OutVolt = PCA9422_BUCK1_OUT_VAL(750000);
        PCA9422_WritePowerModeConfigs(&pca9422Handle, pca9422CurrMode, pca9422CurrModeCfg);
    }
    else
    {
    }
}

void BOARD_RestorePmicVoltageAfterDeepSleep(void)
{
    if (pmicVoltChangedForDeepSleep)
    {
        PCA9422_GetCurrentPowerMode(&pca9422Handle, &pca9422CurrMode);
        PCA9422_ReadPowerModeConfigs(&pca9422Handle, pca9422CurrMode, &pca9422CurrModeCfg);
        pca9422CurrModeCfg.sw1OutVolt = pmicVoltValueBeforeChange;
        PCA9422_WritePowerModeConfigs(&pca9422Handle, pca9422CurrMode, pca9422CurrModeCfg);
        pmicVoltChangedForDeepSleep = false;
    }
    else
    {
    }
}

void BOARD_SetPmicVoltageBeforeDeepPowerDown(void)
{
    PCA9422_GetCurrentPowerMode(&pca9422Handle, &pca9422CurrMode);
    PCA9422_ReadPowerModeConfigs(&pca9422Handle, pca9422CurrMode, &pca9422CurrModeCfg);

    /* Wakeup from deep power down is same as POR, and need VDDCORE >= 1.0V. Otherwise
       0.9V LVD reset value may cause wakeup failure. */
    if (pca9422CurrModeCfg.sw1OutVolt < PCA9422_BUCK1_OUT_VAL(1000000))
    {
        pca9422CurrModeCfg.sw1OutVolt = PCA9422_BUCK1_OUT_VAL(1000000);
        PCA9422_WritePowerModeConfigs(&pca9422Handle, pca9422CurrMode, pca9422CurrModeCfg);
    }
    else
    {
    }
}
