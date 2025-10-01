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
#include "fsl_pint.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"
#include "pmic_support.h"
#include "pmic_pca9422.h"
#include "glf70583.h"

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
#define SHORT_PRESS_HEX_VALUE 0x21
#define LONG_PRESS_HEX_VALUE 0x23


/* ============= 被動模式相關定義 ============= */
typedef enum {
    MODE_PASSIVE_IDLE,
    MODE_PASSIVE_ACK,
    MODE_ACTIVE
} spi_operation_mode_t;

static volatile spi_operation_mode_t operation_mode = MODE_PASSIVE_IDLE;
static uint8_t passive_rx_buffer[FIXED_BUFFER_SIZE] = {0};
static uint8_t passive_ff_buffer[FIXED_BUFFER_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF};
static volatile bool passive_mode_busy = false;

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

#define BTN_NOTIFY_EDGE   (1UL << 0)  // 在sButtonTaskHandle，來自 FunKey PINT ISR 的雙邊緣事件，
#define BTN_NOTIFY_DBL    (1UL << 1)  // 在sButtonTaskHandle，雙擊計時器到時，
#define PWR_NOTIFY_EDGE   (1UL << 0)  // 在sPowerKeyTaskHandle，來自 PowerKey PINT ISR 的邊緣事件

/* === 按鍵行為參數 === */
#define BTN_ACTIVE_LEVEL          0   // active-low: 按下=0，放開=1；若相反改為 1
#define BTN_DEBOUNCE_MS          30
#define BTN_LONG_MS            1000
#define BTN_DBLCLICK_GAP_MS     300
#define PWR_ACTIVE_LEVEL          0      // active-low: 按下=0；若為 active-high 改為 1
#define PWR_DEBOUNCE_MS          30      // 軟體去抖
#define PWR_LONG_MS            3000      // 長按 3 秒（放開才判斷）
#define PWR_MIN_SHORT_MS         50      // 太短的抖動/誤觸（<50ms）不算短按
#define POWER_KEY_PINT_CH  		  0   // PINT 通道
#define FUN_KEY1_PINT_CH  		  1   // PINT 通道
/* === 按鈕任務的 handle 與軟體定時器 === */
static TaskHandle_t   sButtonTaskHandle = NULL;
static TimerHandle_t  sBtnDblTimer  = NULL;
static TaskHandle_t sPowerKeyTaskHandle = NULL;

// <<< MODIFIED: 核心架構變更，使用 Queue 取代 Event Group 進行任務間通訊 >>>
static QueueHandle_t spi_request_queue = NULL;
static EventGroupHandle_t spi_event_group = NULL;
static SemaphoreHandle_t spi_semaphore = NULL;

/*****************************************************************************************************/
/* ===== I2C EventGroup bits (for unified I2C_Task) ===== */
#define TOUCH_EVENT_BIT      (1UL << 0)
#define CHARGER_EVENT_BIT    (1UL << 1)

/* ===== I2C synchronization objects ===== */
static EventGroupHandle_t i2c_event_group = NULL;
static SemaphoreHandle_t  i2c_mutex       = NULL;
static TaskHandle_t       sI2CTaskHandle  = NULL;

/* ===== I2C task prototype & external device handlers ===== */
static void I2C_Task(void *pvParameters);


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void button_task(void *pvParameters);
static void spi_handler_task(void *pvParameters); // <<< MODIFIED: 新的 SPI 消費者任務
static void passive_handler_task(void *pvParameters);
static void spi_task(void *pvParameters);
static void execute_active_spi_transmission(uint8_t hex_value); // <<< NEW: 獨立的 SPI 執行函式
static void SPI_StartFrame_PreloadTX(uint8_t* srcBuff, uint32_t frameSize);
static inline void SPI_EnableRxTxInterrupt(void);
static inline void SPI_DisableRxTxInterrupt(void);
static uint8_t calculateChecksum(uint8_t* data, uint8_t len);
static void init_passive_mode(void);

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * @brief 計算數據的校驗和 (Checksum)。
 * @details 遍歷數據中的每個位元，計算值為 '1' 的位元總數。
 * @param data 指向數據緩衝區的指標。
 * @param len 數據的長度 (bytes)。
 * @return 計算出的 8 位元校驗和。
 */
static uint8_t calculateChecksum(uint8_t* data, uint8_t len)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (byte & (1 << j)) {
                count++;
            }
        }
    }
    return count;
}

/**
 * @brief 啟用 SPI 的接收(Rx)和發送(Tx)中斷。
 * @details 此函式會設置 SPI 控制器以在接收 FIFO 達到水平或發送 FIFO 未滿時觸發中斷。
 */
