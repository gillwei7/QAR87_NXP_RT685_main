/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdint.h>
#include <string.h>

#include "fsl_common.h"

#include "board.h"
#include "GlobalDef.h"

#if EnableConversa==1


#include "CircularBufManagement.h"
#include "CircularBuf.h"
#include "SubFunc.h"

#if EnableUsbComAndAudio==1

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_audio.h"
//#include "usb_device_cdc_acm.h"
//#include "usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "audio_unified.h"


T_CirUacDnAudioBuf_MCh UacDnAudioBuf_MCh;
T_MCh32BitUacDnAudioSample UacDnAudioBuf_MCh_DataArea
[
    UacDnBuf_LengthInSamples + UacDnBuf_MaxReadLengthInSamples
];

T_CirUacUpAudioBuf_MCh UacUpAudioBuf_MCh;
T_MCh32BitUacUpAudioSample UacUpAudioBuf_MCh_DataArea
[
    UacUpBuf_LengthInSamples + UacUpBuf_MaxReadLengthInSamples
];

#if Rt685I2SToNvtIsI2SMaster==0
	//only when NT is I2S master, we use cir buffer
	#if Rt685I2SToNvtBitWidth==32
		T_CircularAudioBuf_S64 I2SRxFrNt_CirBuf;
		S64 I2SRxFrNt_CirBuf_S64_DataArea[I2SNt_CirBuf_LenInSamples + AudioFrameSizeInSamplePerCh];	//read to this buffer is from i2s1,3, read size is AudioFrameSizeInSamplePerCh
		T_CircularAudioBuf_S64 I2STxToNt_CirBuf;
		S64 I2STxToNt_CirBuf_S64_DataArea[I2SNt_CirBuf_LenInSamples + AudioFrameSizeInSamplePerCh_I2SToNvt];	//read to this buffer is from i2sTxToNt, read size is AudioFrameSizeInSamplePerCh_I2SToNvt
	#endif
	#if Rt685I2SToNvtBitWidth==16
		T_CircularAudioBuf_S32 I2SRxFrNt_CirBuf;
		S32 I2SRxFrNt_CirBuf_S32_DataArea[I2SNt_CirBuf_LenInSamples + AudioFrameSizeInSamplePerCh];	//read to this buffer is from i2s1,3, read size is AudioFrameSizeInSamplePerCh
		T_CircularAudioBuf_S32 I2STxToNt_CirBuf;
		S32 I2STxToNt_CirBuf_S32_DataArea[I2SNt_CirBuf_LenInSamples + AudioFrameSizeInSamplePerCh_I2SToNvt];	//read to this buffer is from i2sTxToNt, read size is AudioFrameSizeInSamplePerCh_I2SToNvt
	#endif
#endif


S32 AllZeroBuf_48PointsSingleCh_16Bit [48*8];	//reserve Zeros for 8chs
#endif

__attribute__((__section__(".data.$RamSharedWithDsp")))
volatile T_CommonVarSharedByDspAndMcu VarBlockSharedByDspAndMcu;

T_CircularAudioBuf_S16 BTDnAudioBuf_S16;
S16 BTDnAudioBuf_DataArea
[
 BTDnAudioBuf_Len_InSamples + BTDnAudioBuf_MaxReadLen_InSamples
];

T_CircularAudioBuf_S16 BTUpAudioBuf_S16;
S16 BTUpAudioBuf_DataArea
[
 BTUpAudioBuf_Len_InSamples + BTUpAudioBuf_MaxReadLen_InSamples
];

/*
void ClearBTUpDnAudioBufDataArea(void)
{
	DbgPin5Up();
	DbgPin6Up();
	memset(BTDnAudioBuf_DataArea,0,sizeof(BTDnAudioBuf_DataArea));
	memset(BTUpAudioBuf_DataArea,0,sizeof(BTUpAudioBuf_DataArea));
	DbgPin5Dn();
	DbgPin6Dn();
}
*/
void InitAudioCircularBuf(void)
{
	InitCirAudioBuf_S16(&BTUpAudioBuf_S16,
			BTUpAudioBuf_DataArea,BTUpAudioBuf_Len_InSamples
			);
	InitCirAudioBuf_S16(&BTDnAudioBuf_S16,
			BTDnAudioBuf_DataArea,BTDnAudioBuf_Len_InSamples
			);

	//ClearBTUpDnAudioBufDataArea();

	#if EnableUsbComAndAudio==1
		InitCirUacUpAudioBuf_MultiCh(&UacUpAudioBuf_MCh,
				UacUpAudioBuf_MCh_DataArea,UacUpBuf_LengthInSamples
				);
		InitCirUacDnAudioBuf_MultiCh(&UacDnAudioBuf_MCh,
				UacDnAudioBuf_MCh_DataArea,UacDnBuf_LengthInSamples
				);
		memset(AllZeroBuf_48PointsSingleCh_16Bit,0,sizeof(AllZeroBuf_48PointsSingleCh_16Bit));
	#endif





	#if Rt685I2SToNvtIsI2SMaster==0
		//only when NT is I2S master, we use cir buffer
		#if Rt685I2SToNvtBitWidth==32
			InitCirAudioBuf_S64(&I2SRxFrNt_CirBuf,
					I2SRxFrNt_CirBuf_S64_DataArea,I2SNt_CirBuf_LenInSamples
					);
			InitCirAudioBuf_S64(&I2STxToNt_CirBuf,
					I2STxToNt_CirBuf_S64_DataArea,I2SNt_CirBuf_LenInSamples
					);
		#endif
		#if Rt685I2SToNvtBitWidth==16
			InitCirAudioBuf_S32(&I2SRxFrNt_CirBuf,
					I2SRxFrNt_CirBuf_S32_DataArea,I2SNt_CirBuf_LenInSamples
					);
			InitCirAudioBuf_S32(&I2STxToNt_CirBuf,
					I2STxToNt_CirBuf_S32_DataArea,I2SNt_CirBuf_LenInSamples
					);
		#endif
	#endif
}



#endif
