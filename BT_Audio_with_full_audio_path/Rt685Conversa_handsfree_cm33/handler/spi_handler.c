/*
 * spi_handler.c
 *
 *  Created on: 2025年10月15日
 *      Author: 11301026
 */
#if 1
#include "spi_handler.h"
#include "i2c_component_handler.h"
#include "system_status.h"
#include "app_handsfree.h"
#include "hal_led.h"
#include "fsl_crc.h"
#include "fsl_spi_dma.h"
#include "fsl_dma.h"
#include "hal_common.h"
#include "ringtone_handler.h"
#include "scenario_state.h"
#include "spi_command_set.h"

#define USE_DMA 1

#define USE_NEW_COMMAND 1

extern EventGroupHandle_t i2c_event_group ;

EventGroupHandle_t spi_event_group ;

QueueHandle_t spi_request_queue = NULL;

/*******************************************************************************
 * Variables
 ******************************************************************************/
static uint8_t srcBuff[BUFFER_SIZE];
static uint8_t destBuff[BUFFER_SIZE];
static volatile uint32_t txIndex = BUFFER_SIZE;
static volatile uint32_t rxIndex = BUFFER_SIZE;

static volatile uint8_t global_pending_msgtype = 0; // 用來存 hex_value
static volatile uint8_t global_pending_command = 0; // 用來存 hex_value

static volatile uint32_t currentFrameSize = 0;
static volatile uint8_t currentFrame = 0;
static uint8_t* currentSrcBuff = NULL;

/* Globals for Streaming */
static stream_ctx_t g_stream = {0}; // 初始化

extern volatile SystemStatus ss;
uint8_t Novatek_boot_completed = 0;
SemaphoreHandle_t sys_bus_mutex = NULL;

SemaphoreHandle_t spi_streaming_done_semaphore;

static volatile bool global_priority_cmd_pending = false; /* === 高優先級指令旗標 === */
static volatile bool g_streaming_tx_success = false; // 紀錄最後一包串流是否成功

static bool s_dma_initialized = false;

static void InitCrc32(CRC_Type *base, uint32_t seed);
static void spi_setting_reset(void);
static void spi_setting_reset_receive_mode(void);
static void spi_dma_setting_reset(void);
static void spi_dma_setting_reset_receive_mode(void);
static void printf_tx_data(void);
static void printf_rx_data(void);
static bool spi_check_rx_data_crc32(const uint8_t *data, size_t length);
static uint32_t spi_calculate_tx_data_crc32(const uint8_t *data, size_t length);
static void SPI_StartFrame_PreloadTX(uint8_t* srcBuff, uint32_t frameSize);
#if USE_NEW_COMMAND
static void spi_process_atomic_event(uint8_t event_id, const uint8_t *args);
static void spi_prepare_command_packet_data(uint8_t msg_type, uint8_t cmd_id);
#else if
static void spi_prepare_command_packet_data(uint8_t hex_value);
static void spi_process_atomic_command(uint8_t cmd, uint8_t val);
#endif
static void spi_prepare_generic_packet(uint8_t msg_type, uint32_t packet_id, uint8_t *payload_ptr, uint32_t payload_len);


static void SPI_SlaveUserCallback(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData);
static void EXAMPLE_SlaveDMASetup(void);
static void EXAMPLE_SlaveStartDMATransfer(void);

#define TRANSFER_SIZE 532U

uint8_t slaveRxData[TRANSFER_SIZE] = {0};
uint8_t slaveTxData[TRANSFER_SIZE] = {0};

dma_handle_t slaveTxHandle;
dma_handle_t slaveRxHandle;

spi_dma_handle_t slaveHandle;


uint8_t queued_cmd;

volatile bool isTransferCompleted = false;
volatile slave_fsm_state_t g_slave_state = S_IDLE;

#define MSG_CMD_SESSION_START  0x01
#define MSG_CMD_SESSION_END    0x02
#define MSG_DATA_FIRMWARE      0x10
#define MSG_DATA_FILE          0x11
#define MSG_CMD_ATOMIC_EXEC    0x20
#define MSG_CMD_ATOMIC_STATUS  0x21
#define MSG_CMD_ABORT          0x23

/* 會話上下文 (Session Context) - 用於記憶長傳輸狀態 */
typedef struct {
    bool     active;        // 是否處於會話中
    uint32_t expected_id;   // 預期接收的下一個 Packet ID
    uint32_t received_bytes;// 已接收總位元組數 (用於除錯)
    uint32_t current_crc; // 用於存儲軟體 CRC 的累積值
} session_ctx_t;

/* 靜態變數，保持狀態跨越多次 handle_passive_receive 調用 */
static session_ctx_t g_session = { .active = false, .expected_id = 0, .received_bytes = 0 };


static msg_notification_info_t g_msg_info = {0};


/* 宣告全域變數來儲存時間資訊 (可給予預設值) */
static spi_command_time_info_t s_system_time = {
    .year   = 2026,
    .month  = 3,
    .day    = 16,
    .hour   = 12,
    .minute = 0,
    .second = 0
};

void set_system_time(uint16_t year, uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second)
{
	s_system_time.year = year ;
	s_system_time.month = month ;
	s_system_time.day  = day ;
	s_system_time.hour = hour ;
	s_system_time.minute = minute ,
	s_system_time.second = second ;
}

void application_examples_atomic_status(void)
{
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_SYS_STATUS);
	vTaskDelay(100);

	s_system_time.day  = 17;
	s_system_time.hour = 11;
	s_system_time.minute = 11,
	s_system_time.second = 11;
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_TIME_SYNC);
	vTaskDelay(100);

    message_processing(APP_TYPE_EMAIL, "SW_weekly_report_0311_Daryl", "Dear Lydia, Jason FYI Best Regards Daryl");
    send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_MSG_NOTIFICATION);
    vTaskDelay(100);
    message_processing(APP_TYPE_WECHAT, "壓電瓷磚（Piezoelectric tiles）技術", "壓電瓷磚技術是一種透過行人走路的動能轉換為電能的綠色能源解決方案。當人踏在瓷磚上時，壓力會使內部的壓電材料產生變形，從而產生壓電效應，將機械能轉化為電能。這種瓷磚能儲存能量以供驅動 LED 燈、數位顯示器或感測器使用，適合高人流量區域。");
    send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_MSG_NOTIFICATION);
    vTaskDelay(100);
    message_processing(APP_TYPE_LINE, "今天星期三", "晚上吃好料");
    send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_MSG_NOTIFICATION);
    vTaskDelay(100);

    send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_VERSION_INFO_SYNC);

}
void application_examples_atomic_exec(void)
{

    // 建立陣列
    const uint8_t codes[] = {
        0x01, 0x02, 0x04,
        0x11, 0x15, 0x17, 0x19,
        0x12, 0x13, 0x14, 0x1C,
        0x1D, 0x21, 0x22, 0x23,
    };

    // 計算陣列長度
    const size_t count = sizeof(codes) / sizeof(codes[0]);

	// 逐一輸出（索引 + 十六進位值）
	for (size_t i = 0; i < count; ++i) {
		send_spi_request(CMD_ATOMIC_EXEC, codes[i]);
		vTaskDelay(100);
	}

	set_scenario_state(SCENARIO_STATE_HOME);
	send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_SWITCH_UI_PAGE);

}

void message_processing(app_msg_type_t app_type, const char *title, const char *body)
{
    // 1. 儲存 AppType
    g_msg_info.app_type = (uint8_t)app_type;

    // 2. 清空原本的緩衝區 (可選，但比較安全)
    memset(g_msg_info.title, 0, MAX_MSG_TITLE_LEN);
    memset(g_msg_info.body, 0, MAX_MSG_BODY_LEN);

    // 3. 處理 Title 字串
    /* --- 3-1. 處理 Title 字串 (自動補空白) --- */
    if (title != NULL && title[0] != '\0') {
        size_t t_len = strlen(title);

        // 確保長度不超過 Buffer，並且預留 1 個 Byte 給「自動空白」
        if (t_len >= MAX_MSG_TITLE_LEN) {
            t_len = MAX_MSG_TITLE_LEN - 1;
        }

        // 拷貝原始字串
        memcpy(g_msg_info.title, title, t_len);

        // 在字串最後強制補上一個空白字元 (0x20) 當作替死鬼
        g_msg_info.title[t_len] = ' ';

        // 實際要傳送的長度是 原始長度 + 1
        g_msg_info.title_len = (uint8_t)(t_len + 1);
    } else {
        g_msg_info.title_len = 0;
    }

    /* --- 3-2. 處理 Body 字串 (自動補空白) --- */
    if (body != NULL && body[0] != '\0') {
        size_t b_len = strlen(body);

        // 確保長度不超過 Buffer，並且預留 1 個 Byte 給「自動空白」
        if (b_len >= MAX_MSG_BODY_LEN) {
            b_len = MAX_MSG_BODY_LEN - 1;
        }

        // 拷貝原始字串
        memcpy(g_msg_info.body, body, b_len);

        // 在字串最後強制補上一個空白字元 (0x20)
        g_msg_info.body[b_len] = ' ';

        // 實際要傳送的長度是 原始長度 + 1
        g_msg_info.body_len = (uint16_t)(b_len + 1);
    } else {
        g_msg_info.body_len = 0;
    }
}


#if USE_NEW_COMMAND
static void spi_process_atomic_event(uint8_t event_id,const uint8_t *args)
{
	switch (event_id) {

		case CMD_ATOMIC_EVENT_SYSTEM_BOOT_DONE:

			PRINTF("[App] Nova boot completed\r\n");
			Novatek_boot_completed = 1;
	//			hal_led_refresh();
			battery_timer_start();
			led_post_event(HAL_LED_EVENT_REFRESH);
			set_ringtone_state(Ringtone_PowerON);
			break;

		case CMD_ATOMIC_EVENT_CAMERA_ACTIVATED:
			PRINTF("[SPI][Event] CAMERA_ACTIVATED \r\n ");
			break;

		case CMD_ATOMIC_EVENT_PHOTO_CAPTURED:
			PRINTF("[SPI][Event] PHOTO_CAPTURED \r\n ");
			led_post_event(HAL_LED_EVENT_REFRESH);
			ss_set_capture_status(STATUS_END);
			break;

		case CMD_ATOMIC_EVENT_CAMERA_CLOSED:
			PRINTF("[SPI][Event] CAMERA_CLOSED \r\n ");
			break;

		case CMD_ATOMIC_EVENT_RECORDING_STARTED:
			PRINTF("[SPI][Event] RECORDING_STARTED \r\n ");
			ss_set_recording_status(STATUS_START);
			break;

		case CMD_ATOMIC_EVENT_RECORDING_STOPPED:
			PRINTF("[SPI][Event] RECORDING_STOPPED \r\n ");
			ss_set_recording_status(STATUS_END);
			break;

		case CMD_ATOMIC_EVENT_VIDEO_PLAY_STARTED:
			PRINTF("[SPI][Event] VIDEO_PLAY_STARTED \r\n ");
			break;
		case CMD_ATOMIC_EVENT_VIDEO_PLAY_PAUSED:
			PRINTF("[SPI][Event] VIDEO_PLAY_PAUSED \r\n ");
			if (get_media_status() == MUSIC_PAUSE) {
				set_media_status(MUSIC_PLAYING);
			} else if (get_media_status() == MUSIC_PLAYING) {
				set_media_status(MUSIC_PAUSE);
			}
			break;

		case CMD_ATOMIC_EVENT_WIFI_CONNECTED:
			PRINTF("[SPI][Event] WIFI_CONNECTED \r\n ");
			break;

		case CMD_ATOMIC_EVENT_WIFI_DISCONNECTED:
			PRINTF("[SPI][Event] WIFI_DISCONNECTED \r\n ");
//			set_ringtone_state(Ringtone_WiFi_Disconnected);

			break;
		case CMD_ATOMIC_EVENT_MSG_NOTIFIED:
			PRINTF("[SPI][Event] MSG_NOTIFIED \r\n ");
			break;

		case CMD_ATOMIC_EVENT_UI_PAGE_CHANGED:

			uint8_t CurrentPageID,CurrentSubPageID ,ChangeReason ,ResultCode ;

			CurrentPageID = args[0];
			CurrentSubPageID = args[1];
			ChangeReason  = args[2];
			ResultCode = args[3];

			PRINTF("[App] UI Page Changed: \r\n ");
			PRINTF("\t CurrentPageID:0x%02X CurrentSubPageID:0x%02X \r\n",CurrentPageID,CurrentSubPageID);
			PRINTF("\t ChangeReason:0x%02X  ResultCode:0x%02X \r\n",ChangeReason,ResultCode);

			break;

		case CMD_ATOMIC_EVENT_OTA_STARTED:
			PRINTF("[SPI][Event] OTA_STARTED \r\n ");
			break;

		case CMD_ATOMIC_EVENT_OTA_FINISHED:
			PRINTF("[SPI][Event] OTA_FINISHED \r\n ");
			break;

		case CMD_ATOMIC_EVENT_UNKNOWN_CMD_ERROR:
			PRINTF("[SPI][Event] UNKNOWN_CMD_ERROR \r\n ");
			break;

	}

}

