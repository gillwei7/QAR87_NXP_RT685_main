/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

#include "fsl_pca9420.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
#define DEMO_PCA9420_INTB_HANDLER PMC_PMIC_IRQHandler
#define DEMO_PCA9420_INTB_IRQ     PMC_PMIC_IRQn

#define PIO0_28_PORT 0U //J27[3]
#define PIO0_28_PIN  28U

#define PIO0_29_PORT 0U //J27[2]
#define PIO0_29_PIN  29U

//I2S
#define DEMO_I2S_MASTER_CLOCK_FREQUENCY CLOCK_GetMclkClkFreq()
#define DEMO_AUDIO_BIT_WIDTH            (16)
#define DEMO_AUDIO_SAMPLE_RATE          (48000)
#define DEMO_AUDIO_PROTOCOL             kCODEC_BusI2S
#define DEMO_I2S_TX                     (I2S1)//(I2S3)
#define DEMO_DMA                        (DMA0)
#define DEMO_I2S_TX_CHANNEL             (3)//(7)
#define DEMO_I2S_CLOCK_DIVIDER          16
#define DEMO_I2S_TX_MODE                kI2S_MasterSlaveNormalMaster
#define DEMO_CODEC_VOLUME               100U
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
void BOARD_ConfigPMICModes(pca9420_modecfg_t *cfg, uint32_t num);
/*${prototype:end}*/

#endif /* _APP_H_ */
