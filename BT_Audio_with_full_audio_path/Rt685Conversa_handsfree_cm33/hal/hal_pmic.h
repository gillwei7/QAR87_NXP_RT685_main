/*
 * hal_pmic.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_PMIC_H_
#define HAL_PMIC_H_

#include "hal_common.h"
#include "pmic_support.h"
#include "pmic_pca9422.h"
#include "glf70583.h"


void hal_pmic_pca9422_init(void);
void hal_pmic_pca9422_power_down(void);
void hal_pmic_pca9422_enter_ship_mode(void);


void hal_pmic_glf70583_init (uint8_t, uint8_t);
void hal_pmic_glf70583_actual_board_init (void);
void BOARD_InitPMICs(void);


#endif /* HAL_PMIC_H_ */
#endif