#else if
static void spi_process_atomic_command(uint8_t cmd, uint8_t val)
{
    switch (cmd) {
        case 0x00:
            if (val == 0x00) {
                PRINTF("[App] CMD:00 VAL:%02X -> Nova do nothing \r\n", val);
                hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_DISABLE);
                led_post_event(HAL_LED_EVENT_REFRESH);
            }
            else if (val == 0x01) {
                PRINTF("[App] CMD:00 VAL:%02X -> Capture Start \r\n", val);
                led_post_event(HAL_LED_EVENT_PHOTO_CAPTURE);
                // ToDo:使NXP發出 "滴～喀嚓 "聲音
                ss_set_capture_status(STATUS_START);
            }
            else if (val == 0x03) {
                PRINTF("[App] CMD:00 VAL:%02X -> Capture Completed \r\n", val);
                led_post_event(HAL_LED_EVENT_REFRESH);
                ss_set_capture_status(STATUS_END);
            }
            else if (val == 0x04) {
                PRINTF("[App] CMD:00 VAL:%02X -> Recording Start \r\n", val);
                hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_ENABLE);
                led_post_event(HAL_LED_EVENT_REFRESH);
                ss_set_recording_status(STATUS_START);
            }
            else if (val == 0x05) {
                PRINTF("[App] CMD:00 VAL:%02X -> Recording Completed \r\n", val);
                hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_DISABLE);
                led_post_event(HAL_LED_EVENT_REFRESH);
                ss_set_recording_status(STATUS_END);
            }
            else if (val == 0x06) {
                PRINTF("[App] CMD:00 VAL:%02X -> Media Playing \r\n", val);
                if (get_media_status() == MUSIC_PAUSE) {
                    set_media_status(MUSIC_PLAYING);
                }
            }
            else if (val == 0x07) {
                PRINTF("[App] CMD:00 VAL:%02X -> Media Pause \r\n", val);
                if (get_media_status() == MUSIC_PLAYING) {
                    set_media_status(MUSIC_PAUSE);
                }
            }
            else if (val == 0x08) {
                PRINTF("[App] CMD:00 VAL:%02X -> Wi-Fi connection \r\n", val);
            }
            else if (val == 0x09) {
                PRINTF("[App] CMD:00 VAL:%02X -> WiFi disconnection \r\n", val);
                set_ringtone_state(Ringtone_WiFi_Disconnected);
            }
            break;

        case 0x11: // Nova boot completed
            if (val == 0x11) {
                PRINTF("[App] CMD:11 VAL:11 -> Nova boot completed\r\n");
                Novatek_boot_completed = 1;
                hal_led_refresh();
                battery_timer_start();
                set_ringtone_state(Ringtone_PowerON);
            }
            break;

        case 0x40: // Novatek update usage state
            PRINTF("[App] CMD:40 VAL:%02X -> Update Usage State \r\n", val);
            set_scenario_state(val);
            break;

        case 0x50: // Update Layer
            PRINTF("[App] CMD:50 VAL:%02X -> Update Layer \r\n", val);
            ss.layer = val;
            break;

        default:
            PRINTF("[App] Unknown CMD=0x%02X (VAL=0x%02X)\r\n", cmd, val);
            break;
    }
}
#endif

static void spi_dma_setting_reset(void)
{
	spi_setting_reset();
	EXAMPLE_SlaveDMASetup();
}

static void spi_dma_setting_reset_receive_mode(void)
{
	memset(slaveTxData, 0, TRANSFER_SIZE);
	spi_dma_setting_reset();
    EXAMPLE_SlaveStartDMATransfer();
}

static void SPI_SlaveUserCallback(SPI_Type *base, spi_dma_handle_t *handle, status_t status, void *userData)
{
    if (status == kStatus_Success)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        isTransferCompleted = true;

        /* DMA callback runs in IRQ context, so use ISR-safe API. */
        if (spi_event_group != NULL)
        {
            xEventGroupSetBitsFromISR(spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT, &xHigherPriorityTaskWoken);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

}

static void EXAMPLE_SlaveStartDMATransfer(void)
{
    uint32_t i = 0U;
    spi_transfer_t slaveXfer;

    /* 重置旗標 */
    isTransferCompleted = false;

    /* Create handle for slave instance. */
    SPI_SlaveTransferCreateHandleDMA(SOC_SPI_SLAVE, &slaveHandle, SPI_SlaveUserCallback, NULL, &slaveTxHandle,
                                     &slaveRxHandle);

    slaveXfer.txData   = (uint8_t *)&slaveTxData;
    slaveXfer.rxData   = (uint8_t *)&slaveRxData;
    slaveXfer.dataSize = TRANSFER_SIZE * sizeof(slaveTxData[0]);
    slaveXfer.configFlags = kSPI_FrameAssert;

    /* Start transfer, when transmission complete, the SPI_SlaveUserCallback will be called. */
    if (kStatus_Success != SPI_SlaveTransferDMA(SOC_SPI_SLAVE, &slaveHandle, &slaveXfer))
    {
        PRINTF("There is an error when start SPI_SlaveTransferDMA \r\n");
    }
}

static void EXAMPLE_SlaveDMASetup(void)
{
    /* DMA init */
    //DMA_Init(EXAMPLE_DMA);

    if (s_dma_initialized) {
        // 非第一次：先清除 CH26/CH27 的殘留硬體狀態
        DMA_AbortTransfer(&slaveTxHandle);
        DMA_AbortTransfer(&slaveRxHandle);
    }

    /* configure channel/priority and create handle for TX and RX. */
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_SPI_SLAVE_TX_CHANNEL);
    DMA_EnableChannel(EXAMPLE_DMA, EXAMPLE_SPI_SLAVE_RX_CHANNEL);
    DMA_SetChannelPriority(EXAMPLE_DMA, EXAMPLE_SPI_SLAVE_TX_CHANNEL, kDMA_ChannelPriority0);
    DMA_SetChannelPriority(EXAMPLE_DMA, EXAMPLE_SPI_SLAVE_RX_CHANNEL, kDMA_ChannelPriority1);
    DMA_CreateHandle(&slaveTxHandle, EXAMPLE_DMA, EXAMPLE_SPI_SLAVE_TX_CHANNEL);
    DMA_CreateHandle(&slaveRxHandle, EXAMPLE_DMA, EXAMPLE_SPI_SLAVE_RX_CHANNEL);

    s_dma_initialized = true;
}

static void spi_setting_reset(void)
{
    SPI_DisableInterrupts(SOC_SPI_SLAVE, kSPI_RxLvlIrq | kSPI_TxLvlIrq);
    spi_slave_config_t slave_config;
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)SOC_SPI_SPOL;

    /* 重置 SPI IP，這會瞬間清空硬體 FIFO 裡殘留的 */
    SPI_Deinit(SOC_SPI_SLAVE);
    SPI_SlaveInit(SOC_SPI_SLAVE, &slave_config);
}

static void spi_setting_reset_receive_mode(void)
{
	spi_setting_reset();

    memset(srcBuff, 0, BUFFER_SIZE);
    SPI_StartFrame_PreloadTX(srcBuff, BUFFER_SIZE);
    SPI_EnableInterrupts(SOC_SPI_SLAVE, kSPI_RxLvlIrq | kSPI_TxLvlIrq);
}

static void printf_rx_data(void)
{
	for ( uint32_t i = 0; i < BUFFER_SIZE; i++)
	{
		PRINTF("0x%02X ", destBuff[i]);
		if ((i + 1) % 10 == 0)
		{
			PRINTF("\r\n");
		}
	}
}

static void printf_tx_data(void)
{
	for (uint32_t i = 0; i < BUFFER_SIZE; i++)
	{
		PRINTF("0x%02X ", srcBuff[i]);
		if ((i + 1) % 10 == 0)
		{
			PRINTF("\r\n");
		}
	}
}

static void spi_prepare_generic_packet(uint8_t msg_type, uint32_t packet_id, uint8_t *payload_ptr, uint32_t payload_len)
{
    /* 定義常數 */
    const uint32_t header_len  = HEADER_LEN; // 16
    const uint32_t payload_max = PAYLOAD_LEN; // 512

    uint32_t crc32Result = 0;

    /* 1. 清空緩衝區 (Padding 0x00) */
    memset(srcBuff, 0, BUFFER_SIZE);

    /* 2. 填寫 Header (Offset 0x00 ~ 0x0F) */
    srcBuff[0] = 0xAA; // SYNC_HEAD Low
    srcBuff[1] = 0x55; // SYNC_HEAD High
    srcBuff[2] = 0x02; // PROT_VER
    srcBuff[3] = msg_type; // [動態] MSG_TYPE

    // PACKET_ID (Little Endian)
    srcBuff[4] = (uint8_t)(packet_id & 0xFF);
    srcBuff[5] = (uint8_t)((packet_id >> 8) & 0xFF);
    srcBuff[6] = (uint8_t)((packet_id >> 16) & 0xFF);
    srcBuff[7] = (uint8_t)((packet_id >> 24) & 0xFF);

    // DATA_LEN (Payload 的有效長度)
    srcBuff[8]  = (uint8_t)(payload_len & 0xFF);
    srcBuff[9]  = (uint8_t)((payload_len >> 8) & 0xFF);
    srcBuff[10] = (uint8_t)((payload_len >> 16) & 0xFF);
    srcBuff[11] = (uint8_t)((payload_len >> 24) & 0xFF);

    // Reserved at 12~15 is 0x00 (memset done)

    /* 3. 填寫 Payload */
    if (payload_ptr != NULL && payload_len > 0) {
        // 確保不超過 512 bytes
        uint32_t copy_len = (payload_len > payload_max) ? payload_max : payload_len;
        memcpy(&srcBuff[header_len], payload_ptr, copy_len);
    }

    /* 4. 計算單包 CRC32 (Header 16 + Payload 512 = 528 Bytes) */
    crc32Result = spi_calculate_tx_data_crc32(srcBuff, header_len + payload_max);

    /* 5. 填寫 CRC (Offset 528) */
    uint8_t *pCrc = &srcBuff[header_len + payload_max];
    pCrc[0] = (uint8_t)(crc32Result & 0xFF);
    pCrc[1] = (uint8_t)((crc32Result >> 8) & 0xFF);
    pCrc[2] = (uint8_t)((crc32Result >> 16) & 0xFF);
    pCrc[3] = (uint8_t)((crc32Result >> 24) & 0xFF);

    PRINTF("[SPI][Gen] Type=0x%02X, ID=%u, Len=%u, CRC=0x%08X\r\n", msg_type, packet_id, payload_len, crc32Result);
}

