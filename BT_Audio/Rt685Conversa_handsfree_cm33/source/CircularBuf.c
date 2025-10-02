/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <stdint.h>
#include <string.h>

#include "fsl_common.h"

#include "GlobalDef.h"

#if EnableConversa==1


#include "CircularBufManagement.h"
#include "CircularBuf.h"


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
    UacDnBuf_LengthInMs * AUDIO_OUT_SAMPLING_RATE_KHZ + UacDnBuf_MaxReadLengthInSamples
];

T_CirUacUpAudioBuf_MCh UacUpAudioBuf_MCh;
T_MCh32BitUacUpAudioSample UacUpAudioBuf_MCh_DataArea
[
    UacUpBuf_LengthInSamples + UacUpBuf_MaxReadLengthInSamples
];



S32 AllZeroBuf_48PointsSingleCh_16Bit [48*8];
#endif

__attribute__((__section__(".data.$RamSharedWithDsp")))
T_CommonVarSharedByDspAndMcu VarBlockSharedByDspAndMcu;

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



void InitAudioCircularBuf(void)
{
	InitCirAudioBuf_S16(&BTUpAudioBuf_S16,
			BTUpAudioBuf_DataArea,BTUpAudioBuf_Len_InSamples
			);
	InitCirAudioBuf_S16(&BTDnAudioBuf_S16,
			BTDnAudioBuf_DataArea,BTDnAudioBuf_Len_InSamples
			);

	#if EnableUsbComAndAudio==1
		InitCirUacUpAudioBuf_MultiCh(&UacUpAudioBuf_MCh,
				UacUpAudioBuf_MCh_DataArea,UacUpBuf_LengthInSamples
				);
		InitCirUacDnAudioBuf_MultiCh(&UacDnAudioBuf_MCh,
				UacDnAudioBuf_MCh_DataArea,UacDnBuf_LengthInMs * AUDIO_OUT_SAMPLING_RATE_KHZ
				);
		memset(AllZeroBuf_48PointsSingleCh_16Bit,0,sizeof(AllZeroBuf_48PointsSingleCh_16Bit));
	#endif
			
}



#endif
