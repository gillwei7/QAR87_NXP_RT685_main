/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "fsl_common.h"
#include "board.h"
#include "pmic_support.h"

#if UsingQAR87Board == 1
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
    //B36932 InitPCA9422Charger();


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

#else
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


/*******************************************************************************
 * Variables
 ******************************************************************************/
pca9420_handle_t pca9420Handle;
static pca9420_modecfg_t pca9420CurrModeCfg;
static pca9420_mode_t pca9420CurrMode;
static const pca9420_sw1_out_t pca9420VoltLevel[5] = {
    kPCA9420_Sw1OutVolt1V150, kPCA9420_Sw1OutVolt1V000, kPCA9420_Sw1OutVolt0V900,
    kPCA9420_Sw1OutVolt0V800, kPCA9420_Sw1OutVolt0V700,
};

/* Frequency levels defined in power library. */
extern const uint32_t powerFullCm33FreqLevel[2][5];
extern const uint32_t powerFullDspFreqLevel[2][5];



/* Functions defined in power library. */
extern void POWER_DisableLVD(void);
extern void POWER_RestoreLVD(void);

/*******************************************************************************
 * Code
 ******************************************************************************/
static pca9420_sw1_out_t BOARD_CalcVoltLevel(const uint32_t *freqLevels, uint32_t freq)
{
    uint32_t i;

    for (i = 0; i < 4; i++)
    {
        if (freq > freqLevels[i])
        {
            break;
        }
    }

    return pca9420VoltLevel[i];
}

void BOARD_InitPmic(void)
{
    pca9420_config_t pca9420Config;

    CLOCK_AttachClk(kSFRO_to_FLEXCOMM15);
    BOARD_PMIC_I2C_Init();
    PCA9420_GetDefaultConfig(&pca9420Config);
    pca9420Config.I2C_SendFunc    = BOARD_PMIC_I2C_Send;
    pca9420Config.I2C_ReceiveFunc = BOARD_PMIC_I2C_Receive;
    pca9420Config.powerGoodEnable = kPCA9420_PGoodDisabled;
    PCA9420_Init(&pca9420Handle, &pca9420Config);
}

void BOARD_SetPmicVoltageForFreq(power_part_temp_range_t temp_range, uint32_t main_clk_freq, uint32_t dsp_main_clk_freq)
{
    power_lvd_falling_trip_vol_val_t lvdVolt;
    uint32_t idx = (uint32_t)temp_range;
    pca9420_sw1_out_t mainVolt, dspVolt, volt;

    PCA9420_GetCurrentMode(&pca9420Handle, &pca9420CurrMode);
    PCA9420_ReadModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);

    lvdVolt = POWER_GetLvdFallingTripVoltage();

    /* Enter FBB mode first */
    if (POWER_GetBodyBiasMode(kCfg_Run) != kPmu_Fbb)
    {
        POWER_EnterFbb();
    }

    mainVolt = BOARD_CalcVoltLevel(&powerFullCm33FreqLevel[idx][0], main_clk_freq);
    dspVolt  = BOARD_CalcVoltLevel(&powerFullDspFreqLevel[idx][0], dsp_main_clk_freq);
    volt     = MAX(mainVolt, dspVolt);

    if (volt < kPCA9420_Sw1OutVolt0V800)
    {
        POWER_DisableLVD();
    }
    else
    {
        if (volt < kPCA9420_Sw1OutVolt0V900)
        {
            PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(lvdVolt, kLvdFallingTripVol_795);
        }
        else if (volt < kPCA9420_Sw1OutVolt1V000)
        {
            PMIC_DECREASE_LVD_LEVEL_IF_HIGHER_THAN(lvdVolt, kLvdFallingTripVol_885);
        }
    }

    /* Configure vddcore voltage value */
    pca9420CurrModeCfg.sw1OutVolt = volt;
    PCA9420_WriteModeConfigs(&pca9420Handle, pca9420CurrMode, &pca9420CurrModeCfg, 1);

    if (volt >= kPCA9420_Sw1OutVolt0V800)
    {
        POWER_RestoreLVD();
    }
}
#endif