#if USE_NEW_COMMAND
static void spi_prepare_command_packet_data(uint8_t msg_type, uint8_t cmd_id)
{
    uint32_t crc32Result = 0;

    /* 1. 清空緩衝區 (Padding 0x00) */
    memset(srcBuff, 0, BUFFER_SIZE);

    /* 2. 填寫 Header (Offset 0x00 ~ 0x0F) */
    // Offset 0x00: SYNC_HEAD (0x55AA) -> Little Endian: AA 55
    srcBuff[0] = 0xAA;
    srcBuff[1] = 0x55;

    // Offset 0x02: PROT_VER (0x02)
    srcBuff[2] = 0x02;

    // Offset 0x03: MSG_TYPE (依據傳入參數: 0x20 或 0x21)
    srcBuff[3] = msg_type;

    // Offset 0x04: PACKET_ID (原子指令固定填 0x00000000，memset 已清零)
    // Offset 0x0C: RESERVED  (固定填 0x00000000，memset 已清零)

    /* 3. 填寫 Payload (Offset 0x10 ~) */
    srcBuff[HEADER_LEN + 0] = cmd_id;   // payload[0] = cmd_id
    srcBuff[HEADER_LEN + 1] = 0x01;     // payload[1] = fmt_ver (目前固定 0x01)

    uint16_t arg_len = 0;
    uint8_t *pArgs = &srcBuff[HEADER_LEN + 4]; // 參數寫入的起始位置 (payload[4..])

    /* 根據 msg_type 與 cmd_id 填寫對應的參數 (Args) */
     if (msg_type == CMD_ATOMIC_EXEC) // CMD_ATOMIC_EXEC (NXP -> Novatek 控制指令)
     {
         switch (cmd_id)
         {

			 case CMD_ATOMIC_EXEC_SWITCH_UI_PAGE: // SWITCH_UI_PAGE
				 arg_len = 2;
				 pArgs[0] = get_ui_page_id();
				 pArgs[1] = 0x00; // TargetSubPageID
				 break;

             case CMD_ATOMIC_EXEC_MEDIA_PLAY_PAUSE: // MEDIA PLAY/PAUSE
                 arg_len = 1;
                 pArgs[0] = get_media_play_pause_cmd(); // 0x00: Toggle, 0x01: Force Play, 0x02: Force Pause (請依實際需求帶入)
                 break;

             default:
                 // 其他控制指令如 OPEN OE(0x01), TAKE PICTURE(0x21) 等，無參數
                 arg_len = 0;
                 break;
         }
     }
     else if (msg_type == CMD_ATOMIC_STATUS) // CMD_ATOMIC_STATUS (NXP -> Novatek 資訊同步)
     {
         switch (cmd_id)
         {
			 case CMD_ATOMIC_STATUS_SYS_STATUS: // SYS STATUS SYNC (依照新規格 4.3 修正為 4 Bytes)
				 arg_len = 4;
				 pArgs[0] = ss_get_battery();         // BatteryLevel (0-100)
				 pArgs[1] = ss_is_charging() ? 1 : 0; // ChargingStatus (0/1)
				 pArgs[2] = ss_bt_is_on() ? 1 : 0;    // BT_ConnStatus (0/1)
				 pArgs[3] = ss_ble_is_on() ? 1 : 0;   // BLE_ConnStatus (0/1)
				 break;

             case CMD_ATOMIC_STATUS_TIME_SYNC: // TIME SYNC
                 arg_len = 7;

                 /* Args[0-1]: Year (uint16_t, Big Endian) */
                 /* Big Endian: 高位元組存放在低記憶體位址 (pArgs[0]) */
                 pArgs[0] = (uint8_t)((s_system_time.year >> 8) & 0xFF);
                 pArgs[1] = (uint8_t)(s_system_time.year & 0xFF);

                 /* Args[2]: Month (1-12) */
                 pArgs[2] = s_system_time.month;

                 /* Args[3]: Day (1-31) */
                 pArgs[3] = s_system_time.day;

                 /* Args[4]: Hour (0-23) */
                 pArgs[4] = s_system_time.hour;

                 /* Args[5]: Minute (0-59) */
                 pArgs[5] = s_system_time.minute;

                 /* Args[6]: Second (0-59) */
                 pArgs[6] = s_system_time.second;
                 break;

             case CMD_ATOMIC_STATUS_MSG_NOTIFICATION: // MSG NOTIFICATION
                 /* arg_len = 4 + N + M */
                 arg_len = 4 + g_msg_info.title_len + g_msg_info.body_len;

                 /* Args[0]: AppType */
                 pArgs[0] = g_msg_info.app_type;

                 /* Args[1]: TitleLength (N) */
                 pArgs[1] = g_msg_info.title_len;

                 /* Args[2]: BodyLength_L (N) */
                 pArgs[2] = (uint8_t)(g_msg_info.body_len & 0xFF);
                 /* Args[3]: BodyLength_H (N) */
                 pArgs[3] = (uint8_t)((g_msg_info.body_len >> 8) & 0xFF);

                 if (g_msg_info.title_len > 0) {
                     memcpy(&pArgs[4], g_msg_info.title, g_msg_info.title_len);
                 }

                 if (g_msg_info.body_len > 0) {
                     memcpy(&pArgs[4 + g_msg_info.title_len], g_msg_info.body, g_msg_info.body_len);
                 }


                 break;

             case CMD_ATOMIC_STATUS_VERSION_INFO_SYNC: // VERSION INFO

                 /* 1. 取得字串長度 (N) */
                 // 這裡強制轉型成 const char* 給 strlen 使用
                 size_t n_len = strlen((const char *)HAL_MCU_APP_VERSION);

                 /* 2. 防護機制：限制長度不超過建議值 128 (同時也防禦 255 的溢位) */
                 if (n_len > 128) {
                     n_len = 128;
                 }

                 /* 3. 設定總 arg_len = 1 (長度欄位) + N (字串內容) */
                 arg_len = (uint16_t)(1 + n_len);

                 /* 4. Args[0]: VersionStringLength (N) */
                 pArgs[0] = (uint8_t)n_len;

                 /* 5. Args[1 ~ N]: VersionString */
                 if (n_len > 0) {
                     // 拷貝字串內容，從 pArgs[1] 開始放
                     memcpy(&pArgs[1], HAL_MCU_APP_VERSION, n_len);
                 }

                 break;

         /*ToDo: Structure processing of special instructions
          *
          *
             case CMD_ATOMIC_STATUS_VOLUME_INFO_SYNC: // VOLUME INFO SYNC (依照新規格 4.5)
                 arg_len = 2;
                 pArgs[0] = audio_status.vol_level;   // VolumeLevel (0-100)
                 pArgs[1] = audio_status.is_mute;     // MuteStatus (0/1)
                 break;

          */
             default:
                 arg_len = 0;
                 break;
         }
     }

     // payload[2] = arg_len_L
     srcBuff[HEADER_LEN + 2] = (uint8_t)(arg_len & 0xFF);
     // payload[3] = arg_len_H
     srcBuff[HEADER_LEN + 3] = (uint8_t)((arg_len >> 8) & 0xFF);

     /* 計算 DATA_LEN: Payload 表頭(4) + 參數長度(arg_len) */
     uint32_t payload_valid_len = 4 + arg_len;

     // Offset 0x08: DATA_LEN (4 Bytes, Little Endian)
     srcBuff[8]  = (uint8_t)(payload_valid_len & 0xFF);
     srcBuff[9]  = (uint8_t)((payload_valid_len >> 8) & 0xFF);
     srcBuff[10] = (uint8_t)((payload_valid_len >> 16) & 0xFF);
     srcBuff[11] = (uint8_t)((payload_valid_len >> 24) & 0xFF);

     /* 4. 計算 CRC32 */
     // 計算範圍：Header (16) + Payload (512) = 528 Bytes (含 Padding 的 0x00)
     uint32_t calc_len = HEADER_LEN + PAYLOAD_LEN;
     crc32Result = spi_calculate_tx_data_crc32(srcBuff, calc_len);

     /* 5. 填寫 CRC (Offset 0x210 / 528) - Little Endian */
     uint8_t *pCrc = &srcBuff[calc_len];
     pCrc[0] = (uint8_t)(crc32Result & 0xFF);
     pCrc[1] = (uint8_t)((crc32Result >> 8) & 0xFF);
     pCrc[2] = (uint8_t)((crc32Result >> 16) & 0xFF);
     pCrc[3] = (uint8_t)((crc32Result >> 24) & 0xFF);

     PRINTF("[SPI][TX] Prepared Atomic Cmd: MSG_TYPE=0x%02X, CMD_ID=0x%02X, ArgsLen=%d, CRC=0x%08X\r\n",
            msg_type, cmd_id, arg_len, crc32Result);

     //printf_tx_data();
}


