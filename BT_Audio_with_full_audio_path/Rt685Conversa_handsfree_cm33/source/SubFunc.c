/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdio.h>
#include <stdlib.h>

#include "arm_math.h"
#include "pin_mux.h"
#include "fsl_device_registers.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "fsl_iopctl.h"
#include "GlobalDef.h"

#include "SubFunc.h"

void OpeningBlink(U32 l)
{
	int d;
	d=l;
	while(d--)
	{
		LedOn_R();
		delay_ms(20);
		LedOff_R();
		delay_ms(20);
	}
	d=l;
	while(d--)
	{
		LedOn_G();
		delay_ms(20);
		LedOff_G();
		delay_ms(20);
	}
	d=l;
	while(d--)
	{
		LedOn_B();
		delay_ms(20);
		LedOff_B();
		delay_ms(20);
	}
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
		delay_ms(1);
		switch(WhichPin)
		{
			case 5: DbgPin5Dn(); break;
			case 6: DbgPin6Dn(); break;
			case 7: DbgPin7Dn(); break;
			case 8: DbgPin8Dn(); break;
		}
		delay_ms(1);
	}

}

gpio_pin_config_t GPIO_Input_config =
{
	kGPIO_DigitalInput,
    0,
};
gpio_pin_config_t GPIO_Output_config =
{
    kGPIO_DigitalOutput,
    1,
};

void InitDbgPin(void)
{
	GPIO_PortInit(GPIO, 0);
	GPIO_PortInit(GPIO, 1);

    GPIO_PinInit(GPIO, LedBluPinPort, LedBluPin, &GPIO_Output_config);
    GPIO_PinInit(GPIO, LedRedPinPort, LedRedPin, &GPIO_Output_config);
    GPIO_PinInit(GPIO, LedGrnPinPort, LedGrnPin, &GPIO_Output_config);

    GPIO_PinInit(GPIO, DbgPin5Port, DbgPin5, &GPIO_Output_config);
    GPIO_PinInit(GPIO, DbgPin6Port, DbgPin6, &GPIO_Output_config);
    GPIO_PinInit(GPIO, DbgPin7Port, DbgPin7, &GPIO_Output_config);
    GPIO_PinInit(GPIO, DbgPin8Port, DbgPin8, &GPIO_Output_config);

    //GPIO_PinInit(GPIO, BtnPin1Port, BtnPin1, &GPIO_Input_config);
    //GPIO_PinInit(GPIO, BtnPin2Port, BtnPin2, &GPIO_Input_config);
    //GPIO_PinInit(GPIO, BtnPin3Port, BtnPin3, &GPIO_Input_config);
    //GPIO_PinInit(GPIO, BtnPin4Port, BtnPin4, &GPIO_Input_config);

    DbgPin5Dn();
    DbgPin6Dn();
    DbgPin7Dn();
    DbgPin8Dn();

    delay_ms(1);

	//while(1)
	{
		for(int i=0;i<5;i++)
		{
			DbgPin5Up();DelayMsByReadingCycCnt(1);
			DbgPin5Dn();DelayMsByReadingCycCnt(1);
		}
		for(int i=0;i<6;i++)
		{
			DbgPin6Up();DelayMsByReadingCycCnt(1);
			DbgPin6Dn();DelayMsByReadingCycCnt(1);
		}
		for(int i=0;i<7;i++)
		{
			DbgPin7Up();DelayMsByReadingCycCnt(1);
			DbgPin7Dn();DelayMsByReadingCycCnt(1);
		}
		for(int i=0;i<8;i++)
		{
			DbgPin8Up();DelayMsByReadingCycCnt(1);
			DbgPin8Dn();DelayMsByReadingCycCnt(1);
		}
	}
}

void delay_us(U32 d)
{
    volatile uint32_t i = 0;
	for (i = 0; i < 3*d; i++)
	{
		__NOP();
	}
}
void delay_ms(U32 d)
{
    volatile uint32_t i = 0;
    volatile uint32_t j = 0;
    for (j = 0; j < d; j++)
		for (i = 0; i < 30000; i++)
		{
			__NOP();
		}
}

