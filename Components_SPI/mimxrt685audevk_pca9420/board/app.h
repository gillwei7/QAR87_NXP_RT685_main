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

//Is the Burning program on the Dev-Board?
#define USE_DEV_BOARD 1

#define GPIO0_PORT 0U
#define GPIO1_PORT 1U
#define GPIO2_PORT 2U

/*PCA9422*/
#define DVS_CTR0_PORT 	  1U
#define DVS_CTR0_PIN  	  9U
#define DVS_CTR1_PORT 	  0U
#define DVS_CTR1_PIN  	 29U
#define DVS_CTR2_PORT 	  2U
#define DVS_CTR2_PIN  	 31U
#define SLEEP_MODE0_PORT  2U
#define SLEEP_MODE0_PIN  27U
#define STBY_MODE0_PORT   0U
#define STBY_MODE0_PIN   24U
#define PCA9422_INTB_PORT 2U
#define PCA9422_INTB_PIN 28U

#define EXAMPLE_PINT_BASE PINT
#define POWERKEY_PINT_PIN_INT0_SRC kINPUTMUX_GpioPort0Pin5ToPintsel  /* Power Key */
#define FUNKEY_PINT_PIN_INT1_SRC kINPUTMUX_GpioPort0Pin26ToPintsel   /* Fun Key */
#define POWER_KEY_PORT 0U
#define POWER_KEY_PIN  5U
#define FUN_KEY1_N_PORT 0U
#define FUN_KEY1_N_PIN  26U

#define RESET553_N_PORT 0U
#define RESET553_N_PIN  27U
#define AMP_RESET_PORT 0U
#define AMP_RESET_PIN  0U
#define TOUCH_INT_PORT 1U
#define TOUCH_INT_PIN  8U
#define CHARG_INT_PORT 1U
#define CHARG_INT_PIN  15U
#define PWR_SW1_PORT 0U		//NT98532 POWER ON
#define PWR_SW1_PIN  6U

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