#else if
static void spi_prepare_command_packet_data(uint8_t hex_value)
{
    /* V2.3.0 封包結構常數 */
    // Total: 532 Bytes
    // Header: 16 Bytes
    // Payload: 512 Bytes
    // CRC: 4 Bytes at the end

    uint32_t crc32Result = 0;
    
    /* 1. 清空緩衝區 (Padding 0x00) */
    // 這一步非常重要，因為 Protocol 要求 Payload 不足 512 的部分必須補零
    memset(srcBuff, 0, BUFFER_SIZE);

    /* 2. 填寫 Header (Offset 0x00 ~ 0x0F) */
    
    // Offset 0x00: SYNC_HEAD (0x55AA) -> Little Endian: AA 55
    srcBuff[0] = 0xAA; 
    srcBuff[1] = 0x55;
    
    // Offset 0x02: PROT_VER (0x02)
    srcBuff[2] = 0x02;
    
    // Offset 0x03: MSG_TYPE (0x20 = CMD_ATOMIC_EXEC)
    // 根據您的需求，這是Slave主動發起的指令，歸類為原子執行
    srcBuff[3] = 0x20;

    // Offset 0x04: PACKET_ID (4 Bytes) 
    // 原子指令不需要序號，保持 0x00000000 (memset 已處理)

    // Offset 0x10 ~ : Payload
    
    uint32_t payload_len = 0;

    if (hex_value == SYSTEM_STATUS_HEX_VALUE) // 0x93
    {
        /* System Status: 7 Bytes Payload */
        // payload[0] = cmd_id (0x93)
        // payload[1] = format_ver (0x01)
        // payload[2] = arg_len (3)
        // payload[3] = flags (0x00)
        // payload[4] = ss.flags
        // payload[5] = ss.layer
        // payload[6] = ss.batt
        
        srcBuff[HEADER_LEN + 0] = hex_value;
        srcBuff[HEADER_LEN + 1] = 0x01;
        srcBuff[HEADER_LEN + 2] = 0x03; // Args length
        srcBuff[HEADER_LEN + 3] = 0x00;
        srcBuff[HEADER_LEN + 4] = ss.flags;
        srcBuff[HEADER_LEN + 5] = ss.layer;
        srcBuff[HEADER_LEN + 6] = ss.batt;
        
        payload_len = 7;
    }
    else
    {
        /* Generic Atomic Command: 4 Bytes Payload */
        // payload[0] = cmd_id (hex_value)
        // payload[1] = format_ver (0x01)
        // payload[2] = arg_len (0)
        // payload[3] = flags (0x00)
        
        srcBuff[HEADER_LEN + 0] = hex_value;
        srcBuff[HEADER_LEN + 1] = 0x01;
        srcBuff[HEADER_LEN + 2] = 0x00; // Args length
        srcBuff[HEADER_LEN + 3] = 0x00;
        
        payload_len = 4;
    }

    // Offset 0x08: DATA_LEN (4 Bytes) 
    srcBuff[8]  = (uint8_t)(payload_len & 0xFF);
    srcBuff[9]  = (uint8_t)((payload_len >> 8) & 0xFF);
    srcBuff[10] = (uint8_t)((payload_len >> 16) & 0xFF);
    srcBuff[11] = (uint8_t)((payload_len >> 24) & 0xFF);

    // Offset 0x0C: RESERVED (4 Bytes) -> 保持 0x00 (memset 已處理)

    /* 4. 計算 CRC32 */
    // 計算範圍：Header (16) + Payload (512) = 528 Bytes
    // 注意：即使有效數據只有 few bytes，CRC 計算必須涵蓋完整的 512 bytes payload (含補零)
    uint32_t calc_len = HEADER_LEN + PAYLOAD_LEN; // 16 + 512 = 528
    crc32Result = spi_calculate_tx_data_crc32(srcBuff, calc_len);

    /* 5. 填寫 CRC (Offset 0x210 / 528) - Little Endian */
    // CRC 位於 BUFFER 的最後 4 個 Bytes
    uint8_t *pCrc = &srcBuff[calc_len]; 
    pCrc[0] = (uint8_t)(crc32Result & 0xFF);
    pCrc[1] = (uint8_t)((crc32Result >> 8) & 0xFF);
    pCrc[2] = (uint8_t)((crc32Result >> 16) & 0xFF);
    pCrc[3] = (uint8_t)((crc32Result >> 24) & 0xFF);

    PRINTF("[SPI][TX] Prepared Atomic Cmd: 0x%02X, CRC: 0x%08X\r\n", hex_value, crc32Result);
}
#endif

static void InitCrc32(CRC_Type *base, uint32_t seed)
{
    crc_config_t config;

    config.polynomial    = kCRC_Polynomial_CRC_32;
    config.reverseIn     = true;
    config.complementIn  = false;
    config.reverseOut    = true;
    config.complementOut = true;
    config.seed          = seed;

    CRC_Init(base, &config);
}

static uint32_t spi_calculate_tx_data_crc32(const uint8_t *data, size_t length)
{
    CRC_Type *base = CRC_ENGINE;

    /* 1. 初始化 (Seed = 0xFFFFFFFF) */
    InitCrc32(base, 0xFFFFFFFFU);

    /* 2. 寫入資料 */
    CRC_WriteData(base, (uint8_t *)data, length);

    /* 3. 回傳結果 */
    return CRC_Get32bitResult(base);
}

static bool spi_check_rx_data_crc32(const uint8_t *data, size_t length)
{
    PRINTF("[SPI][Debug] Packet Head: %02X %02X %02X %02X\r\n", data[0], data[1], data[2], data[3]);
    bool isPass = false;

    /* 基本檢查：確保指標有效且長度大於 CRC 長度 (4 bytes) */
    if ((data == NULL) || (length <= 4))
    {
        PRINTF("[SPI][Receive] Error: Invalid data pointer or length too short.\r\n");
        return false; // <-- 修改這裡：回傳 false 而不是空的 return
    }

    CRC_Type *base = CRC_ENGINE; 
    uint32_t calcCrc32 = 0;
    uint32_t rxCrc32 = 0;
    uint32_t dataToCalcLen = length - 4; /* V2.3.0 固定為 532 - 4 = 528 */

    /* 1. 初始化 CRC-32 模組 */
    InitCrc32(base, 0xFFFFFFFFU);

    /* 2. 寫入資料進行計算 (Header + Payload) */
    CRC_WriteData(base, (uint8_t *)data, dataToCalcLen);

    /* 3. 取得計算結果 */
    calcCrc32 = CRC_Get32bitResult(base);

    /* 4. 提取封包末尾的 CRC (Offset 528) */
    const uint8_t *pCrcPtr = &data[dataToCalcLen];
    rxCrc32 = (uint32_t)pCrcPtr[0] |
              ((uint32_t)pCrcPtr[1] << 8) |
              ((uint32_t)pCrcPtr[2] << 16) |
              ((uint32_t)pCrcPtr[3] << 24);

    /* 5. 比對結果 */
    if (calcCrc32 == rxCrc32)
    {
        isPass = true;
    }
    else
    {
        PRINTF("[SPI][Receive] CRC FAIL! Calc: 0x%08x, Rx: 0x%08x\r\n", calcCrc32, rxCrc32);
        isPass = false;
    }

    return isPass;
}

static uint32_t crc32_soft(uint32_t crc, const uint8_t *buf, size_t len)
{
    /* Linux crc32_le 邏輯: 
     * 1. 輸入時反轉 (Invert)
     * 2. 計算 (Polynomial: 0xEDB88320 -> 0x04C11DB7 的反轉)
     * 3. 輸出時反轉 (Invert)
     */
    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
    }
    return ~crc;
}

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

void SPI_SLAVE_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if ((SPI_GetStatusFlags(SOC_SPI_SLAVE) & kSPI_RxNotEmptyFlag) && (rxIndex > 0U))
    {
        destBuff[BUFFER_SIZE - rxIndex] = SPI_ReadData(SOC_SPI_SLAVE);
        rxIndex--;
    }

    if ((SPI_GetStatusFlags(SOC_SPI_SLAVE) & kSPI_TxNotFullFlag) && (txIndex > 0U))
    {
        SPI_WriteData(SOC_SPI_SLAVE, (uint16_t)(srcBuff[BUFFER_SIZE - txIndex]), 0);
        txIndex--;
    }

    if ((rxIndex == 0U) && (txIndex == 0U))
    {
        SPI_DisableInterrupts(SOC_SPI_SLAVE, kSPI_RxLvlIrq | kSPI_TxLvlIrq);
        xEventGroupSetBitsFromISR(spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT, &xHigherPriorityTaskWoken);
    }

    /* Force context switch if needed */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    SDK_ISR_EXIT_BARRIER;
}

/* [線性故事 1] 處理主動發送流程 (符合 V2.3.0 時序) */
static void handle_active_transmit(uint8_t msg_type_hex,uint8_t cmd_hex)
{
    PRINTF("\n[SPI][TX] Start Active Request Sequence (Cmd: 0x%02X).\r\n", cmd_hex);

    /* 1. 準備資料 & 狀態: S_TX_REQ */
    g_slave_state = S_TX_REQ;
    spi_dma_setting_reset();
    spi_prepare_command_packet_data(msg_type_hex,cmd_hex);
    memcpy(slaveTxData, srcBuff, BUFFER_SIZE);
    
    // 清除舊事件，避免誤判
    xEventGroupClearBits(spi_event_group, MASTER_GRANT_ACK_EVENT_BIT | SPI_TRANSFER_COMPLETE_EVENT_BIT);

    /* 2. 發起請求: Pull Low */
    PRINTF("[SPI][TX] 1. Pull Low (Request)\r\n");
    GPIO_PinWrite(GPIO, 2, 15, 0);
    g_slave_state = S_WAIT_GRANT; // 轉移狀態：等待授權

    /* 3. 阻塞等待 Grant */
    PRINTF("[SPI][TX] 2. Wait for Grant...\r\n");
    EventBits_t bits = xEventGroupWaitBits(
        spi_event_group, 
        MASTER_GRANT_ACK_EVENT_BIT | SPI_TRANSFER_COMPLETE_EVENT_BIT, // 未來可加入 SPI_CS_RISE_EVENT_BIT 來處理搶佔
        pdFALSE, pdFALSE, pdMS_TO_TICKS(500));
    
    // 優先檢查是否被搶佔 (DMA 傳輸完成)
    if ((bits & SPI_TRANSFER_COMPLETE_EVENT_BIT) != 0) {
        PRINTF("[SPI][TX] Preempted by Master! Yielding to RX.\r\n");
        // 釋放 Ready 線 (雖然 ISR 已經拉高了，保險起見)
        GPIO_PinWrite(GPIO, 2, 15, 1);
        g_slave_state = S_IDLE; 
        // 這裡不清除 SPI_TRANSFER_COMPLETE_EVENT_BIT，直接 Return
        // 讓主迴圈 (spi_handler_task) 接手處理 handle_passive_receive
        return; 
    }   

    if ((bits & MASTER_GRANT_ACK_EVENT_BIT) != 0) {
        xEventGroupClearBits(spi_event_group, MASTER_GRANT_ACK_EVENT_BIT);

        /* [關鍵原子操作] 收到授權 -> S_DMA_TX_SETUP -> S_DMA_TX_RUN */
        g_slave_state = S_DMA_TX_SETUP;
        
        // A. 先裝彈 (避免 0x4ECC 錯位)
        EXAMPLE_SlaveStartDMATransfer();
        
        // B. 再釋放 Ready
        GPIO_PinWrite(GPIO, 2, 15, 1);
        
        g_slave_state = S_DMA_TX_RUN; // 標記為 Run，ISR 看到 CS Rise 就不會拉低 Ready
        PRINTF("[SPI][TX] 3. Grant OK. DMA Armed & Ready High.\r\n");

        /* 4. 等待傳輸完成 (Hardware DMA) */
        bits = xEventGroupWaitBits(
            spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT, 
            pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));

        if ((bits & SPI_TRANSFER_COMPLETE_EVENT_BIT) != 0) {
            g_slave_state = S_TX_VERIFY; // 傳輸完成，進入驗證態
            PRINTF("[SPI][TX] 4. Transfer Done. Waiting for Master ACK...\r\n");

            /* 5. 等待 Master 簽收 (ACK 脈衝) */
            bits = xEventGroupWaitBits(
                spi_event_group, MASTER_GRANT_ACK_EVENT_BIT, 
                pdTRUE, pdFALSE, pdMS_TO_TICKS(500));

            if ((bits & MASTER_GRANT_ACK_EVENT_BIT) != 0) {
                PRINTF("[SPI][TX] 5. Master ACK Received. Success!\r\n");
                // === 等待 ACK 脈衝結束 ===
                uint32_t pulse_timeout = 15;
                while (GPIO_PinRead(GPIO, 1, 10) == 1 && pulse_timeout > 0) {
                    vTaskDelay(pdMS_TO_TICKS(1)); 
                    pulse_timeout--;
                }
            } else {
                PRINTF("[SPI][TX] Warning: Master ACK Timeout.\r\n");
            }
        } else {
            PRINTF("[SPI][TX] Error: DMA Transfer Timeout!\r\n");
        }
    } else {
        PRINTF("[SPI][TX] Error: Grant Timeout!\r\n");
    }

    /* 6. 結尾清理：強制回到 IDLE */
    GPIO_PinWrite(GPIO, 2, 15, 1); // 確保 Ready 為 High
    g_slave_state = S_IDLE;
    /* 清除優先指令旗標，讓被暫停的 Streaming 可以繼續 */
    global_priority_cmd_pending = false; 
    spi_dma_setting_reset_receive_mode(); // 恢復 RX 監聽
    xEventGroupClearBits(spi_event_group, SPI_CS_RISE_EVENT_BIT); // 清除殘留的 CS 事件
    PRINTF("[SPI] TX Sequence End. Back to IDLE.\r\n");
}

