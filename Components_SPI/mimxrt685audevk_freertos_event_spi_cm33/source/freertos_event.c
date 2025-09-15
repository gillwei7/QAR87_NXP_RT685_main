/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "semphr.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"
#include "pmic_support.h"
#include "pmic_pca9422.h"

#include "fsl_spi.h"
#include <string.h>
#include <stdlib.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define USE_EVENT 1
#define USE_SEMAPHORE 0

#define SCAN_I2C_ADDRESS_ENABLE 1

/*======<SPI>=======*/
#define FIXED_BUFFER_SIZE (4)
#define NUM_FRAMES (3)

#define MAX_FRAME_SIZE 64
#define TRIGGER_HEX_VALUE 0x92
#define TRIGGER_FRAME2_VALUE 0x23

static uint8_t g_special_toggle_flag = 0;

/* ============= 被動模式相關定義 ============= */
typedef enum {
    MODE_PASSIVE_IDLE,      // 被動等待模式（GPIO 低位，等待接收）
    MODE_PASSIVE_ACK,       // 被動 ACK 模式（準備接收第二個 frame）
    MODE_ACTIVE             // 主動傳輸模式（GPIO 高位）
} spi_operation_mode_t;

static volatile spi_operation_mode_t operation_mode = MODE_PASSIVE_IDLE;
static uint8_t passive_rx_buffer[FIXED_BUFFER_SIZE] = {0};  // 保存被動模式收到的 [AA 11 11 CS]
static uint8_t passive_ff_buffer[FIXED_BUFFER_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};  // 被動模式回傳用
static volatile bool passive_mode_busy = false;  // 表示被動模式正在處理中

/* 三組數據緩衝區 */
static uint8_t dataFrame1[FIXED_BUFFER_SIZE] = {0xAA, 0x00, 0x04, 0x00};
static uint8_t dataFrame2[FIXED_BUFFER_SIZE] = {0xAA, 0x00, 0x21, 0x00};
static uint8_t dataFrame3[FIXED_BUFFER_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t dynamicFrameBuffer[MAX_FRAME_SIZE] = {0};
static uint8_t destBuff[MAX_FRAME_SIZE] = {0};
static volatile uint32_t currentFrameSize = 0;

static uint32_t txIndex = 0;
static uint32_t rxIndex = 0;
static volatile bool spi_irq_enabled = false;

static volatile uint8_t currentFrame = 0;
static uint8_t* currentSrcBuff = NULL;

/* event bits */
#define EVT_TRANSFER_DONE (1UL << 0)
#define EVT_ALL_FRAMES_DONE (1UL << 1)
#define EVT_PASSIVE_RX_DONE (1UL << 2)
#define EVT_PASSIVE_NEED_INIT (1UL << 3)

static EventGroupHandle_t spi_event_group = NULL;
static SemaphoreHandle_t spi_semaphore = NULL;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void console_task(void *pvParameters);
static void spi_task(void *pvParameters);
static void passive_handler_task(void *pvParameters);
static void SPI_StartFrame_PreloadTX(uint8_t* srcBuff, uint32_t frameSize);
static inline void SPI_EnableRxTxInterrupt(void);
static inline void SPI_DisableRxTxInterrupt(void);
static uint8_t calculateChecksum(uint8_t* data, uint8_t len);
static void init_passive_mode(void);

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint8_t calculateChecksum(uint8_t* data, uint8_t len)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (byte & (1 << j))
            {
                count++;
            }
        }
    }
    return count;
}

