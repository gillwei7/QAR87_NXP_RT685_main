
#ifndef __MainAudioFlow_h__
#define __MainAudioFlow_h__


#define AudioPdmPortsBitMapFlag_Mic01			0x01
#define AudioPdmPortsBitMapFlag_Mic23			0x02

#if EnableMic45==1
	#define AudioPdmPortsBitMapFlag_Mic45			0x04
#endif

#if EnableMic67==1
	#define AudioPdmPortsBitMapFlag_Mic67			0x08
#endif

#define AudioI2sPortsBitMapFlag_Fc1				0x10
#define AudioI2sPortsBitMapFlag_Fc3				0x20
//#define AudioI2sPortsBitMapFlag_Fc2				0x01
//#define AudioI2sPortsBitMapFlag_Fc4				0x08

extern int BTAudioBitWidth;
extern int BTAudioFs;

extern volatile U8 PdmIsStarted;
extern volatile U8 I2sIsStarted;

extern volatile U8 DmaTxRxIsDone;

extern int AOD_BTDnBuf;
extern int AOD_BTUpBuf;
extern uint8_t domainId;


extern U32 MainAudioFlowMode;		//0: no audio flow, 1,2,3: flow1,2,3

extern volatile U8 DmaTxRxIsDone;


extern int CheckTimePoint_CurrentIntrIsAStartingOne(void);

extern void CloseAllActivedPDMPorts(void);
extern void CloseAllActivedI2SPorts(void);


#endif

