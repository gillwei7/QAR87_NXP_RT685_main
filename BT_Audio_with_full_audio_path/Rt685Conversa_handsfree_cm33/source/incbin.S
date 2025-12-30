/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef DSP_IMAGE_COPY_TO_RAM
#define DSP_IMAGE_COPY_TO_RAM 1
#endif

#if DSP_IMAGE_COPY_TO_RAM==1

#if defined(__GNUC__) || defined(__ARMCC_VERSION)

        .section .OpusSbcBin , "ax" @progbits @preinit_array
        .global dsp_text_image_start
        .type dsp_text_image_start, %object
        .align 4
dsp_text_image_start:

		.incbin "dsp_text_release.bin"

        .global dsp_text_image_end
        .type dsp_text_image_end, %object
dsp_text_image_end:
        .global dsp_data_image_start
        .type dsp_data_image_start, %object
        .align 4
dsp_data_image_start:

		.incbin "dsp_data_release.bin"


        .global dsp_data_image_end
        .type dsp_data_image_end, %object
dsp_data_image_end:
        .global dsp_ncache_image_start
        .type dsp_ncache_image_start, %object
        .align 4
dsp_ncache_image_start:

		.incbin "dsp_ncache_release.bin"

        .global dsp_ncache_image_end
        .type dsp_ncache_image_end, %object
dsp_ncache_image_end:
        .global dsp_text_image_size
        .type dsp_text_image_size, %object
        .align 4
dsp_text_image_size:
        .int  dsp_text_image_end - dsp_text_image_start
        .global dsp_data_image_size
        .type dsp_data_image_size, %object
        .align 4
dsp_data_image_size:
        .int  dsp_data_image_end - dsp_data_image_start
        .global dsp_ncache_image_size
        .type dsp_ncache_image_size, %object
        .align 4
dsp_ncache_image_size:
        .int  dsp_ncache_image_end - dsp_ncache_image_start
//        .end

#endif

#endif // DSP_IMAGE_COPY_TO_RAM==1


		.global SbcFileBeg1
		.global SbcFileEnd1
		.global SbcFileBeg2
		.global SbcFileEnd2
		.global SbcFileBeg3
		.global SbcFileEnd3

		.global OpusFileBeg1
		.global OpusFileEnd1
		.global OpusFileBeg2
		.global OpusFileEnd2
		.global OpusFileBeg3
		.global OpusFileEnd3
		.global OpusFileBeg4
		.global OpusFileEnd4
		.global OpusFileBeg5
		.global OpusFileEnd5
		.global OpusFileBeg6
		.global OpusFileEnd6
	    .global OpusFileBeg7
		.global OpusFileEnd7
		.global OpusFileBeg8
		.global OpusFileEnd8
		.global OpusFileBeg9
		.global OpusFileEnd9
		.global OpusFileBeg10
		.global OpusFileEnd10
		.global OpusFileBeg11
		.global OpusFileEnd11
		.global OpusFileBeg12
		.global OpusFileEnd12
		.global OpusFileBeg13
		.global OpusFileEnd13
		.global OpusFileBeg14
		.global OpusFileEnd14

		.type SbcFileBeg1, %object
		.type SbcFileEnd1, %object
		.type SbcFileBeg2, %object
		.type SbcFileEnd2, %object
		.type SbcFileBeg3, %object
		.type SbcFileEnd3, %object

		.type OpusFileBeg1, %object
		.type OpusFileEnd1, %object
		.type OpusFileBeg2, %object
		.type OpusFileEnd2, %object
		.type OpusFileBeg3, %object
		.type OpusFileEnd3, %object
		.type OpusFileBeg4, %object
		.type OpusFileEnd4, %object
		.type OpusFileBeg5, %object
		.type OpusFileEnd5, %object
		.type OpusFileBeg6, %object
		.type OpusFileEnd6, %object
		.type OpusFileBeg7, %object
		.type OpusFileEnd7, %object
		.type OpusFileBeg8, %object
		.type OpusFileEnd8, %object
		.type OpusFileBeg9, %object
		.type OpusFileEnd9, %object
		.type OpusFileBeg10, %object
		.type OpusFileEnd10, %object
		.type OpusFileBeg11, %object
		.type OpusFileEnd11, %object
		.type OpusFileBeg12, %object
		.type OpusFileEnd12, %object
		.type OpusFileBeg13, %object
		.type OpusFileEnd13, %object
		.type OpusFileBeg14, %object
		.type OpusFileEnd14, %object

        //.section OpusSbcBin //,  "ax" @progbits @preinit_array

        .align 4;
SbcFileBeg1:
		//.incbin "1111Hz_16KHz.sbc"
		.incbin "sbc_test_02.sbc"
SbcFileEnd1:
		.int 0
SbcFileBeg2:
		.incbin "1111Hz_44_1KHz.sbc"
SbcFileEnd2:
		.int 0
SbcFileBeg3:
		.incbin "1111Hz_48KHz.sbc"
SbcFileEnd3:
		.int 0


		//bigger, higher bit rate
        .align 4;
OpusFileBeg1:
		.incbin "1.PowerON.opus"
OpusFileEnd1:
		.int 0

        .align 4;
OpusFileBeg2:
		.incbin "2.PowerOff.opus"
OpusFileEnd2:
		.int 0

        .align 4;
OpusFileBeg3:
		.incbin "3.Low Battery_cut_20251230.opus"
OpusFileEnd3:
		.int 0

        .align 4;
OpusFileBeg4:
		.incbin "4.WiFi_DisCN.opus"
OpusFileEnd4:
		.int 0

        .align 4;
OpusFileBeg5:
		.incbin "5.BT_DisCN.opus"
OpusFileEnd5:
		.int 0

        .align 4;
OpusFileBeg6:
		.incbin "6.start_recording.opus"
OpusFileEnd6:
		.int 0

        .align 4;
OpusFileBeg7:
		.incbin "7.Cam_Rec_Stop.opus"
OpusFileEnd7:
		.int 0

        .align 4;
OpusFileBeg8:
		.incbin "8.photo_Capture.opus"
OpusFileEnd8:
		.int 0

        .align 4;
OpusFileBeg9:
		.incbin "9.StartVideoAI.opus"
OpusFileEnd9:
		.int 0

        .align 4;
OpusFileBeg10:
		.incbin "10.StopVideoAI.opus"
OpusFileEnd10:
		.int 0

        .align 4;
OpusFileBeg11:
		.incbin "11.StartTranslation.opus"
OpusFileEnd11:
		.int 0

        .align 4;
OpusFileBeg12:
		.incbin "12.StopTranslation.opus"
OpusFileEnd12:
		.int 0

        .align 4;
OpusFileBeg13:
		.incbin "Beep1_16KHz_1Ch_16Kbps.opus"
OpusFileEnd13:
		.int 0

        .align 4;
OpusFileBeg14:
		.incbin "14.bt_connected.opus"
OpusFileEnd14:
		.int 0

        .end
