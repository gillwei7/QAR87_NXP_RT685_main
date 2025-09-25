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
#include "fsl_sema42.h"

#include "pin_mux.h"
#include "board_hifi4.h"
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "fsl_inputmux.h"

#include "GlobalDef.h"
#include "SubFunc.h"
#include "AudioProc_Conversa.h"
#include "Sweep.h"
#include "TestDspFunctions.h"

#include "GainingAndMixing.h"
#include "IIR.h"
#include "SRCProc.h"

#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"

extern int NonCacheable_start, NonCacheable_end;
extern int NonCacheable_init_start, NonCacheable_init_end;

T_CommonVarSharedByDspAndMcu *PtrVarBlockSharedByDspAndMcu;

int CycCntHistoryIdx=0;


U32 VComTestDataSpace[200];
U32 AudioFrameCnt=0;
uint8_t domainId;

volatile int VarBlockSharedByDspAndMcuIsSet;

uint8_t APP_GetDSPCoreDomainID(void)
{
    return 3U;
}

U32 MU_U32InfoFromMcu;
void APP_MU_IRQHandler(void)
{
    uint32_t flag = 0;

    flag = MU_GetStatusFlags(APP_MU);

    if ((flag & kMU_Rx0FullFlag) == kMU_Rx0FullFlag)
    {
    	MU_U32InfoFromMcu = MU_ReceiveMsgNonBlocking(APP_MU, CHN_MU_REG_NUM);
    	if(MU_U32InfoFromMcu==EvtFlag_AudioFrameIsReadyToProcess)
    	{

								CycCntA=read_ccount();

					AudioProcessOnEachFrame();

								CycCntB=read_ccount();
								PtrVarBlockSharedByDspAndMcu->U32CycCntHistory[CycCntHistoryIdx]= CycCntB-CycCntA;		//1437169 cycles, 1.43M/4.5M * 600M = 190MIPS
								PtrVarBlockSharedByDspAndMcu->MonitorInfoArray1[3]=CycCntB-CycCntA;
								CycCntHistoryIdx++;
								if(CycCntHistoryIdx>=100)
									CycCntHistoryIdx=0;
			        MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, EvtFlag_DspProcessingIsFiinished);
    	}else
    	{
    		PtrVarBlockSharedByDspAndMcu=(T_CommonVarSharedByDspAndMcu *)MU_U32InfoFromMcu;
    	    SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
    			PRINTF("RT685 DSP: PtrVarBlockSharedByDspAndMcu address is set to: %x\r\n", (U32)PtrVarBlockSharedByDspAndMcu);
    	    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
    	    VarBlockSharedByDspAndMcuIsSet=1;
    	}
    }
}

void XOS_INIT(void)
{
	//xos_set_clock_freq(XOS_CLOCK_FREQ);
    xos_set_clock_freq(CLOCK_GetFreq(kCLOCK_DspCpuClk));
	xos_start_system_timer(-1, 0);
}
void XOS_EnableMuIntr(void)
{
    int32_t	 retStatusXos  = XOS_OK;


    /* DSP interrupt only can be enable after XOS is started. */
    retStatusXos = xos_register_interrupt_handler(APP_MU_IRQn, (XosIntFunc *)APP_MU_IRQHandler, NULL);
    if (retStatusXos != XOS_OK)
    {
		PRINTF("RT685 DSP: register intr fail \r\n");
    }
    retStatusXos = xos_interrupt_enable(APP_MU_IRQn);
    if (retStatusXos != XOS_OK)
    {
		PRINTF("RT685 DSP: enable intr fail \r\n");
    }
	/* Enable transmit and receive interrupt */
	MU_EnableInterrupts(APP_MU, kMU_Rx0FullInterruptEnable);
	//PRINTF("RT685 DSP: intr is configured \r\n");
}
/*
void BOARD_InitClock(void)
{
	// DSP_INT0_SEL18 = DMA1
	INPUTMUX_AttachSignal(INPUTMUX, 18U, kINPUTMUX_Dmac1ToDspInterrupt);
}
*/
#if 0
//from conversa software pack demo
#define XOS_SYSTEM_CLOCK_FREQUENCY 594e6	// Provide the system clock frequency to XOS Todo check if we can collect it dynamically
#define XOS_TIMER_PRIORITY_HIGHER	-1	 	// Highest priority timer
#define XOS_TIMER_MODE_TICKLESS      0	 	// timer tickless mode

