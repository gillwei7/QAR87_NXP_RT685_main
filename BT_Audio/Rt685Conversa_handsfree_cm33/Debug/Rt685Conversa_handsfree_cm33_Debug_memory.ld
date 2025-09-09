/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * Generated linker script file for MIMXRT685S
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v24.12 [Build 148] [2025-01-10] on 2025年9月9日 下午2:54:36
 */

MEMORY
{
  /* Define each memory region */
  BOARD_FLASH (rx) : ORIGIN = 0x8000000, LENGTH = 0x800000 /* 8M bytes (alias Flash) */  
  LITTLEFS_FLASH_region (rx) : ORIGIN = 0x8800000, LENGTH = 0x800000 /* 8M bytes (alias Flash2) */  
  SRAM (rwx) : ORIGIN = 0x80000, LENGTH = 0x180000 /* 1536K bytes (alias RAM) */  
  Audio_IO_Data (rwx) : ORIGIN = 0x20000000, LENGTH = 0x40000 /* 256K bytes (alias RAM2) */  
  RamSharedWithDsp (rwx) : ORIGIN = 0x20060000, LENGTH = 0x20000 /* 128K bytes (alias RAM3) */  
  USB_RAM (rwx) : ORIGIN = 0x40140000, LENGTH = 0x4000 /* 16K bytes (alias RAM4) */  
}

  /* Define a symbol for the top of each memory region */
  __base_BOARD_FLASH = 0x8000000  ; /* BOARD_FLASH */  
  __base_Flash = 0x8000000 ; /* Flash */  
  __top_BOARD_FLASH = 0x8000000 + 0x800000 ; /* 8M bytes */  
  __top_Flash = 0x8000000 + 0x800000 ; /* 8M bytes */  
  __base_LITTLEFS_FLASH_region = 0x8800000  ; /* LITTLEFS_FLASH_region */  
  __base_Flash2 = 0x8800000 ; /* Flash2 */  
  __top_LITTLEFS_FLASH_region = 0x8800000 + 0x800000 ; /* 8M bytes */  
  __top_Flash2 = 0x8800000 + 0x800000 ; /* 8M bytes */  
  __base_SRAM = 0x80000  ; /* SRAM */  
  __base_RAM = 0x80000 ; /* RAM */  
  __top_SRAM = 0x80000 + 0x180000 ; /* 1536K bytes */  
  __top_RAM = 0x80000 + 0x180000 ; /* 1536K bytes */  
  __base_Audio_IO_Data = 0x20000000  ; /* Audio_IO_Data */  
  __base_RAM2 = 0x20000000 ; /* RAM2 */  
  __top_Audio_IO_Data = 0x20000000 + 0x40000 ; /* 256K bytes */  
  __top_RAM2 = 0x20000000 + 0x40000 ; /* 256K bytes */  
  __base_RamSharedWithDsp = 0x20060000  ; /* RamSharedWithDsp */  
  __base_RAM3 = 0x20060000 ; /* RAM3 */  
  __top_RamSharedWithDsp = 0x20060000 + 0x20000 ; /* 128K bytes */  
  __top_RAM3 = 0x20060000 + 0x20000 ; /* 128K bytes */  
  __base_USB_RAM = 0x40140000  ; /* USB_RAM */  
  __base_RAM4 = 0x40140000 ; /* RAM4 */  
  __top_USB_RAM = 0x40140000 + 0x4000 ; /* 16K bytes */  
  __top_RAM4 = 0x40140000 + 0x4000 ; /* 16K bytes */  