void DelayMsByReadingCycCnt(int MsToDelay)
{
	static volatile unsigned int DelayMsByReadingCycCnt_CurrentCnt;
	unsigned int TargetCycleCnt;
	GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
	TargetCycleCnt=DelayMsByReadingCycCnt_CurrentCnt + MsToDelay*250000;

	if(TargetCycleCnt<DelayMsByReadingCycCnt_CurrentCnt)
	{
		//target value is round back
		while(DelayMsByReadingCycCnt_CurrentCnt>TargetCycleCnt)
			GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
		while(DelayMsByReadingCycCnt_CurrentCnt<TargetCycleCnt)
			GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
	}else
	{
		//target value is NOT round back
		while(DelayMsByReadingCycCnt_CurrentCnt<TargetCycleCnt)
			GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
	}
}
void DelayUsByReadingCycCnt(int UsToDelay)
{
	static volatile unsigned int DelayMsByReadingCycCnt_CurrentCnt;
	unsigned int TargetCycleCnt;
	GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
	TargetCycleCnt=DelayMsByReadingCycCnt_CurrentCnt + UsToDelay*250;

	if(TargetCycleCnt<DelayMsByReadingCycCnt_CurrentCnt)
	{
		//target value is round back
		while(DelayMsByReadingCycCnt_CurrentCnt>TargetCycleCnt)
			GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
		while(DelayMsByReadingCycCnt_CurrentCnt<TargetCycleCnt)
			GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
	}else
	{
		//target value is NOT round back
		while(DelayMsByReadingCycCnt_CurrentCnt<TargetCycleCnt)
			GET_CYCLE_COUNTER(DelayMsByReadingCycCnt_CurrentCnt);
	}
}

void TestGetCycCnt(void)
{
	volatile U32 a,b;
	GET_CYCLE_COUNTER(a);
	delay_ms(1);
	GET_CYCLE_COUNTER(b);
	b-=a;
	if((b<500000)&&(b>200000))
	{
		PRINTF("RT685 MCU: cycle cnt reading is good, %d.\r\n", b);
	}else
	{
		PRINTF("RT685 MCU: cycle cnt reading is NOT good, %d.\r\n", b);
	}
}

uint32_t RawBtnStatePre;
uint32_t FilteredBtnStatePre;
uint32_t RawBtnState;
uint32_t FilteredBtnState;
TBtnEvtVarGroup BtnEvtVarGroup[TotalBtnNum];

