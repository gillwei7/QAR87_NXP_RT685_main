/*
 * spi_handler.c
 *
 *  Created on: 2025年10月15日
 *      Author: 11301026
 */

#include "spi_handler.h"
#include "i2c_component_handler.h"
#include "system_status.h"

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
static uint8_t dataFrame4[STATUS_BUFFER_SIZE] = {0xAA, 0x00, 0x93, 0x00, 0x00, 0x00, 0x00};

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

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void execute_active_spi_transmission(uint8_t hex_value); // <<< NEW: 獨立的 SPI 執行函式
static void SPI_StartFrame_PreloadTX(uint8_t* srcBuff, uint32_t frameSize);
static inline void SPI_EnableRxTxInterrupt(void);
static inline void SPI_DisableRxTxInterrupt(void);
static uint8_t calculateChecksum(uint8_t* data, uint8_t len);
static void init_passive_mode(void);
static void handle_passive_ack_frame(const uint8_t *frame);
static void handle_active_ack_frame(const uint8_t *frame);

// <<< MODIFIED: 核心架構變更，使用 Queue 取代 Event Group 進行任務間通訊 >>>
QueueHandle_t spi_request_queue = NULL;
EventGroupHandle_t spi_event_group = NULL;
SemaphoreHandle_t spi_semaphore = NULL;

extern volatile SystemStatus ss ;
uint8_t Novatek_boot_completed = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/
static void handle_active_ack_frame(const uint8_t *frame)
{
    const uint8_t prefix = frame[0];
    const uint8_t cmd    = frame[1];
    const uint8_t val    = frame[2];
    const uint8_t cs_rx  = frame[3];
    const uint8_t cs_exp = calculateChecksum((uint8_t*)frame, 3);

    // 基本防呆（其實前面 ISR 已驗過，但保險再檢一次）
    if (prefix != 0xAA || cs_rx != cs_exp) {
        PRINTF("[Active] Handle: prefix/cs error (got %02X, cs %02X!=%02X)\r\n", prefix, cs_rx, cs_exp);
        return;
    }

    switch (cmd) {
        case 0x00:
            if (val == 0x00) {
                PRINTF("[Active] ACK:[00 00] Nova close all component(led,Dmic,AMP) \r\n");
                led_post_event(LED_EVT_ALL_OFF);
                // TODO: Dmic,AMP
            }
            break;
        default:
            PRINTF("[Active] CMD=0x%02X (VAL=0x%02X) not handled yet\r\n", cmd, val);
            // TODO: 依你們協議擴充
            break;
    }
}