status_t initXos(void)
{
    status_t retStatus     = kStatus_Success;
    //status_t retStatusFunc = kStatus_Success;
    int32_t	 retStatusXos  = XOS_OK;

    /*
     *  Provide the system clock frequency to XOS
     */
	xos_set_clock_freq(XOS_SYSTEM_CLOCK_FREQUENCY); 	// TODO	use dynamic method xos_set_clock_freq(CLOCK_GetFreq(kCLOCK_DspCpuClk));

	/*
	 * Set the xos system timer
	 */

	retStatusXos = xos_start_system_timer( XOS_TIMER_PRIORITY_HIGHER,				// select the timer to use
										   XOS_TIMER_MODE_TICKLESS );				// select the mode (tickless or number of clock between ticks)*/
    if (retStatusXos != XOS_OK)
    {
    	retStatus = kStatus_Fail;
    }

    return retStatus;
}

//status_t initXos_interrupt(AUDIO_workFlowHandle_st* p_audioWorkFlow_handle)
status_t initXos_interrupt(void)
{
    status_t retStatus     = kStatus_Success;
    //status_t retStatusFunc = kStatus_Success;
    int32_t	 retStatusXos  = XOS_OK;

    /*
     *  Enable Message Unit interrupt
     */

    /* Set interrupt handler for MU
     * 	 Note: DSP interrupt only can be enable after XOS is started. */
    retStatusXos = xos_register_interrupt_handler( APP_MU_IRQn,						// interrupt number is Message Unit
    											   (XosIntFunc *)APP_MU_IRQHandler,    // IRQ handler function
												   //(void*) p_audioWorkFlow_handle
												   NULL
												   );	// parameters sent to the IRQ handler function
    if (retStatusXos != XOS_OK)
    {
    	retStatus = kStatus_Fail;
    }

    /*
     *  Enable Message Unit interrupt
     */
    retStatusXos = xos_interrupt_enable ( APP_MU_IRQn );

    if (retStatusXos != XOS_OK)
    {
    	retStatus = kStatus_Fail;
    }

	/* Enable core communication RX0 Interrupt line.
	     Note: Rx0 message receive is now handled by MU interrupt handler */
#ifdef DEBUG_L1
	PRINTF("DSP init: Enable interrupt for message unit Rx0\r\n");
#endif

	MU_EnableInterrupts( MUB,									// Message Unit base address
						 kMU_Rx0FullInterruptEnable );			// MU interrupt source is Rx0 full

    return retStatus;
}
#endif
volatile void *HeapBegPtr;
volatile void *StackEndPtr;

#include "CircularBufManagement.h"

extern XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.
extern XosMutex 		  g_audio_vitBufferMutex;										// VIT buffer mutex for accessing VIT buffer on Audio and VIT task
extern T_CircularAudioBuf_S16  VitCircBuff;
extern T_CircularAudioBuf_S16  VitCircBuff_RawMic;
extern VIT_DetectionStatus_en 	g_vitDetectionResult;
extern PL_UINT16 				g_vitVcDetectionId;
extern PL_UINT32 g_vitFramecount;

extern status_t swProcessVIT( AUDIO_vit_st* 	 p_definitionVIT,
					   PL_INT16*     			 p_inputAudioData,
					   PL_INT16*     			 p_inputAudioData_RawMic,
					   PL_INT32	     			 inputAudioDataSize_sample,
					   VIT_DetectionStatus_en* 	 p_vitDetectionResult,
					   PL_UINT16* 	 			 p_vitVcDetectionId);