static inline void SPI_EnableRxTxInterrupt(void)
{
    SPI_EnableInterrupts(EXAMPLE_SPI_SLAVE, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
    spi_irq_enabled = true;
    if (operation_mode == MODE_ACTIVE) {
        PRINTF("SPI Enable() - Frame %d\r\n", currentFrame + 1);
    }
}

/**
 * @brief 禁用 SPI 的接收(Rx)和發送(Tx)中斷。
 * @details 防止 SPI 控制器產生 Rx/Tx 相關的中斷。
 */
static inline void SPI_DisableRxTxInterrupt(void)
{
    SPI_DisableInterrupts(EXAMPLE_SPI_SLAVE, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
    spi_irq_enabled = false;
    if (operation_mode == MODE_ACTIVE) {
        PRINTF("SPI Disable() - Frame %d complete\r\n", currentFrame + 1);
    }
}

/**
 * @brief 開始一個新的 SPI 幀傳輸並預加載 TX FIFO。
 * @details 清除任何殘留的 RX 數據，設置當前幀的大小和源緩衝區，
 * 然後盡可能多地將數據從源緩衝區寫入 TX FIFO，直到 FIFO 滿或數據發送完畢。
 * @param srcBuff 指向要發送的數據的源緩衝區。
 * @param frameSize 要發送的數據幀的大小。
 */
static void SPI_StartFrame_PreloadTX(uint8_t* srcBuff, uint32_t frameSize)
{
    while (SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_RxNotEmptyFlag) {
        (void)SPI_ReadData(EXAMPLE_SPI_SLAVE);
    }
    currentFrameSize = frameSize;
    if (currentFrameSize > MAX_FRAME_SIZE) {
        currentFrameSize = MAX_FRAME_SIZE;
    }
    txIndex = currentFrameSize;
    rxIndex = currentFrameSize;
    currentSrcBuff = srcBuff;
    while ((txIndex > 0U) && (SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_TxNotFullFlag)) {
        SPI_WriteData(EXAMPLE_SPI_SLAVE, currentSrcBuff[currentFrameSize - txIndex], 0);
        txIndex--;
    }
}

/**
 * @brief 初始化 SPI 進入被動模式。
 * @details 確保設備不處於主動模式或忙碌狀態，然後設置 SPI 為被動監聽狀態，
 * 準備接收來自 Master 的數據。它會預加載 0xFF 作為默認的傳輸數據。
 */
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

/**
 * @brief SPI Slave 的中斷服務常式 (Interrupt Service Routine, ISR)。
 * @details 此函式在 SPI 中斷發生時被調用。它處理數據的接收和發送，
 * 並根據當前的操作模式 (主動/被動) 處理不同的邏輯，
 * 例如在被動模式下驗證接收到的數據、準備 ACK，或在主動模式下發送事件信號通知任務傳輸完成。
 */
void SPI_SLAVE_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ((SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_RxNotEmptyFlag) && (rxIndex > 0U)) {
        destBuff[currentFrameSize - rxIndex] = SPI_ReadData(EXAMPLE_SPI_SLAVE);
        rxIndex--;
    }
    if ((SPI_GetStatusFlags(EXAMPLE_SPI_SLAVE) & kSPI_TxNotFullFlag) && (txIndex > 0U)) {
        SPI_WriteData(EXAMPLE_SPI_SLAVE, currentSrcBuff[currentFrameSize - txIndex], 0);
        txIndex--;
    }
    if ((rxIndex == 0U) && (txIndex == 0U)) {
        SPI_DisableRxTxInterrupt();
        if (operation_mode == MODE_PASSIVE_IDLE) {
            passive_mode_busy = true;
            if (destBuff[0] == 0xAA) {
                uint8_t expected_cs = calculateChecksum(destBuff, 3);
                if (destBuff[3] == expected_cs) {
                    memcpy(passive_rx_buffer, destBuff, FIXED_BUFFER_SIZE);
                    PRINTF("\n[Passive] Received valid frame [%02X %02X %02X %02X], will send back on next transaction.\r\n",
                           passive_rx_buffer[0], passive_rx_buffer[1], passive_rx_buffer[2], passive_rx_buffer[3]);
                    operation_mode = MODE_PASSIVE_ACK;
                    SPI_StartFrame_PreloadTX(passive_rx_buffer, FIXED_BUFFER_SIZE);
                    if (!spi_irq_enabled) {
                        SPI_EnableRxTxInterrupt();
                    }
                    SDK_ISR_EXIT_BARRIER;
                    return;
                } else {
                    PRINTF("\n[Passive] Invalid checksum! Prefix OK, but expected %02X, got %02X\r\n", expected_cs, destBuff[3]);
                    passive_mode_busy = false;
                    xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
                }
            } else {
                PRINTF("\n[Passive] Invalid frame prefix! Expected 0xAA, got 0x%02X\r\n", destBuff[0]);
                passive_mode_busy = false;
                xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
            }
        } else if (operation_mode == MODE_PASSIVE_ACK) {
            if (destBuff[0] == 0x11 && destBuff[1] == 0x11 && destBuff[2] == 0x11 && destBuff[3] == 0x11) {
                PRINTF("[Passive] Received ACK [11 11 11 11], sent back [%02X %02X %02X %02X]\r\n",
                       passive_rx_buffer[0], passive_rx_buffer[1], passive_rx_buffer[2], passive_rx_buffer[3]);
                PRINTF("[Passive] Sequence completed successfully!\r\n\n");
            } else {
                PRINTF("[Passive] Unexpected ACK: [%02X %02X %02X %02X]\r\n",
                       destBuff[0], destBuff[1], destBuff[2], destBuff[3]);
            }
            passive_mode_busy = false;
            operation_mode = MODE_PASSIVE_IDLE;
            xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_RX_DONE | EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
        } else if (operation_mode == MODE_ACTIVE) {
            xEventGroupSetBitsFromISR(spi_event_group, EVT_TRANSFER_DONE, &xHigherPriorityTaskWoken);
        }
#if USE_SEMAPHORE
        xSemaphoreGiveFromISR(spi_semaphore, &xHigherPriorityTaskWoken);
#endif
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief 執行一次完整的主動 SPI 傳輸序列。
 * @details 此函式將 SPI 切換到主動模式，準備要發送的數據幀 (根據傳入的 hex_value 修改第二幀)，
 * 然後透過 GPIO 拉高信號通知主機。接著，它會依次發送三個預定義的數據幀，
 * 並在完成後將 GPIO 拉低，最後將 SPI 切換回被動模式。
 * @param hex_value 要放入第二個數據幀的特定十六進制值 (例如，按鍵事件)。
 */
static void execute_active_spi_transmission(uint8_t hex_value)
{
    PRINTF("\n--- Executing Active SPI for value: 0x%02X ---\r\n", hex_value);

    /* ============= 進入主動模式 ============= */
    operation_mode = MODE_ACTIVE;
    SPI_DisableRxTxInterrupt();
    while (passive_mode_busy) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    spi_slave_config_t slave_config = {0};
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)EXAMPLE_SPI_SPOL;
    SPI_Deinit(EXAMPLE_SPI_SLAVE);
    SPI_SlaveInit(EXAMPLE_SPI_SLAVE, &slave_config);

    memset(destBuff, 0, sizeof(destBuff));
    PRINTF(">>> FIFOs cleared for ACTIVE mode <<<\r\n");

    uint8_t frames_to_send = 3;
    uint8_t* frame2_ptr = dataFrame2;
    uint32_t frame2_size = FIXED_BUFFER_SIZE;

    if (hex_value == TRIGGER_HEX_VALUE) {
        PRINTF("Dynamic frame trigger (0x92) is not supported in this mode.\r\n");
        // 如果未來需要，可以在此處添加其他任務互動以獲取字串
    } else {
        dataFrame1[2] = 0x04;
        dataFrame1[3] = calculateChecksum(dataFrame1, 3);
        dataFrame2[2] = hex_value;
        dataFrame2[3] = calculateChecksum(dataFrame2, 3);
        frame2_ptr = dataFrame2;
        frame2_size = FIXED_BUFFER_SIZE;
        PRINTF("Updated Fixed Frame 2: [%02X %02X %02X %02X]\r\n",
               dataFrame2[0], dataFrame2[1], dataFrame2[2], dataFrame2[3]);
    }

    currentFrame = 0;
    SPI_StartFrame_PreloadTX(dataFrame1, FIXED_BUFFER_SIZE);

    GPIO_PinWrite(GPIO, 2, 15, 1);
    PRINTF(">>> Entering ACTIVE mode: GPIO pin set HIGH <<<\r\n");

    if (!spi_irq_enabled) {
        SPI_EnableRxTxInterrupt();
    }

    for (int i = 0; i < frames_to_send; i++) {
        xEventGroupWaitBits(spi_event_group, EVT_TRANSFER_DONE, pdTRUE, pdFALSE, portMAX_DELAY);

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

        if (currentFrame == 2) {
            if (sent_frame_size >= 2) {
                if (destBuff[0] == 0xAA) {
                    PRINTF("Frame 3 RX prefix check: PASSED (0xAA).\r\n");
                    uint8_t receivedChecksum = destBuff[sent_frame_size - 1];
                    uint8_t calculatedChecksum = calculateChecksum(destBuff, sent_frame_size - 1);
                    if (receivedChecksum == calculatedChecksum) {
                        PRINTF("Frame 3 RX checksum check: PASSED (Received: 0x%02X, Calculated: 0x%02X).\r\n", receivedChecksum, calculatedChecksum);
                    } else {
                        PRINTF("Frame 3 RX checksum check: FAILED (Received: 0x%02X, but expected: 0x%02X).\r\n", receivedChecksum, calculatedChecksum);
                    }
                } else {
                    PRINTF("Frame 3 RX prefix check: FAILED (Expected: 0xAA, Received: 0x%02X).\r\n", destBuff[0]);
                }
            } else {
                PRINTF("Frame 3 RX check: SKIPPED (Frame size %d is too small for validation).\r\n", sent_frame_size);
            }
        }
        currentFrame++;
        if (currentFrame < frames_to_send) {
            if (currentFrame == 1)
                SPI_StartFrame_PreloadTX(frame2_ptr, frame2_size);
            else if (currentFrame == 2)
                SPI_StartFrame_PreloadTX(dataFrame3, FIXED_BUFFER_SIZE);
            if (!spi_irq_enabled) {
                SPI_EnableRxTxInterrupt();
            }
        }
    }

    GPIO_PinWrite(GPIO, 2, 15, 0);
    PRINTF(">>> Active transmission done! GPIO pin set LOW <<<\r\n");
    PRINTF(">>> Returning to PASSIVE mode...\r\n\n");

    operation_mode = MODE_PASSIVE_IDLE;
    xEventGroupSetBits(spi_event_group, EVT_PASSIVE_NEED_INIT);
    vTaskDelay(pdMS_TO_TICKS(100));
}

/**
 * @brief SPI 處理任務 (消費者)。
 * @details 這是一個 FreeRTOS 任務，它無限循環地等待來自 `spi_request_queue` 的請求。
 * 一旦從佇列中接收到一個值 (例如，由 `button_task` 發送的按鍵事件)，
 * 它就會調用 `execute_active_spi_transmission` 來執行主動 SPI 傳輸。
 * 這種生產者-消費者模式可以將事件的產生 (按鈕) 與事件的處理 (SPI傳輸) 解耦。
 */
static void spi_handler_task(void *pvParameters)
{
    uint8_t received_value;

    // 初始化 SPI frame 的 checksum
    dataFrame1[3] = calculateChecksum(dataFrame1, 3);
    dataFrame2[3] = calculateChecksum(dataFrame2, 3);

    PRINTF("=== SPI Slave Ready ===\r\n");
    PRINTF("GPIO is LOW: Passive mode active, waiting for Master...\r\n");
    PRINTF("SPI Handler Task is ready, waiting for requests from the queue.\r\n");

    while (1)
    {
        // 從佇列中等待並接收訊息，如果沒有訊息，任務會在此處被 block 住
        if (xQueueReceive(spi_request_queue, &received_value, portMAX_DELAY) == pdPASS)
        {
            // 成功收到請求，呼叫執行函式
            execute_active_spi_transmission(received_value);
        }
    }
}

/**
 * @brief 被動模式處理任務。
 * @details 這個任務負責管理 SPI 的被動模式。它在啟動時初始化 SPI 進入被動模式，
 * 並等待事件信號 (EVT_PASSIVE_RX_DONE 或 EVT_PASSIVE_NEED_INIT)。
 * 當需要重新初始化被動模式時 (例如，一次主動傳輸完成後或被動接收失敗後)，
 * 此任務會確保 SPI 正確地返回到監聽狀態。
 */
static void passive_handler_task(void *pvParameters)
{
    EventBits_t bits;
    vTaskDelay(pdMS_TO_TICKS(100));
    init_passive_mode();

    while (1) {
        bits = xEventGroupWaitBits(spi_event_group,
                                   EVT_PASSIVE_RX_DONE | EVT_PASSIVE_NEED_INIT,
                                   pdTRUE,
                                   pdFALSE,
                                   portMAX_DELAY);
        if (bits & EVT_PASSIVE_NEED_INIT) {
            vTaskDelay(pdMS_TO_TICKS(10));
            if (GPIO_PinRead(GPIO, 2, 15) == 0 && operation_mode != MODE_ACTIVE) {
                init_passive_mode();
            }
        }
        if (bits & EVT_PASSIVE_RX_DONE) {
            PRINTF("[Passive] Ready for next sequence.\r\n");
        }
    }
}

/**
 * @brief SPI 監控任務 (可能用於調試)。
 * @details 這是一個簡單的任務，等待 `EVT_TRANSFER_DONE` 事件，並在事件發生時打印一條訊息。
 * 在目前的架構中，主要的主動傳輸邏輯由 `execute_active_spi_transmission` 處理，
 * 這個任務可能是一個輔助或遺留的監控功能。
 */
static void spi_task(void *pvParameters)
{
    EventBits_t bits;
    while (1) {
        bits = xEventGroupWaitBits(spi_event_group, EVT_TRANSFER_DONE, pdTRUE, pdFALSE, portMAX_DELAY);
        if ((bits & EVT_TRANSFER_DONE) != 0) {
            PRINTF("spi_task: Frame transfer done.\r\n");
        }
    }
}



/* 讀腳位 */
static inline uint8_t pwr_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, POWER_KEY_PORT, POWER_KEY_PIN);
}