/* 專門處理 Streaming 的發送流程 (State Machine) */
static void handle_streaming_transmit(void)
{
    // 0. 安全檢查：如果沒啟動測試，直接退出
    if (!g_stream.active) return;

    // 1. 根據當前狀態準備資料 (Prepare Packet)
    const uint32_t PAYLOAD_SIZE = 512;
    const uint32_t TOTAL_FILE_SIZE = g_stream.total_pkts * PAYLOAD_SIZE;
    uint8_t dummy_payload[512]; // Stack variable
    
    // [Phase 1: Start]
    if (g_stream.current_pkt == 0) {
        uint32_t file_size_le = TOTAL_FILE_SIZE;
        spi_prepare_generic_packet(MSG_CMD_SESSION_START, 0, (uint8_t*)&file_size_le, sizeof(uint32_t));
    }
    // [Phase 3: End]
    else if (g_stream.current_pkt > g_stream.total_pkts) {
        // 全域 CRC 放在 END 封包
        spi_prepare_generic_packet(MSG_CMD_SESSION_END, g_stream.total_pkts + 1, (uint8_t*)&g_stream.global_crc, sizeof(uint32_t));
    }
    // [Phase 2: Data]
    else {
        // 準備 DATA 封包 (ID = current_pkt)
        memset(dummy_payload, ((g_stream.current_pkt - 1) % 0xFF), PAYLOAD_SIZE);
        spi_prepare_generic_packet(MSG_DATA_FILE, g_stream.current_pkt, dummy_payload, PAYLOAD_SIZE);
    }
    
    g_streaming_tx_success = false; // 預設為失敗
    /* 強制重置 SPI 引擎，確保清除上一包留下的 RX DMA 狀態 */
    SPI_Deinit(SOC_SPI_SLAVE);
    spi_slave_config_t slave_config;
    SPI_SlaveGetDefaultConfig(&slave_config);
    slave_config.sselPol = (spi_spol_t)SOC_SPI_SPOL;
    SPI_SlaveInit(SOC_SPI_SLAVE, &slave_config);
    EXAMPLE_SlaveDMASetup(); // 重新配置 DMA 控制器

    // 清除舊事件
    xEventGroupClearBits(spi_event_group, MASTER_GRANT_ACK_EVENT_BIT | SPI_TRANSFER_COMPLETE_EVENT_BIT);

    /* 1. 發起請求: Pull Low */
    g_slave_state = S_TX_REQ;
    GPIO_PinWrite(GPIO, 2, 15, 0); // Request
    g_slave_state = S_WAIT_GRANT;

    /* 2. 阻塞等待 Grant */
    EventBits_t bits = xEventGroupWaitBits(
        spi_event_group, 
        MASTER_GRANT_ACK_EVENT_BIT | SPI_TRANSFER_COMPLETE_EVENT_BIT,
        pdFALSE, pdFALSE, pdMS_TO_TICKS(500)); // Timeout 500ms

    if ((bits & SPI_TRANSFER_COMPLETE_EVENT_BIT) != 0) {
        /* 發生搶佔了！Master 沒給 Grant 就直接傳完了 */
        PRINTF("[SPI] Preemption detected during Stream TX. Yielding...\r\n");
        
        // A. 釋放 Ready 線 (回到安全狀態)
        GPIO_PinWrite(GPIO, 2, 15, 1);
        g_slave_state = S_IDLE;

        // B. 安排重試：重新設定 STREAMING 事件，讓下一輪主迴圈再次呼叫此函式
        xEventGroupSetBits(spi_event_group, SLAVE_STREAMING_DATA_EVENT_BIT);

        // C. 直接退出：保留 COMPLETE 位元，讓主迴圈去執行 handle_passive_receive
        return; 
    }    

    if ((bits & MASTER_GRANT_ACK_EVENT_BIT) != 0) {
        // [修正] 手動清除 Grant 事件
        xEventGroupClearBits(spi_event_group, MASTER_GRANT_ACK_EVENT_BIT);

        /* 收到授權 -> Arming -> Run */
        g_slave_state = S_DMA_TX_SETUP;

        /* 確保資料搬移到 DMA Buffer */
        memcpy(slaveTxData, srcBuff, BUFFER_SIZE);
        
        // A. 裝彈
        EXAMPLE_SlaveStartDMATransfer();
        
        // B. 釋放 Ready
        GPIO_PinWrite(GPIO, 2, 15, 1);
        g_slave_state = S_DMA_TX_RUN;

        /* 3. 等待傳輸完成 */
        bits = xEventGroupWaitBits(
            spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT, 
            pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));

        if ((bits & SPI_TRANSFER_COMPLETE_EVENT_BIT) != 0) {
            g_slave_state = S_TX_VERIFY;

            /* 4. 等待 Master ACK 脈衝 */
            bits = xEventGroupWaitBits(
                spi_event_group, MASTER_GRANT_ACK_EVENT_BIT, 
                pdTRUE, pdFALSE, pdMS_TO_TICKS(500));

            if ((bits & MASTER_GRANT_ACK_EVENT_BIT) != 0) {
                PRINTF("[Stream] ACK Rise detected.\r\n");
                uint32_t timeout_cnt = 0;
                while (GPIO_PinRead(GPIO, 1, 10) != 0) {
                    vTaskDelay(pdMS_TO_TICKS(1)); // 每 1ms 檢查一次
                    timeout_cnt++;
                    if (timeout_cnt > 20) { // 正常脈衝是 10ms，超過 20ms 視為異常
                        PRINTF("[Stream] Warning: ACK pulse stuck High!\r\n");
                        break;
                    }
                }
                g_streaming_tx_success = true; 
                PRINTF("[Stream] ACK Fall detected (Safe to proceed).\r\n");
            } else {
                PRINTF("[Stream] Warning: ACK Timeout.\r\n");
            }
        } else {
            PRINTF("[Stream] Error: DMA Timeout.\r\n");
        }
    } else {
        PRINTF("[Stream] Error: Grant Timeout.\r\n");
    }

    /* 5. 結尾清理 */
    GPIO_PinWrite(GPIO, 2, 15, 1);
    g_slave_state = S_IDLE;
    spi_dma_setting_reset_receive_mode(); // 恢復 RX
    xEventGroupClearBits(spi_event_group, SPI_CS_RISE_EVENT_BIT | MASTER_GRANT_ACK_EVENT_BIT);
    
    /* 6. 狀態機判斷 */
    if (g_streaming_tx_success) {
        // 如果剛才送的是 Data Packet，要更新 Global CRC
        if (g_stream.current_pkt > 0 && g_stream.current_pkt <= g_stream.total_pkts) {
             // 重算一次 CRC (因為 memory copy 可能被覆蓋)
             memset(dummy_payload, ((g_stream.current_pkt - 1) % 0xFF), PAYLOAD_SIZE);
             g_stream.global_crc = crc32_soft(g_stream.global_crc, dummy_payload, PAYLOAD_SIZE);
        }

        PRINTF("[Stream] Pkt %d Success.\r\n", g_stream.current_pkt);
        
        // [成功]：推進狀態
        if (g_stream.current_pkt > g_stream.total_pkts) {
            // 全部結束
            PRINTF("[SPI] Streaming Test Completed! Global CRC: 0x%08X\r\n", g_stream.global_crc);
            g_stream.active = false;
        } else {
            // 進到下一包
            g_stream.current_pkt++;
            g_stream.retry_count = 0;
            // [關鍵] 自我觸發：通知 Task 繼續處理下一包
            xEventGroupSetBits(spi_event_group, SLAVE_STREAMING_DATA_EVENT_BIT);
        }
    } else {
        // [失敗]：重試邏輯
        g_stream.retry_count++;
        PRINTF("[Stream] Pkt %d Failed/Preempted. Retry %d/5\r\n", g_stream.current_pkt, g_stream.retry_count);
        
        if (g_stream.retry_count >= 5) {
            PRINTF("[SPI] Streaming Aborted (Max Retries).\r\n");
            g_stream.active = false; // 放棄
        } else {
            // [關鍵] 自我觸發：通知 Task 重試這一包
            vTaskDelay(pdMS_TO_TICKS(50)); 
            xEventGroupSetBits(spi_event_group, SLAVE_STREAMING_DATA_EVENT_BIT);
        }
    }
}