static inline void SPI_EnableRxTxInterrupt(void)
{
    SPI_EnableInterrupts(EXAMPLE_SPI_SLAVE, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
    spi_irq_enabled = true;
    if (operation_mode == MODE_ACTIVE) {
        PRINTF("SPI Enable() - Frame %d\r\n", currentFrame + 1);
    }
}

static inline void SPI_DisableRxTxInterrupt(void)
{
    SPI_DisableInterrupts(EXAMPLE_SPI_SLAVE, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
    spi_irq_enabled = false;
    if (operation_mode == MODE_ACTIVE) {
        PRINTF("SPI Disable() - Frame %d complete\r\n", currentFrame + 1);
    }
}

static void SPI_StartFrame_PreloadTX(uint8_t* srcBuff, uint32_t frameSize)
{
    /* Clear RX FIFO */
    while (SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_RxNotEmptyFlag)
    {
        (void)SPI_ReadData(EXAMPLE_SPI_SLAVE);
    }

    currentFrameSize = frameSize;
    if (currentFrameSize > MAX_FRAME_SIZE)
    {
        currentFrameSize = MAX_FRAME_SIZE;
    }

    txIndex = currentFrameSize;
    rxIndex = currentFrameSize;
    currentSrcBuff = srcBuff;

    /* Preload TX FIFO */
    while ((txIndex > 0U) && (SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_TxNotFullFlag))
    {
        SPI_WriteData(EXAMPLE_SPI_SLAVE, currentSrcBuff[currentFrameSize - txIndex], 0);
        txIndex--;
    }
}

/* 初始化被動模式 */
static void init_passive_mode(void)
{
    if (operation_mode != MODE_ACTIVE && !passive_mode_busy) {
        operation_mode = MODE_PASSIVE_IDLE;
        SPI_StartFrame_PreloadTX(passive_ff_buffer, FIXED_BUFFER_SIZE);

        if (!spi_irq_enabled) {
            SPI_EnableRxTxInterrupt();
        }
    }
}

void SPI_SLAVE_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ((SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_RxNotEmptyFlag) && (rxIndex > 0U))
    {
        destBuff[currentFrameSize - rxIndex] = SPI_ReadData(EXAMPLE_SPI_SLAVE);
        rxIndex--;
    }

    if ((SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_TxNotFullFlag) && (txIndex > 0U))
    {
        SPI_WriteData(EXAMPLE_SPI_SLAVE, currentSrcBuff[currentFrameSize - txIndex], 0);
        txIndex--;
    }

    if ((rxIndex == 0U) && (txIndex == 0U))
    {
        SPI_DisableRxTxInterrupt();

        /* 處理不同模式 */
		if (operation_mode == MODE_PASSIVE_IDLE)
		{
			// 被動模式：收到第一個 frame
			passive_mode_busy = true;

			// 步驟 1: 只檢查首碼是否為 0xAA
			if (destBuff[0] == 0xAA)
			{
				// 步驟 2: 驗證 checksum
				uint8_t expected_cs = calculateChecksum(destBuff, 3);
				if (destBuff[3] == expected_cs)
				{
					// 驗證通過，保存收到的任何有效資料
					memcpy(passive_rx_buffer, destBuff, FIXED_BUFFER_SIZE);
					PRINTF("\n[Passive] Received valid frame [%02X %02X %02X %02X], will send back on next transaction.\r\n",
						   passive_rx_buffer[0], passive_rx_buffer[1], passive_rx_buffer[2], passive_rx_buffer[3]);

					// 切換到 ACK 模式，準備回傳剛剛保存的資料
					operation_mode = MODE_PASSIVE_ACK;
					SPI_StartFrame_PreloadTX(passive_rx_buffer, FIXED_BUFFER_SIZE);

					if (!spi_irq_enabled) {
						SPI_EnableRxTxInterrupt();
					}

					SDK_ISR_EXIT_BARRIER;
					return; // 處理完畢，直接返回
				}
				else
				{
					// Checksum 錯誤
					PRINTF("\n[Passive] Invalid checksum! Prefix OK, but expected %02X, got %02X\r\n", expected_cs, destBuff[3]);
					passive_mode_busy = false;
					xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
				}
			}
			else
			{
				// 首碼不是 0xAA，視為無效資料
				PRINTF("\n[Passive] Invalid frame prefix! Expected 0xAA, got 0x%02X\r\n", destBuff[0]);
				passive_mode_busy = false;
				xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
			}
		}
        else if (operation_mode == MODE_PASSIVE_ACK)
        {
            // 被動模式：收到第二個 frame (ACK)
            if (destBuff[0] == 0x11 && destBuff[1] == 0x11 &&
                destBuff[2] == 0x11 && destBuff[3] == 0x11)
            {
                PRINTF("[Passive] Received ACK [11 11 11 11], sent back [%02X %02X %02X %02X]\r\n",
                       passive_rx_buffer[0], passive_rx_buffer[1],
                       passive_rx_buffer[2], passive_rx_buffer[3]);
                PRINTF("[Passive] Sequence completed successfully!\r\n\n");
            }
            else
            {
                PRINTF("[Passive] Unexpected ACK: [%02X %02X %02X %02X]\r\n",
                       destBuff[0], destBuff[1], destBuff[2], destBuff[3]);
            }

            // 完成被動序列，回到等待狀態
            passive_mode_busy = false;
            operation_mode = MODE_PASSIVE_IDLE;
            xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_RX_DONE | EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
        }
        else if (operation_mode == MODE_ACTIVE)
        {
            // 主動模式
            xEventGroupSetBitsFromISR(spi_event_group, EVT_TRANSFER_DONE, &xHigherPriorityTaskWoken);
        }

#if USE_SEMAPHORE
        xSemaphoreGiveFromISR(spi_semaphore, &xHigherPriorityTaskWoken);
#endif
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    SDK_ISR_EXIT_BARRIER;
}

void APP_GPIO_INTA_IRQHandler(void)
{
    GPIO_PinClearInterruptFlag(GPIO, APP_SW_PORT, APP_SW_PIN, 0);
    SDK_ISR_EXIT_BARRIER;
}

/* 被動處理任務 */
static void passive_handler_task(void *pvParameters)
{
    EventBits_t bits;

    // 初始化被動模式
    vTaskDelay(pdMS_TO_TICKS(100));
    init_passive_mode();

    while (1)
    {
        bits = xEventGroupWaitBits(spi_event_group,
                                   EVT_PASSIVE_RX_DONE | EVT_PASSIVE_NEED_INIT,
                                   pdTRUE,
                                   pdFALSE,
                                   portMAX_DELAY);

        if (bits & EVT_PASSIVE_NEED_INIT)
        {
            vTaskDelay(pdMS_TO_TICKS(10));

            // 只有在 GPIO 低位且不在主動模式時才初始化
            if (GPIO_PinRead(GPIO, 0, 29) == 0 && operation_mode != MODE_ACTIVE)
            {
                init_passive_mode();
            }
        }

        if (bits & EVT_PASSIVE_RX_DONE)
        {
            PRINTF("[Passive] Ready for next sequence.\r\n");
        }
    }
}

static void console_task(void *pvParameters)
{
    uint8_t hex_value;

    dataFrame1[3] = calculateChecksum(dataFrame1, 3);
    dataFrame2[3] = calculateChecksum(dataFrame2, 3);

    PRINTF("=== SPI Slave Ready ===\r\n");
    PRINTF("GPIO is LOW: Passive mode active, waiting for Master...\r\n");
    PRINTF("Initial Frame 1: [%02X %02X %02X %02X]\r\n",
           dataFrame1[0], dataFrame1[1], dataFrame1[2], dataFrame1[3]);
    PRINTF("Initial Frame 2: [%02X %02X %02X %02X]\r\n",
           dataFrame2[0], dataFrame2[1], dataFrame2[2], dataFrame2[3]);
    PRINTF("Initial Frame 3: [%02X %02X %02X %02X]\r\n",
           dataFrame3[0], dataFrame3[1], dataFrame3[2], dataFrame3[3]);

    while (1)
    {
        PRINTF("Input a byte to start active transmission (current flag is %d):\r\n", g_special_toggle_flag);

        do {
            hex_value = (uint8_t)GETCHAR();
        } while (hex_value == '\r' || hex_value == '\n');

        PRINTF("You typed: 0x%02X\r\n", hex_value);

        /* ============= 進入主動模式 ============= */
        // 1. 先停止被動模式
        operation_mode = MODE_ACTIVE;

        // 2. 停用 SPI 中斷
        SPI_DisableRxTxInterrupt();

        // 3. 等待任何進行中的被動傳輸完成
        while (passive_mode_busy) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // 4. 重新初始化 SPI 來清空 FIFO
        spi_slave_config_t slave_config = {0};
        SPI_SlaveGetDefaultConfig(&slave_config);
        slave_config.sselPol = (spi_spol_t)EXAMPLE_SPI_SPOL;
        SPI_Deinit(EXAMPLE_SPI_SLAVE);
        SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slave_config);

        // 5. 清空接收緩衝區
        memset(destBuff, 0, sizeof(destBuff));

        PRINTF(">>> FIFOs cleared for ACTIVE mode <<<\r\n");

        uint8_t frames_to_send = 3;
        bool trigger_frame2_was_sent = false;

        uint8_t* frame2_ptr = dataFrame2;
        uint32_t frame2_size = FIXED_BUFFER_SIZE;

        // 處理特殊的 0x92 輸入
        if (hex_value == TRIGGER_HEX_VALUE)
        {
            char input_str[MAX_FRAME_SIZE - 5] = {0};
            PRINTF("Please enter a string to send:\r\n");

            char ch;
            int i = 0;
            while ((ch = GETCHAR()) != '\r' && i < (sizeof(input_str) - 1))
            {
                PUTCHAR(ch);
                input_str[i++] = ch;
            }
            input_str[i] = '\0';
            PRINTF("\r\nString to send: %s\r\n", input_str);

            uint8_t str_len = strlen(input_str);

            dataFrame1[2] = str_len + 4;
            dataFrame1[3] = calculateChecksum(dataFrame1, 3);
            PRINTF("Updated Frame 1 for this transaction: [%02X %02X %02X %02X]\r\n",
                   dataFrame1[0], dataFrame1[1], dataFrame1[2], dataFrame1[3]);

            dynamicFrameBuffer[0] = 0xAA;
            dynamicFrameBuffer[1] = 0x00;
            dynamicFrameBuffer[2] = TRIGGER_HEX_VALUE;
            memcpy(&dynamicFrameBuffer[3], input_str, str_len);

            uint8_t checksum = calculateChecksum(dynamicFrameBuffer, 3 + str_len);
            dynamicFrameBuffer[3 + str_len] = checksum;

            frame2_ptr = dynamicFrameBuffer;
            frame2_size = 3 + str_len + 1;

            PRINTF("Updated Dynamic Frame 2 (size %d): [", frame2_size);
            for(int j = 0; j < frame2_size; ++j) PRINTF("%02X ", frame2_ptr[j]);
            PRINTF("]\r\n");
        }
        else
        {
            dataFrame1[2] = 0x04;
            dataFrame1[3] = calculateChecksum(dataFrame1, 3);

            dataFrame2[2] = hex_value;
            dataFrame2[3] = calculateChecksum(dataFrame2, 3);

            if (hex_value == TRIGGER_FRAME2_VALUE)
            {
                trigger_frame2_was_sent = true;
                PRINTF("Frame 2 is the SPECIAL trigger frame [AA 00 23 CS]. Will check Frame 3 RX for response.\r\n");
            }

            frame2_ptr = dataFrame2;
            frame2_size = FIXED_BUFFER_SIZE;

            PRINTF("Updated Fixed Frame 2: [%02X %02X %02X %02X]\r\n",
                   dataFrame2[0], dataFrame2[1], dataFrame2[2], dataFrame2[3]);
        }

        currentFrame = 0;
        SPI_StartFrame_PreloadTX(dataFrame1, FIXED_BUFFER_SIZE);

        GPIO_PinWrite(GPIO, 0, 29, 1);
        PRINTF(">>> Entering ACTIVE mode: GPIO pin set HIGH <<<\r\n");

        if (!spi_irq_enabled)
        {
            SPI_EnableRxTxInterrupt();
        }

        for (int i = 0; i < frames_to_send; i++)
        {
#if USE_EVENT
            xEventGroupWaitBits(spi_event_group,
                                EVT_TRANSFER_DONE,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);
#endif
            uint8_t* sent_frame_ptr;
            uint32_t sent_frame_size;

            if (currentFrame == 0) {
                sent_frame_ptr = dataFrame1;
                sent_frame_size = FIXED_BUFFER_SIZE;
            } else if (currentFrame == 1) {
                sent_frame_ptr = frame2_ptr;
                sent_frame_size = frame2_size;
            } else {
                sent_frame_ptr = dataFrame3;
                sent_frame_size = FIXED_BUFFER_SIZE;
            }

            PRINTF("Frame %d sent (size %d): [", currentFrame + 1, sent_frame_size);
            for(int j = 0; j < sent_frame_size; ++j) PRINTF("%02X ", sent_frame_ptr[j]);
            PRINTF("]\r\n");

            PRINTF("RX from master (size %d): [", sent_frame_size);
            for(int j = 0; j < sent_frame_size; ++j) PRINTF("%02X ", destBuff[j]);
            PRINTF("]\r\n");

            if (currentFrame == 2)
            {
                if (sent_frame_size >= 2)
                {
                    if (destBuff[0] == 0xAA)
                    {
                        PRINTF("Frame 3 RX prefix check: PASSED (0xAA).\r\n");

                        uint8_t receivedChecksum = destBuff[sent_frame_size - 1];
                        uint8_t calculatedChecksum = calculateChecksum(destBuff, sent_frame_size - 1);

                        if (receivedChecksum == calculatedChecksum)
                        {
                            PRINTF("Frame 3 RX checksum check: PASSED (Received: 0x%02X, Calculated: 0x%02X).\r\n",
                                   receivedChecksum, calculatedChecksum);
                        }
                        else
                        {
                            PRINTF("Frame 3 RX checksum check: FAILED (Received: 0x%02X, but expected: 0x%02X).\r\n",
                                   receivedChecksum, calculatedChecksum);
                        }
                    }
                    else
                    {
                        PRINTF("Frame 3 RX prefix check: FAILED (Expected: 0xAA, Received: 0x%02X).\r\n", destBuff[0]);
                    }
                }
                else
                {
                    PRINTF("Frame 3 RX check: SKIPPED (Frame size %d is too small for validation).\r\n", sent_frame_size);
                }

                if (trigger_frame2_was_sent)
                {
                    if (sent_frame_size == 4 && destBuff[0] == 0xAA &&
                        destBuff[1] == 0x00 && destBuff[2] == 0x02)
                    {
                        uint8_t calculated_cs = calculateChecksum(destBuff, 3);
                        if (destBuff[3] == calculated_cs)
                        {
                            PRINTF(">>> Special condition MET! Frame 2 was trigger, and received valid Frame 3 response.\r\n");
                            g_special_toggle_flag = !g_special_toggle_flag;
                            PRINTF(">>> g_special_toggle_flag is now: %d\r\n", g_special_toggle_flag);
                        }
                        else
                        {
                            PRINTF(">>> Special condition: Frame 3 response received, but CHECKSUM FAILED.\r\n");
                        }
                    }
                    else
                    {
                        PRINTF(">>> Special condition: Frame 2 was trigger, but Frame 3 response is not [AA 00 02 CS].\r\n");
                    }
                }
            }

            currentFrame++;

            if (currentFrame < frames_to_send)
            {
                if (currentFrame == 1)
                    SPI_StartFrame_PreloadTX(frame2_ptr, frame2_size);
                else if (currentFrame == 2)
                    SPI_StartFrame_PreloadTX(dataFrame3, FIXED_BUFFER_SIZE);

                if (!spi_irq_enabled)
                {
                    SPI_EnableRxTxInterrupt();
                }
            }
        }

        GPIO_PinWrite(GPIO, 0, 29, 0);
        PRINTF(">>> Active transmission done! GPIO pin set LOW <<<\r\n");
        PRINTF(">>> Returning to PASSIVE mode...\r\n\n");

        operation_mode = MODE_PASSIVE_IDLE;
        xEventGroupSetBits(spi_event_group, EVT_PASSIVE_NEED_INIT);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void spi_task(void *pvParameters)
{
    EventBits_t bits;
    while (1)
    {
        bits = xEventGroupWaitBits(spi_event_group,
                                   EVT_TRANSFER_DONE,
                                   pdTRUE,
                                   pdFALSE,
                                   portMAX_DELAY);
        if ((bits & EVT_TRANSFER_DONE) != 0)
        {
            PRINTF("spi_task: Frame transfer done.\r\n");
        }
    }
}

static void Scan_I2C_Devices(I3C_Type *base)
{
    uint8_t dummyData = 0x00;
    status_t result;

    PRINTF("[I2C]Scanning I2C addresses...\n");

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) // I2C valid 7-bit address range
    {
        result = BOARD_I3C_Send(base, addr, 0x00, 0, &dummyData, 0);

        if (result == kStatus_Success)
        {
        	PRINTF("[I2C]Device found at 0x%02X\n", addr);
        }
    }

    PRINTF("[I2C]Scan complete.\n");
}

int main(void)
{
    BOARD_InitHardware();
    BOARD_I3C_Init(BOARD_PMIC_I3C_BASEADDR, BOARD_PMIC_I3C_CLOCK_FREQ);

#if USE_EVENT
    spi_event_group = xEventGroupCreate();
    if (spi_event_group == NULL)
    {
        PRINTF("Failed to create event group\r\n");
        while (1);
    }
#endif

#if USE_SEMAPHORE
    spi_semaphore = xSemaphoreCreateBinary();
    if (spi_semaphore == NULL)
    {
        PRINTF("Failed to create semaphore\r\n");
        while (1);
    }
#endif

#if SCAN_I2C_ADDRESS_ENABLE
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);
#endif


    /* Init GPIO */
    GPIO_PortInit(GPIO, 0);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, 0, 29, &output_int_config);
    GPIO_PinWrite(GPIO, 0, 29, 0);  // 預設為低位

    /* Init PCA9422 PMIC. */
 	BOARD_InitPmic();
 	PRINTF("-------------- PCA9422 BOARD_InitPmic OK--------------\r\n");
 	/* Apply PMIC mode and voltage settings */
 	BOARD_Init_PMICConfigure();
 	PRINTF("-------------- PCA9422 BOARD_Init_PMICConfigure OK--------------\r\n");

    /* init SPI peripheral */
    spi_slave_config_t slave_config = {0};
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)EXAMPLE_SPI_SPOL;
    SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slave_config);

    NVIC_SetPriority(EXAMPLE_SPI_SLAVE_IRQ, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(EXAMPLE_SPI_SLAVE_IRQ);

    /* create tasks */
    if (xTaskCreate(console_task, "CONSOLE", configMINIMAL_STACK_SIZE + 500, NULL,
                    tskIDLE_PRIORITY + 2, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

    if (xTaskCreate(spi_task, "SPI_TASK", configMINIMAL_STACK_SIZE + 100, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

    if (xTaskCreate(passive_handler_task, "PASSIVE", configMINIMAL_STACK_SIZE + 200, NULL,
                    tskIDLE_PRIORITY + 3, NULL) != pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1);
    }

    vTaskStartScheduler();

    for (;;);
}
