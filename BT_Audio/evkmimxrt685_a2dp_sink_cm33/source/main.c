/*
 * Copyright 2020 - 2021, 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include "app_a2dp_sink.h"
#include "aw88166.h"
#include "qar87_config.h"
#include "fsl_gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
extern void BOARD_InitHardware(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

int main(void)
{
    BOARD_InitHardware();

    if (xTaskCreate(app_board_init_task, "app_board_init_task", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL) != pdPASS)
    {
        PRINTF("init task creation failed!\r\n");
        while (1)
            ;
    }

    if (xTaskCreate(app_a2dp_sink_task, "app_a2dp_sink_task", configMINIMAL_STACK_SIZE * 8, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        PRINTF("a2dp task creation failed!\r\n");
        while (1)
            ;
    }


    vTaskStartScheduler();

    for (;;)
        ;
}