/* [線性故事 2] 處理被動接收流程 (整合 Session 邏輯與全域校驗完整版) */
static void handle_passive_receive(void)
{
    // 標記狀態：處理中
    g_slave_state = S_PROCESSING;
    PRINTF("[SPI][RX] DMA Done. Analyzing UFP Packet...\r\n");

    bool retry_loop = false;

    /* 使用 do-while 迴圈來處理「收到重傳包」的情況 */
    do {
        retry_loop = false; // 預設不需要重試

        // 1. 搬運資料
        memcpy(destBuff, slaveRxData, BUFFER_SIZE);

        // 2. 校驗與解析
        bool crcResult = spi_check_rx_data_crc32(destBuff, sizeof(destBuff));

        if (crcResult) 
        {
            /* [成功路徑] */
            uint16_t sync_head = (uint16_t)destBuff[0] | ((uint16_t)destBuff[1] << 8);
            
            if (sync_head == 0x55AA) {
                // PRINTF("[SPI][RX] Processing Done. Preparing Next...\r\n");
                uint8_t msg_type = destBuff[3];
                uint32_t pkt_id  = *(uint32_t*)&destBuff[4];
                uint32_t data_len = *(uint32_t*)&destBuff[8];
                
                PRINTF("[SPI][RX] Valid UFP: Type=0x%02X, ID=%u, Len=%u\r\n", msg_type, pkt_id, data_len);
            
                /* 這裡處理業務邏輯 (Switch Case...) - [V2.3.0 Session Logic] */
                bool logic_success = true;

                /* 取得 Payload 指標 (跳過 16 bytes Header) */
                uint8_t *payload = &destBuff[HEADER_LEN];

                switch (msg_type) 
                {
                    /* --- 會話建立 (Session Start) --- */
                    case MSG_CMD_SESSION_START:
                        PRINTF("[Session] START Detected. Resetting Global CRC (Soft).\r\n");
                        g_session.active = true;
                        g_session.expected_id = 1; 
                        g_session.received_bytes = 0;
                        
                        /* [修正] 初始化軟體 CRC 種子為 0 */
                        /* Linux crc32_le(0, ...) 內部會 ^ 0xFFFFFFFF，所以這裡設 0 即可 */
                        g_session.current_crc = 0; 
                        break;

                    /* --- 數據流 (Data Streaming) --- */
                    case MSG_DATA_FILE:
                    case MSG_DATA_FIRMWARE:
                        if (!g_session.active) {
                            PRINTF("[Session] Error: Data received without Session Start!\r\n");
                        } 
                        else if (pkt_id == g_session.expected_id) {
                            /* [正常接收] */
                            
                            /* [修正] 使用軟體 CRC 進行增量計算 */
                            /* 注意：必須傳入上一輪的結果 g_session.current_crc */
                            g_session.current_crc = crc32_soft(g_session.current_crc, &destBuff[HEADER_LEN], 512);
                            
                            g_session.expected_id++; 
                            g_session.received_bytes += data_len;
                            // PRINTF("[Session] Recv: %u bytes\r\n", data_len);
                        } 
                        else if (pkt_id == (g_session.expected_id - 1)) {
                            /* [重複包] Master 沒收到上一次的 ACK，重傳了舊包 */
                            PRINTF("[Session] Warning: Duplicate ID %u. ACK only.\r\n", pkt_id);
                        } 
                        else {
                            /* [錯誤] 跳號 (E03) */
                            PRINTF("[Session] Error: ID Mismatch! Exp: %u, Rx: %u\r\n", g_session.expected_id, pkt_id);
                            logic_success = false; 
                        }
                        break;

                    /* --- 會話結束 (Session End) --- */
                    case MSG_CMD_SESSION_END:
                        if (g_session.active) {
                            /* [全域校驗結算] */
                            uint32_t final_global_crc = g_session.current_crc; 
                            
                            /* 假設 Master 將整份檔案的全域 CRC 放在 END 封包 Payload 的前 4 Bytes */
                            uint32_t master_global_crc = *(uint32_t*)&destBuff[HEADER_LEN];
                            
                            PRINTF("[Session] END Detected. Total Received Bytes: %u\r\n", g_session.received_bytes);
                            PRINTF("[Session] Global CRC Check: Calc=0x%08X, Master=0x%08X\r\n", 
                                   final_global_crc, master_global_crc);

                            if (final_global_crc == master_global_crc) {
                                PRINTF("[Session] SUCCESS! Global integrity verified.\r\n");
                            } else {
                                PRINTF("[Session] FAILED! Global CRC Mismatch.\r\n");
                                logic_success = false; // 觸發錯誤流程
                            }
                            g_session.active = false;
                        }
                        break;

                    /* --- 原子指令 (插隊) --- */
                    //case MSG_CMD_ATOMIC_EXEC:
                    case CMD_ATOMIC_EVENT:
                        //PRINTF("[APP] Atomic Command (ID=0x%02X) Executed.\r\n", destBuff[HEADER_LEN]);
						{
						    // 提取 Header 欄位
						    uint8_t event_id = payload[0];
						    uint8_t fmt_ver  = payload[1];
						    uint8_t arg_len  = payload[2];
						    uint8_t flags    = payload[3];

							PRINTF("[SPI][RX] Atomic EVENT: event_id=0x%02X format_ver=0x%02X flags=0x%02X args_len=%d \r\n", event_id,fmt_ver,flags,arg_len);

							const uint8_t *args = &payload[4];

						    // 判斷並印出 Args
						    if (arg_len > 0) {
						    	PRINTF("Arguments  : ");
						        // 指向 arg 起始位置 (payload[4])

						        for (int i = 0; i < arg_len; i++) {
						        	PRINTF("%02X ", args[i]);
						        }
						        PRINTF("\n");

						    } else {
						    	PRINTF("Arguments  : (None)\n");
						    }
						    PRINTF("====================================\n\n");

							// 呼叫業務邏輯函式
							spi_process_atomic_event(event_id,args);
						}
                        break;
                    
                    case MSG_CMD_ABORT:
                        PRINTF("[Session] ABORT Command Received. Resetting.\r\n");
                        g_session.active = false;
                        g_session.expected_id = 0;
                        break;

                    default:
                        PRINTF("[SPI] Unknown Msg Type: 0x%02X\r\n", msg_type);
                        break;
                }

                if (logic_success) {
                    /* 邏輯正確 -> 準備下一包 -> 發送 ACK */
                    /* 這裡不要拉高 GPIO！
                     * 因為我們的 DMA 還沒設好，如果現在拉高，Master 會追撞上來。
                     * 讓 Busy (Low) 繼續保持，Master 會在線上等我們。
                     */
                     // GPIO_PinWrite(GPIO, 2, 15, 1);
                }
                else {
                    /* [E03] 邏輯或全域校驗錯誤 -> 跳轉至錯誤處理區塊，進入 ERROR_HOLD */
                    PRINTF("[SPI][RX] Logic/Global Error. Entering ERROR_HOLD.\r\n");
                    goto error_hold_entry; 
                }
            } 
            else {
                PRINTF("[SPI][RX] Error: Invalid SYNC_HEAD (0x%04X)\r\n", sync_head);
                goto error_hold_entry; // 跳轉至錯誤鎖死邏輯，迫使 Master 重傳
            }
        } 
        else 
        {
            /* [失敗路徑] CRC 失敗 */
            error_hold_entry: // 邏輯錯誤跳轉標籤

            PRINTF("[SPI][RX] CRC/Logic FAIL! Entering ERROR_HOLD. Waiting for Master Retry...\r\n");
            g_slave_state = S_ERROR_HOLD;
            
            // A. 預備 DMA 接重傳 (重要！否則接不到資料)
            spi_dma_setting_reset_receive_mode();
            
            // B. 等待重傳 (由 CS 下降緣 -> DMA 完成 觸發)
            EventBits_t retryBits = xEventGroupWaitBits(
                spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT, 
                pdTRUE, pdFALSE, pdMS_TO_TICKS(500));
                
            if ((retryBits & SPI_TRANSFER_COMPLETE_EVENT_BIT) == 0) {
                // 超時：Master 放棄了
                PRINTF("[SPI][RX] Global Timeout! Master gave up. Forced Reset.\r\n");
                GPIO_PinWrite(GPIO, 2, 15, 1); // 自我解鎖
            } else {
                // 收到重傳包：設定旗標，讓迴圈重跑一次
                PRINTF("[SPI][RX] Retry Packet Received. Re-analyzing...\r\n");
                
                // [關鍵] 清除位元，準備下一輪判定
                xEventGroupClearBits(spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT);
                
                retry_loop = true; // <--- 觸發下一次迴圈
            }
        }

    } while (retry_loop); // 如果收到重傳，就回到上面 memcpy 再次檢查

    /* 結尾清理 */
    g_slave_state = S_IDLE;
    memset(slaveRxData, 0x00, TRANSFER_SIZE);
    
    // 3. 配置 DMA 接收 (Arming) - 先配置好 DMA
    spi_dma_setting_reset_receive_mode();

    // 4. [最後一步] 萬事備妥，才拉高 Ready 線 (ACK)
    // 告訴 Master：「上一包處理完了，而且下一包我也準備好接了，來吧！」
    GPIO_PinWrite(GPIO, 2, 15, 1); 
    
    // 5. 確保沒有殘留的 CS 事件，避免回到主迴圈誤觸
    xEventGroupClearBits(spi_event_group, SPI_CS_RISE_EVENT_BIT);
}



void spi_handler_task(void *pvParameters)
{
    // Initialize system bus mutex for synchronization between SPI and I2C components
    if (sys_bus_mutex == NULL) {
        sys_bus_mutex = xSemaphoreCreateMutex();
        configASSERT(sys_bus_mutex != NULL);
    }
    /* 初始化 */
    memset(destBuff, 0, BUFFER_SIZE);
    g_slave_state = S_IDLE;
    GPIO_PinWrite(GPIO, 2, 15, 1); // 初始 Ready High
    spi_dma_setting_reset_receive_mode(); // 被動監聽開啟

    PRINTF("[SPI] Task Started. FSM State: S_IDLE\r\n");

    EventBits_t bits;
    uint8_t queued_cmd;

    while (1)
    {
        /* 主迴圈：只負責分流 */
        /* 使用 pdTRUE 自動清除觸發位元，因為子函式會處理後續 */
        bits = xEventGroupWaitBits(
            spi_event_group,
            SPI_TRANSFER_COMPLETE_EVENT_BIT |   // 1. Master 送完資料給 Slave 了
            SLAVE_TRANSFER_EVENT_BIT |          // 2. 有原子指令要發送 (如按鍵)
            SLAVE_STREAMING_DATA_EVENT_BIT |    // 3. 有串流封包要發送 (如檔案測試)
            SPI_CS_RISE_EVENT_BIT,              // 4. CS 拉高事件 (用於偵測結束)
            pdFALSE,                             // 執行完後不自動清除
            pdFALSE,                            // 只要其中一個 Bit 成立就喚醒
            pdMS_TO_TICKS(20)                   // 週期醒來檢查 queue 命令
        );

        /* 分流邏輯 */

        // 1. 最高優先權：SPI 傳輸完成 (被動接收 或 搶佔發生)
        if ((bits & SPI_TRANSFER_COMPLETE_EVENT_BIT) != 0)
        {
            // 手動清除事件
            xEventGroupClearBits(spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT);
            handle_passive_receive();
        }
        else if ((bits & SLAVE_TRANSFER_EVENT_BIT) != 0)
        {
            // 進入主動發送故事線
            xEventGroupClearBits(spi_event_group, SLAVE_TRANSFER_EVENT_BIT);
            handle_active_transmit(global_pending_msgtype,global_pending_command);
        }
        else if ((bits & SLAVE_STREAMING_DATA_EVENT_BIT) != 0)
        {
            // 處理串流資料發送事件
            xEventGroupClearBits(spi_event_group, SLAVE_STREAMING_DATA_EVENT_BIT);
            handle_streaming_transmit();
        }
        else if ((bits & SPI_CS_RISE_EVENT_BIT) != 0)
        {
            // 純粹的狀態變更 Log，實際上已由 ISR 處理 GPIO
            // PRINTF("[SPI] CS Rise Detected (State: %d)\r\n", g_slave_state);
            xEventGroupClearBits(spi_event_group, SPI_CS_RISE_EVENT_BIT);
        }
        else if ((bits & MASTER_GRANT_ACK_EVENT_BIT) != 0)
        {
            // 在 IDLE 狀態收到這個通常是雜訊或殘留
            xEventGroupClearBits(spi_event_group, MASTER_GRANT_ACK_EVENT_BIT);
            PRINTF("[SPI] Ignored stray Grant/ACK signal.\r\n");
        }
        /*
        // 相容舊架構：處理由 send_state_to_soc() 送進來的 queue 命令
        if ((spi_request_queue != NULL) &&
            (xQueueReceive(spi_request_queue, &queued_cmd, 0) == pdPASS))
        {
            global_pending_command = queued_cmd;
            global_priority_cmd_pending = true;
            handle_active_transmit(global_pending_command);
        }
        */
    }
}

void send_spi_request(uint8_t msg_type, uint8_t cmd_id)
{
    PRINTF("[Debug] send_spi_request() -> MSG_TYPE=0x%02X, CMD_ID=0x%02X called. GroupHandle: %p\r\n", msg_type,cmd_id, spi_event_group);

    global_pending_msgtype = msg_type;
    global_pending_command = cmd_id;
    global_priority_cmd_pending = true;
    xEventGroupSetBits(spi_event_group, SLAVE_TRANSFER_EVENT_BIT);
}



