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
#include "CircularBufManagement.h"

#include "SubFunc.h"
#include "AudioProc_Conversa.h"
#include "Sweep.h"
#include "TestDspFunctions.h"

#include "GainingAndMixing.h"
#include "IIR.h"
#include "SRCProc.h"

#include "AudioProc_Conversa.h"
#include "AudioProc_Vit.h"
#include "AudioProc_Eap.h"
#include "MeterAndCompressor.h"
#include "DspMainAudioFlow.h"
#include "AudioDecoder.h"

extern int NonCacheable_start, NonCacheable_end;
extern int NonCacheable_init_start, NonCacheable_init_end;
extern XosSem 	 		  g_audioTask_audioVitProcessSemaphore;   						// Audio VIT task semaphore used to control the DSP audio process start/wait state.

XosEvent MuEventFromMcu;
XosEvent MuEventFromFrameProcTask;


volatile T_CommonVarSharedByDspAndMcu *PtrVarBlockSharedByDspAndMcu=NULL;

int CycCntHistoryIdx=0;

int ToTempSkipVitPorcess=0;
U32 VComTestDataSpace[200];
U32 AudioFrameCnt=0;
uint8_t domainId;
ConversaTuningCfg_t CurrentConversaTuningCfg;
EapTuningCfg_t      CurrentEAPTuningCfg;

uint8_t APP_GetDSPCoreDomainID(void)
{
    return 3U;
}



void AudioFrameProcess(U32 Mode)
{
//	DbgPin6Up();

	//get MIPS data
	CycCntA=read_ccount();

	//process audio according to different use cases
	int Opt=0;

	switch(Mode)
	{
		//may need to get option word from PtrVarBlockSharedByDspAndMcu->U32ControlPara
		case MuEvtMcuToDsp_AudioFrmIsReady_HfpCall:
			DspMainAudioFlowProcOneFrame_HfpCall(MuEvtMcuToDsp_AudioFrmIsReady_HfpCall);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy:
			DspMainAudioFlowProcOneFrame_HfpCall(MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg:
			DspMainAudioFlowProcOneFrame_AudioIoDbg(Opt);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_VideoRecording:
			DspMainAudioFlowProcOneFrame_VideoRecording(Opt);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_MediaPlayer:
			DspMainAudioFlowProcOneFrame_MediaPlayer(Opt);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer:
			DspMainAudioFlowProcOneFrame_MusicPlayer(Opt);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_Translation:
			DspMainAudioFlowProcOneFrame_Translation(Opt);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_AiConversation:
			DspMainAudioFlowProcOneFrame_AiConversation(Opt);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_VideoAi:
			DspMainAudioFlowProcOneFrame_VideoAi(Opt);
		break;
		default:
		break;
	}

	//get MIPS data
	CycCntB=read_ccount();
	PtrVarBlockSharedByDspAndMcu->MonitorInfoArray1[3]=CycCntB-CycCntA;

	#if 1
		//save the cyc count history
		PtrVarBlockSharedByDspAndMcu->CycCntHistory[CycCntHistoryIdx]= CycCntB-CycCntA;		//1437169 cycles, 1.43M/4.5M * 600M = 190MIPS
	CycCntHistoryIdx++;
		if(CycCntHistoryIdx>=CycCntHistoryLength)
		CycCntHistoryIdx=0;
	#endif

	//send response to MCU
	switch(Mode)
	{
		case MuEvtMcuToDsp_AudioFrmIsReady_HfpCall:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_HfpCall);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_HomeVitStandBy:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_HomeVitStandBy);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_AudioIoDbg:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_AudioIoDbg);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_VideoRecording:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_VideoRecording);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_MediaPlayer:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_MediaPlayer);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_MusicPlayer:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_MusicPlayer);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_Translation:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_Translation);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_AiConversation:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_AiConversation);
		break;
		case MuEvtMcuToDsp_AudioFrmIsReady_VideoAi:
			MU_SendMsgNonBlocking(APP_MU, CHN_MU_REG_NUM, MuEvtDspToMcu_AudioProcIsFinished_VideoAi);
		break;
		default:
		break;
	}

	AudioFrameCnt++;
//	DbgPin6Dn();
}

void APP_MU_IRQHandler(void)
{
    uint32_t flag = 0;
    U32 MU_U32InfoFromMcu;

    flag = MU_GetStatusFlags(APP_MU);
    //DbgPin7Up();
    if ((flag & kMU_Rx0FullFlag) == kMU_Rx0FullFlag)
    {
    	MU_U32InfoFromMcu = MU_ReceiveMsgNonBlocking(APP_MU, CHN_MU_REG_NUM);

    	if(MU_U32InfoFromMcu>0xffff)
    	{
    		PtrVarBlockSharedByDspAndMcu=(T_CommonVarSharedByDspAndMcu *)MU_U32InfoFromMcu;
   			PRINTF_M("RT685 DSP: PtrVarBlockSharedByDspAndMcu address is set to: %x\r\n", (U32)PtrVarBlockSharedByDspAndMcu);
    	}else
    	{
			xos_event_set(&MuEventFromMcu,MU_U32InfoFromMcu);
    	}
    }
    //DbgPin7Dn();
}