/* 讀腳位 */
static inline uint8_t btn_raw_read(void)
{
    return (uint8_t)GPIO_PinRead(GPIO, FUN_KEY1_N_PORT, FUN_KEY1_N_PIN);
}

/* 雙擊計時器回呼：只通知任務 */
static void vBtnDblTimerCb(TimerHandle_t xTimer)
{
    if (sButtonTaskHandle) {
        (void)xTaskNotify(sButtonTaskHandle, BTN_NOTIFY_DBL, eSetBits);
    }
}

/**
 * @brief 按鈕處理任務 (生產者)。
 * @details 此任務持續監控一個 GPIO 按鈕的狀態，並能區分短按和長按。
 * 當檢測到有效的按鍵事件時，它會將對應的十六進制值 (SHORT_PRESS_HEX_VALUE 或 LONG_PRESS_HEX_VALUE)
 * 作為一個請求發送到 `spi_request_queue` 消息佇列中，以觸發 SPI 傳輸。
 */
static void button_task(void *pvParameters)
{
	   const TickType_t debounceTicks   = pdMS_TO_TICKS(BTN_DEBOUNCE_MS);
	   const TickType_t longTicks       = pdMS_TO_TICKS(BTN_LONG_MS);

	    sButtonTaskHandle = xTaskGetCurrentTaskHandle();

	    /* 只需要「雙擊」單次定時器 */
	    sBtnDblTimer  = xTimerCreate("btn_dbl", pdMS_TO_TICKS(BTN_DBLCLICK_GAP_MS),
	                                 pdFALSE, NULL, vBtnDblTimerCb);
	    configASSERT(sBtnDblTimer);

	    /* 初始狀態 */
	    uint8_t stable_level = btn_raw_read();
	    bool    is_pressed   = (stable_level == BTN_ACTIVE_LEVEL);
	    bool    dbl_pending  = false;            // 是否正在等待第二次短按
	    TickType_t press_start_tick = 0;

	    if (is_pressed) {
	        // 上電時剛好按住：當作剛按下
	        press_start_tick = xTaskGetTickCount();
	    }

	    for (;;)
	    {
	        uint32_t notifyBits = 0;
	        (void)xTaskNotifyWait(0, 0xFFFFFFFFu, &notifyBits, portMAX_DELAY);

	        /* A) 雙擊窗口到期：若仍在等待第二下且目前沒有按住 → 單擊成立 */
	        if (notifyBits & BTN_NOTIFY_DBL) {
	            if (dbl_pending && !is_pressed) {
	                dbl_pending = false;
	                PRINTF("[Button] Short Press detected.\r\n");
	                PRINTF("[Button] Short Press detected. Sending 0x%02X\r\n", SHORT_PRESS_HEX_VALUE);
	                uint8_t v = SHORT_PRESS_HEX_VALUE;
	                (void)xQueueSend(spi_request_queue, &v, 0);
	            }
	            // 若此時已經在第二次按壓中（is_pressed==true），不回報短按，
	            // 等放開時再判斷是雙擊或長按（第二次按壓可能超過 1 秒而成為長按）
	        }

	        /* B) 來自 PINT 的邊緣事件：去抖後判斷按下/放開 */
	        if (notifyBits & BTN_NOTIFY_EDGE) {
	            uint8_t l1 = btn_raw_read();
	            vTaskDelay(debounceTicks);
	            uint8_t l2 = btn_raw_read();
	            uint8_t new_level = l2;

	            if (new_level != stable_level) {
	                stable_level = new_level;
	                bool now_pressed = (stable_level == BTN_ACTIVE_LEVEL);
	                TickType_t now = xTaskGetTickCount();

	                if (now_pressed && !is_pressed) {
	                    /* 放開 -> 按下：記錄開始時間 */
	                    is_pressed = true;
	                    press_start_tick = now;
	                    // 不動 dbl_pending，讓第二次按壓可以覆蓋成「雙擊或長按」
	                }
	                else if (!now_pressed && is_pressed) {
	                    /* 按下 -> 放開：在此刻才判定長按/短按/雙擊 */
	                    is_pressed = false;
	                    TickType_t press_dur = now - press_start_tick;

	                    if (press_dur >= longTicks) {
	                        /* 長按（放開才觸發）→ 最高優先權 */
	                        PRINTF("[Button] Long Press (on-release) detected. \r\n");
	                        PRINTF("[Button] Long Press (on-release) detected. Sending 0x%02X\r\n",
	                               LONG_PRESS_HEX_VALUE);
	                        uint8_t v = LONG_PRESS_HEX_VALUE;
	                        (void)xQueueSend(spi_request_queue, &v, 0);

	                        /* 任何待定的單擊作廢 */
	                        dbl_pending = false;
	                        (void)xTimerStop(sBtnDblTimer, 0);
	                    } else {
	                        /* 未達長按：處理短按/雙擊 */
	                        if (dbl_pending) {
	                            /* 第二次在時間窗內完成 → 雙擊 */
	                            dbl_pending = false;
	                            (void)xTimerStop(sBtnDblTimer, 0);
	                            PRINTF("[Button] Double Click detected.\r\n");
	                            //PRINTF("[Button] Double Click detected. Sending 0x%02X\r\n",
	                            //      DOUBLE_CLICK_HEX_VALUE);
	                            //uint8_t v = DOUBLE_CLICK_HEX_VALUE;
	                            //(void)xQueueSend(spi_request_queue, &v, 0);
	                        } else {
	                            /* 第一次短按：開窗等第二下 */
	                            dbl_pending = true;
	                            (void)xTimerStop(sBtnDblTimer, 0);
	                            (void)xTimerStart(sBtnDblTimer, 0);
	                        }
	                    }
	                }
	            } // level changed
	        } // EDGE
	    } // for

}