void start_spi_streaming_test(void) {
    if (g_stream.active) {
        PRINTF("[SPI] Streaming already active!\r\n");
        return; // 防止重複觸發
    }

    PRINTF("[SPI] Starting Async Streaming Test...\r\n");
    g_stream.active = true;
    g_stream.current_pkt = 0; // 0 = Start Packet, 1~20 = Data, 21 = End Packet
    g_stream.total_pkts = 20;
    g_stream.retry_count = 0;
    g_stream.global_crc = 0;

    // 觸發 Task 開始處理第一包 (Session Start)
    // 這裡我們復用 SLAVE_STREAMING_DATA_EVENT_BIT
    xEventGroupSetBits(spi_event_group, SLAVE_STREAMING_DATA_EVENT_BIT);
}

void spi_command_handler_init(void)
{
    /* 初始化 */
    memset(destBuff, 0, BUFFER_SIZE);
    g_slave_state = S_IDLE;
    GPIO_PinWrite(GPIO, 2, 15, 1); // 初始 Ready High
    spi_dma_setting_reset_receive_mode(); // 被動監聽開啟

    PRINTF("[SPI] Task Started. FSM State: S_IDLE\r\n");

}

void spi_command_handler(void)
{
    /* 主迴圈：只負責分流 */
    /* 使用 pdTRUE 自動清除觸發位元，因為子函式會處理後續 */
	EventBits_t bits;

    bits = xEventGroupWaitBits(
        spi_event_group,
        SPI_TRANSFER_COMPLETE_EVENT_BIT |   // 1. Master 送完資料給 Slave 了
        SLAVE_TRANSFER_EVENT_BIT |          // 2. 有原子指令要發送 (如按鍵)
        SLAVE_STREAMING_DATA_EVENT_BIT |    // 3. 有串流封包要發送 (如檔案測試)
        SPI_CS_RISE_EVENT_BIT,              // 4. CS 拉高事件 (用於偵測結束)
        pdFALSE,                             // 執行完後不自動清除
        pdFALSE,                            // 只要其中一個 Bit 成立就喚醒
        pdMS_TO_TICKS(20)                   // 週期醒來檢查 queue 命令
    );

    /* 分流邏輯 */

    // 1. 最高優先權：SPI 傳輸完成 (被動接收 或 搶佔發生)
    if ((bits & SPI_TRANSFER_COMPLETE_EVENT_BIT) != 0)
    {
        // 手動清除事件
        xEventGroupClearBits(spi_event_group, SPI_TRANSFER_COMPLETE_EVENT_BIT);
        handle_passive_receive();
    }
    else if ((bits & SLAVE_TRANSFER_EVENT_BIT) != 0)
    {
        // 進入主動發送故事線
        xEventGroupClearBits(spi_event_group, SLAVE_TRANSFER_EVENT_BIT);
        handle_active_transmit(global_pending_msgtype,global_pending_command);
    }
    else if ((bits & SLAVE_STREAMING_DATA_EVENT_BIT) != 0)
    {
        // 處理串流資料發送事件
        xEventGroupClearBits(spi_event_group, SLAVE_STREAMING_DATA_EVENT_BIT);
        handle_streaming_transmit();
    }
    else if ((bits & SPI_CS_RISE_EVENT_BIT) != 0)
    {
        // 純粹的狀態變更 Log，實際上已由 ISR 處理 GPIO
        // PRINTF("[SPI] CS Rise Detected (State: %d)\r\n", g_slave_state);
        xEventGroupClearBits(spi_event_group, SPI_CS_RISE_EVENT_BIT);
    }
    else if ((bits & MASTER_GRANT_ACK_EVENT_BIT) != 0)
    {
        // 在 IDLE 狀態收到這個通常是雜訊或殘留
        xEventGroupClearBits(spi_event_group, MASTER_GRANT_ACK_EVENT_BIT);
        PRINTF("[SPI] Ignored stray Grant/ACK signal.\r\n");
    }
    /*
    // 相容舊架構：處理由 send_state_to_soc() 送進來的 queue 命令
    if ((spi_request_queue != NULL) &&
        (xQueueReceive(spi_request_queue, &queued_cmd, 0) == pdPASS))
    {
        global_pending_command = queued_cmd;
        global_priority_cmd_pending = true;
        handle_active_transmit(global_pending_command);
    }
    */
}

#if 0
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
static uint8_t execute_active_spi_transmission(uint8_t hex_value); // <<< NEW: 獨立的 SPI 執行函式
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

SemaphoreHandle_t sys_bus_mutex = NULL;

// NEW: 被動模式事件佇列與事件型別
typedef enum {
    PASSIVE_EVT_NEED_INIT = 1,
    PASSIVE_EVT_RX_DONE   = 2,
} passive_evt_t;
QueueHandle_t passive_evt_queue = NULL;


// NEW: 主動傳輸完成事件佇列與事件型別（供 execute_active_spi_transmission 等待）
typedef enum {
    TRANSFER_EVT_DONE = 1,
} transfer_evt_t;
QueueHandle_t transfer_evt_queue = NULL;


// NEW: 同步等待多個 Queue 的 QueueSet（整合 passive + active）
QueueSetHandle_t spi_evt_set = NULL;


extern volatile SystemStatus ss ;
uint8_t Novatek_boot_completed = 0;

uint8_t received_value;
passive_evt_t  passive_evt;
QueueSetMemberHandle_t activated;


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
                hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_DISABLE);
                led_post_event(HAL_LED_EVENT_REFRESH); //While LED ON
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
				PRINTF("[Passive] ACK:[00 %02X] Nova do nothing \r\n",val);
				hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH); //While LED ON
			}
			else if (val == 0x01) {
				PRINTF("[Passive] ACK:[00 %02X] Capture Start \r\n",val);
				led_post_event(HAL_LED_EVENT_PHOTO_CAPTURE);//While LED ON
				// ToDo:使NXP發出 "滴～喀嚓 "聲音
				ss_set_capture_status(STATUS_START);
			}
			else if (val == 0x03) {
				PRINTF("[Passive] ACK:[00 %02X] Capture Completed \r\n",val);
				led_post_event(HAL_LED_EVENT_REFRESH);
				ss_set_capture_status(STATUS_END);
			}
			else if (val == 0x04) {
				PRINTF("[Passive] ACK:[00 %02X] Recording Start \r\n",val);
				hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_ENABLE);
				led_post_event(HAL_LED_EVENT_REFRESH); //While LED ON
				// ToDo:使NXP發出 "登登"聲音
				ss_set_recording_status(STATUS_START);
			}
			else if (val == 0x05) {
				PRINTF("[Passive] ACK:[00 %02X] Recording Completed \r\n",val);
				hal_led_set_situation(HAL_LED_EVENT_RECORDING, SITUATION_DISABLE);
				led_post_event(HAL_LED_EVENT_REFRESH); //While LED ON
				// ToDo:使NXP發出 "等登"聲音
				ss_set_recording_status(STATUS_END);
			}
			else if (val == 0x06) {
				PRINTF("[Passive] ACK:[00 %02X] Media Playing \r\n",val);
				if (get_media_status() == MUSIC_PAUSE) {
					set_media_status(MUSIC_PLAYING);
				}
			}
			else if (val == 0x07) {
				PRINTF("[Passive] ACK:[00 %02X] Media Pause \r\n",val);
				if (get_media_status() == MUSIC_PLAYING) {
					set_media_status(MUSIC_PAUSE);
				}
			}
			else if (val == 0x08) {
				PRINTF("[Passive] ACK:[00 %02X] Wi-Fi connection \r\n",val);
				// ToDo:手機Wi-Fi連線時，發出提示聲音
			}
			else if (val == 0x09) {
				PRINTF("[Passive] ACK:[00 %02X] WiFi disconnection \r\n",val);
                set_ringtone_state(Ringtone_WiFi_Disconnected);
			}
			break;

        case 0x11: //Nova boot completed
			if (val == 0x11) {
				PRINTF("[Passive] ACK:[11 11] Nova boot completed\r\n");
				hal_led_refresh();
				battery_timer_start();
#if UsingQAR87BoardHwVersion == 0 // Dev Board
				Novatek_boot_completed = 1;
#endif
                set_ringtone_state(Ringtone_PowerON);
			}
			break;
        case 0x40: //Novatek update usage state
				PRINTF("[Passive] ACK:[40 %02X] Update Usage State \r\n",val);
				set_scenario_state(val);
			break;
        case 0x50: //Update Layer
				PRINTF("[Passive] ACK:[50 %02X] Update Layer \r\n",val);
				ss.layer = val;
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
                    //xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
                    passive_evt_t evt = PASSIVE_EVT_NEED_INIT;
		            if (passive_evt_queue) {
		                xQueueSendFromISR(passive_evt_queue, &evt, &xHigherPriorityTaskWoken);
		            }

                }
            } else {
                PRINTF("\n[Passive] Invalid frame prefix! Expected 0xAA, got 0x%02X\r\n", destBuff[0]);
                passive_mode_busy = false;
                //xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);
		        passive_evt_t evt = PASSIVE_EVT_NEED_INIT;
		        if (passive_evt_queue) {
		            xQueueSendFromISR(passive_evt_queue, &evt, &xHigherPriorityTaskWoken);
		        }

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
            //xEventGroupSetBitsFromISR(spi_event_group, EVT_PASSIVE_RX_DONE | EVT_PASSIVE_NEED_INIT, &xHigherPriorityTaskWoken);

            // 先送 NEED_INIT，再送 RX_DONE，以維持原本 WaitBits 同時被滿足時「先 init 再 parse」的行為
			passive_evt_t evt = PASSIVE_EVT_NEED_INIT;
			if (passive_evt_queue) {
				xQueueSendFromISR(passive_evt_queue, &evt, &xHigherPriorityTaskWoken);
				evt = PASSIVE_EVT_RX_DONE;
				xQueueSendFromISR(passive_evt_queue, &evt, &xHigherPriorityTaskWoken);
			}

        } else if (operation_mode == MODE_ACTIVE) {
            //xEventGroupSetBitsFromISR(spi_event_group, EVT_TRANSFER_DONE, &xHigherPriorityTaskWoken);
	        // NEW: 同步送 Queue，供 execute_active_spi_transmission() 的 Queue 版等待
	        transfer_evt_t tevt = TRANSFER_EVT_DONE;
	        if (transfer_evt_queue) {
	            xQueueSendFromISR(transfer_evt_queue, &tevt, &xHigherPriorityTaskWoken);
	        }


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
static uint8_t execute_active_spi_transmission(uint8_t hex_value)
{
#if 0
    if (sys_bus_mutex != NULL) {
        xSemaphoreTake(sys_bus_mutex, pdMS_TO_TICKS(500));
    }
#endif
    PRINTF("\n--- Executing Active SPI for value: 0x%02X ---\r\n", hex_value);

	uint8_t execute_active_spi_transmission_completed = kStatus_Fail;

    const TickType_t timeout = pdMS_TO_TICKS(500);
    TickType_t start = xTaskGetTickCount();

    while (passive_mode_busy)
    {
        vTaskDelay(pdMS_TO_TICKS(5));
        if ((xTaskGetTickCount() - start) > timeout)
        {
            PRINTF("[Active] Timeout waiting passive ACK. Force re-init passive.\r\n");

            // 逾時時，用最安全的方式把 PASSIVE 狀態 reset
            passive_mode_busy = false;
            operation_mode = MODE_PASSIVE_IDLE;
            init_passive_mode();  // 重新預載 0xFF 並確保 IRQ 開啟
            break;
        }
    }

    /* ============= 進入主動模式 ============= */
    operation_mode = MODE_ACTIVE;
    SPI_DisableRxTxInterrupt();

    // NEW: 確保主動傳輸完成事件佇列存在
    if (transfer_evt_queue == NULL) {
        transfer_evt_queue = xQueueCreate(10, sizeof(transfer_evt_t));
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
        //xEventGroupWaitBits(spi_event_group, EVT_TRANSFER_DONE, pdTRUE, pdFALSE, pdMS_TO_TICKS(500));

        // NEW: 用 Queue 等待 ISR 通知「一個 frame 傳完」
        transfer_evt_t tevt;
        if (xQueueReceive(transfer_evt_queue, &tevt, pdMS_TO_TICKS(500)) != pdPASS) {
            // 若 queue 接收失敗，直接跳出（防守性處理）
            break;
        }
        //（可選）檢查事件型別
        if (tevt != TRANSFER_EVT_DONE) {
            continue;
        }

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
            if (currentFrame == 1) {
                SPI_StartFrame_PreloadTX(frame2_ptr, frame2_size);
            } else if (currentFrame == 2) {
                SPI_StartFrame_PreloadTX(dataFrame3, FIXED_BUFFER_SIZE);
            }
            if (!spi_irq_enabled) {
                SPI_EnableRxTxInterrupt();
            }
        } else {
        	execute_active_spi_transmission_completed = kStatus_Success;
        }
    }

    GPIO_PinWrite(GPIO, 2, 15, 0);
    PRINTF(">>> Active transmission done! GPIO pin set LOW <<<\r\n");
    PRINTF(">>> Returning to PASSIVE mode...\r\n\n");

    operation_mode = MODE_PASSIVE_IDLE;
    //xEventGroupSetBits(spi_event_group, EVT_PASSIVE_NEED_INIT);
	if (passive_evt_queue) {
		passive_evt_t evt = PASSIVE_EVT_NEED_INIT;
		(void)xQueueSend(passive_evt_queue, &evt, 0);
	}
#if 0
    if (sys_bus_mutex != NULL) {
        xSemaphoreGive(sys_bus_mutex);
    }
#endif
    return execute_active_spi_transmission_completed;
}

