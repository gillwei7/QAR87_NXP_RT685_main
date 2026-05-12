/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "uart2_data_port.h"
#include "fsl_usart.h"
#include "fsl_clock.h"
#include "fsl_reset.h"

#ifndef UART2_DATA_PORT_BAUDRATE
#define UART2_DATA_PORT_BAUDRATE (115200U)
#endif

/* Same FRG as BOARD_DEBUG_UART_FRG_CLK when Using_UART2ToPrint (see board.h). */
static const clock_frg_clk_config_t s_uart2FrgClk = {
    .num           = 2U,
    .sfg_clock_src = kCLOCK_FrgPllDiv,
    .divider       = 255U,
    .mult          = 0U,
};

static uint8_t s_uart2DataPortInited;

void UART2_DataPort_Init(void)
{
    if (s_uart2DataPortInited != 0U)
    {
        return;
    }

    CLOCK_SetFRGClock(&s_uart2FrgClk);
    (void)CLOCK_AttachClk(kFRG_to_FLEXCOMM2);
    RESET_PeripheralReset(kFC2_RST_SHIFT_RSTn);

    {
        usart_config_t cfg;

        USART_GetDefaultConfig(&cfg);
        cfg.baudRate_Bps = UART2_DATA_PORT_BAUDRATE;
        cfg.enableTx     = true;
        cfg.enableRx     = true;
        (void)USART_Init(USART2, &cfg, CLOCK_GetFlexCommClkFreq(2U));
    }

    s_uart2DataPortInited = 1U;
}
