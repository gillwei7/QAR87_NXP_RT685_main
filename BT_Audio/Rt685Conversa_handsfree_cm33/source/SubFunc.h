/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef __SubFunc___
#define __SubFunc___


#define     LedGrnPinPort               0
#define     LedBluPinPort               0
#define     LedRedPinPort               0

#define     LedGrnPin                   14
#define     LedBluPin                   26
#define     LedRedPin                   31

#define     DbgPin5Port                 0
#define     DbgPin6Port                 0
#define     DbgPin7Port                 0
#define     DbgPin8Port                 0

#define     DbgPin5                     3
#define     DbgPin6                     4
#define     DbgPin7                     19
#define     DbgPin8                     20

#define     DbgPin5Up()                 GPIO_PinWrite(GPIO, DbgPin5Port, DbgPin5, 1)
#define     DbgPin5Dn()                 GPIO_PinWrite(GPIO, DbgPin5Port, DbgPin5, 0)
#define     DbgPin6Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin6, 1)
#define     DbgPin6Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin6, 0)

#define     DbgPin7Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin7, 1)
#define     DbgPin7Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin7, 0)
#define     DbgPin8Up()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin8, 1)
#define     DbgPin8Dn()                 GPIO_PinWrite(GPIO, DbgPin6Port, DbgPin8, 0)


#define     LedOn_G()                   GPIO_PinWrite(GPIO, LedGrnPinPort, LedGrnPin, 1)
#define     LedOff_G()                  GPIO_PinWrite(GPIO, LedGrnPinPort, LedGrnPin, 0)
#define     LedOn_R()                   GPIO_PinWrite(GPIO, LedRedPinPort, LedRedPin, 1)
#define     LedOff_R()                  GPIO_PinWrite(GPIO, LedRedPinPort, LedRedPin, 0)
#define     LedOn_B()                   GPIO_PinWrite(GPIO, LedBluPinPort, LedBluPin, 1)
#define     LedOff_B()                  GPIO_PinWrite(GPIO, LedBluPinPort, LedBluPin, 0)

#define     BtnPin1Port                   1
#define     BtnPin2Port                   0
//#define     BtnPin3Port                   2
//#define     BtnPin4Port                   2

#define     BtnPin1                       1
#define     BtnPin2                       10
//#define     BtnPin3                       8
//#define     BtnPin4                       9

#define		RdBtn1Input()					GPIO_PinRead(GPIO, BtnPin1Port, BtnPin1)
#define		RdBtn2Input()					GPIO_PinRead(GPIO, BtnPin2Port, BtnPin2)
//#define		RdBtn3Input()					GPIO_PinRead(GPIO, BtnPin3Port, BtnPin3)
//#define		RdBtn4Input()					GPIO_PinRead(GPIO, BtnPin4Port, BtnPin4)




extern void OpeningBlink(U32 l);

extern void InitDbgPin(void);
extern void delay_us(U32 d);
extern void delay_ms(U32 d);

void DelayMsByReadingCycCnt(int MsToDelay);
void DelayUsByReadingCycCnt(int UsToDelay);

extern void TestGetCycCnt(void);


extern void BOARD_I3C_ReleaseBus(void);

extern int EnableMicBufRingForDbg;
extern int Dmic01RingBuffer[AudioFrameSizeInSamplePerCh*10];


#define TotalBtnNum						2			//could be max 32
#define ButtonDoubleClickInterval       70          //70*10ms=700ms
#define ButtonLongPressInterval         5           //5*(1~4)ms=150ms
#define ButtonLongPressFirstInterval    100         //100*10ms=1000ms

#define BTN_EVT_NONE                            0x00
#define BTN_EVT_SING_PRESS                      0x01
#define BTN_EVT_LONG_PRESS_1                    0x02
#define BTN_EVT_LONG_PRESS_2                    0x03
#define BTN_EVT_LONG_PRESS_RELEASE_1            0x04

typedef struct
{
    uint8_t BtnEvt1;     //when double clicked event not enabled, the main program should check this value for single press event and long press event
    uint8_t BtnEvt2;     //when double clicked event enabled, the main program should check this value for single press event and double press event
    uint8_t BtnDnCnt;
    uint8_t BtnIsDn;
    uint8_t BtnLongPressEvtEnabled;
    uint8_t BtnLongPressType;                           // 1: ���������¼� ��2: ���������¼�
    uint8_t BtnLongPressEvtIsAlreadyGenerated;
    uint8_t DoubleClickEvtEnabled;
    uint8_t DoubleClickEvtCnt;
    uint8_t BtnDnCntLmt;
    uint8_t ButtonLongPressIntervalAmp;
    uint8_t LongPressEventIsLastRelease;
    uint16_t BtnEvtCntSigl;
    uint16_t BtnEvtCntDoub;
    uint16_t BtnEvtCntLong;
    uint32_t LongPressInterval;
    uint32_t GeneralTimerCntPreForLongPress;
} TBtnEvtVarGroup;

extern TBtnEvtVarGroup BtnEvtVarGroup[TotalBtnNum];
extern void GenBtnEvt(void);


extern void ProcessAudio_AfterAudioInputBufIsReady(void);

#endif

