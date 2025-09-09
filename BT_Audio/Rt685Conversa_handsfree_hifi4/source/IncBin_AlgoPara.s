
        .text
        .global ProfileBin_start

        .type ProfileBin_start, %object



        .align 4
ProfileBin_start:
		//.incbin "xxxx.bin"
ProfileBin_end:
		.byte 0,0,0,0


        .global ProfileBin_size
        .type ProfileBin_size, %object
        .align 4
ProfileBin_size:
        .int  ProfileBin_end - ProfileBin_start
		.byte 0,0,0,0