void InitBtnEvt(void)
{
	BtnEvtVarGroup[0].BtnLongPressEvtEnabled=1;
	BtnEvtVarGroup[0].BtnLongPressType=2;
	BtnEvtVarGroup[1].BtnLongPressEvtEnabled=1;
	BtnEvtVarGroup[1].BtnLongPressType=2;
	FilteredBtnStatePre=0xffff;
}
void GenBtnEvt(void)
{
    uint8_t i;
    TBtnEvtVarGroup *TmpPtr1;
    RawBtnState = ((RdBtn1Input()<<0)|(RdBtn2Input()<<1));
    //RawBtnState = ~RawBtnState;

    // Y = ((~A)&(B&C))|(A&(B|C))
    FilteredBtnState = ((~FilteredBtnStatePre)&(RawBtnStatePre&RawBtnState))|(FilteredBtnStatePre&(RawBtnStatePre|RawBtnState));
    RawBtnStatePre = RawBtnState;

    TmpPtr1=BtnEvtVarGroup;
    for(i=0;i<TotalBtnNum;i++)
    {
	    //if((i==24)||(i==25)) continue;	//these 2 are encoder input pin A,B
        if((~FilteredBtnState)&(0x01<<i))
        {   //now BTN is down, and only need to do processing for long press event
            TmpPtr1->BtnIsDn=1;
            if(TmpPtr1->BtnLongPressEvtEnabled)
            {   //long press is enabled
                if(FilteredBtnStatePre&(0x01<<i))
                {   //it is first into btn down period
                    TmpPtr1->BtnDnCntLmt=ButtonLongPressFirstInterval;
                    TmpPtr1->BtnDnCnt = 0;
                    TmpPtr1->BtnLongPressEvtIsAlreadyGenerated = 0;
                    TmpPtr1->BtnEvt1 = BTN_EVT_NONE;
                    TmpPtr1->BtnEvt2 = BTN_EVT_NONE;
                    TmpPtr1->BtnEvtCntLong=0;
                };

                TmpPtr1->BtnDnCnt++;
                if(TmpPtr1->BtnLongPressType == 1)
                {
                    if(TmpPtr1->BtnDnCnt >= TmpPtr1->BtnDnCntLmt)
                    {
                        TmpPtr1->BtnEvt1 = BTN_EVT_LONG_PRESS_1;               //button long press event
                        TmpPtr1->LongPressEventIsLastRelease=0;
                        TmpPtr1->BtnDnCntLmt = ButtonLongPressInterval*TmpPtr1->ButtonLongPressIntervalAmp;
                        TmpPtr1->BtnDnCnt = 0;

                        //following is for speeding up Long press event 1 generating
                        if(TmpPtr1->BtnEvtCntLong<5)
                        {
                            TmpPtr1->BtnEvtCntLong++;
                            TmpPtr1->ButtonLongPressIntervalAmp=4;
                        }else
                        {
                            if(TmpPtr1->BtnEvtCntLong<(5+10))
                            {
                                TmpPtr1->BtnEvtCntLong++;
                                TmpPtr1->ButtonLongPressIntervalAmp=2;
                            }else
                            {
                                TmpPtr1->ButtonLongPressIntervalAmp=1;
                            };
                        };

                        //the following 2 lines are for double click checking, record the time stamp for the generated event
                        //TmpPtr1->LongPressInterval=GeneralTimerCnt-TmpPtr1->GeneralTimerCntPreForLongPress;
                        //TmpPtr1->GeneralTimerCntPreForLongPress=GeneralTimerCnt_LowLevel;
                    };
                }
                else if(TmpPtr1->BtnLongPressType == 2)
                {
                    if(!(TmpPtr1->BtnLongPressEvtIsAlreadyGenerated))
                    {
                        if(TmpPtr1->BtnDnCnt >= TmpPtr1->BtnDnCntLmt)
                        {
                            TmpPtr1->BtnEvt1 = BTN_EVT_LONG_PRESS_2;
                            TmpPtr1->BtnLongPressEvtIsAlreadyGenerated = 1;
                        };
                    }else
                        TmpPtr1->BtnDnCnt = 0;
                };
            }else
            {   //long press not enabled
                if(FilteredBtnStatePre&(0x01<<i))
                {   //now BTN is first pressed down
                        TmpPtr1->BtnEvt1=BTN_EVT_SING_PRESS;
                };
            };
        }
        else
        {   //now BTN is UP
            TmpPtr1->LongPressEventIsLastRelease=1;
            TmpPtr1->BtnIsDn=0;
            if(TmpPtr1->BtnLongPressEvtEnabled)
                if((~FilteredBtnStatePre)&(0x01<<i))
                {   //long press is enabled, and now button is first released, no matter it is now in long press type 1 or 2,
                    if(!(TmpPtr1->BtnLongPressEvtIsAlreadyGenerated))
                    {
                        TmpPtr1->BtnEvt1=BTN_EVT_SING_PRESS;
                    }
                    //else
                    //{
                    //    TmpPtr1->BtnEvt1=BTN_EVT_LONG_PRESS_1;
                    //}
                };
        }

        TmpPtr1++;
    }
    FilteredBtnStatePre=FilteredBtnState;
}





#if 0	//folding --- release I3C pins


