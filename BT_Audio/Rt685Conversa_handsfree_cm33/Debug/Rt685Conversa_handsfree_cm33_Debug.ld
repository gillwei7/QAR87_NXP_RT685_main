/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * Generated linker script file for MIMXRT685S
 * Created from linkscript.ldt by FMCreateLinkLibraries
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v24.12 [Build 148] [2025-01-10] on 2025年9月25日 下午12:22:23
 */

INCLUDE "Rt685Conversa_handsfree_cm33_Debug_library.ld"
INCLUDE "Rt685Conversa_handsfree_cm33_Debug_memory.ld"

ENTRY(ResetISR)

SECTIONS
{
     .text_Flash2 : ALIGN(4)
    {
       FILL(0xff)
        *(.text_Flash2) /* for compatibility with previous releases */
        *(.text_LITTLEFS_FLASH_region) /* for compatibility with previous releases */
        *(.text.$Flash2)
        *(.text.$LITTLEFS_FLASH_region)
        *(.text_Flash2.*) /* for compatibility with previous releases */
        *(.text_LITTLEFS_FLASH_region.*) /* for compatibility with previous releases */
        *(.text.$Flash2.*)
        *(.text.$LITTLEFS_FLASH_region.*)
        *(.rodata.$Flash2)
        *(.rodata.$LITTLEFS_FLASH_region)
        *(.rodata.$Flash2.*)
        *(.rodata.$LITTLEFS_FLASH_region.*)            } > LITTLEFS_FLASH_region

    /* Image Vector Table and Boot Data for booting from external flash */
    .boot_hdr : ALIGN(4)
    {
        FILL(0x00)
        __boot_hdr_start__ = ABSOLUTE(.) ;
        . = 0x400 ;
        __flash_conf_hdr_start__ = ABSOLUTE(.) ;
        KEEP(*(.flash_conf))
        __flash_conf_hdr_end__ = ABSOLUTE(.) ;
        . = 0x1000 ;
         __boot_hdr_end__ = ABSOLUTE(.) ;
    } >BOARD_FLASH

    /* MAIN TEXT SECTION */
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data));
        LONG(  SIZEOF(.data));
        LONG(LOADADDR(.data_RAM2));
        LONG(    ADDR(.data_RAM2));
        LONG(  SIZEOF(.data_RAM2));
        LONG(LOADADDR(.data_RAM3));
        LONG(    ADDR(.data_RAM3));
        LONG(  SIZEOF(.data_RAM3));
        LONG(LOADADDR(.data_RAM4));
        LONG(    ADDR(.data_RAM4));
        LONG(  SIZEOF(.data_RAM4));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        LONG(    ADDR(.bss_RAM2));
        LONG(  SIZEOF(.bss_RAM2));
        LONG(    ADDR(.bss_RAM3));
        LONG(  SIZEOF(.bss_RAM3));
        LONG(    ADDR(.bss_RAM4));
        LONG(  SIZEOF(.bss_RAM4));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */

        *(.after_vectors*)