void XOS_INIT(void)
{
	//xos_set_clock_freq(XOS_CLOCK_FREQ);
    xos_set_clock_freq(CLOCK_GetFreq(kCLOCK_DspCpuClk));
	xos_start_system_timer(-1, xos_msecs_to_cycles(1));
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

#include "CircularBufManagement.h"


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


#if TestAlgoInitAndDeInit==1
	extern void TestHeap_VitInitAndDeInit(int l);
	extern void TestHeap_EapInitAndDeInit(int l);
	extern void TestHeap_ConversaInitAndDeInit(int l);
	extern void TestHeap_AsrcInitAndDeInit(int l);
	extern void TestHeap_OpusDecodingInitAndDeInit(int l);
	extern void TestHeap_SbcDecodingInitAndDeInit(int l);
#endif

extern int InitOpusDecoder(void);

XosThread thread_VitAndAudioDec;
XosThread thread_AudioFrameProc;

int Task_VitAndAudioDec(void * arg, int32_t wake_value);
int Task_AudioFrameProc(void * arg, int32_t wake_value);

#define THREAD_STACK_SIZE_VitAndAudioDec	(10*1024)
#define THREAD_PRIORITY_VitAndAudioDec	11
U32 ThreadStackMem_VitAndAudioDec[THREAD_STACK_SIZE_VitAndAudioDec];

#define THREAD_STACK_SIZE_AudioFrameProc	(10*1024)
#define THREAD_PRIORITY_AudioFrameProc	12				//higher than VIT
U32 ThreadStackMem_AudioFrameProc[THREAD_STACK_SIZE_AudioFrameProc];

#if TestAlgoInitAndDeInit==1
extern void PrepareMainAudioFlowPointersAndInputFloatData(int NeedToCvtMicToFlt, int NeedToCvtUacDnToFlt, int NeedToCvtAmpI2SInToFlt, int NeedToCvtNvtI2SInToFlt);
extern void ConversaProcessAndFeedToVit(float **PtrArray_MicIn, float *PtrRxIn, float **PtrArray_OutSignals, S16 *RawMicSigForVitRef, int ToByPassConversa);

void DryRunAllAlgo(int l)
{
	PRINTF("RT685 DSP: DryRunAllAlgo start \r\n");
	for(int j=0;j<l;j++)
	{
		PL_FLOAT*  pp_inputAudioData_Tx_FLT[4];
		PL_FLOAT*  pp_OutputAudioSignals   [5];	//Note: PtrArray_OutSignals: from 0 to 4: RxOut, TxOut, AecOut, BfOut, NlpOut
		S16 RawMicSignal16BitForVitRef[AudioFrameSizeInSamplePerCh_16KHz];
		float ConversaRefIn [AudioFrameSizeInSamplePerCh_16KHz];

		PrepareMainAudioFlowPointersAndInputFloatData(1,1,1,1);

		//put in some dummy values
		for(int i=0;i<AudioFrameSizeInSamplePerCh_16KHz;i++)
		{
			SrcPtrFlt_DmicIn0[i]= 0.001*(i+j);
			SrcPtrFlt_DmicIn1[i]= 0.002*(i+j);
			SrcPtrFlt_DmicIn2[i]=-0.001*(i+j);
			SrcPtrFlt_DmicIn3[i]=-0.002*(i+j);
			ConversaRefIn[i]= 0.111*(i%11+j);
			RawMicSignal16BitForVitRef[i]=i*i;
		}

		/*Conversa Mic position must be:
		*	CH0 = nose
		*	CH1 = left
		*	CH2 = right
		*	CH3 = middle of right
		*/
		pp_inputAudioData_Tx_FLT[0]=SrcPtrFlt_DmicIn0;
		pp_inputAudioData_Tx_FLT[1]=SrcPtrFlt_DmicIn2;
		pp_inputAudioData_Tx_FLT[2]=SrcPtrFlt_DmicIn3;
		pp_inputAudioData_Tx_FLT[3]=SrcPtrFlt_DmicIn1;

		//dry run conversa
		ConversaProcessAndFeedToVit(pp_inputAudioData_Tx_FLT, ConversaRefIn, pp_OutputAudioSignals, RawMicSignal16BitForVitRef,0);


		//dry run VIT
		PL_INT16*           p_conversaToVitBuff_16b  = NULL;
		PL_INT16*           p_conversaToVitBuff_16b_RawMic  = NULL;
		AUDIO_vit_st*       p_swIpVIT_handle    	 = &vitPluginParams;
		VIT_ReturnStatus_en VIT_Status;

		if(CirAudioBuf_SpaceOccupiedInSamples_S16(&VitCircBuff) >= AudioFrameSizeInSamplePerCh_16KHz)
		{
			p_conversaToVitBuff_16b = CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff, p_swIpVIT_handle->vitConfig.framesize);
			p_conversaToVitBuff_16b_RawMic= CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff_RawMic, p_swIpVIT_handle->vitConfig.framesize);

			VIT_Status = swProcessVIT( p_swIpVIT_handle,
									   p_conversaToVitBuff_16b,
									   p_conversaToVitBuff_16b_RawMic,
									   p_swIpVIT_handle->vitConfig.framesize,
									   &g_vitDetectionResult,
									   &g_vitVcDetectionId
									 );
			if (VIT_Status != VIT_SUCCESS)
			{
				PRINTF_M("  VIT_Process error : %d\n", VIT_Status);
			};
		}else
		{
			PRINTF_M("  VIT_Process error Empty cir buf\n");
		}


		//dry run EAP
		S16 *EapInPtrs[LVM_MAX_STREAMS*2];	//this is LVM_MAX_STREAMS stereo channels
		S16 *EapOtPtrs[2];					//this is 1 stereo channel

		//input stream[0] for EAP
		EapInPtrs[0]=AudioOneFrameBuf_SbcDecodedL;
		EapInPtrs[1]=AudioOneFrameBuf_SbcDecodedR;
		//input stream[1] for EAP
		EapInPtrs[2]=AudioOneFrameBuf_OpusDecodedL;
		EapInPtrs[3]=AudioOneFrameBuf_OpusDecodedR;
		//input stream[2] for EAP --- zeros
		EapInPtrs[4]=(S16 *)AudioOneFrameBuf_48KHz_AllZeros;
		EapInPtrs[5]=EapInPtrs[4]+AudioFrameSizeInSamplePerCh_48KHz;

		//output stream pointers
		EapOtPtrs[0]=DstPtrS16_I2SAmpL;
		EapOtPtrs[1]=DstPtrS16_I2SAmpR;

		EapProcess(EapOtPtrs, EapInPtrs, AudioFrameSizeInSamplePerCh_48KHz, EapTuningCfg_MusicPlay);
	}
	PRINTF("RT685 DSP: DryRunAllAlgo is successful \r\n");
}
void TestHeap_AllAlgoInitDeInit(int l)
{
	void *HeapPtr1;
	void *HeapPtr2;
	HeapPtr1=GetCurrentHeapTail(3000);
	int TestCnt=l;

	while(1)
	{
		InitConversa(ConversaTuningCfg_HfpVoiceCall, 1);
		delay_ms(100);
		InitEap(EapTuningCfg_MusicPlay, 1);
		delay_ms(100);
		initCreateVIT(&vitPluginParams);
		delay_ms(100);
		DryRunAllAlgo(100);
			DeInitConversa();
			DeInitEap(&eap_1_Handle);
			VIT_Deinit(&vitPluginParams);
				HeapPtr2=GetCurrentHeapTail(3000);
				if(HeapPtr1!=HeapPtr2)
				{
					PRINTF("RT685 DSP: heap base address was, %x\r\n",    (U32)HeapPtr1);
					PRINTF("RT685 DSP: heap base address now is, %x\r\n", (U32)HeapPtr2);
					PRINTF("RT685 DSP: DeInitEap is NOT successful \r\n");
					while(1) {};
				}

		InitConversa(ConversaTuningCfg_NearEnd, 1);
		delay_ms(100);
		InitEap(EapTuningCfg_MusicPlay, 1);
		delay_ms(100);
		initCreateVIT(&vitPluginParams);
		delay_ms(100);
		DryRunAllAlgo(100);
			VIT_Deinit(&vitPluginParams);
			DeInitEap(&eap_1_Handle);
			DeInitConversa();
				HeapPtr2=GetCurrentHeapTail(3000);
				if(HeapPtr1!=HeapPtr2)
				{
					PRINTF("RT685 DSP: heap base address was, %x\r\n",    (U32)HeapPtr1);
					PRINTF("RT685 DSP: heap base address now is, %x\r\n", (U32)HeapPtr2);
					PRINTF("RT685 DSP: DeInitEap is NOT successful \r\n");
					while(1) {};
				}

		InitConversa(ConversaTuningCfg_FarEnd, 1);
		delay_ms(100);
		InitEap(EapTuningCfg_MusicPlay, 1);
		delay_ms(100);
		initCreateVIT(&vitPluginParams);
		delay_ms(100);
		DryRunAllAlgo(100);
			VIT_Deinit(&vitPluginParams);
			DeInitEap(&eap_1_Handle);
			DeInitConversa();
				HeapPtr2=GetCurrentHeapTail(3000);
				if(HeapPtr1!=HeapPtr2)
				{
					PRINTF("RT685 DSP: heap base address was, %x\r\n",    (U32)HeapPtr1);
					PRINTF("RT685 DSP: heap base address now is, %x\r\n", (U32)HeapPtr2);
					PRINTF("RT685 DSP: DeInitEap is NOT successful \r\n");
					while(1) {};
				}

		delay_ms(100);
		TestCnt--;
		if(!TestCnt)
			break;
	}
	PRINTF("RT685 DSP: TestHeap_AllAlgoInitDeInit is successful \r\n");
}
#endif