/*! @name PIO2_30 (coord P16), J18[2]/U8[3]/U17[B6]/SDA_CODEC
  @{ */
/* Routed pin properties */
/*!
 * @brief Peripheral name */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL GPIO
/*!
 * @brief Signal name */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_SIGNAL PIO2
/*!
 * @brief Signal channel */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_CHANNEL 30

/* Symbols to be used with GPIO driver */
/*!
 * @brief GPIO peripheral base pointer */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_GPIO GPIO
/*!
 * @brief GPIO pin mask */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_GPIO_PIN_MASK (1U << 30U)
/*!
 * @brief PORT peripheral base pointer */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT 2U
/*!
 * @brief PORT pin number */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN 30U
/*!
 * @brief PORT pin mask */
#define BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN_MASK (1U << 30U)
/* @} */

/*! @name PIO2_29 (coord N17), J18[1]/U8[2]/U17[A6]/SCL_CODEC
  @{ */
/* Routed pin properties */
/*!
 * @brief Peripheral name */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL GPIO
/*!
 * @brief Signal name */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_SIGNAL PIO2
/*!
 * @brief Signal channel */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_CHANNEL 29

/* Symbols to be used with GPIO driver */
/*!
 * @brief GPIO peripheral base pointer */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_GPIO GPIO
/*!
 * @brief GPIO pin mask */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_GPIO_PIN_MASK (1U << 29U)
/*!
 * @brief PORT peripheral base pointer */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT 2U
/*!
 * @brief PORT pin number */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN 29U
/*!
 * @brief PORT pin mask */
#define BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN_MASK (1U << 29U)
/* @} */

#define IOPCTL_PIO_SLEW_RATE_SLOW 0x80u /*!<@brief Slow mode */


void BOARD_InitI3CPinsAsGPIO(void)
{

    /* Enables the clock for the GPIO2 module */
    CLOCK_EnableClock(kCLOCK_HsGpio2);

    gpio_pin_config_t I3C0_SCL_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };
    /* Initialize GPIO functionality on pin PIO2_29 (pin N17)  */
    GPIO_PinInit(BOARD_INITI3CPINSASGPIO_I3C0_SCL_GPIO, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, &I3C0_SCL_config);

    gpio_pin_config_t I3C0_SDA_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };
    /* Initialize GPIO functionality on pin PIO2_30 (pin P16)  */
    GPIO_PinInit(BOARD_INITI3CPINSASGPIO_I3C0_SDA_GPIO, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, &I3C0_SDA_config);

    const uint32_t I3C0_SCL = (/* Pin is configured as PIO2_29 */
                               IOPCTL_PIO_FUNC0 |
                               /* Enable pull-up / pull-down function */
                               IOPCTL_PIO_PUPD_EN |
                               /* Enable pull-up function */
                               IOPCTL_PIO_PULLUP_EN |
                               /* Enables input buffer function */
                               IOPCTL_PIO_INBUF_EN |
                               /* Slow mode */
                               IOPCTL_PIO_SLEW_RATE_SLOW |
                               /* Normal drive */
                               IOPCTL_PIO_FULLDRIVE_DI |
                               /* Analog mux is disabled */
                               IOPCTL_PIO_ANAMUX_DI |
                               /* Pseudo Output Drain is disabled */
                               IOPCTL_PIO_PSEDRAIN_DI |
                               /* Input function is not inverted */
                               IOPCTL_PIO_INV_DI);
    /* PORT2 PIN29 (coords: N17) is configured as PIO2_29 */
    IOPCTL_PinMuxSet(IOPCTL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, I3C0_SCL);

    const uint32_t I3C0_SDA = (/* Pin is configured as PIO2_30 */
                               IOPCTL_PIO_FUNC0 |
                               /* Enable pull-up / pull-down function */
                               IOPCTL_PIO_PUPD_EN |
                               /* Enable pull-up function */
                               IOPCTL_PIO_PULLUP_EN |
                               /* Enables input buffer function */
                               IOPCTL_PIO_INBUF_EN |
                               /* Slow mode */
                               IOPCTL_PIO_SLEW_RATE_SLOW |
                               /* Normal drive */
                               IOPCTL_PIO_FULLDRIVE_DI |
                               /* Analog mux is disabled */
                               IOPCTL_PIO_ANAMUX_DI |
                               /* Pseudo Output Drain is disabled */
                               IOPCTL_PIO_PSEDRAIN_DI |
                               /* Input function is not inverted */
                               IOPCTL_PIO_INV_DI);
    /* PORT2 PIN30 (coords: P16) is configured as PIO2_30 */
    IOPCTL_PinMuxSet(IOPCTL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, I3C0_SDA);
}