static void power_key_task(void *pvParameters)
{

    const TickType_t debounceTicks = pdMS_TO_TICKS(PWR_DEBOUNCE_MS);
    const TickType_t longTicks     = pdMS_TO_TICKS(PWR_LONG_MS);
    const TickType_t minShortTicks = pdMS_TO_TICKS(PWR_MIN_SHORT_MS);

    sPowerKeyTaskHandle = xTaskGetCurrentTaskHandle();

    /* 取當前穩定狀態 */
    uint8_t stable_level = pwr_raw_read();
    bool    is_pressed   = (stable_level == PWR_ACTIVE_LEVEL);
    TickType_t press_start_tick = 0;

    if (is_pressed) {
        /* 上電時剛好被按住：當作剛按下，等待放開後再判斷是否為長按 */
        press_start_tick = xTaskGetTickCount();
    }

    for (;;)
    {
        uint32_t notifyBits = 0;
        (void)xTaskNotifyWait(0, 0xFFFFFFFFu, &notifyBits, portMAX_DELAY);

        if (notifyBits & PWR_NOTIFY_EDGE) {
            /* 軟體去抖：延遲後再讀取一次，採用第二次結果 */
            uint8_t l1 = pwr_raw_read();
            (void)l1; // 可視需要使用
            vTaskDelay(debounceTicks);
            uint8_t l2 = pwr_raw_read();
            uint8_t new_level = l2;

            if (new_level != stable_level) {
                stable_level = new_level;
                bool now_pressed = (stable_level == PWR_ACTIVE_LEVEL);
                TickType_t now = xTaskGetTickCount();

                if (now_pressed && !is_pressed) {
                    /* 放開 -> 按下：記錄開始時間 */
                    is_pressed = true;
                    press_start_tick = now;
                }
                else if (!now_pressed && is_pressed) {
                    /* 按下 -> 放開：在放開此刻判斷短按或長按 */
                    is_pressed = false;

                    TickType_t press_dur = now - press_start_tick;

                    if (press_dur >= longTicks) {
                        /* 長按（放開才觸發） */
                        PRINTF("[PWR] Long Press (>=%ums) detected.\r\n",(unsigned)PWR_LONG_MS);

                    } else if (press_dur >= minShortTicks) {
                        /* 短按 */
                        PRINTF("[PWR] Short Press detected.\r\n");

                    } else {
                        /* 小於最小短按時間：視為抖動/誤觸，忽略 */
                        // no-op
                    }
                }
                /* 其他情況（例如重複相同邏輯電平）不需處理 */
            }
        }
    }
}