int main(void)
{

	//only when NOT starting XOS, call this following line. If to really start XOS by xos_start(0), MUST NOT call xos_start_main
	xos_start_main("main", 7, 0);		//xos_start_main() Init XOS and convert main() into a thread.
	XOS_INIT();

	/* Disable DSP cache for noncacheable sections. */
	//xthal_set_region_attribute((uint32_t *)&NonCacheable_start,     _control_start                   - (uint32_t)&NonCacheable_start,      XCHAL_CA_BYPASS, 0);
	//xthal_set_region_attribute((uint32_t *)&NonCacheable_init_start,(uint32_t)&NonCacheable_init_end - (uint32_t)&NonCacheable_init_start, XCHAL_CA_BYPASS, 0);
	xthal_set_region_attribute((uint32_t *)0x20040000,0x40000, XCHAL_CA_BYPASS, 0);

    /* Init board hardware. */
    CLOCK_SetXtalFreq(24000000U);                    /* Sets external XTAL OSC freq */
    INPUTMUX_Init(INPUTMUX);
	BOARD_InitDebugConsole();	//already inited in MCU
	//BOARD_InitClock();			//this function cannot be removed --- !

    INPUTMUX_AttachSignal(INPUTMUX, 1U, kINPUTMUX_MuBToDspInterrupt);

	#if 0
		InitGainAndMixing();
		InitIirFilters();
		//set to 48KHz FsIn as starting --- only 48KHz local I2S Fs is considered in this demo
		//                                          Amplitude(),      FreqBeg,   FreqEnd,  Sweeping Duration,  Fs
		//                                        in between +/- 1      Hz          Hz            Sec          Hz
		InitSineToneGenerator(&SineToneGenerator1,     0.99f,          0.01f,     24000.0f,      20.0f,        48000);
		InitSineToneGenerator(&SineToneGenerator2,     0.10f,          1000.0f,    1000.0f,      20.0f,        48000);
	#endif

	Init_AudioProcessOnEachFrame();

	BlinkDbgPinNTime(8,8);
	BlinkDbgPinNTime(7,7);
	BlinkDbgPinNTime(6,6);
	BlinkDbgPinNTime(5,5);

	VarBlockSharedByDspAndMcuIsSet=0;

	/* MUB init */
	MU_Init(APP_MU);

    /* SEMA42 init */
    SEMA42_Init(APP_SEMA42);
    domainId = APP_GetDSPCoreDomainID();

    SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
    	PRINTF("RT685 DSP: Started -----IW611 BT HFP with Conversa------- \r\n");
		PRINTF("RT685 DSP: started ----------- DspVer 0.1.3 ------------- \r\n");
    	PRINTF("RT685 DSP: Started -----IW611 BT HFP with Conversa------- \r\n");
    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

	InitVit();

	/* Send flag to CM33 core to indicate DSP Core has started */
	MU_SetFlags(APP_MU, BOOT_FLAG);


	MU_U32InfoFromMcu = (U32) MU_ReceiveMsg( MUB, CHN_MU_REG_NUM);									// wait until message is present in the Message Unit on channel DSP to MCU for address of g_appHandle
	PtrVarBlockSharedByDspAndMcu=(T_CommonVarSharedByDspAndMcu *)MU_U32InfoFromMcu;

	XOS_EnableMuIntr();

	//while(!VarBlockSharedByDspAndMcuIsSet) {};

	if(PtrVarBlockSharedByDspAndMcu->U32ControlPara[10]==0x1234abcd)
	{
	    SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
		PRINTF("RT685 DSP: PtrVarBlockSharedByDspAndMcu init value matches \r\n");
	    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
	}else
	{
	    SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
		PRINTF("RT685 DSP: PtrVarBlockSharedByDspAndMcu init value DOESNT match!!! \r\n");
	    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
	}

	HeapBegPtr=GetCurrentHeapTail(32);
	StackEndPtr=GetCurrentStackHead();


	//init SRC
    SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	//             (ptr to handle.             int InputBlockSizeInSamples,  int inFs, int outFs, int ChNum,          alloc_memory ptr array,           ptr numbers,              int NeedToDisplay)
	InitCadenceAsrc(&SRC_process_Ref_handle,   AudioFrameSizeInSamplePerCh/2,   8000,     16000,      1,       (void **)&g_pv_arr_alloc_memory_Ref,   &g_w_malloc_count_Ref,                0     );
	InitCadenceAsrc(&SRC_process_TxOut_handle, AudioFrameSizeInSamplePerCh  ,  16000,      8000,      1,       (void **)&g_pv_arr_alloc_memory_TxOut, &g_w_malloc_count_TxOut,              0     );
    SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);


	void *HeapPtr1;
	HeapPtr1=GetCurrentHeapTail(32);

	//SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	//PRINTF("RT685 DSP: heap base address, %x\r\n", (U32)HeapPtr1);
	//SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
	//InitVit();

	HeapPtr1=GetCurrentHeapTail(32);
	SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	PRINTF("RT685 DSP: heap base address, %x\r\n", (U32)HeapPtr1);
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

	InitConversa();

	HeapPtr1=GetCurrentHeapTail(32);
	SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
	PRINTF("RT685 DSP: heap base address, %x\r\n", (U32)HeapPtr1);
	SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);

