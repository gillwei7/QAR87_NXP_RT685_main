/*
 * pmic_pca9422_init.c
 *
 *  Created on: 2025年7月30日
 *      Author: 11301026
 */
#include "pmic_pca9422.h"
#include "fsl_power.h"
#include <assert.h>
#include "pmic_support.h"


extern pca9422_handle_t pca9422Handle;  // Must be defined elsewhere

void BOARD_Config_PMICModes(pca9422_modecfg_t *cfg, pca9422_power_mode_t mode)
{
    assert(cfg);

    /* Configuration PMIC mode to align with power lib like below:
     *  0b00    run mode, no special.
     *  0b01    deep sleep mode, vddcore 0.6V.
     *  0b10    deep powerdown mode, vddcore off.
     *  0b11    full deep powerdown mode vdd1v8 and vddcore off. */

    /* Mode 1: VDDCORE 0.6V. */
    cfg[kPCA9422_SleepMode].sw1OutVolt = 600000U;
}

void BOARD_Config_PMICEnMode(pca9422_enmodecfg_t *cfg)
{
    assert(cfg);

    /* Configuration PMIC mode to align with power lib like below:
     *  0b00    run mode, no special.
     *  0b01    deep sleep mode, vddcore 0.6V.
     *  0b10    deep powerdown mode, vddcore off.
     *  0b11    full deep powerdown mode vdd1v8 and vddcore off. */

    /* Mode 2: VDDCORE off. */
    cfg->sw1OutEnMode = kPCA9422_EnmodeOnActiveSleep;

    /* Mode 3: VDDCORE, VDD1V8 and VDDIO off. */
    cfg->sw2OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;
    cfg->ldo2OutEnMode = kPCA9422_EnmodeOnActiveSleepStandby;
}

void BOARD_Config_PMICRegEnable(pca9422_regulatoren_t *cfg)
{
    assert(cfg);

    /* All regulators enable in RUN state. */
    cfg->sw1Enable  = true;
    cfg->sw2Enable  = true;
    cfg->sw3Enable  = true;
    cfg->sw4Enable  = true; //SW4 off，To close on the small board
    cfg->ldo1Enable = true;
    cfg->ldo2Enable = true; //LDO2 off，To close on the small board
    cfg->ldo3Enable = true;
    cfg->ldo4Enable = true;
}

void BOARD_Init_PMICConfigure(void)
{
    pca9422_modecfg_t pca9422ModeCfg[kPCA9422_PowerModeMax];
    pca9422_enmodecfg_t pca9422EnmodeCfg;
    pca9422_regulatoren_t pca9422RegEnCfg;
    int i;

    /* BE CAUTIOUS TO SET CORRECT VOLTAGE RANGE ACCORDING TO YOUR BOARD/APPLICATION. PAD SUPPLY BEYOND THE RANGE DO
       HARM TO THE SILICON. */
    power_pad_vrange_t vrange = {.Vdde0Range = kPadVol_171_198,
                                 .Vdde1Range = kPadVol_171_198,
                                 .Vdde2Range = kPadVol_171_198,
                                 .Vdde3Range = kPadVol_300_360,
                                 .Vdde4Range = kPadVol_171_198};

    /* Configure PMIC Vddcore value according to CM33 clock. DSP not used in this demo. */
    BOARD_SetPmicVoltageForFreq(CLOCK_GetFreq(kCLOCK_CoreSysClk), 0U); //B36932
    //BOARD_SetPmicVoltageForFreq(CLOCK_GetFreq(kCLOCK_CoreSysClk), 600000000U); //B36932

    /* Indicate to power library that PMIC is used. */
    POWER_UpdatePmicRecoveryTime(1);

    POWER_SetPadVolRange(&vrange);

    /* Configure PMIC modes. */
    for (i = 0; i < ARRAY_SIZE(pca9422ModeCfg); i++)
    {
    	PCA9422_GetDefaultPowerModeConfig(&pca9422ModeCfg[i]);
    }
    for (i = 0; i < ARRAY_SIZE(pca9422ModeCfg); i++)
    {
		BOARD_Config_PMICModes(pca9422ModeCfg, i);
		PCA9422_WritePowerModeConfigs(&pca9422Handle, i, pca9422ModeCfg[i]);
    }
    /* Configure ENMODE */
    PCA9422_GetDefaultEnModeConfig(&pca9422EnmodeCfg);
    BOARD_Config_PMICEnMode(&pca9422EnmodeCfg);
    PCA9422_WriteEnModeConfig(&pca9422Handle, pca9422EnmodeCfg);
    /* Configure Regulator Enable */
    PCA9422_GetDefaultRegEnableConfig(&pca9422RegEnCfg);
    BOARD_Config_PMICRegEnable(&pca9422RegEnCfg);
    PCA9422_WriteRegEnableConfig(&pca9422Handle, pca9422RegEnCfg);
    /* Set Buck DVS configuration */
    /* BUCKxOUT_DVSx by DVS Pin in active mode and BUCKxOUT_SLEEP in sleep mode */
    PCA9422_SetBuckDVSControl(&pca9422Handle, kPCA9422_RegulatorSwitch1, (uint8_t)kPCA9422_PinInActiveAndBxOUTSLEEPInSleep);
    PCA9422_SetBuckDVSControl(&pca9422Handle, kPCA9422_RegulatorSwitch2, (uint8_t)kPCA9422_PinInActiveAndBxOUTSLEEPInSleep);
    PCA9422_SetBuckDVSControl(&pca9422Handle, kPCA9422_RegulatorSwitch3, (uint8_t)kPCA9422_PinInActiveAndBxOUTSLEEPInSleep);
}

