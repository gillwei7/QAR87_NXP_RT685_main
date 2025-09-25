/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <xtensa/config/core.h>
#include <xtensa/xos.h>

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"

#include "fsl_dma.h"
#include "fsl_mu.h"

#include "pin_mux.h"
#include "board_hifi4.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"

#include "GlobalDef.h"
#include "SubFunc.h"
#include "AudioProc_Conversa.h"

volatile U32 CycCntA;
volatile U32 CycCntB;
volatile U32 CycCnt1;
volatile U32 CycCnt2;

__attribute__((__section__(".iram.text")))
void delay_us(U32 d)
{
	static volatile int delay_us_cnt;
    volatile uint32_t i = 0;
	for (i = 0; i < 3*d; i++)
	{
		//__NOP();
		delay_us_cnt++;
	}
}
__attribute__((__section__(".iram.text")))
void delay_ms(U32 d)
{
	static volatile int delay_ms_cnt;
    volatile uint32_t i = 0;
    volatile uint32_t j = 0;
    for (j = 0; j < d; j++)
		for (i = 0; i < 30000; i++)
		{
			//__NOP();
			delay_ms_cnt++;
		}
}

void *GetCurrentHeapTail(int sizeToAttemp)
{
    int *p;
    p=NULL;
    p=malloc(sizeToAttemp);     //total heap size is 0x40000
    if(p==NULL)
        while(1){};
    free(p);
    return p;
}

void *GetCurrentStackHead(void)
{
    volatile int p;
    return (void *)&p;
}

void BlinkDbgPinNTime(int WhichPin, int NTimes)
{
	int i;
	for(i=0;i<NTimes;i++)
	{
		switch(WhichPin)
		{
			case 5: DbgPin5Up(); break;
			case 6: DbgPin6Up(); break;
			case 7: DbgPin7Up(); break;
			case 8: DbgPin8Up(); break;
		}
		delay_us(200);
		switch(WhichPin)
		{
			case 5: DbgPin5Dn(); break;
			case 6: DbgPin6Dn(); break;
			case 7: DbgPin7Dn(); break;
			case 8: DbgPin8Dn(); break;
		}
		delay_us(200);
	}
}

