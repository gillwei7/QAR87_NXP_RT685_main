/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * OTA/MCUBoot shell helpers below are copied from
 * evkmimxrt685_ota_mcuboot_basic/source/ota_mcuboot_basic.c (same repo).
 * Commands are registered on the app UART shell via app_mcuboot_shell_commands_register()
 * from app_shell_init() (same handle as the "bt" command).
 */

#include "fsl_debug_console.h"
#include "fsl_device_registers.h"
#include "fsl_shell.h"
#include "fsl_common.h"
#include <porting.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno/errno.h>
#include <stdbool.h>
#include <sys/atomic.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/slist.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/hfp_hf.h>
#include "FreeRTOS.h"
#include "task.h"

#include <ctype.h>

#include "mcuboot_app_support.h"
#include "mflash_drv.h"
#include "xmodem.h"
#include "platform_bindings.h"

#ifdef MCUBOOT_OTA_SB3_SUPPORT
#include "sb3_api.h"
#endif

#include "app_handsfree.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes (handsfree app)
 ******************************************************************************/
extern void BOARD_InitHardware(void);

/*******************************************************************************
 * Prototypes (from ota_mcuboot_basic.c)
 ******************************************************************************/
static shell_status_t shellCmd_image(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t shellCmd_xmodem(shell_handle_t shellHandle, int32_t argc, char **argv);
#ifdef MCUBOOT_OTA_SB3_SUPPORT
static shell_status_t shellCmd_xmodem_sb3(shell_handle_t shellHandle, int32_t argc, char **argv);
#endif
static shell_status_t shellCmd_mem(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t shellCmd_reboot(shell_handle_t shellHandle, int32_t argc, char **argv);

static int flash_sha256(uint32_t offset, size_t size, uint8_t sha256[32]);
static status_t get_fixed_secondary_partition_info(uint32_t image, partition_t *ptn);

/*******************************************************************************
 * Variables (from ota_mcuboot_basic.c; not used by main() until integrated)
 ******************************************************************************/

static SHELL_COMMAND_DEFINE(image,
                            "\n\"image [info]\"          : Print image information"
                            "\n\"image test [imgNum]\"   : Mark candidate slot of given image number as ready for test"
                            "\n\"image accept [imgNum]\" : Mark active slot of given image number as accepted"
                            "\n\"image erase [imgNum]\"  : Erase candidate slot of given image number"
                            "\n",
                            shellCmd_image,
                            SHELL_IGNORE_PARAMETER_COUNT);

static SHELL_COMMAND_DEFINE(mem,
                            "\n\"mem read addr [size]\" : Read memory at given address"
                            "\n\"mem erase addr \"      : Erase sector containing given address"
                            "\n",
                            shellCmd_mem,
                            SHELL_IGNORE_PARAMETER_COUNT);

static SHELL_COMMAND_DEFINE(xmodem, "\n\"xmodem [imgNum]\": Start receiving with XMODEM-CRC\n", shellCmd_xmodem,
                            SHELL_IGNORE_PARAMETER_COUNT);
#ifdef MCUBOOT_OTA_SB3_SUPPORT
static SHELL_COMMAND_DEFINE(xmodem_sb3, "\n\"xmodem_sb3\": Start SB3 receiving with XMODEM-CRC\n", shellCmd_xmodem_sb3,
                            SHELL_IGNORE_PARAMETER_COUNT);
#endif
static SHELL_COMMAND_DEFINE(reboot, "\n\"reboot\": Triggers software reset\n", shellCmd_reboot, 0);

/*
 * Buffer used to handover data from XMODEM to flash programming routine.
 * Uses 4B alignment to be compatible with mflash.
 */
static uint32_t s_otaProgbuf[1024 / sizeof(uint32_t)];

static hashctx_t s_sha256_xmodem_ctx;

/*******************************************************************************
 * Code (from ota_mcuboot_basic.c)
 ******************************************************************************/

static void hexdump(const void *src, size_t size)
{
    const unsigned char *src8 = src;
    const int CNT = 16;

    for (size_t i = 0; i < size; i++)
    {
        int n = (int)(i % (size_t)CNT);
        if (n == 0)
        {
            PRINTF("%08x  ", (uint32_t)src + (uint32_t)i);
        }
        PRINTF("%02X ", src8[i]);
        if ((i && n == CNT - 1) || (i + 1 == size))
        {
            int rem = CNT - 1 - n;
            for (int j = 0; j < rem; j++)
            {
                PRINTF("   ");
            }
            PRINTF("|");
            for (int j = n; j >= 0; j--)
            {
                PUTCHAR(isprint((int)src8[i - (size_t)j]) ? (int)src8[i - (size_t)j] : '.');
            }
            PRINTF("|\n");
        }
    }
    PUTCHAR('\n');
}

static void print_hash(const void *src, size_t size)
{
    const unsigned char *src8 = src;
    for (size_t i = 0; i < size; i++)
    {
        PRINTF("%02X", src8[i]);
    }
}

static shell_status_t shellCmd_image(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    int image = 0;
    int ret;
    status_t status;
    uint32_t imgstate;

    (void)shellHandle;

    if (argc > 3)
    {
        PRINTF("Too many arguments.\n");
        return kStatus_SHELL_Error;
    }

    /* image [info] */

    if (argc == 1 || (argc == 2 && !strcmp(argv[1], "info")))
    {
        bl_print_image_info(flash_sha256);
        return kStatus_SHELL_Success;
    }

    if (argc < 2)
    {
        PRINTF("Wrong arguments. See 'help'\n");
        return kStatus_SHELL_Error;
    }

    if (argc == 3)
    {
        char *parse_end;
        image = (int)strtol(argv[2], &parse_end, 10);

        if (image < 0 || image >= MCUBOOT_IMAGE_NUMBER || *parse_end != '\0')
        {
            PRINTF("Wrong image number.\n");
            return kStatus_SHELL_Error;
        }
    }

    status = bl_get_image_state(image, &imgstate);
    if (status != kStatus_Success)
    {
        PRINTF("Failed to get state of image %u (status %d)", image, status);
        return kStatus_SHELL_Error;
    }

    /* image test [imgNum] */

    if (!strcmp(argv[1], "test"))
    {
        status = bl_update_image_state(image, kSwapType_ReadyForTest);
        if (status != kStatus_Success)
        {
            PRINTF("FAILED to mark image state as ReadyForTest (status=%d)\n", status);
            return kStatus_SHELL_Error;
        }
    }

    /* image accept [imgNum] */

    else if (!strcmp(argv[1], "accept"))
    {
        if (imgstate != kSwapType_Testing)
        {
            PRINTF("Image state is not set as Testing. Nothing to accept.\n", status);
            return kStatus_SHELL_Error;
        }

        status = bl_update_image_state(image, kSwapType_Permanent);
        if (status != kStatus_Success)
        {
            PRINTF("FAILED to accept image (status=%d)\n", status);
            return kStatus_SHELL_Error;
        }
    }

    /* image erase [imgNum] */

    else if (!strcmp(argv[1], "erase"))
    {
        partition_t ptn;

        ret = bl_get_update_partition_info(image, &ptn);
        if (ret != kStatus_Success)
        {
            PRINTF("Failed to determine update partition\n");
            return kStatus_SHELL_Error;
        }

        uint32_t slotaddr = ptn.start;
        uint32_t slotsize = ptn.size;
        uint32_t slotcnt  = (slotsize - 1 + MFLASH_SECTOR_SIZE) / MFLASH_SECTOR_SIZE;

        PRINTF("Erasing inactive slot...");
        for (int i = 0; i < (int)slotcnt; i++)
        {
            ret = mflash_drv_sector_erase(slotaddr);
            if (ret)
            {
                PRINTF("\nFailed to erase sector at 0x%x (ret=%d)\n", slotaddr, ret);
                return kStatus_SHELL_Error;
            }
            slotaddr += MFLASH_SECTOR_SIZE;
        }
        PRINTF("done\n");
    }

    else
    {
        PRINTF("Wrong arguments. See 'help'\n");
        return kStatus_SHELL_Error;
    }

    return kStatus_SHELL_Success;
}

static shell_status_t shellCmd_mem(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    int ret;
    uint32_t addr;
    uint32_t size = 128;
    char *parse_end;

    (void)shellHandle;

    if (argc < 3 || argc > 4)
    {
        PRINTF("Wrong argument count\n");
        return kStatus_SHELL_Error;
    }

    addr = (uint32_t)strtol(argv[2], &parse_end, 0);
    if (*parse_end != '\0')
    {
        PRINTF("Bad address\n");
        return kStatus_SHELL_Error;
    }

    if (argc == 4)
    {
        size = (uint32_t)strtol(argv[3], &parse_end, 0);
        if (*parse_end != '\0')
        {
            PRINTF("Bad size\n");
            return kStatus_SHELL_Error;
        }
    }

    /* mem read addr [size] */

    if (!strcmp(argv[1], "read"))
    {
#ifdef MFLASH_PAGE_INTEGRITY_CHECKS
        if (mflash_drv_is_readable(addr) != kStatus_Success)
        {
            PRINTF("Page not readable\n");
            return kStatus_SHELL_Error;
        }
#endif
        hexdump((void *)(uintptr_t)addr, (size_t)size);
    }

    /* mem erase addr */

    else if (!strcmp(argv[1], "erase"))
    {
        ret = mflash_drv_sector_erase(addr & ~(MFLASH_SECTOR_SIZE - 1));
        if (ret)
        {
            PRINTF("Failed to erase sector (ret=%d)\n", ret);
            return kStatus_SHELL_Error;
        }
    }

    else
    {
        PRINTF("Wrong arguments. See 'help'\n");
        return kStatus_SHELL_Error;
    }

    return kStatus_SHELL_Success;
}

static int process_received_data(uint32_t dst_addr, uint32_t offset, uint32_t size)
{
    int ret;
    uint32_t *data = s_otaProgbuf;
    uint32_t addr   = dst_addr + offset;

    /* 1kB programming buffer should be ok with all page size alignments */
    while (size != 0U)
    {
        size_t chunk = (size < MFLASH_PAGE_SIZE) ? size : MFLASH_PAGE_SIZE;

        ret = mflash_drv_page_program(addr, data);
        if (ret)
        {
            PRINTF("Failed to program flash at %x (ret %d)\n", addr, ret);
            return -1;
        }

        sha256_update(&s_sha256_xmodem_ctx, data, chunk);
        addr += (uint32_t)chunk;
        data += chunk / sizeof(uint32_t);
        size -= (uint32_t)chunk;
    }

    return 0;
}

#ifdef MCUBOOT_OTA_SB3_SUPPORT
static int process_received_data_sb3(uint32_t dst_addr, uint32_t offset, uint32_t size)
{
    int ret;
    uint32_t *data = s_otaProgbuf;
    uint32_t chunk_sz;

    static uint32_t sb_size;
    static uint32_t bytes_processed;

    (void)dst_addr;

    if (offset == 0)
    {
        /* first chunk */
        if (!sb3_parse_header(data, &sb_size))
        {
            return -1;
        }
        bytes_processed = 0;
    }

    if (sb_size == bytes_processed)
    {
        /* just in case */
        return -1;
    }

    if (sb_size - bytes_processed > size)
    {
        chunk_sz = size;
    }
    else
    {
        /* last chunk */
        chunk_sz = sb_size - bytes_processed;
    }

    /* Processing SB3 image */
    ret = sb3_api_pump((uint8_t *)data, chunk_sz);
    if (ret != kStatus_Success)
    {
        PRINTF("sb3_api_pump failed/n");
        return -1;
    }

    bytes_processed += chunk_sz;
    return 0;
}
#endif

static shell_status_t shellCmd_xmodem(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    int image = 0;
    long recvsize;
    uint8_t sha256_recv[32], sha256_flash[32];
    partition_t prt_ota;

    (void)shellHandle;

    if (argc > 3)
    {
        PRINTF("Too many arguments.\n");
        return kStatus_SHELL_Error;
    }

    if (argc == 3)
    {
        char *parse_end;
        image = (int)strtol(argv[2], &parse_end, 10);

        if (image < 0 || image >= MCUBOOT_IMAGE_NUMBER || *parse_end != '\0')
        {
            PRINTF("Wrong image number.\n");
            return kStatus_SHELL_Error;
        }
    }

    if (get_fixed_secondary_partition_info((uint32_t)image, &prt_ota) != kStatus_Success)
    {
        PRINTF("FAILED to determine address for download\n");
        return kStatus_SHELL_Error;
    }

    PRINTF("XMODEM target slot id: %u\n", (unsigned int)FLASH_AREA_IMAGE_SECONDARY((uint32_t)image));
    PRINTF("XMODEM target start (offset):   0x%08X\n", (unsigned int)prt_ota.start);
    PRINTF("XMODEM target start (physical): 0x%08X\n", (unsigned int)(BOOT_FLASH_BASE + prt_ota.start));
    PRINTF("XMODEM target size : 0x%08X (%u bytes)\n", (unsigned int)prt_ota.size, (unsigned int)prt_ota.size);
    PRINTF("Started xmodem download into flash (offset 0x%X, physical 0x%X)\n",
           (unsigned int)prt_ota.start,
           (unsigned int)(BOOT_FLASH_BASE + prt_ota.start));

    struct xmodem_cfg cfg = {
        .putc               = xmodem_putc,
        .getc               = xmodem_getc,
        .canread            = xmodem_canread,
        .canread_retries    = xmodem_canread_retries,
        .dst_addr           = prt_ota.start,
        .maxsize            = prt_ota.size,
        .buffer             = (uint8_t *)s_otaProgbuf,
        .buffer_size        = sizeof(s_otaProgbuf),
        .buffer_full_callback = process_received_data,
    };

    sha256_init(&s_sha256_xmodem_ctx);

    PRINTF("Initiated XMODEM-CRC transfer. Receiving... (Press 'x' to cancel)\n");

    recvsize = xmodem_receive(&cfg);

    /* With some terminals it takes a while before they recover receiving to the console */
    SDK_DelayAtLeastUs(1000000, SystemCoreClock);

    if (recvsize < 0)
    {
        PRINTF("\nTransfer failed (%d)\n", (int)recvsize);
        return kStatus_SHELL_Error;
    }
    PRINTF("\nReceived %u bytes\n", (unsigned int)recvsize);

    sha256_finish(&s_sha256_xmodem_ctx, sha256_recv);
    flash_sha256(prt_ota.start, (size_t)recvsize, sha256_flash);

    PRINTF("SHA256 of received data: ");
    print_hash(sha256_recv, 10);
    PRINTF("...\n");

    PRINTF("SHA256 of flashed data:  ");
    print_hash(sha256_flash, 10);
    PRINTF("...\n");

    if (bl_update_image_state((uint32_t)image, kSwapType_ReadyForTest) != kStatus_Success)
    {
        PRINTF("FAILED to mark image as ReadyForTest.\n");
        return kStatus_SHELL_Error;
    }
    PRINTF("Image marked ReadyForTest. Run reboot to switch to downloaded image.\n");

    return kStatus_SHELL_Success;
}

#ifdef MCUBOOT_OTA_SB3_SUPPORT
static shell_status_t shellCmd_xmodem_sb3(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    long recvsize;
    partition_t prt_ota;

    (void)shellHandle;

    if (argc > 2)
    {
        PRINTF("Too many arguments.\n");
        return kStatus_SHELL_Error;
    }

    if (get_fixed_secondary_partition_info(0, &prt_ota) != kStatus_Success)
    {
        PRINTF("FAILED to determine address for download\n");
        return kStatus_SHELL_Error;
    }

    PRINTF("XMODEM SB3 target slot id: %u\n", (unsigned int)FLASH_AREA_IMAGE_SECONDARY(0));
    PRINTF("XMODEM SB3 target start (offset):   0x%08X\n", (unsigned int)prt_ota.start);
    PRINTF("XMODEM SB3 target start (physical): 0x%08X\n", (unsigned int)(BOOT_FLASH_BASE + prt_ota.start));
    PRINTF("XMODEM SB3 target size : 0x%08X (%u bytes)\n", (unsigned int)prt_ota.size, (unsigned int)prt_ota.size);

    /* Todo add provisioning check */
    if (sb3_api_init() != kStatus_Success)
    {
        PRINTF("sb3_iap_init failed/n");
        return kStatus_SHELL_Error;
    }

    if (sb3_check_provisioning(false) == 0)
    {
        PRINTF("WARNING! Device doesn't seem to be configured properly! Check instructions in readme file.\n");
    }

    PRINTF("Started xmodem processing SB3\n");
    PRINTF("Make sure this device is provisioned to accept secure binary and its load address is 0x%X\n", prt_ota.start);

    struct xmodem_cfg cfg = {
        .putc               = xmodem_putc,
        .getc               = xmodem_getc,
        .canread            = xmodem_canread,
        .canread_retries    = xmodem_canread_retries,
        .dst_addr           = prt_ota.start,
        .maxsize            = prt_ota.size,
        .buffer             = (uint8_t *)s_otaProgbuf,
        .buffer_size        = sizeof(s_otaProgbuf),
        .buffer_full_callback = process_received_data_sb3,
    };

    sha256_init(&s_sha256_xmodem_ctx);

    PRINTF("Initiated XMODEM-CRC transfer. Receiving... (Press 'x' to cancel)\n");

    recvsize = xmodem_receive(&cfg);

    SDK_DelayAtLeastUs(1000000, SystemCoreClock);

    if (recvsize < 0)
    {
        PRINTF("\nTransfer failed (%d)\n", (int)recvsize);
        return kStatus_SHELL_Error;
    }
    PRINTF("\nReceived %u bytes\n", (unsigned int)recvsize);

    PRINTF("SB3 has been processed\n");
    sb3_api_finalize();
    sb3_api_deinit();

    return kStatus_SHELL_Success;
}
#endif

static shell_status_t shellCmd_reboot(shell_handle_t shellHandle, int32_t argc, char **argv)
{
    (void)shellHandle;
    (void)argc;
    (void)argv;

    PRINTF("System reset!\n");
    NVIC_SystemReset();

    /* return kStatus_SHELL_Success; */
}

static int flash_sha256(uint32_t offset, size_t size, uint8_t sha256[32])
{
    uint32_t buf[128 / sizeof(uint32_t)];
    status_t status;
    hashctx_t sha256ctx;

    sha256_init(&sha256ctx);

    while (size > 0)
    {
        size_t chunk = (size > sizeof(buf)) ? sizeof(buf) : size;
        /* mflash demands size to be in multiples of 4 */
        size_t chunkAlign4 = (chunk + 3) & (~(size_t)3);

        status = mflash_drv_read(offset, buf, chunkAlign4);
        if (status != kStatus_Success)
        {
            return (int)status;
        }

        sha256_update(&sha256ctx, (unsigned char *)buf, chunk);

        size -= chunk;
        offset += (uint32_t)chunk;
    }

    sha256_finish(&sha256ctx, sha256);

    return kStatus_Success;
}

static status_t get_fixed_secondary_partition_info(uint32_t image, partition_t *ptn)
{
    uint32_t faid;

    if ((ptn == NULL) || (image >= MCUBOOT_IMAGE_NUMBER))
    {
        return kStatus_InvalidArgument;
    }

    faid = FLASH_AREA_IMAGE_SECONDARY(image);
    if (faid >= MCUBOOT_IMAGE_SLOT_NUMBER)
    {
        return kStatus_Fail;
    }

    ptn->start = boot_flash_map[faid].fa_off;
    ptn->size  = boot_flash_map[faid].fa_size;

    return kStatus_Success;
}

void app_mcuboot_shell_commands_register(shell_handle_t shellHandle)
{
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(image));
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(xmodem));
#ifdef MCUBOOT_OTA_SB3_SUPPORT
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(xmodem_sb3));
#endif
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(mem));
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(reboot));
}