static void I2C_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(
            i2c_event_group,
            TOUCH_EVENT_BIT | CHARGER_EVENT_BIT,
            pdTRUE,     /* clear on exit */
            pdFALSE,    /* wait for any bit */
            portMAX_DELAY);

        /* --- TOUCH event --- */
        if ((bits & TOUCH_EVENT_BIT) != 0)
        {
            if (xSemaphoreTake(i2c_mutex, portMAX_DELAY) == pdTRUE)
            {
                /* 由你既有的 aw93305.c/.h 提供 */
            	AW93305_EXTI_Callback();
                xSemaphoreGive(i2c_mutex);
            }

            /* 任務側重新啟用觸控中斷（先清旗標再開） */
            GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
            GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
        }

    }
}


/**
 * @brief 掃描 I2C 總線上的設備。
 * @details 此函式遍歷所有有效的 7 位 I2C 地址 (從 0x08 到 0x77)，
 * 並嘗試與每個地址進行通信。如果通信成功 (收到 ACK)，
 * 則會在控制台打印出找到的設備地址。
 * @param base 指向 I3C/I2C 控制器的基地址。
 */
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

void pint_intr_callback(pint_pin_int_t pintr, uint32_t pmatch_status)
{

    BaseType_t xHPW = pdFALSE;

    /* FUN_KEY1：通知 button 任務 */
    if ((pintr == FUN_KEY1_PINT_CH) && (sButtonTaskHandle != NULL)) {
        (void)xTaskNotifyFromISR(sButtonTaskHandle, BTN_NOTIFY_EDGE, eSetBits, &xHPW);
    }

    /* PowerKey：通知 power key 任務 */
    if ((pintr == POWER_KEY_PINT_CH) && (sPowerKeyTaskHandle != NULL)) {
        (void)xTaskNotifyFromISR(sPowerKeyTaskHandle, PWR_NOTIFY_EDGE, eSetBits, &xHPW);
    }

    portYIELD_FROM_ISR(xHPW);


	/*
    PRINTF("\f\r\nPINT Pin Interrupt %d event detected.\r\n", pintr);
    uint8_t pin_state;
    if(pintr==0)
    {
    	pin_state = GPIO_PinRead(GPIO,POWER_KEY_PORT,POWER_KEY_PIN);

    }
    else if(pintr==1)
    {
    	pin_state = GPIO_PinRead(GPIO,FUN_KEY1_N_PORT,FUN_KEY1_N_PIN);
    }
    PRINTF(" pin_state:%d \r\n",pin_state);
    */
}

