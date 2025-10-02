/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __SubFunc_H___
#define __SubFunc_H___

#include <xtensa/tie/xt_timer.h>



void delay_us(U32 d);
void delay_ms(U32 d);

extern void ErrDisplay(U32 ErrCode, U32 SubErrCode);

extern volatile U32 CycCntA;
extern volatile U32 CycCntB;
extern volatile U32 CycCnt1;
extern volatile U32 CycCnt2;

extern void DbgPin_INIT();
extern void LED_TOGGLE();

extern void BlinkDbgPinNTime(int WhichPin, int NTimes);
extern void BlinkLedPinNTime(int WhichPin, int NTimes);
extern void *GetCurrentHeapTail(int sizeToAttemp);
extern void *GetCurrentStackHead(void);

#endif