/*******************************************************************************
 * Code (application main — unchanged flow)
 ******************************************************************************/

int main(void)
{
    BOARD_InitHardware();

    if (xTaskCreate(hfp_hf_a2dp_task, "hfp_hf_a2dp_task", configMINIMAL_STACK_SIZE * 8, NULL, /*was configMINIMAL_STACK_SIZE * 8*/
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        PRINTF("pherial hrs task creation failed!\r\n");
        while (1)
            ;
    }

    vTaskStartScheduler();
    for (;;)
        ;
}

/*

shell commands: ---- in UART

bt connect_paired 1

bt playopus 0
bt playopus 1
bt playopus 2
bt playsbc 0
bt playsbc 1
bt playsbc 2



shell commands: ---- in USB com

//--- to play opus file ---
a 0
a 1
a 2

//--- to play sbc file ---
b 0
b 1
b 2

*/

/*

Fc ports in this project:

ToAMp: fc1 ToAmp, fc3 FrAMP  (fc1 clk share to fc3), fc1 is master, AMP is slave
ToBt:  fc4 FrBT,  fc2 ToBT   (fc2 clk share to fc4), fc2 is slave, BT is master
ToNvt: fc5 toNvt, fc6 FrNvt  (fc5 clk share to fc6), fc5 is master, NVT is slave

*/

/*

know bug:

play a2dp
make a tel incoming call.
Reject the call
it goes back to a2dp play.
but, the playing is broken. The printing info shows the sbc buffer goes less and less, while 1000 frame period is good --> DSP side is eating sbc too quickly????????  Later to find time to fix !!!





*/
