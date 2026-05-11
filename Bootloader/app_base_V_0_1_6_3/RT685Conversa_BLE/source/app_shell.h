/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __APP_SHELL_H__
#define __APP_SHELL_H__

#include "fsl_shell.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * API
 ******************************************************************************/

void app_shell_init(void);

/*!
 * @brief Register MCUBoot-style shell commands (image, xmodem, mem, reboot) on the given shell handle.
 *
 * Defined in main.c next to the SHELL_COMMAND_DEFINE tables copied from ota_mcuboot_basic.
 */
void app_mcuboot_shell_commands_register(shell_handle_t shellHandle);

#endif /* __APP_SHELL_H__ */
