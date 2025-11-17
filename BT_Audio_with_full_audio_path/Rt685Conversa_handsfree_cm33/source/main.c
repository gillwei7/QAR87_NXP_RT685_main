/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


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

#if UsingQAR87Board == 1
#include "hal_common.h"
#endif
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
    hal_board_init();

    if (xTaskCreate(hfp_hf_a2dp_task, "hfp_hf_a2dp_task", configMINIMAL_STACK_SIZE * 8, NULL,		//was configMINIMAL_STACK_SIZE * 8
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




/*

shell commands:

bt connect_paired 1

bt playopus 2
bt playsbc 1

*/

/*


ToAMp: fc1 ToAmp, fc3 FrAMP  (fc1 clk share to fc3), fc1 is master, AMP is slave
ToBt:  fc4 toBT, fc2 FrBT //B36932 seems incorrect, should be fc4 frBT, fc toBT//  (fc2 clk share to fc4), fc2 is slave, BT is master
ToNvt: fc5 toNvt, fc6 FrNvt  (fc5 clk share to fc6), fc5 is master, NVT is slave


*/
