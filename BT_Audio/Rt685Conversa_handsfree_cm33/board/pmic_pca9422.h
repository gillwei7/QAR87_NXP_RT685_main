/*
 * pmic_pca9422.h
 *
 *  Created on: 2025年7月30日
 *      Author: 11301026
 */

#ifndef PMIC_PCA9422_H_
#define PMIC_PCA9422_H_

#include "fsl_pca9422.h"

void BOARD_Init_PMICConfigure(void);
void BOARD_Config_PMICModes(pca9422_modecfg_t *cfg, pca9422_power_mode_t mode);
void BOARD_Config_PMICEnMode(pca9422_enmodecfg_t *cfg);
void BOARD_Config_PMICRegEnable(pca9422_regulatoren_t *cfg);



#endif /* PMIC_PCA9422_H_ */