static void handle_passive_ack_frame(const uint8_t *frame)
{
    const uint8_t prefix = frame[0];
    const uint8_t cmd    = frame[1];
    const uint8_t val    = frame[2];
    const uint8_t cs_rx  = frame[3];
    const uint8_t cs_exp = calculateChecksum((uint8_t*)frame, 3);

    // 基本防呆（其實前面 ISR 已驗過，但保險再檢一次）
    if (prefix != 0xAA || cs_rx != cs_exp) {
        PRINTF("[Passive] Handle: prefix/cs error (got %02X, cs %02X!=%02X)\r\n", prefix, cs_rx, cs_exp);
        return;
    }

    switch (cmd) {
        case 0x00:
        	if (val == 0x00) {
				PRINTF("[Passive] ACK:[00 00] Nova do nothing \r\n");
				led_post_event(LED_EVT_ALL_OFF);
			}

        	else if (val == 0x01) {
                PRINTF("[Passive] ACK:[00 01] Nova take photo success \r\n");
                led_post_event(LED_EVT_PHOTO_CAPTURE);
            }

            else if (val == 0x03) {
				PRINTF("[Passive] ACK:[00 03] Nova record video success \r\n");
				led_post_event(LED_EVT_VIDEO_CAPTURE);
			}
			break;

        case 0x11: //Nova boot completed
			if (val == 0x11) {
				PRINTF("[Passive] ACK:[11 11] Nova boot completed\r\n");
				Novatek_boot_completed = 1;
				led_post_event(LED_EVT_ALL_OFF);
			}
			break;

        default:
            PRINTF("[Passive] CMD=0x%02X (VAL=0x%02X) not handled yet\r\n", cmd, val);
            // TODO: 依你們協議擴充
            break;
    }
}

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
    SPI_EnableInterrupts(SOC_SPI_SLAVE, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
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
    SPI_DisableInterrupts(SOC_SPI_SLAVE, kSPI_TxLvlIrq | kSPI_RxLvlIrq);
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
    while (SPI_GetStatusFlags(SOC_SPI_SLAVE) & kSPI_RxNotEmptyFlag) {
        (void)SPI_ReadData(SOC_SPI_SLAVE);
    }
    currentFrameSize = frameSize;
    if (currentFrameSize > MAX_FRAME_SIZE) {
        currentFrameSize = MAX_FRAME_SIZE;
    }
    txIndex = currentFrameSize;
    rxIndex = currentFrameSize;
    currentSrcBuff = srcBuff;
    while ((txIndex > 0U) && (SPI_GetStatusFlags(SOC_SPI_SLAVE) & kSPI_TxNotFullFlag)) {
        SPI_WriteData(SOC_SPI_SLAVE, currentSrcBuff[currentFrameSize - txIndex], 0);
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

    if ((SPI_GetStatusFlags(SOC_SPI_SLAVE) & kSPI_RxNotEmptyFlag) && (rxIndex > 0U)) {
        destBuff[currentFrameSize - rxIndex] = SPI_ReadData(SOC_SPI_SLAVE);
        rxIndex--;
    }
    if ((SPI_GetStatusFlags(SOC_SPI_SLAVE) & kSPI_TxNotFullFlag) && (txIndex > 0U)) {
        SPI_WriteData(SOC_SPI_SLAVE, currentSrcBuff[currentFrameSize - txIndex], 0);
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
    slave_config.sselPol = (spi_spol_t)SOC_SPI_SPOL;
    SPI_Deinit(SOC_SPI_SLAVE);
    SPI_SlaveInit(SOC_SPI_SLAVE, &slave_config);

    memset(destBuff, 0, sizeof(destBuff));
    PRINTF(">>> FIFOs cleared for ACTIVE mode <<<\r\n");

    uint8_t frames_to_send = 3;
    uint8_t* frame2_ptr = dataFrame2;
    uint32_t frame2_size = FIXED_BUFFER_SIZE;

    if (hex_value == SYSTEM_STATUS_HEX_VALUE) {
    	dataFrame1[2] = 0x07;
		dataFrame1[3] = calculateChecksum(dataFrame1, 3);
		dataFrame4[2] = hex_value;
		dataFrame4[3] = ss.flags;
		dataFrame4[4] = ss.layer;
		dataFrame4[5] = ss.batt;
		dataFrame4[6] = calculateChecksum(dataFrame4, 6);
		frame2_ptr = dataFrame4;
		frame2_size = STATUS_BUFFER_SIZE;
		PRINTF("Updated Fixed Frame 2: [%02X %02X %02X %02X %02X %02X %02X]\r\n",
			   dataFrame4[0], dataFrame4[1], dataFrame4[2], dataFrame4[3],
			   	   dataFrame4[4], dataFrame4[5], dataFrame4[6]);

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

        handle_active_ack_frame(destBuff); //Processing RX received in active mode

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
void spi_handler_task(void *pvParameters)
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
void passive_spi_handler_task(void *pvParameters)
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

            PRINTF("[Passive] Sequence completed. Parsing frame...\r\n");
            handle_passive_ack_frame(passive_rx_buffer);
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
void spi_task(void *pvParameters)
{
    EventBits_t bits;
    while (1) {
        bits = xEventGroupWaitBits(spi_event_group, EVT_TRANSFER_DONE, pdTRUE, pdFALSE, portMAX_DELAY);
        if ((bits & EVT_TRANSFER_DONE) != 0) {
            PRINTF("spi_task: Frame transfer done.\r\n");
        }
    }
}