#if 0
	//ConfigAndStartVitTask();
	while(1)
	{
		delay_ms(1000);
	};
#else

	PL_INT32 retStatusXos  = XOS_OK;
	status_t retStatus     = kStatus_Success;
	VIT_ReturnStatus_en VIT_Status;

	/* Create audio task semaphore */
	retStatusXos = xos_sem_create(	&g_audioTask_audioVitProcessSemaphore,  	// pointer to audio vit task semaphore
									XOS_SEM_WAIT_PRIORITY,						// Wake waiters in priority order
									0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create audio VIT task semaphore (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	else
	{
	}

	/* Create VIT bufer access mutex */
	retStatusXos =xos_mutex_create( &g_audio_vitBufferMutex,							// pointer to vit buffer mutex read/write for access
									XOS_MUTEX_WAIT_FIFO,
									0);

	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create audio VIT mutex (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	else
	{
	}

	status_t retStatusFunc = kStatus_Success;
	AUDIO_vit_st*       p_swIpVIT_handle    	 = &vitPluginParams;
	PL_INT16*           p_conversaToVitBuff_16b  = NULL;
	PL_INT16*           p_conversaToVitBuff_16b_RawMic  = NULL;


	PRINTF("\n*************************************\r\n");
	PRINTF("[DSP audioVITProcessTask] is running...\r\n");

	while (retStatusXos == XOS_OK)
	{
		retStatusXos = xos_sem_tryget( &g_audioTask_audioVitProcessSemaphore );    // try to decrement the audio task main semaphore counter and return error when semaphore count = 0
	}
	PRINTF("*************************************\r\n\n");


	while(1)
	{

		/*
		 *  WAIT MCU ASK DSP TO RUN AN AUDIO PROCESS
		 *
		 *      - wait until g_audioTask_mainSemaphore > 0
		 */
		//retStatusXos = xos_sem_get( &g_audioTask_audioVitProcessSemaphore );   	// Wait until the audio vit task semaphore can be decremented
		retStatusXos = xos_sem_tryget( &g_audioTask_audioVitProcessSemaphore );   	// Wait until the audio vit task semaphore can be decremented

		if ( retStatusXos != XOS_OK )
		{
			//PRINTF("FAIL - [DSP audioProcessTask]: Semaphore mechanism (error = %d)\r\n",retStatusXos);
			retStatus = kStatus_Fail;
		}else
		{
			//xos_mutex_lock(&g_audio_vitBufferMutex);
			p_conversaToVitBuff_16b = CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff, p_swIpVIT_handle->vitConfig.framesize);
			p_conversaToVitBuff_16b_RawMic= CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff_RawMic, p_swIpVIT_handle->vitConfig.framesize);

			//xos_mutex_unlock(&g_audio_vitBufferMutex);

			/********************************************************************
			 * VIT PROCESS
			 *		- Copy current samples from source
			 *		- Check if there are enogugh samples to run VIT
			 *		- Run VIT process
			 *		- Send message to MCU to indicate VIT process finished
			 *******************************************************************/
			DbgPin8Up();
			VIT_Status = swProcessVIT( p_swIpVIT_handle,
									   p_conversaToVitBuff_16b,
									   p_conversaToVitBuff_16b_RawMic,
									   p_swIpVIT_handle->vitConfig.framesize,
									   &g_vitDetectionResult,
									   &g_vitVcDetectionId
									 );

			DbgPin8Dn();

				if (VIT_Status != VIT_SUCCESS)
				{
					PRINTF("  VIT_Process error : %d\n", VIT_Status);
					retStatusFunc = kStatus_Fail;
				}

			#if 0
				/* handle return error code */
				/*
				if (retStatusFunc != kStatus_Success)
				{
					PRINTF("[AUDIO_vitTask] FAIL: VIT process return error code = %d\r\n", retStatusFunc);
					retStatus = retStatusFunc;
				}
			 	*/

				if(!(g_vitFramecount%20))
				{
					SEMA42_Lock(APP_SEMA42, SEMA42_GATE, domainId);
						PRINTF("RT685 DSP: active \r\n");
					SEMA42_Unlock(APP_SEMA42, SEMA42_GATE);
				}
			#endif
		}
	}

#endif
}





