/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UART2_DATA_PORT_H
#define UART2_DATA_PORT_H

#include "fsl_common.h"

/*!
 * @brief Clock and USART2 for raw RX/TX (e.g. Xmodem), independent of DbgConsole and Using_UART2ToPrint.
 *
 * FC2 pins are expected to be muxed elsewhere (e.g. BOARD_InitScoPins on QAR87).
 * Safe to call more than once (subsequent calls are no-ops).
 */
void UART2_DataPort_Init(void);

#endif /* UART2_DATA_PORT_H */