void CommandToPlayWakeSound(void)
{
	PtrVarBlockSharedByDspAndMcu->NeedToStartPlayOpus=1;
	PtrVarBlockSharedByDspAndMcu->PlayOpusFileIdx=12;		//gill wakesound opus file from 0 change to 12
}
extern int XOS_Test(void);
int main(void)
{

	void *HeapPtr1;
	void *HeapPtr2;
	void *StackPtr;

	//only when NOT starting XOS, call this following line. If to really start XOS by xos_start(0), MUST NOT call xos_start_main
	//xos_start_main("main", 7, 0);		//xos_start_main() Init XOS and convert main() into a thread.

	/* Disable DSP cache for noncacheable sections. */
	//xthal_set_region_attribute((uint32_t *)&NonCacheable_start,     _control_start                   - (uint32_t)&NonCacheable_start,      XCHAL_CA_BYPASS, 0);
	//xthal_set_region_attribute((uint32_t *)&NonCacheable_init_start,(uint32_t)&NonCacheable_init_end - (uint32_t)&NonCacheable_init_start, XCHAL_CA_BYPASS, 0);
	xthal_set_region_attribute((uint32_t *)0x20040000,0x40000, XCHAL_CA_BYPASS, 0);

    /* Init board hardware. */
    CLOCK_SetXtalFreq(24000000U);                    /* Sets external XTAL OSC freq */
    INPUTMUX_Init(INPUTMUX);
	BOARD_InitDebugConsole();	//already inited in MCU
	//BOARD_InitClock();			//this function cannot be removed --- !

	//XOS_Test();
	XOS_INIT();

    INPUTMUX_AttachSignal(INPUTMUX, 1U, kINPUTMUX_MuBToDspInterrupt);

	BlinkDbgPinNTime(8,8);
	BlinkDbgPinNTime(7,7);
	BlinkDbgPinNTime(6,6);
	BlinkDbgPinNTime(5,5);

	/* MUB init */
	MU_Init(APP_MU);

    /* SEMA42 init */
    SEMA42_Init(APP_SEMA42);
    domainId = APP_GetDSPCoreDomainID();

	/* Send flag to CM33 core to indicate DSP Core has started */
	MU_SetFlags(APP_MU, BOOT_FLAG);

	U32 MU_U32InfoFromMcu;
	MU_U32InfoFromMcu = (U32) MU_ReceiveMsg( MUB, CHN_MU_REG_NUM);									// wait until message is present in the Message Unit on channel DSP to MCU for address of g_appHandle
	PtrVarBlockSharedByDspAndMcu=(T_CommonVarSharedByDspAndMcu *)MU_U32InfoFromMcu;

	if(PtrVarBlockSharedByDspAndMcu->Cm33Hifi1HandShakeCheck==0x1234abcd)
	{
		PRINTF_M("RT685 DSP: PtrVarBlockSharedByDspAndMcu init value matches \r\n");
	}else
	{
		PRINTF_M("RT685 DSP: PtrVarBlockSharedByDspAndMcu init value DOESNT match!!! \r\n");
	    while(1){};	//should not come here
	}

	//wait for another MCU event to go on
	MU_U32InfoFromMcu = (U32) MU_ReceiveMsg( MUB, CHN_MU_REG_NUM);									// wait until message is present in the Message Unit on channel DSP to MCU for address of g_appHandle
	//now, means MCU has started the work state management task

	InitDspMainAudioFlow();		//in this fucntion, it clears the print wr/rd idx --- so needs to put here, before the first effective print
	#if 1	//to be closed later
		//from here, print to MCU can work
		PRINTF("RT685 DSP: Started ---IW611 BT HFP A2DP with Conversa---- \r\n");
		PRINTF("RT685 DSP: started ----------- DspVer 1.7.0 ------------- \r\n");
		PRINTF("RT685 DSP: Started ---IW611 BT HFP A2DP with Conversa---- \r\n");
		PRINTF("RT685 DSP: DSP freq: %d Hz\r\n",CLOCK_GetFreq(kCLOCK_DspCpuClk));
	#endif
	MU_SetFlags(APP_MU, BOOT_FLAG_2);	//send BOOT_FLAG_2 MU to MCU to let the work state management task go on

	HeapPtr1=GetCurrentHeapTail(32);
	PRINTF("RT685 DSP: heap base address, %x\r\n", (U32)HeapPtr1);


	InitGainAndMixing();
	//InitIirFilters();
	//set to 48KHz FsIn as starting --- only 48KHz local I2S Fs is considered in this demo
	//                                          Amplitude(),      FreqBeg,   FreqEnd,  Sweeping Duration,  Fs
	//                                        in between +/- 1      Hz          Hz            Sec          Hz
	InitSineToneGenerator(&SineToneGenerator1,     0.99f,          0.1f,      24000.0f,      20.0f,        48000);
	InitSineToneGenerator(&SineToneGenerator2,     0.10f,          1000.0f,    1000.0f,      20.0f,        48000);
	InitMeterAndCompressor();

	//cadence library init and deinit test
	#if TestAlgoInitAndDeInit==1
		TestHeap_AsrcInitAndDeInit(2);
		TestHeap_OpusDecodingInitAndDeInit(2);
		TestHeap_SbcDecodingInitAndDeInit(2);
	#endif

	//init SRC
	//             (ptr to handle.    int InputBlockSizeInSamples,        int inFs, int outFs, int ChNum,  EnableAsrc   NeedToDisplay)
	InitCadenceAsrc(&SRC_ConversaRx1,   AudioFrameSizeInSamplePerCh_16KHz/2,   8000,     16000,      1,           0,             0     );
	InitCadenceAsrc(&SRC_ConversaRx2,   AudioFrameSizeInSamplePerCh_48KHz  ,  48000,     16000,      2,           0,             0     );
	InitCadenceAsrc(&SRC_ConversaTx1,   AudioFrameSizeInSamplePerCh_16KHz  ,  16000,      8000,      1,           0,             0     );

#if UseUacDnAudioForConversaTuning_VoiceCall16KHz==1
	InitCadenceAsrc(&SRC_ConversaUacDn, AudioFrameSizeInSamplePerCh_48KHz  ,  48000,     16000,      1,           0,             0     );
#endif



	#if EnableOpusDec==1
		InitOpusOutputCirBuf(16000,1);
		OpusDecoderIsRunning=0;
	#endif

	#if EnableOpusDec==1
		InitSbcOutputCirBuf(48000,1);
		SbcDecoderIsRunning=0;
		SbcDecoderIsInited=0;
		SbcDecoderIsMutedButStillRunning=0;
	#endif


	//init NXP main algorithms
	#if TestAlgoInitAndDeInit==1
		TestHeap_ConversaInitAndDeInit(2);
		TestHeap_VitInitAndDeInit(2);
		TestHeap_EapInitAndDeInit(2);

		TestHeap_AllAlgoInitDeInit(1);
	#endif

	InitVit();
	InitConversa(ConversaTuningCfg_HfpVoiceCall,1);		//to print
	//DeInitConversa();
	//InitConversa(ConversaTuningCfg_HfpVoiceCall,1);		//to print

	#if EAP_ENABLE==1
		//               CfgIdx,      ToPrint_Level
		InitEap( EapTuningCfg_MusicPlay,    2      );	//print level 2 means to print complete info, 1 prints some, 0 prints none
		//      IsStereo and EAP preset=1
	#endif

	HeapPtr2=GetCurrentHeapTail(32);
	PRINTF("RT685 DSP: heap base address, %x\r\n", (U32)HeapPtr2);
	PRINTF("RT685 DSP: heap used: %d\r\n", (U32)HeapPtr2 - (U32)HeapPtr1);
	StackPtr=GetCurrentStackHead();
	PRINTF("RT685 DSP: stack tail address, %x\r\n", (U32)StackPtr);


	PL_INT32 retStatusXos  = XOS_OK;
	status_t retStatus     = kStatus_Success;

	XOS_EnableMuIntr();

	retStatusXos = xos_event_create(&MuEventFromMcu, 0xFFFF, 0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create MuEventFromMcu (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	retStatusXos = xos_event_create(&MuEventFromFrameProcTask, 0xFFFF, 0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create MuEventFromFrameProcTask (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}

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

	/* Create VIT/decoder bufer access mutex */
	retStatusXos =xos_mutex_create( &g_audio_vitBufferMutex,							// pointer to vit buffer mutex read/write for access
									XOS_MUTEX_WAIT_FIFO,
									0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create audio VIT mutex (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}

	retStatusXos =xos_mutex_create( &g_audio_SbcDecoderMutex,							// pointer to vit buffer mutex read/write for access
									XOS_MUTEX_WAIT_FIFO,
									0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create g_audio_SbcDecoderMutex (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}

	retStatusXos =xos_mutex_create( &g_audio_OpusDecoderMutex,							// pointer to vit buffer mutex read/write for access
									XOS_MUTEX_WAIT_FIFO,
									0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create g_audio_OpusDecoderMutex (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}


	PRINTF("\n*************************************\r\n");
	PRINTF("[DSP audioVITProcessTask] is running...\r\n");

	while (retStatusXos == XOS_OK)
	{
		retStatusXos = xos_sem_tryget( &g_audioTask_audioVitProcessSemaphore );    // try to decrement the audio task main semaphore counter and return error when semaphore count = 0
	}
	PRINTF("*************************************\r\n\n");

	//PRINTF("RT685 DSP: Xoslib cfg: %x\r\n",	xos_get_config_sig_lib());

	#if 1
	retStatusXos = xos_thread_create(&thread_VitAndAudioDec, NULL, Task_VitAndAudioDec, NULL, "Task_VitAndAudioDec",
						ThreadStackMem_VitAndAudioDec,
						THREAD_STACK_SIZE_VitAndAudioDec,
						THREAD_PRIORITY_VitAndAudioDec,
						0, 0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create Task_VitAndAudioDec (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	#endif

	#if 1
	retStatusXos = xos_thread_create(&thread_AudioFrameProc, NULL, Task_AudioFrameProc, NULL, "Task_AudioFrameProc",
						ThreadStackMem_AudioFrameProc,
						THREAD_STACK_SIZE_AudioFrameProc,
						THREAD_PRIORITY_AudioFrameProc,
						0, 0);
	if ( retStatusXos != XOS_OK )
	{
		PRINTF("FAIL - DSP init: Create Task_AudioFrameProc (error = %d)\r\n",retStatusXos);
		retStatus = kStatus_Fail;
	}
	#endif
	xos_start(0);
}

int Task_VitAndAudioDec(void * arg, int32_t wake_value)
{
	PL_INT32 retStatusXos  = XOS_OK;
	status_t retStatusFunc = kStatus_Success;
	AUDIO_vit_st*       p_swIpVIT_handle    	 = &vitPluginParams;
	PL_INT16*           p_conversaToVitBuff_16b  = NULL;
	PL_INT16*           p_conversaToVitBuff_16b_RawMic  = NULL;
	VIT_ReturnStatus_en VIT_Status;

	while(0)
	{
		xos_thread_sleep(xos_msecs_to_cycles(20-2));	//10ms delay
		DbgPin8Up();
		PRINTF_M("aaabbbccc\r\n");
		DbgPin8Dn();
	}

	while(1)
	{
		int SbcDecodingSkipCnt=0;		//use this counter to skip SbcDecoding for some time, for some purpose (such as wait Sbc buffer half full)

		//wait till either:
		//	1: a 10ms time out or
		//  2: a semaphore saying that VIT buffer is ready for one VIT processing --- this is 30ms each time
		retStatusXos=xos_sem_get_timeout(&g_audioTask_audioVitProcessSemaphore, xos_msecs_to_cycles(10-2));		//time out: 10ms
		if ( XOS_ERR_TIMEOUT == retStatusXos )
		{
			//time out --- to do decoding and other event checking and processing
//			DbgPin6Up();

			//each cycle of the following part is: 10.0ms

			#if EnableOpusDec==1
				//OPUS decoding temp place here --- later may move to a new task
				//start Opus decoding, between Opus file 1~6
				if(PtrVarBlockSharedByDspAndMcu->NeedToStartPlayOpus == 1)
				{
					if(OpusDecoderIsRunning)
					{
						//already decoding OPUS, should not start again
						PtrVarBlockSharedByDspAndMcu->NeedToStartPlayOpus=0;
					}else
					{
						//OPUS decoding is not started, now we start it
						xos_mutex_lock(&g_audio_OpusDecoderMutex);
							CirAudioBuf_ClearAllSamples_S32(&OpusOutputCirBuf_LRMixed);
						xos_mutex_unlock(&g_audio_OpusDecoderMutex);
						InitOpusDecoder();
						InitOpusDecoderForOneOpusFile(PtrVarBlockSharedByDspAndMcu->PlayOpusFileIdx);	//idx=0 stands for the first opus file

						#if EAP_ENABLE==1
							if((EapIsNowBeingUsed)&&((CurrentEAPTuningCfg==EapTuningCfg_MusicPlay)||(CurrentEAPTuningCfg==EapTuningCfg_MediaPlay)))
							{
								#if 0
									//do the EAP mixing with soft gain ramping up/down , only when EAP is used and EAP is in music play or media play configure

									//the start of the prompt OPUS decoding, command EAP to decrease the gain of the input stream[0] and stream[2]
									LVM_ControlParams_t currentParams;
									LVM_ReturnStatus_en retStatusEAP = LVM_SUCCESS;

									//Call get  EAP control parameters
									currentParams.BuildStructAlignCheck = LVM_BUILD_STRUCT_CHECK_VALUE;

									retStatusEAP = LVM_GetControlParameters(&eap_1_Handle, &currentParams);
									if(retStatusEAP != LVM_SUCCESS)
									{
										PRINTF_M("LVM_GetControlParameters: fail with error %i \r\n",retStatusEAP );
									}


									EapRefusesStartingANewPromptPlaying = true;			//don't allow start another prompt playing
									currentParams.pMIXER_StreamParams[0].TargetGain 		= (LVM_MIXER_VOLUME_MAX / 8); 		// Set audio playback stream to -24dB
									currentParams.pMIXER_StreamParams[0].TimeConstantMs 	= NOTIF_VOLUME_TRANSITION_TIME_MS;	// Time to arrive to target volume
									currentParams.pMIXER_StreamParams[0].pCallback 			= (LVM_Callback)eapNotificationCallbackDown;
									currentParams.pMIXER_StreamParams[2].TargetGain 		= (LVM_MIXER_VOLUME_MAX / 8); 		// Set audio playback stream to -24dB
									currentParams.pMIXER_StreamParams[2].TimeConstantMs 	= NOTIF_VOLUME_TRANSITION_TIME_MS;	// Time to arrive to target volume


									//set this command, will let EAP change the gain of stream [0] and [2] to the target gain value in NOTIF_VOLUME_TRANSITION_TIME_MS
									//and then will call back to eapNotificationCallbackDown, to let the prompt audio data be allowed to be written to EAP input stream [1]
									retStatusEAP = LVM_SetControlParameters(eap_1_Handle.hInstance, &currentParams);
									if(retStatusEAP != LVM_SUCCESS)
									{
										PRINTF_M("LVM_SetControlParameters: fail with error %i \r\n",retStatusEAP );
									}
								#else
									LVM_ReturnStatus_en retStatusEAP = LVM_SUCCESS;

									EapAllowsWritingStrm1AsPromptAudio = PL_FALSE;
									//eap_1_Handle.pControlParams->BuildStructAlignCheck = LVM_BUILD_STRUCT_CHECK_VALUE;
									eap_1_Handle.pControlParams->pMIXER_StreamParams[0].TargetGain 		= LVM_MIXER_VOLUME_MAX/8; 			// Set audio playback stream back to 0dB --- Quanta should change the target gain to the proper variable
									eap_1_Handle.pControlParams->pMIXER_StreamParams[0].TimeConstantMs 	= NOTIF_VOLUME_TRANSITION_TIME_MS;	// Time to arrive to target volume
									eap_1_Handle.pControlParams->pMIXER_StreamParams[0].pCallback 		= (LVM_Callback)eapNotificationCallbackDown;
									eap_1_Handle.pControlParams->pMIXER_StreamParams[1].TargetGain 		= LVM_MIXER_VOLUME_MAX;
									eap_1_Handle.pControlParams->pMIXER_StreamParams[2].TargetGain 		= LVM_MIXER_VOLUME_MAX/8; 			// Set audio playback stream back to 0dB --- Quanta should change the target gain to the proper variable
									eap_1_Handle.pControlParams->pMIXER_StreamParams[2].TimeConstantMs 	= NOTIF_VOLUME_TRANSITION_TIME_MS;	// Time to arrive to target volume

									//set this command, will let EAP recover the gain of stream [0] and [2] back to the target gain value in NOTIF_VOLUME_TRANSITION_TIME_MS
									//and then will call back to eapNotificationCallbackUp, to let the next prompt playing is allowed
									retStatusEAP = LVM_SetControlParameters(eap_1_Handle.hInstance, eap_1_Handle.pControlParams);
									if(retStatusEAP != LVM_SUCCESS)
									{
										PRINTF_M("LVM_SetControlParameters: fail with error %i \r\n",retStatusEAP );
									}
								#endif

								#if EnableDebugPrint==1
									PRINTF_M("	DSP: OPUS play is started\r\n");
								#endif
							}
						#endif

						PtrVarBlockSharedByDspAndMcu->NeedToStartPlayOpus=0;
						OpusDecoderIsRunning=1;
					}
				}

				//call Opus decoding after started
				if(OpusDecoderIsRunning)
				{
					if(OpusDecoderIsRunning==2)
					{
						//decoding is finished
						PtrVarBlockSharedByDspAndMcu->NeedToStartPlayOpus=2; // for cm33: disable amp

						DeInitOpusDecoder();
						DeInitOpusDecoderForOneOpusFile();
					}else
					{
						//decoding one frame --- 20ms: all opus files are made with framesize=20ms

						CycCntA=read_ccount();
							DbgPin6Up();
							OpusDecodeProcess();
							DbgPin6Dn();
						CycCntB=read_ccount();
						PtrVarBlockSharedByDspAndMcu->MonitorInfoArray2[1]=CycCntB-CycCntA;
						//PRINTF_M("Cycles: %d \n", CycCntB-CycCntA);			//measured: 2~11 MIPS, the tested 6 OPUS files
					}
				}
			#endif

			#if EnableSbcDec==1
				//Sbc decoding temp place here --- later may move to a new task
				//start Sbc decoding, between Sbc file 1~6
				if(PtrVarBlockSharedByDspAndMcu->NeedToStartPlaySbc)
				{
					//PRINTF_M("NeedToStartPlaySbc is 1 \n");
					if(SbcDecoderIsRunning)
					{
						//already decoding SBC, should not start again
						PtrVarBlockSharedByDspAndMcu->NeedToStartPlaySbc=0;
					}else
					{
						//SBC decoding is not started, now we start it
						if(!SbcDecoderIsInited)
						{
							//completely new init
							xos_mutex_lock(&g_audio_SbcDecoderMutex);
								CirAudioBuf_ClearAllSamples_S32(&SbcOutputCirBuf_LRMixed);
							xos_mutex_unlock(&g_audio_SbcDecoderMutex);
							InitSbcDecoder();
							//PRINTF_M("InitSbcDecoder done1 %d \n", SbcDecoderIsInited);
						}

						if(SbcDecoderIsInited==1)
						{
							//first step init is done, the next init step needs enough raw sbc data in the cir buffer
							int e;
							e=InitSbcDecoderForOneSbcFile(PtrVarBlockSharedByDspAndMcu->PlaySbcFileIdx);	//idx=0 stands for the first sbc file, idx=0xffff stands for BT a2dp stream
							//PRINTF_M("InitSbcDecoder done2 %d %d\n", SbcDecoderIsInited,e);
						}

						if(SbcDecoderIsInited==2)	//this is to consider when reading a2dp sbc stream from cir buffer, not enough data in the cir buffer
						{
							//enough raw sbc data in the cir buffer, and init is OK
							PtrVarBlockSharedByDspAndMcu->NeedToStartPlaySbc=0;
							SbcDecoderIsRunning=1;
							SbcDecoderIsMutedButStillRunning=0;
							//PRINTF_M("InitSbcDecoder done3 ==2 \n");
						}else
						{
							//NOT enough raw sbc data in the cir buffer, and init is NOT OK --- will come back to init again
						}
					}
				}

				//call Sbc decoding after started
				if(SbcDecoderIsRunning)
				{
					if(SbcDecoderIsRunning==2)
					{
						//decoding is finished
						DeInitSbcDecoder();
						DeInitSbcDecoderForOneSbcFile();
						SbcDecoderIsInited=0;
						CirAudioBuf_ClearAllSamples_S8((T_CircularAudioBuf_S8 *)&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw);
						SbcDecoderIsRunning=0;
						SbcDecoderIsMutedButStillRunning=0;
					}else
					{
						//decoding one frame --- 20ms: all Sbc files are made with framesize=20ms
						int e;
						//PRINTF_M("SbcDecodeProcess\n");
						CycCntA=read_ccount();
							if(SbcDecodingSkipCnt)
							{
								SbcDecodingSkipCnt--;
								e=0;
							}else
							{
								DbgPin6Up();
								e=SbcDecodeProcess(PtrVarBlockSharedByDspAndMcu->PlaySbcFileIdx);
								DbgPin6Dn();
							}
						CycCntB=read_ccount();
						PtrVarBlockSharedByDspAndMcu->MonitorInfoArray2[1]=CycCntB-CycCntA;

						//only print error
						// gill
						// Now A2DP is connected, but phone may stop play music, and sbc buffer not transfered
						// disable print log temporarily
						if(e==123)
						{
							//run out of Sbc cir buffer --- if this happens, should wait till sbc cir buffer is half full --- this is to wait about 300us
							SbcDecodingSkipCnt=10;		//skip 30*10=300ms
							PRINTF_M("RT685 DSP: run out of Sbc --- wait 300ms to let sbc buffer half full \r\n");
						}

//						if(e)
//							PRINTF("SbcDecodeProcess err: %d \n", e);
						//PRINTF("Cycles: %d \n", CycCntB-CycCntA);			//measured: 2~11 MIPS, the tested 6 Sbc files
					}
				}

				if(PtrVarBlockSharedByDspAndMcu->NeedToStopA2dpSbc==1)
				{
					//this is to stop
					if(SbcDecoderIsRunning)
					{
						//decoding is finished
						DeInitSbcDecoder();
						DeInitSbcDecoderForOneSbcFile();
						SbcDecoderIsInited=0;
						CirAudioBuf_ClearAllSamples_S8((T_CircularAudioBuf_S8 *)&PtrVarBlockSharedByDspAndMcu->CirBuf_SbcRaw);
						SbcDecoderIsRunning=0;
						SbcDecoderIsMutedButStillRunning=0;
					}else
					{
						//should not happen
					}
					PtrVarBlockSharedByDspAndMcu->NeedToStopA2dpSbc=0;
				}

				#if 1	//pause and un-pause are not really realized
				if(PtrVarBlockSharedByDspAndMcu->NeedToStopA2dpSbc==2)	//search for "VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=2;" in MCU side
				{
					//this is to pause
					if(SbcDecoderIsRunning)
					{
							//SbcDecoderIsMutedButStillRunning=1;	//use this is to really realize pause/mute
							SbcDecoderIsMutedButStillRunning=0;		//this is to disable pause --- not really realized
							PRINTF_M("Dsp, sbc pause effect\n");
					}else
					{
						//should not happen
					}
					PtrVarBlockSharedByDspAndMcu->NeedToStopA2dpSbc=0;
				}
				if(PtrVarBlockSharedByDspAndMcu->NeedToStopA2dpSbc==3)	//search for "VarBlockSharedByDspAndMcu.NeedToStopA2dpSbc=3;" in MCU side
				{
					//this is to un-pause	//not sure if BT side will send this request
					if(SbcDecoderIsRunning)
					{
						SbcDecoderIsMutedButStillRunning=0;
							PRINTF_M("Dsp, sbc UN-pause effect\n");
					}else
					{
						//should not happen
					}
					PtrVarBlockSharedByDspAndMcu->NeedToStopA2dpSbc=0;
				}
				#endif

			#endif

//			DbgPin6Dn();

			#if 1	//MCU request checking and processing --- to be finished by quanta
				if(PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_McuCmdToDsp])
				{
					//some may need to set [ControlParaIdx_DspJobDone]=1; --- cause MCU is waiting to see job done
					//some may not need to --- cause MCU doesn't care if job is done or not
					switch(PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_McuCmdToDsp])
					{
						case McuToDspReqeust_IncMasterVol:
								//PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=1;
							break;
						case McuToDspReqeust_DecMasterVol:
								//PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=1;
							break;
						case McuToDspReqeust_SetMasterVol:
								//PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=1;
							break;
						case McuToDspReqeust_MuteMasterVol:
								PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=1;
							break;

						case McuToDspReqeust_UnMuteMasterVol:
								PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=1;
							break;

						default:
							break;
					}
					PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_McuCmdToDsp]=0;
				}
			#endif

			//-----------------------------------------------switch conversa tuning config when MCU side requests--------------------------------------------------
			//---beg---
			#if 1	//folding
				//audio frame thread/task has higher priority, should be safe to deinit conversa here, when requested
				if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchConversaTuningCfg!=ConversaTuningCfg_NoChange)
				{
					if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchConversaTuningCfg==ConversaTuningCfg_ShutDownConversa)
					{
						//to shut down
						DeInitConversa();

						#if EnableDebugPrint==1
							PRINTF_M("	DSP: conversa is shut down\r\n");
						#endif

						//need to set [ControlParaIdx_DspJobDone]=1; --- cause MCU is waiting to see job done
						PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=ConversaTuningCfg_ShutDownConversa;
					}else
					{
						//to switch to a different cfg
						if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchConversaTuningCfg==CurrentConversaTuningCfg)
						{
							//already in the desired conversa tuning cfg
							#if EnableDebugPrint==1
								PRINTF_M("	DSP: conversa is already inited\r\n");
							#endif
						}else
						{
							//confirmed the new cfg is different from current
							DeInitConversa();
							InitConversa(PtrVarBlockSharedByDspAndMcu->NeedToSwitchConversaTuningCfg, 0);	//no need to print

							#if EnableDebugPrint==1
								PRINTF_M("	DSP: conversa init is done\r\n");
							#endif
						}
						//need to set [ControlParaIdx_DspJobDone]=1; --- cause MCU is waiting to see job done
						PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=PtrVarBlockSharedByDspAndMcu->NeedToSwitchConversaTuningCfg;
					}

					PtrVarBlockSharedByDspAndMcu->NeedToSwitchConversaTuningCfg=ConversaTuningCfg_NoChange;
				}
			#endif
			//---end---
			//-----------------------------------------------switch conversa tuning config when MCU side requests--------------------------------------------------

			//-----------------------------------------------switch EAP tuning config when MCU side requests--------------------------------------------------
			//---beg---
			#if EAP_ENABLE==1	//folding
				//audio frame thread/task has higher priority, should be safe to deinit conversa here, when requested
				if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg!=EapTuningCfg_NoChange)
				{
					if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg==EapTuningCfg_ShutDownEap)
					{
						//to shut down
						DeInitEap(&eap_1_Handle);

						#if EnableDebugPrint==1
							PRINTF_M("	DSP: EAP is shut down\r\n");
						#endif

						//need to set [ControlParaIdx_DspJobDone]=1; --- cause MCU is waiting to see job done
						PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=EapTuningCfg_ShutDownEap;
					}else
					{
						//to switch to a different cfg
						if(PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg==CurrentEAPTuningCfg)
						{
							//already in the desired EAP tuning cfg
							#if EnableDebugPrint==1
								PRINTF_M("	DSP: EAP is already inited\r\n");
							#endif
		}else
		{
							//confirmed the new cfg is different from current
							DeInitEap(&eap_1_Handle);
							InitEap(PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg, 0);	//no need to print

							#if EnableDebugPrint==1
								PRINTF_M("	DSP: EAP init is done\r\n");
							#endif
						}

						//need to set [ControlParaIdx_DspJobDone]=1; --- cause MCU is waiting to see job done
						PtrVarBlockSharedByDspAndMcu->U32ControlPara[ControlParaIdx_DspJobDone]=PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg;
					}
					PtrVarBlockSharedByDspAndMcu->NeedToSwitchEapTuningCfg=EapTuningCfg_NoChange;
				}
			#endif
			//---end---
			//-----------------------------------------------switch EAP tuning config when MCU side requests--------------------------------------------------

			#if 0	//print something to show DSP is active
				static U32 BackGroundTskCnt1=0;
				if(!(BackGroundTskCnt1%20))
				{
					//20ms each print
					//DbgPin8Up();
						PRINTF_M("RT685 DSP: active 1\r\n");
					//DbgPin8Dn();
				}
				BackGroundTskCnt1++;
			#endif
				}else
				{
			#if 1	//VIT process
				if(!ToTempSkipVitPorcess)
				{
					xos_mutex_lock(&g_audio_vitBufferMutex);	//--- the other side, can NOT call xos_mutex_lock from intr, but now, algo processing is moved to task(thread), CAN call this now
						p_conversaToVitBuff_16b = CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff, p_swIpVIT_handle->vitConfig.framesize);
						p_conversaToVitBuff_16b_RawMic= CirAudioBuf_ReadSamples_GetRdPtr_S16(&VitCircBuff_RawMic, p_swIpVIT_handle->vitConfig.framesize);
					xos_mutex_unlock(&g_audio_vitBufferMutex); //--- the other side, can NOT call xos_mutex_lock from intr, but now, algo processing is moved to task(thread), CAN call this now

					/********************************************************************
					 * VIT PROCESS
					 *		- Copy current samples from source
					 *		- Check if there are enogugh samples to run VIT
					 *		- Run VIT process
					 *		- Send message to MCU to indicate VIT process finished
					 *******************************************************************/
					DbgPin6Up();
					VIT_Status = swProcessVIT( p_swIpVIT_handle,
											   p_conversaToVitBuff_16b,
											   p_conversaToVitBuff_16b_RawMic,
											   p_swIpVIT_handle->vitConfig.framesize,
											   &g_vitDetectionResult,
											   &g_vitVcDetectionId
											 );
					DbgPin6Dn();


					if (VIT_Status != VIT_SUCCESS)
					{
						PRINTF_M("  VIT_Process error : %d\n", VIT_Status);
						retStatusFunc = kStatus_Fail;
					}
				}
			#endif

			#if 0	//print something to show DSP is active
				static U32 BackGroundTskCnt2=0;
				if(!(BackGroundTskCnt2%29))
				{
					//20ms each print
					//DbgPin8Up();
						PRINTF_M("RT685 DSP: active 2\r\n");
					//DbgPin8Dn();
				}
				BackGroundTskCnt2++;
			#endif
		}
	}
	return 0;
}

int Task_AudioFrameProc(void * arg, int32_t wake_value)
{
	while(0)
	{
		//this is a test to check if this task is active or not
		xos_thread_sleep(xos_msecs_to_cycles(10-2));	//10ms delay
		DbgPin7Up();
		PRINTF_M("111222333\r\n");
		DbgPin7Dn();
	}

	while(1)
	{
		xos_event_wait_any(&MuEventFromMcu, 0xFFFF);

		U32 MU_U32InfoFromMcu;
		xos_event_get(&MuEventFromMcu, &MU_U32InfoFromMcu);
		xos_event_clear(&MuEventFromMcu, 0xffff);

		AudioFrameProcess(MU_U32InfoFromMcu);

		#if 0	//print something to show DSP is active --- this part open: dsp stucks
			static U32 BackGroundTskCnt3=0;
			if(!(BackGroundTskCnt3%25))
			{
				//20ms each print
				//DbgPin8Up();
					PRINTF_M("RT685 DSP: active 3\r\n");
				//DbgPin8Dn();
			}
			BackGroundTskCnt3++;
		#endif
	}
	return 0;
}


