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

    if (xTaskCreate(hfp_hf_a2dp_task, "hfp_hf_a2dp_task", configMINIMAL_STACK_SIZE * 8, NULL,  tskIDLE_PRIORITY + 1, NULL) != pdPASS)
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

shell commands: ---- in UART

bt connect_paired 1

bt playopus 0
bt playopus 1
bt playopus 2
bt playsbc 0
bt playsbc 1
bt playsbc 2



shell commands: ---- in USB com

//--- to play opus file ---
a 0
a 1
a 2

//--- to play sbc file ---
b 0
b 1
b 2

*/

/*

Fc ports in this project:

ToAMp: fc1 ToAmp, fc3 FrAMP  (fc1 clk share to fc3), fc1 is master, AMP is slave
ToBt:  fc4 FrBT,  fc2 ToBT   (fc2 clk share to fc4), fc2 is slave, BT is master
ToNvt: fc5 toNvt, fc6 FrNvt  (fc5 clk share to fc6), fc5 is master, NVT is slave

*/




/*

know bug 1:

play a2dp
make a tel incoming call.
Reject the call
it goes back to a2dp play.
but, the playing is broken. The printing info shows the sbc buffer goes less and less, while 1000 frame period is good --> DSP side is eating sbc too quickly????????  Later to find time to fix !!!


known bug 2:
in bt music playing, say hey quanta, and immediately start a wechat call --> crashes


known issue 3 --- not sure:
Using_UART2ToPrint Using_UART5ToPrint (either in mcu setting or dsp setting) set to 1 may cause a2dp play unstable --- crash in 5~20 minutes ???



*/


