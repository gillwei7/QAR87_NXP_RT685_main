/*
 * Copyright 2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "qar87_config.h"

#include "fsl_debug_console.h"
#include <porting.h>
#include <string.h>
#include <errno/errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/slist.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hfp_hf.h>
#include "FreeRTOS.h"
#include "task.h"

#include "app_handsfree.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if !PIN_CONFIG_DEV_BOARD
extern void BOARD_InitHardware(void);
#else
//extern void dev_BOARD_InitHardware(void);
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

int main(void)
{
#if !PIN_CONFIG_DEV_BOARD
    BOARD_InitHardware();
#else
    dev_BOARD_InitHardware();
#endif

    if (xTaskCreate(peripheral_hfp_hf_task, "peripheral_hfp_hf_task", configMINIMAL_STACK_SIZE * 8, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        PRINTF("pherial hrs task creation failed!\r\n");
        while (1)
            ;
    }

    vTaskStartScheduler();
    for (;;)
        ;
}