void GPIO_INTA_DriverIRQHandler(void)
{

	BaseType_t xHPW = pdFALSE;

	uint32_t status_1 = GPIO_PortGetInterruptStatus(GPIO, GPIO1_PORT, kGPIO_InterruptA);

    if (status_1 & (1 << TOUCH_INT_PIN)) { //Touch
        GPIO_PinDisableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);
        GPIO_PinClearInterruptFlag(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);


        if (i2c_event_group)
        {
            xEventGroupSetBitsFromISR(i2c_event_group, TOUCH_EVENT_BIT, &xHPW);
        }


        //PRINTF("[Debug] TOUCH_GPIO_INTA_IRQHandler \r\n");
    }

    portYIELD_FROM_ISR(xHPW);
    SDK_ISR_EXIT_BARRIER;
}

/**
 * @brief 主程式進入點。
 * @details 負責初始化硬體、時鐘、GPIO、I2C 和 SPI 等周邊設備。
 * 同時，它會建立 FreeRTOS 所需的事件組、佇列和各個任務 (如按鈕處理、SPI 處理等)，
 * 最後啟動 FreeRTOS 排程器，將控制權交給作業系統。
 */
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

    /* <<< NEW: 建立訊息佇列 >>> */
    // 佇列長度為 10，每個訊息的大小是一個 uint8_t
    spi_request_queue = xQueueCreate(10, sizeof(uint8_t));
    if (spi_request_queue == NULL)
    {
        PRINTF("Failed to create spi_request_queue\r\n");
        while (1);
    }