static void i2c_release_bus_delay(void)
{
    uint32_t i = 0;
    for (i = 0; i < 100; i++)
    {
        __NOP();
    }
}

void BOARD_I3C_ReleaseBus(void)
{
    uint8_t i = 0;

    GPIO_PortInit(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT);
    GPIO_PortInit(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT);

    BOARD_InitI3CPinsAsGPIO();

    /* Drive SDA low first to simulate a start */
    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 0U);
    i2c_release_bus_delay();

    /* Send 9 pulses on SCL */
    for (i = 0; i < 9; i++)
    {
        GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                      BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 0U);
        i2c_release_bus_delay();

        GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                      BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 1U);
        i2c_release_bus_delay();

        GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                      BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 1U);
        i2c_release_bus_delay();
        i2c_release_bus_delay();
    }

    /* Send stop */
    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SCL_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SCL_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SCL_PIN, 1U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_INITI3CPINSASGPIO_I3C0_SDA_PERIPHERAL, BOARD_INITI3CPINSASGPIO_I3C0_SDA_PORT,
                  BOARD_INITI3CPINSASGPIO_I3C0_SDA_PIN, 1U);
    i2c_release_bus_delay();
}

#endif




#if 0
CRC_Type *CRC_base = CRC_ENGINE;
void InitCrc32(CRC_Type *base, uint32_t seed)
{
    crc_config_t config;

    config.polynomial    = kCRC_Polynomial_CRC_32;
    config.reverseIn     = false;
    config.complementIn  = false;
    config.reverseOut    = false;
    config.complementOut = false;	//note --- these true false settings are all set to false, different from the driver demo program
    config.seed          = seed;

    CRC_Init(base, &config);
}

void ResetCrc32Seed(void)
{
	CRC_base->SEED = 0xffffffff;
}

void Init_CrcEngine(void)
{
	//U32 checksum32;

	CRC_base = (CRC_Type *)CRC_ENGINE;
    InitCrc32(CRC_base, 0xFFFFFFFFU);
    //CRC_WriteData(CRC_base, 0, 10);
    //checksum32 = CRC_Get32bitResult(CRC_base);
}

#endif





#if 0

48KHz: effective spectrum: 0~24KHz   ---> 8~24KHz will be noise (spectrum aliasing) of 16KHz
16KHz: effective spectrum: 0~8KHz

each frame, music 48 samples (1ms)   --> 16 samples

step1: LPF filter --- remove  8~16KHz  (8KHz low pass)
	FIR filter

setp2: 1/3 decimation
x1,x2,x3,x4,x5,x6,x7,.... ,x45,x46,x47
x1,      x4,      x7,      x45



void convert48To16_L(float *DstPtr, float *SrcPtr, int Length)		//Length is always 48
{

}
void convert48To16_R(float *DstPtr, float *SrcPtr, int Length)		//Length is always 48
{

}


#endif



