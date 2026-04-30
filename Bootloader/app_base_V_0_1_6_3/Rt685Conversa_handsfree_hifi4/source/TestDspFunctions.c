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

#include "board_hifi4.h"
#include "fsl_common.h"
#include "fsl_gpio.h"

#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"

#include "GlobalDef.h"
#include "TestDspFunctions.h"




inline uint32_t read_ccount(void)
{
	unsigned int ccount;
	__asm__(
		"rsr %0, ccount"
		: "=a" (ccount) : : "memory"
	);
	return ccount;
}

#if 0
//some code for testing other functions of NatureDSP lib --- not debugged, just try to see if compile and build are successful
//--- beg---

bkfir32x32_handle_t FirFilter1;

__attribute__((aligned(8))) float FirCoef[20]={0.1,0.1};		//should be a multiple of 4
float Xn[100];
float Yn[100];
void TestDspFuntions(void)
{
	int l,i;
	int a,b;


	l=bkfirf_alloc (20,1);
	float *p=malloc(l);
	FirFilter1=bkfirf_init(p, 20, 1, FirCoef);

	while(1)
	{

		for(i=0;i<100;i++)
			Xn[i]=i/100;


		a=read_ccount();
		bkfirf_process(FirFilter1,	Yn, Xn, 10);
		b=read_ccount();
		b-=a;

		//vec_sqrt32x32 ( Yn, Xn, 100);
	}

}

//--- end---
//some code for testing other functions of NatureDSP lib --- not debugged, just try to see if compile and build are successful
#endif