*(EXCLUDE_FILE(*fsl_adapter_flexspi_nor_flash.o *fsl_flexspi.o *mflash_drv.o) .text*)       KEEP(*freertos*/tasks.o(.rodata*)) /* FreeRTOS Debug Config */
       *(.rodata .rodata.* .constdata .constdata.*)
       . = ALIGN(4);
    } > BOARD_FLASH
    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this.
     */
    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > BOARD_FLASH

    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > BOARD_FLASH
 

  ._bt_settings_area :
  {
    . = ALIGN(4);
    _bt_settings_start = .;
    KEEP(*(SORT(._bt_settings.static.*)))
    _bt_settings_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

  ._settings_handler_static :
  {
    . = ALIGN(4);
    _settings_handler_static_list_start = .;
    KEEP(*(SORT(._settings_handler_static.static.*)))
    _settings_handler_static_list_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

  ._bt_conn_cb :
  {
    . = ALIGN(4);
    _bt_conn_cb_list_start = .;
    KEEP(*(SORT(._bt_conn_cb.static.*)))
    _bt_conn_cb_list_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

  ._bt_ias_cb :
  {
    . = ALIGN(4);
    _bt_ias_cb_list_start = .;
    KEEP(*(SORT(._bt_ias_cb.static.*)))
    _bt_ias_cb_list_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

  ._bt_gatt_service_static :
  {
    . = ALIGN(4);
    _bt_gatt_service_static_list_start = .;
    KEEP(*(SORT(._bt_gatt_service_static.static.*)))
    _bt_gatt_service_static_list_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

  ._bt_l2cap_fixed_chan :
  {
    . = ALIGN(4);
    _bt_l2cap_fixed_chan_list_start = .;
    KEEP(*(SORT(._bt_l2cap_fixed_chan.static.*)))
    _bt_l2cap_fixed_chan_list_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

  ._bt_l2cap_br_fixed_chan :
  {
    . = ALIGN(4);
    _bt_l2cap_br_fixed_chan_list_start = .;
    KEEP(*(SORT(._bt_l2cap_br_fixed_chan.static.*)))
    _bt_l2cap_br_fixed_chan_list_end = .;
    . = ALIGN(4);
  } > BOARD_FLASH

    _etext = .;

  .LITTLEFS_FLASH_region :
  {
    . = ORIGIN(LITTLEFS_FLASH_region) + LENGTH(LITTLEFS_FLASH_region) - 1;
  } > LITTLEFS_FLASH_region

 
    /* USB_RAM */
    .m_usb_data (NOLOAD) :
    {
        *(m_usb_global)
    } > USB_RAM AT> USB_RAM
    /* DATA section for Audio_IO_Data */

    .data_RAM2 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM2 = .) ;
        PROVIDE(__start_data_Audio_IO_Data = .) ;
        *(.ramfunc.$RAM2)
        *(.ramfunc.$Audio_IO_Data)
        *(.data.$RAM2)
        *(.data.$Audio_IO_Data)
        *(.data.$RAM2.*)
        *(.data.$Audio_IO_Data.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM2 = .) ;
        PROVIDE(__end_data_Audio_IO_Data = .) ;
     } > Audio_IO_Data AT>BOARD_FLASH

    /* DATA section for RamSharedWithDsp */

    .data_RAM3 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM3 = .) ;
        PROVIDE(__start_data_RamSharedWithDsp = .) ;
        *(.ramfunc.$RAM3)
        *(.ramfunc.$RamSharedWithDsp)
        *(.data.$RAM3)
        *(.data.$RamSharedWithDsp)
        *(.data.$RAM3.*)
        *(.data.$RamSharedWithDsp.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM3 = .) ;
        PROVIDE(__end_data_RamSharedWithDsp = .) ;
     } > RamSharedWithDsp AT>BOARD_FLASH

    /* DATA section for USB_RAM */

    .data_RAM4 : ALIGN(4)
    {
        FILL(0xff)
        PROVIDE(__start_data_RAM4 = .) ;
        PROVIDE(__start_data_USB_RAM = .) ;
        *(.ramfunc.$RAM4)
        *(.ramfunc.$USB_RAM)
        *(.data.$RAM4)
        *(.data.$USB_RAM)
        *(.data.$RAM4.*)
        *(.data.$USB_RAM.*)
        . = ALIGN(4) ;
        PROVIDE(__end_data_RAM4 = .) ;
        PROVIDE(__end_data_USB_RAM = .) ;
     } > USB_RAM AT>BOARD_FLASH

    /* MAIN DATA SECTION */
    .uninit_RESERVED (NOLOAD) : ALIGN(4)
    {
        _start_uninit_RESERVED = .;
        KEEP(*(.bss.$RESERVED*))
       . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > SRAM AT> SRAM

    /* Main DATA section (SRAM) */
    .data : ALIGN(4)
    {
       FILL(0xff)
       _data = . ;
       PROVIDE(__start_data_RAM = .) ;
       PROVIDE(__start_data_SRAM = .) ;
       *(vtable)
       *(.ramfunc*)
       KEEP(*(CodeQuickAccess))
       KEEP(*(DataQuickAccess))
       *(RamFunction)
	    *(.data*)
	    . = ALIGN(4);
	    _net_buf_pool_list_start = .;
	    KEEP(*(SORT(._net_buf_pool*)))
	    _net_buf_pool_list_end = .;
	    . = ALIGN(4);
	    _k_mem_slab_list_start = .;
	    KEEP(*(SORT(._k_mem_slab*)))
	    _k_mem_slab_list_end = .;
	    . = ALIGN(4);
                *fsl_adapter_flexspi_nor_flash.o(.text .text* .rodata .rodata*)
        *fsl_flexspi.o(.text .text* .rodata .rodata*)
        *mflash_drv.o(.text .text* .rodata .rodata*)
       . = ALIGN(4) ;
       _edata = . ;
       PROVIDE(__end_data_RAM = .) ;
       PROVIDE(__end_data_SRAM = .) ;
    } > SRAM AT>BOARD_FLASH

    /* BSS section for Audio_IO_Data */
    .bss_RAM2 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM2 = .) ;
       PROVIDE(__start_bss_Audio_IO_Data = .) ;
       *(.bss.$RAM2)
       *(.bss.$Audio_IO_Data)
       *(.bss.$RAM2.*)
       *(.bss.$Audio_IO_Data.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM2 = .) ;
       PROVIDE(__end_bss_Audio_IO_Data = .) ;
    } > Audio_IO_Data AT> Audio_IO_Data

    /* BSS section for RamSharedWithDsp */
    .bss_RAM3 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM3 = .) ;
       PROVIDE(__start_bss_RamSharedWithDsp = .) ;
       *(.bss.$RAM3)
       *(.bss.$RamSharedWithDsp)
       *(.bss.$RAM3.*)
       *(.bss.$RamSharedWithDsp.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM3 = .) ;
       PROVIDE(__end_bss_RamSharedWithDsp = .) ;
    } > RamSharedWithDsp AT> RamSharedWithDsp

    /* BSS section for USB_RAM */
    .bss_RAM4 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_bss_RAM4 = .) ;
       PROVIDE(__start_bss_USB_RAM = .) ;
       *(.bss.$RAM4)
       *(.bss.$USB_RAM)
       *(.bss.$RAM4.*)
       *(.bss.$USB_RAM.*)
       . = ALIGN (. != 0 ? 4 : 1) ; /* avoid empty segment */
       PROVIDE(__end_bss_RAM4 = .) ;
       PROVIDE(__end_bss_USB_RAM = .) ;
    } > USB_RAM AT> USB_RAM

    /* MAIN BSS SECTION */
    .bss (NOLOAD) : ALIGN(4)
    {
        _bss = .;
        PROVIDE(__start_bss_RAM = .) ;
        PROVIDE(__start_bss_SRAM = .) ;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(__end_bss_RAM = .) ;
        PROVIDE(__end_bss_SRAM = .) ;
        PROVIDE(end = .);
    } > SRAM AT> SRAM

    /* NOINIT section for Audio_IO_Data */
    .noinit_RAM2 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM2 = .) ;
       PROVIDE(__start_noinit_Audio_IO_Data = .) ;
       *(.noinit.$RAM2)
       *(.noinit.$Audio_IO_Data)
       *(.noinit.$RAM2.*)
       *(.noinit.$Audio_IO_Data.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM2 = .) ;
       PROVIDE(__end_noinit_Audio_IO_Data = .) ;
    } > Audio_IO_Data AT> Audio_IO_Data

    /* NOINIT section for RamSharedWithDsp */
    .noinit_RAM3 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM3 = .) ;
       PROVIDE(__start_noinit_RamSharedWithDsp = .) ;
       *(.noinit.$RAM3)
       *(.noinit.$RamSharedWithDsp)
       *(.noinit.$RAM3.*)
       *(.noinit.$RamSharedWithDsp.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM3 = .) ;
       PROVIDE(__end_noinit_RamSharedWithDsp = .) ;
    } > RamSharedWithDsp AT> RamSharedWithDsp

    /* NOINIT section for USB_RAM */
    .noinit_RAM4 (NOLOAD) : ALIGN(4)
    {
       PROVIDE(__start_noinit_RAM4 = .) ;
       PROVIDE(__start_noinit_USB_RAM = .) ;
       *(.noinit.$RAM4)
       *(.noinit.$USB_RAM)
       *(.noinit.$RAM4.*)
       *(.noinit.$USB_RAM.*)
       . = ALIGN(4) ;
       PROVIDE(__end_noinit_RAM4 = .) ;
       PROVIDE(__end_noinit_USB_RAM = .) ;
    } > USB_RAM AT> USB_RAM

    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        PROVIDE(__start_noinit_RAM = .) ;
        PROVIDE(__start_noinit_SRAM = .) ;
        *(.noinit*)
         . = ALIGN(4) ;
        _end_noinit = .;
       PROVIDE(__end_noinit_RAM = .) ;
       PROVIDE(__end_noinit_SRAM = .) ;        
    } > SRAM AT> SRAM

    /* Reserve and place Heap within memory map */
    _HeapSize = 0x1000;
    .heap (NOLOAD) :  ALIGN(4)
    {
        _pvHeapStart = .;
        . += _HeapSize;
        . = ALIGN(4);
        _pvHeapLimit = .;
    } > SRAM

     _StackSize = 0x1000;
     /* Reserve space in memory for Stack */
    .heap2stackfill (NOLOAD) :
    {
        . += _StackSize;
    } > SRAM
    /* Locate actual Stack in memory map */
    .stack ORIGIN(SRAM) + LENGTH(SRAM) - _StackSize - 0 (NOLOAD) :  ALIGN(4)
    {
        _vStackBase = .;
        . = ALIGN(4);
        _vStackTop = . + _StackSize;
    } > SRAM

    /* Provide basic symbols giving location and size of main text
     * block, including initial values of RW data sections. Note that
     * these will need extending to give a complete picture with
     * complex images (e.g multiple Flash banks).
     */
    _image_start = LOADADDR(.text);
    _image_end = LOADADDR(.data) + SIZEOF(.data);
    _image_size = _image_end - _image_start;

    /* Provide symbols for MIMXRT600 parts for startup code to use
     * to set image to be plain load image or XIP.
     * Config : Plain load image = false
     */
    __imghdr_loadaddress = LOADADDR(.text);
    __imghdr_imagetype = 0;

    /* Linker Symbols */

    /*** littleFS configuration Start ***/
    EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS = __base_LITTLEFS_FLASH_region;
    EDGEFAST_BT_LITTLEFS_STORAGE_SECTOR_SIZE = 0x1000;  /* 4k flash sector size */
    EDGEFAST_BT_LITTLEFS_STORAGE_MAX_SECTORS = (__top_LITTLEFS_FLASH_region - EDGEFAST_BT_LITTLEFS_STORAGE_START_ADDRESS) / EDGEFAST_BT_LITTLEFS_STORAGE_SECTOR_SIZE;
    /*** littleFS configuration End ***/
}