#define FirCoefLength	64
#define FirBlockSize	48
//this is 1/3 FIR LPF
const float B[FirCoefLength] = {
		  0.0003175469796565,0.0006955264527167,0.0005521719831693,-0.001097925697437,
		  -0.004747652678265,  -0.0094239017937, -0.01253477819377, -0.01125116297883,
		  -0.004878921872828, 0.003624288733652, 0.008820569254099, 0.006604939508406,
		  -0.001951919070258,  -0.0102657680642, -0.01074858630556,-0.001354638233184,
		    0.01133430783888,  0.01620443297627, 0.006860978190084, -0.01128773506176,
		   -0.02320430362328, -0.01588336604844, 0.008939285416451,  0.03242022281066,
		    0.03130769275422,-0.001852903841553, -0.04676570260408, -0.06370481877455,
		   -0.01964239937978,   0.0850765718126,   0.2098032994481,    0.294157546337,
		      0.294157546337,   0.2098032994481,   0.0850765718126, -0.01964239937978,
		   -0.06370481877455, -0.04676570260408,-0.001852903841553,  0.03130769275422,
		    0.03242022281066, 0.008939285416451, -0.01588336604844, -0.02320430362328,
		   -0.01128773506176, 0.006860978190084,  0.01620443297627,  0.01133430783888,
		  -0.001354638233184, -0.01074858630556,  -0.0102657680642,-0.001951919070258,
		   0.006604939508406, 0.008820569254099, 0.003624288733652,-0.004878921872828,
		   -0.01125116297883, -0.01253477819377,  -0.0094239017937,-0.004747652678265,
		  -0.001097925697437,0.0005521719831693,0.0006955264527167,0.0003175469796565
};


arm_fir_decimate_instance_f32 FirFilter_48KTo16K_PostLPF_Ch1;
arm_fir_decimate_instance_f32 FirFilter_48KTo16K_PostLPF_Ch2;

float HistState_FirFilter_48KTo16K_PostLPF_Ch1[FirCoefLength+FirBlockSize-1];		//pState is of length numTaps+blockSize-1
float HistState_FirFilter_48KTo16K_PostLPF_Ch2[FirCoefLength+FirBlockSize-1];		//pState is of length numTaps+blockSize-1

void InitFirFilter(void)
{
	arm_fir_decimate_init_f32	(
			&FirFilter_48KTo16K_PostLPF_Ch1,
			FirCoefLength,
			3,
			B,
			HistState_FirFilter_48KTo16K_PostLPF_Ch1,
			FirBlockSize
	);
	arm_fir_decimate_init_f32	(
			&FirFilter_48KTo16K_PostLPF_Ch2,
			FirCoefLength,
			3,
			B,
			HistState_FirFilter_48KTo16K_PostLPF_Ch2,
			FirBlockSize
	);

}

void Convert48KTo16K(arm_fir_decimate_instance_f32 *FirFlt, float *DstPtr, float *SrcPtr, int LengthOfOutput)
{
    arm_fir_decimate_f32	(
            FirFlt,
            SrcPtr,
            DstPtr,
            FirBlockSize
    );
}



void Test_Convert48KTo16K(void)
{
	short int InputBuffer_L[48];
	short int OutputBuffer_L[16];
	short int InputBuffer_R[48];
	short int OutputBuffer_R[16];

	float InBuf_Flt[48];
	float OutBuf_Flt[16];


	InitFirFilter();


	//convert 48k to 16k for ch1 (left)
	arm_q15_to_float(InputBuffer_L, InBuf_Flt, 48);			//convert source data to float
	Convert48KTo16K(&FirFilter_48KTo16K_PostLPF_Ch1, OutBuf_Flt, InBuf_Flt, 16);	//convert 48 float samples to 16 float samples
 	arm_float_to_q15(OutBuf_Flt, OutputBuffer_L, 16);		//convert result data back to short int


	//convert 48k to 16k for ch2 (right)
	arm_q15_to_float(InputBuffer_R, InBuf_Flt, 48);			//convert source data to float
	Convert48KTo16K(&FirFilter_48KTo16K_PostLPF_Ch2, OutBuf_Flt, InBuf_Flt, 16);	//convert 48 float samples to 16 float samples
 	arm_float_to_q15(OutBuf_Flt, OutputBuffer_R, 16);		//convert result data back to short int
}




