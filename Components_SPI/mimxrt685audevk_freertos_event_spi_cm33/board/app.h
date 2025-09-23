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
#define FUN_KEY1_N_PORT 2U
#define FUN_KEY1_N_PIN  19U
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
