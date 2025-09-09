#ifndef __AudioProcess_h__
#define __AudioProcess_h__


#if EnableConversa==1

extern unsigned char NeedToMute_Mic1;
extern unsigned char NeedToMute_Mic2;
extern unsigned char NeedToMute_Mic3;
extern unsigned char NeedToMute_Mic4;
extern unsigned char NeedToMute_SpkL;
extern unsigned char NeedToMute_SpkR;


extern int AUDIOPLL0NUM_StartingUpValue;
extern int AUDIOPLL0NUM_AdjustingValue;

extern void GenerateSinWav_S32(S32 *DstPtrL,S32 *DstPtrR, int L);	//L should be <=128
extern void GenerateSinWav_S16_LRMixed(S16 *DstPtr, int L);			//L should be <=128
extern void GenerateSinWav_S16_SingleCh(S16 *DstPtr, int L);		//L should be <=128



extern void CheckI2SInputBufAodAndAdjustAudioPll(int AodValue);


#endif

#endif
