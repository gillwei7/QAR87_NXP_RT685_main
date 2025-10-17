/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/

#define EXAMPLE_SPI_SLAVE     SPI14
#define EXAMPLE_SPI_SLAVE_IRQ FLEXCOMM14_IRQn
#define SPI_SLAVE_IRQHandler  FLEXCOMM14_IRQHandler

#define EXAMPLE_SPI_SSEL 0
#define EXAMPLE_SPI_SPOL kSPI_SpolActiveAllLow

#define USE_DEV_BOARD 1

#define GPIO0_PORT 0U
#define GPIO1_PORT 1U
#define GPIO2_PORT 2U

#define PWR_SW1_PORT 0U		//NT98532 POWER ON
#define PWR_SW1_PIN  6U
#define RESET553_N_PORT 0U
#define RESET553_N_PIN  27U

/* I2C Component*/
#define TOUCH_INT_PORT 1U
#define TOUCH_INT_PIN  8U
#define CHARG_INT_PORT 1U
#define CHARG_INT_PIN  15U
#define GAUGE_INT_PORT 2U
#define GAUGE_INT_PIN  18U
#define AMP_RESET_PORT 0U
#define AMP_RESET_PIN  0U

#define EXAMPLE_PINT_BASE PINT
#define POWERKEY_PINT_PIN_INT0_SRC kINPUTMUX_GpioPort0Pin5ToPintsel  /* Power Key */
#define FUNKEY_PINT_PIN_INT1_SRC kINPUTMUX_GpioPort0Pin26ToPintsel   /* Fun Key */
#define POWER_KEY_PORT 0U
#define POWER_KEY_PIN  5U
#define FUN_KEY1_N_PORT 0U
#define FUN_KEY1_N_PIN  26U

/* I2S */
#define DEMO_I2S_MASTER_CLOCK_FREQUENCY CLOCK_GetMclkClkFreq()
#define DEMO_AUDIO_BIT_WIDTH            (16)
#define DEMO_AUDIO_SAMPLE_RATE          (48000)
#define DEMO_AUDIO_PROTOCOL             kCODEC_BusI2S
#define DEMO_I2S_TX_toAmp               (I2S1)//(I2S3)
#define DEMO_I2S_TX_toNova              (I2S5)
#define DEMO_DMA                        (DMA0)
#define DEMO_I2S_TX_CHANNEL_toAmp       (3)//(7)
#define DEMO_I2S_TX_CHANNEL_toNova      (11)
#define DEMO_I2S_CLOCK_DIVIDER          16
#define DEMO_I2S_TX_MODE                kI2S_MasterSlaveNormalMaster
#define DEMO_CODEC_VOLUME               100U

/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
