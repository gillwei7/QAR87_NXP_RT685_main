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
#include "fsl_component_serial_manager.h"
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
#include "flash_partitioning.h"

#include "app_handsfree.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes (handsfree app)
 ******************************************************************************/
extern void BOARD_InitHardware(void);
extern void BOARD_InitDebugConsole(void);

/*******************************************************************************
 * Prototypes (from ota_mcuboot_basic.c)
 ******************************************************************************/
static shell_status_t shellCmd_image(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t shellCmd_xmodem(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t shellCmd_mem(shell_handle_t shellHandle, int32_t argc, char **argv);
static shell_status_t shellCmd_reboot(shell_handle_t shellHandle, int32_t argc, char **argv);

static int flash_sha256(uint32_t offset, size_t size, uint8_t sha256[32]);
static status_t get_fixed_secondary_partition_info(uint32_t image, partition_t *ptn);
static void uart2_serial_read_task(void *pvParameters);
static void uart2_mem_read_dump(uint32_t addr, size_t len);
static int uart2_program_one_page_to_flash(uint32_t amba_addr, uint8_t *data, size_t len);

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

static SHELL_COMMAND_DEFINE(reboot, "\n\"reboot\": Triggers software reset\n", shellCmd_reboot, 0);

/*
 * Buffer used to handover data from XMODEM to flash programming routine.
 * Uses 4B alignment to be compatible with mflash.
 */
static uint32_t s_otaProgbuf[1024 / sizeof(uint32_t)];

static hashctx_t s_sha256_xmodem_ctx;
static SERIAL_MANAGER_READ_HANDLE_DEFINE(s_uart2ReadHandleBuffer);

/* Max bytes requested per SerialManager_TryRead (keep 256 for RX ring / pacing). */
#define UART2_RX_BLOCK_BYTES (256U)
/* Accumulate this many UART bytes, then mem dump / program one NOR page / dump again. */
#define UART2_RX_ACCUM_BYTES (MFLASH_PAGE_SIZE)
/* Target AMBA address (same as BOOT_FLASH_CAND_APP in flash_partitioning.h). */
#define UART2_TEST_FLASH_AMBA_ADDR (BOOT_FLASH_CAND_APP)

SDK_ALIGN(static uint8_t s_uart2RxAccum[UART2_RX_ACCUM_BYTES], 4);

/*******************************************************************************
 * Code (from ota_mcuboot_basic.c)
 ******************************************************************************/

static void hexdump(const void *src, size_t size)
{
    static const char kHex[] = "0123456789ABCDEF";
    const unsigned char *src8 = src;
    char line[96];
    size_t pos = 0U;

    while (pos < size)
    {
        size_t n = size - pos;
        if (n > 16U)
        {
            n = 16U;
        }
        {
            char *p          = line;
            uint32_t addr    = (uint32_t)(uintptr_t)src8 + (uint32_t)pos;
            int hi;

            for (hi = 28; hi >= 0; hi -= 4)
            {
                *p++ = kHex[(addr >> (uint32_t)hi) & 0x0FU];
            }
            *p++ = ' ';
            *p++ = ' ';
            for (size_t j = 0; j < n; j++)
            {
                unsigned char b = src8[pos + j];
                *p++ = kHex[b >> 4];
                *p++ = kHex[b & 0x0FU];
                *p++ = ' ';
            }
            for (size_t j = n; j < 16U; j++)
            {
                *p++ = ' ';
                *p++ = ' ';
                *p++ = ' ';
            }
            *p++ = '|';
            for (size_t j = 0; j < n; j++)
            {
                int c = (int)src8[pos + j];
                *p++ = (char)(isprint(c) ? c : '.');
            }
            *p++ = '|';
            *p++ = '\r';
            *p++ = '\n';
            *p   = '\0';
            /* One PRINTF per line so other tasks cannot splice into the middle of a row. */
            PRINTF("%s", line);
        }
        pos += n;
    }
    PRINTF("\r\n");
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
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(mem));
    (void)SHELL_RegisterCommand(shellHandle, SHELL_COMMAND(reboot));
}

/* Same path as shell "mem read addr size" for direct AMBA pointer dump. */
static void uart2_mem_read_dump(uint32_t addr, size_t len)
{
#ifdef MFLASH_PAGE_INTEGRITY_CHECKS
    if (mflash_drv_is_readable(addr) != kStatus_Success)
    {
        PRINTF("mem read: page not readable at 0x%08X\r\n", addr);
        return;
    }
#endif
    PRINTF("mem read 0x%08X %u\r\n", addr, (unsigned int)len);
    hexdump((void *)(uintptr_t)addr, len);
}

/*!
 * @brief Erase sector containing @a amba_addr, then program exactly one page (@a len == MFLASH_PAGE_SIZE).
 */
static int uart2_program_one_page_to_flash(uint32_t amba_addr, uint8_t *data, size_t len)
{
    uint32_t off;
    int32_t st;

    if (amba_addr < MFLASH_BASE_ADDRESS)
    {
        PRINTF("flash: AMBA addr 0x%08X below MFLASH_BASE 0x%08X\r\n", amba_addr, (unsigned int)MFLASH_BASE_ADDRESS);
        return -1;
    }
    off = amba_addr - MFLASH_BASE_ADDRESS;
    if ((off & (MFLASH_PAGE_SIZE - 1U)) != 0U || len != MFLASH_PAGE_SIZE)
    {
        PRINTF("flash: need page-aligned addr and len=%u, got len=%u\r\n",
               (unsigned int)MFLASH_PAGE_SIZE,
               (unsigned int)len);
        return -1;
    }
    if (((uintptr_t)data & 3U) != 0U)
    {
        PRINTF("flash: data must be 4-byte aligned\r\n");
        return -1;
    }

    st = mflash_drv_sector_erase(off & ~(uint32_t)(MFLASH_SECTOR_SIZE - 1U));
    if (st != 0)
    {
        PRINTF("flash: sector_erase failed %d\r\n", (int)st);
        return (int)st;
    }

    st = mflash_drv_page_program(off, (uint32_t *)(void *)data);
    if (st != 0)
    {
        PRINTF("flash: page_program off=0x%X failed %d\r\n", (unsigned int)off, (int)st);
        return (int)st;
    }
    return 0;
}

/*******************************************************************************
 * Code (application main — unchanged flow)
 ******************************************************************************/

static void uart2_serial_read_task(void *pvParameters)
{
    serial_manager_status_t serialStatus;
    serial_read_handle_t readHandle = (serial_read_handle_t)s_uart2ReadHandleBuffer;
    uint32_t receivedLength;
    size_t total_filled;
    uint32_t try_len;
    static uint8_t s_mflash_inited;

    (void)pvParameters;

    serialStatus = SerialManager_OpenReadHandle(g_serialHandle, readHandle);
    if (serialStatus != kStatus_SerialManager_Success)
    {
        PRINTF("UART2 read task open handle failed: %d\r\n", (int)serialStatus);
        vTaskDelete(NULL);
    }

    for (;;)
    {
        total_filled = 0U;
        while (total_filled < UART2_RX_ACCUM_BYTES)
        {
            try_len = (uint32_t)(UART2_RX_ACCUM_BYTES - total_filled);
            if (try_len > UART2_RX_BLOCK_BYTES)
            {
                try_len = UART2_RX_BLOCK_BYTES;
            }
            receivedLength = 0U;
            if (SerialManager_TryRead(readHandle, &s_uart2RxAccum[total_filled], try_len, &receivedLength) ==
                    kStatus_SerialManager_Success &&
                receivedLength > 0U)
            {
                total_filled += (size_t)receivedLength;
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        PRINTF("\r\n--- UART accumulated %u bytes ---\r\n", (unsigned int)UART2_RX_ACCUM_BYTES);

        if (s_mflash_inited == 0U)
        {
            if (mflash_drv_init() != 0)
            {
                PRINTF("mflash_drv_init failed\r\n");
            }
            else
            {
                s_mflash_inited = 1U;
            }
        }

        PRINTF("--- Flash before program (mem read @ 0x%08X) ---\r\n", UART2_TEST_FLASH_AMBA_ADDR);
        uart2_mem_read_dump(UART2_TEST_FLASH_AMBA_ADDR, UART2_RX_ACCUM_BYTES);

        if (s_mflash_inited != 0U)
        {
            PRINTF("--- Programming %u bytes (1 page) to 0x%08X ---\r\n",
                   (unsigned int)UART2_RX_ACCUM_BYTES,
                   UART2_TEST_FLASH_AMBA_ADDR);
            if (uart2_program_one_page_to_flash(UART2_TEST_FLASH_AMBA_ADDR, s_uart2RxAccum, UART2_RX_ACCUM_BYTES) == 0)
            {
                PRINTF("--- Flash after program ---\r\n");
                uart2_mem_read_dump(UART2_TEST_FLASH_AMBA_ADDR, UART2_RX_ACCUM_BYTES);
            }
        }
    }
}

int main(void)
{
    BOARD_InitHardware();
    BOARD_InitDebugConsole();

    /* PUTCHAR -> DbgConsole_SendDataReliable (+ mutex/flush when TX_RELIABLE): needs more than *2 minimal stack. */
    if (xTaskCreate(uart2_serial_read_task, "uart2_read_task", configMINIMAL_STACK_SIZE * 8U, NULL, tskIDLE_PRIORITY + 1,
                    NULL) != pdPASS)
    {
        PRINTF("uart2 read task creation failed!\r\n");
        while (1)
            ;
    }

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
