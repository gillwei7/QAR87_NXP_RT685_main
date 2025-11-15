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

        .section .dspcode , "ax" @progbits @preinit_array
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
		#if 0
			.global OpusFileBeg4
			.global OpusFileEnd4
			.global OpusFileBeg5
			.global OpusFileEnd5
			.global OpusFileBeg6
			.global OpusFileEnd6
		#endif

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
		#if 0
			.type OpusFileBeg4, %object
			.type OpusFileEnd4, %object
			.type OpusFileBeg5, %object
			.type OpusFileEnd5, %object
			.type OpusFileBeg6, %object
			.type OpusFileEnd6, %object
		#endif

        //.section OpusSbcBin //,  "ax" @progbits @preinit_array



#if 1
        .align 4;
SbcFileBeg1:
SbcFileEnd1:
SbcFileBeg2:
SbcFileEnd2:
SbcFileBeg3:
SbcFileEnd3:
OpusFileBeg1:
OpusFileEnd1:
OpusFileBeg2:
OpusFileEnd2:
OpusFileBeg3:
OpusFileEnd3:

		.int 0

        .end
#endif


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

#if 1
		//smaller, lower bit rate
        .align 4;
OpusFileBeg1:
		.incbin "Beep1_16KHz_1Ch_16Kbps.opus"
OpusFileEnd1:
		.int 0

        .align 4;
OpusFileBeg2:
		.incbin "Beep2_48KHz_1Ch_80Kbps.opus"
OpusFileEnd2:
		.int 0

        .align 4;
OpusFileBeg3:
		.incbin "Speech_16KHz_1Ch_12Kbps.opus"
OpusFileEnd3:
		.int 0
#if 0
        .align 4;
OpusFileBeg4:
		.incbin "Speech_16KHz_1Ch_16Kbps.opus"
OpusFileEnd4:
		.int 0

        .align 4;
OpusFileBeg5:
		.incbin "Sweep_48KHz_1Ch_80Kbps.opus"
OpusFileEnd5:
		.int 0

        .align 4;
OpusFileBeg6:
		.incbin "Music_48Khz_2Ch_80Kbps.opus"
OpusFileEnd6:
		.int 0
#endif
#else
		//bigger, higher bit rate
        .align 4;
OpusFileBeg1:
		.incbin "Beep1_16KHz_1Ch_32Kbps.opus"
OpusFileEnd1:
		.int 0

        .align 4;
OpusFileBeg2:
		.incbin "Beep2_48KHz_1Ch_180Kbps.opus"
OpusFileEnd2:
		.int 0

        .align 4;
OpusFileBeg3:
		.incbin "Speech_16KHz_1Ch_16Kbps.opus"
OpusFileEnd3:
		.int 0

        .align 4;
OpusFileBeg4:
		.incbin "Speech_16KHz_1Ch_32Kbps.opus"
OpusFileEnd4:
		.int 0

        .align 4;
OpusFileBeg5:
		.incbin "Sweep_48KHz_1Ch_180Kbps.opus"
OpusFileEnd5:
		.int 0

        .align 4;
OpusFileBeg6:
		.incbin "Music_48Khz_2Ch_180Kbps.opus"
OpusFileEnd6:
		.int 0

#endif


        .end
