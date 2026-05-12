/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_DEBUG_CONSOLE_CONF_H_
#define _FSL_DEBUG_CONSOLE_CONF_H_

/* Burst hex dump (e.g. 1024 bytes -> 2048 chars + CRLF) needs headroom vs ring (size - 1) usable. */
#define DEBUG_CONSOLE_TRANSMIT_BUFFER_LEN 2048U
/* Serial manager RX ring holds at most (size - 1) bytes; need > app RX block (e.g. 1024) + margin. */
#define DEBUG_CONSOLE_RECEIVE_BUFFER_LEN 2048U
#define DEBUG_CONSOLE_TX_RELIABLE_ENABLE 1
// #define DEBUG_CONSOLE_RX_ENABLE 0
#define DEBUG_CONSOLE_PRINTF_MAX_LOG_LEN 128U
#define DEBUG_CONSOLE_SCANF_MAX_LOG_LEN 20U
#define DEBUG_CONSOLE_SYNCHRONIZATION_BM 0
#define DEBUG_CONSOLE_SYNCHRONIZATION_FREERTOS 1
// #define DEBUG_CONSOLE_DISABLE_RTOS_SYNCHRONIZATION 0
// #define DEBUG_CONSOLE_ENABLE_ECHO_FUNCTION 0
// #define BOARD_USE_VIRTUALCOM 0

#endif /* _FSL_DEBUG_CONSOLE_CONF_H_ */
