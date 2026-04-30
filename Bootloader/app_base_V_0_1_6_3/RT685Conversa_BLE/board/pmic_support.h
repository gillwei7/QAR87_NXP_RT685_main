/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef _PMIC_SUPPORT_H_
#define _PMIC_SUPPORT_H_

#if UsingQAR87Board == 1
#include "fsl_pca9422.h"
#else
#include "fsl_pca9420.h"
#endif
#include "fsl_power.h"

/*******************************************************************************
 * DEFINITION
 ******************************************************************************/
#if UsingQAR87Board == 1
extern pca9422_handle_t pca9422Handle;
#else
extern pca9420_handle_t pca9420Handle;
#endif
/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/
#if UsingQAR87Board == 1
void BOARD_InitPmic(void);
void BOARD_SetVddCoreVoltage(uint32_t millivolt);
bool BOARD_SetPmicVoltageForFreq(uint32_t cm33_clk_freq, uint32_t dsp_clk_freq);
void BOARD_SetPmicVoltageBeforeDeepSleep(void);
void BOARD_RestorePmicVoltageAfterDeepSleep(void);
void BOARD_SetPmicVoltageBeforeDeepPowerDown(void);
#else
void BOARD_SetPmicVoltageForFreq(power_part_temp_range_t temp_range, uint32_t main_clk_freq, uint32_t dsp_main_clk_freq);
void BOARD_InitPmic(void);
#endif
#if defined(__cplusplus)
}
#endif /* __cplusplus*/

#endif /* _PMIC_SUPPORT_H_ */