static inline int is_nova_active(void) {
    return GPIO_PinRead(GPIO, 1U, 10U); // Nova忙線
}

/**
 * @brief Unified SPI handler task.
 * @details This FreeRTOS task integrates both active and passive SPI flows:
 * - Waits on a QueueSet that combines `spi_request_queue` (active requests) and `passive_evt_queue` (passive events).
 * - When an active request (e.g., from button_task) arrives, it calls `execute_active_spi_transmission()` to perform the active SPI sequence.
 * - When a passive event arrives, it handles ACK parsing or re-initialization for passive mode.
 *
 * This design ensures event-driven handling for both directions, decouples ISR from task logic via queues,
 * and maintains SPI state consistency without busy polling.
 */
void spi_handler_task(void *pvParameters)
{
    uint8_t received_value;

	gpio_pin_config_t input_pin_config    = {kGPIO_DigitalInput, 0};
	GPIO_PinInit(GPIO, 1U, 10U, &input_pin_config);

    passive_evt_t  passive_evt;
    QueueSetMemberHandle_t activated;

    if (sys_bus_mutex == NULL) {
        sys_bus_mutex = xSemaphoreCreateMutex();
        configASSERT(sys_bus_mutex != NULL);
    }

    // 初始化 SPI frame 的 checksum
    dataFrame1[3] = calculateChecksum(dataFrame1, 3);
    dataFrame2[3] = calculateChecksum(dataFrame2, 3);

    PRINTF("=== SPI Slave Ready ===\r\n");
    PRINTF("GPIO is LOW: Passive mode active, waiting for Master...\r\n");
    //PRINTF("SPI Handler Task is ready, waiting for requests from the queue.\r\n");
    PRINTF("SPI Handler Task unified: waiting for passive/active events via QueueSet.\r\n");


    // NEW: 確保被動事件佇列存在
    if (passive_evt_queue == NULL) {
        passive_evt_queue = xQueueCreate(10, sizeof(passive_evt_t));
    }
    // NEW: 進入被動監聽狀態（原 passive 任務起始行為）
    vTaskDelay(pdMS_TO_TICKS(100));
    init_passive_mode();

    // NEW: 建立並加入 QueueSet（同時監聽 passive_evt_queue 與 spi_request_queue）
    if (spi_evt_set == NULL) {
        // 估計總同時待處理項目數（可視實際 queue 長度調整）
        spi_evt_set = xQueueCreateSet(20);
        configASSERT(spi_evt_set != NULL);
    }
    if (passive_evt_queue) {
        xQueueAddToSet(passive_evt_queue, spi_evt_set);
    }
    if (spi_request_queue) {
        xQueueAddToSet(spi_request_queue, spi_evt_set);
    }

    while (1)
    {
        // NEW: 以 QueueSet 同步等待兩個來源的事件
        activated = xQueueSelectFromSet(spi_evt_set, pdMS_TO_TICKS(500));
        if (activated == passive_evt_queue) {
            if (xQueueReceive(passive_evt_queue, &passive_evt, 0) == pdPASS) {
                switch (passive_evt) {
                case PASSIVE_EVT_NEED_INIT:
                    vTaskDelay(pdMS_TO_TICKS(10));
                    if (GPIO_PinRead(GPIO, 2, 15) == 0 && operation_mode != MODE_ACTIVE) {
                        init_passive_mode();
                    }
                    break;
                case PASSIVE_EVT_RX_DONE:
                    PRINTF("[Passive] Sequence completed. Parsing frame...\r\n");
                    handle_passive_ack_frame(passive_rx_buffer);
                    PRINTF("[Passive] Ready for next sequence.\r\n");
                    break;
                default:
                    break;
                }
            }
        } else if (activated == spi_request_queue) {
            if (xQueueReceive(spi_request_queue, &received_value, 0) == pdPASS) {
            	//若 PASSIVE 還在忙，延後執行或重丟回 queue 等待下一輪 */
				if (passive_mode_busy || operation_mode == MODE_PASSIVE_ACK) {
					PRINTF("[Active] Passive ACK busy, deferring active request 0x%02X\r\n", received_value);
					(void)xQueueSend(spi_request_queue, &received_value, 0); // 丟回去，下一輪再試
					vTaskDelay(pdMS_TO_TICKS(100));
					continue;
				}
		        if (is_nova_active()) {
		            PRINTF("[Active] Nova active (PIO1_10=HIGH), deferring active request 0x%02X\r\n", received_value);
		            (void)xQueueSend(spi_request_queue, &received_value, 0);
		            vTaskDelay(pdMS_TO_TICKS(100));
		            continue;
		        }

		        for (int i = 0; i < SPI_ACTIVE_RETRY_TIME; i++) {
	                if (execute_active_spi_transmission(received_value) == kStatus_Success) {
	                	break;
	                }
	                vTaskDelay(pdMS_TO_TICKS(200));
		        }
		        if (received_value == POWER_LONG_PRESS_HEX_VALUE) {
	                set_ringtone_state(Ringtone_PowerOFF);
		            vTaskDelay(pdMS_TO_TICKS(200));
		        	led_post_event(HAL_LED_EVENT_POWER_OFF_PROGRESS);
		        }
            }
        }
    }
}

void send_spi_request(uint8_t hex_val)
{
	(void)xQueueSend(spi_request_queue, &hex_val, 0);

}

void spi_command_handler_init(void)
{
#if 0
    if (sys_bus_mutex == NULL) {
        sys_bus_mutex = xSemaphoreCreateMutex();
        configASSERT(sys_bus_mutex != NULL);
    }
#endif
    // 初始化 SPI frame 的 checksum
    dataFrame1[3] = calculateChecksum(dataFrame1, 3);
    dataFrame2[3] = calculateChecksum(dataFrame2, 3);

    PRINTF("=== SPI Slave Ready ===\r\n");
    PRINTF("GPIO is LOW: Passive mode active, waiting for Master...\r\n");
    //PRINTF("SPI Handler Task is ready, waiting for requests from the queue.\r\n");
    PRINTF("SPI Handler Task unified: waiting for passive/active events via QueueSet.\r\n");


    // NEW: 確保被動事件佇列存在
    if (passive_evt_queue == NULL) {
        passive_evt_queue = xQueueCreate(10, sizeof(passive_evt_t));
    }
    // NEW: 進入被動監聽狀態（原 passive 任務起始行為）
    vTaskDelay(pdMS_TO_TICKS(10));
    init_passive_mode();

    // NEW: 建立並加入 QueueSet（同時監聽 passive_evt_queue 與 spi_request_queue）
    if (spi_evt_set == NULL) {
        // 估計總同時待處理項目數（可視實際 queue 長度調整）
        spi_evt_set = xQueueCreateSet(20);
        configASSERT(spi_evt_set != NULL);
    }
    if (passive_evt_queue) {
        xQueueAddToSet(passive_evt_queue, spi_evt_set);
    }
    if (spi_request_queue) {
        xQueueAddToSet(spi_request_queue, spi_evt_set);
    }

}

void spi_command_handler(void)
{

	// NEW: 以 QueueSet 同步等待兩個來源的事件

	activated = xQueueSelectFromSet(spi_evt_set, pdMS_TO_TICKS(10));

	if (activated == passive_evt_queue) {
		if (xQueueReceive(passive_evt_queue, &passive_evt, 0) == pdPASS) {
			switch (passive_evt) {
			case PASSIVE_EVT_NEED_INIT:
				vTaskDelay(pdMS_TO_TICKS(10));
				if (GPIO_PinRead(GPIO, 2, 15) == 0 && operation_mode != MODE_ACTIVE) {
					init_passive_mode();
				}
				break;
			case PASSIVE_EVT_RX_DONE:
				PRINTF("[Passive] Sequence completed. Parsing frame...\r\n");
				handle_passive_ack_frame(passive_rx_buffer);
				PRINTF("[Passive] Ready for next sequence.\r\n");
				break;
			default:
				break;
			}
		}
	} else if (activated == spi_request_queue) {
		if (xQueueReceive(spi_request_queue, &received_value, 0) == pdPASS) {
			//若 PASSIVE 還在忙，延後執行或重丟回 queue 等待下一輪 */
			if (passive_mode_busy || operation_mode == MODE_PASSIVE_ACK) {
				PRINTF("[Active] Passive ACK busy, deferring active request 0x%02X\r\n", received_value);
				(void)xQueueSend(spi_request_queue, &received_value, 0); // 丟回去，下一輪再試
				vTaskDelay(pdMS_TO_TICKS(10));
				return;
			}
			if (is_nova_active()) {
				PRINTF("[Active] Nova active (PIO1_10=HIGH), deferring active request 0x%02X\r\n", received_value);
				(void)xQueueSend(spi_request_queue, &received_value, 0);
				vTaskDelay(pdMS_TO_TICKS(10));
				return;
			}

			for (int i = 0; i < SPI_ACTIVE_RETRY_TIME; i++) {
				PRINTF("[Active] execute_active_spi_transmission\r\n");

				if (execute_active_spi_transmission(received_value) == kStatus_Success) {
					break;
				}
				vTaskDelay(pdMS_TO_TICKS(20));
			}
			if (received_value == POWER_LONG_PRESS_HEX_VALUE) {
                set_ringtone_state(Ringtone_PowerOFF);
				vTaskDelay(pdMS_TO_TICKS(200));
				led_post_event(HAL_LED_EVENT_POWER_OFF_PROGRESS);
			}
		}
	}
}
#endif
#endif