#if SCAN_I2C_ADDRESS_ENABLE
    Scan_I2C_Devices(BOARD_PMIC_I3C_BASEADDR);
#endif

	/* Init GPIO */
    GPIO_PortInit(GPIO, GPIO0_PORT);
    GPIO_PortInit(GPIO, GPIO2_PORT);
    gpio_pin_config_t output_int_config = {kGPIO_DigitalOutput, 0,};
    GPIO_PinInit(GPIO, 2, 15, &output_int_config);
    GPIO_PinWrite(GPIO, 2, 15, 0);  // 預設為低位

	GPIO_PinInit(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, &output_int_config);
    GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 0);
    GPIO_PinInit(GPIO, RESET553_N_PORT, RESET553_N_PIN, &output_int_config);

    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config    = {kGPIO_DigitalInput, 0};
    gpio_interrupt_config_t config = {kGPIO_PinIntEnableEdge, kGPIO_PinIntEnableLowOrFall};
    NVIC_SetPriority(GPIO_INTA_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    EnableIRQ(GPIO_INTA_IRQn);

    /* Touch GPIO*/
    GPIO_PinInit(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &sw_config);
    GPIO_SetPinInterruptConfig(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, &config);
    GPIO_PinEnableInterrupt(GPIO, TOUCH_INT_PORT, TOUCH_INT_PIN, kGPIO_InterruptA);

    /* Initialize PINT */ /* Init FUN_KEY1 & Power_Key*/
	PINT_Init(EXAMPLE_PINT_BASE);
	NVIC_SetPriority(PIN_INT0_IRQn + FUN_KEY1_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	NVIC_SetPriority(PIN_INT0_IRQn + POWER_KEY_PINT_CH, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

	PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt0, kPINT_PinIntEnableBothEdges, pint_intr_callback);
	PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt0);
	PINT_PinInterruptConfig(EXAMPLE_PINT_BASE, kPINT_PinInt1, kPINT_PinIntEnableBothEdges, pint_intr_callback);
	PINT_EnableCallbackByIndex(EXAMPLE_PINT_BASE, kPINT_PinInt1);

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

    /* ======================PMIC PCA9422================== */
	uint8_t top_stat = 0;
	glf70583_i2c_read(GLF70583_A_I2C_ADDR,0x00,&top_stat,1);
	PRINTF("[GLF70583]top_stat:%X \n",top_stat);

	//Solution: The manufacturer did not set it to LOAD SWITCH
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0xF5, 0xC6);
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB8);
	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x24, 0xB9);

	// BUCK1 Delay 4ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR,0x66, 0x0C);
	// BUCK2 Delay 2ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x67, 0x08);
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x67, 0x08);
	// BUCK3 Delay 0ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x68, 0x00);
	// LDO1 Delay 5ms
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x6A, 0x12);
	// 0x25->BUCK4、LDO2 off
	glf70583_i2c_write(GLF70583_A_I2C_ADDR, 0x26, 0xE8);
	// 0x26->BUCK1、2、4 ON、Others off
	glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0xD0);
	//glf70583_i2c_write(GLF70583_B_I2C_ADDR, 0x26, 0x40);//BUCK2 ON、Others off

	PRINTF("GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); \n");
	GPIO_PinWrite(GPIO, PWR_SW1_PORT, PWR_SW1_PIN, 1); //Enable GLF70583

	SDK_DelayAtLeastUs(10000, CLOCK_GetFreq(kCLOCK_CoreSysClk));//delay 10ms
	PRINTF("GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1); \n");
	GPIO_PinWrite(GPIO, RESET553_N_PORT, RESET553_N_PIN, 1);

	/* Init I2C Component */
	awinic_single_enter();



	/* ===== A. 建立 I2C EventGroup 與 Mutex ===== */
	i2c_event_group = xEventGroupCreate();
	configASSERT(i2c_event_group);

	i2c_mutex = xSemaphoreCreateMutex();
	configASSERT(i2c_mutex);


	/* 建立 tasks */
    /* <<< MODIFIED: 建立 spi_handler_task 來取代舊的 console_task >>> */
    if (xTaskCreate(spi_handler_task, "SPI_HANDLER", configMINIMAL_STACK_SIZE + 500, NULL,
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

	if (xTaskCreate(button_task, "BUTTON", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL)!= pdPASS)
    {
        PRINTF(" BUTTON Task creation failed!.\r\n");
        while (1);
    }
	if (xTaskCreate(power_key_task, "POWER_KEY", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY + 2, NULL)!= pdPASS)
    {
        PRINTF(" POWER_KEY Task creation failed!.\r\n");
        while (1);
    }
	/* ===== C. 建立 I2C_Task（建議比 passive_handler_task 略高，避免事件延遲） ===== */
	if (xTaskCreate(I2C_Task, "I2C_TASK",
	                configMINIMAL_STACK_SIZE + 256,
	                NULL,
	                tskIDLE_PRIORITY + 3,
	                &sI2CTaskHandle) != pdPASS)
	{
	    PRINTF("I2C_TASK creation failed!\r\n");
	    while (1) { ; }
	}


	vTaskStartScheduler();



    for (;;);
}
