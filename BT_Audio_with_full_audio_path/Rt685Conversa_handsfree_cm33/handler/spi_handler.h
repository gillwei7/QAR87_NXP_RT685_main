/*
 * spi_handler.h
 *
 *  Created on: 2025年10月15日
 *      Author: 11301026
 */

#if 1
#ifndef SPI_HANDLER_H_
#define SPI_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_spi.h"


#define BUFFER_SIZE     (532)  // 總長度 fixed 532
#define HEADER_LEN      (16)   // 0x00 ~ 0x0F
#define PAYLOAD_LEN     (512)  // 0x10 ~ 0x20F (補零填充)
#define CRC_LEN         (4)    // 0x210 ~ 0x213

/* ===== SPI EventGroup bits  ===== */
#define MASTER_TRANSFER_EVENT_BIT      		(1UL << 0)
#define SLAVE_TRANSFER_EVENT_BIT      		(1UL << 1)
#define SPI_TRANSFER_COMPLETE_EVENT_BIT		(1UL << 2)
#define SLAVE_STREAMING_DATA_EVENT_BIT 		(1UL << 3)
#define SPI_CS_RISE_EVENT_BIT 				(1UL << 4)
#define MASTER_GRANT_ACK_EVENT_BIT          (1UL << 5)

/* 完全對應 SPEC-SPI-DMA-002-V2.3.0 的 Slave 狀態定義 */
typedef enum {
    S_IDLE,           // 就緒待命
    S_RX_Active,      // 被動接收中 (Master Clocking)
    S_PROCESSING,     // 解析與校驗 (Busy)
    S_ERROR_HOLD,     // 錯誤鎖死 (Persistent Busy)
    S_TX_REQ,         // 請求發起 (Pull Low)
    S_WAIT_GRANT,     // 等待授權
    S_DMA_TX_SETUP,   // 裝彈與釋放 (Arming)
    S_DMA_TX_RUN,     // 主動傳輸中 (Master Clocking)
    S_TX_VERIFY,      // 等待簽收 (ACK)
    S_TX_LOOP         // 連續傳輸間隙 (IPG) - 目前暫未用到
} slave_fsm_state_t;

extern volatile slave_fsm_state_t g_slave_state;

/*======<SPI>=======*/
#define FIXED_BUFFER_SIZE (4)
#define STATUS_BUFFER_SIZE (7)
#define NUM_FRAMES (3)

#define MAX_FRAME_SIZE 64
#define TRIGGER_HEX_VALUE 0x92
#define SYSTEM_STATUS_HEX_VALUE 0x93
#define POWER_SHORT_PRESS_HEX_VALUE 0x01
#define POWER_LONG_PRESS_HEX_VALUE 0x03
#define SHORT_PRESS_HEX_VALUE 0x21
#define LONG_PRESS_HEX_VALUE 0x23
#define ONE_TOUCH_HEX_VALUE 0x11
#define DOUBLE_TOUCH_HEX_VALUE 0x12
#define PRESS_TOUCH_HEX_VALUE 0x13
#define FORWARD_SLIDE_HEX_VALUE 0x14
#define BACK_SLIDE_HEX_VALUE 0x15
#define USAGE_STATE_HEX_VALUE 0x30
#define MUSIC_START_HEX_VALUE 0x40
#define MUSIC_STOP_HEX_VALUE  0x41

#define SPI_ACTIVE_RETRY_TIME         3


/*=======================================================================*/

/* ------------------------------------------------------------------------- *
 * 訊息類型定義 (MSG_TYPE)
 * ------------------------------------------------------------------------- */
#define CMD_ATOMIC_EXEC                         0x20  // NXP -> Novatek (執行指令)
#define CMD_ATOMIC_STATUS                       0x21  // NXP -> Novatek (狀態同步)
#define CMD_ATOMIC_EVENT                        0x22  // Novatek -> NXP (事件回報)


/*=================================================================*/
/* 定義字串最大長度 (可依據實際 Payload 限制調整) */
#define MAX_MSG_TITLE_LEN 64	// 標題維持 64 Bytes (約 21 個中文字)
#define MAX_MSG_BODY_LEN  440	// 內文大幅擴充！約 146 個中文字
/* (計算公式：512(Payload) - 4(Payload Header) - 1(AppType) - 1(TitleLen) - 2(BodyLen) - 64(Title) = 440 Bytes) */

#define MAX_IP_LEN 	 32
#define MAX_SSID_LEN 64
#define MAX_URL_LEN  64

/* App 類型列舉 */
typedef enum {
    APP_TYPE_SMS       = 0,
    APP_TYPE_LINE      = 1,
    APP_TYPE_WECHAT    = 2,
    APP_TYPE_MESSENGER = 3,
    APP_TYPE_EMAIL     = 4,
    APP_TYPE_OTHERS    = 5
} app_msg_type_t;

/* 訊息通知資料結構 */
typedef struct {
    uint8_t app_type;                 // AppType
    uint8_t title_len;                // TitleLength (N)
    uint16_t body_len;                 // BodyLength (M)
    char title[MAX_MSG_TITLE_LEN];    // 標題緩衝區
    char body[MAX_MSG_BODY_LEN];      // 內文緩衝區
} msg_notification_info_t;

/* ip ssid 資料結構 */
typedef struct {
    uint8_t ip_len;                   // IP Length
    uint8_t ssid_len;                 // SSID Length
    char ip[MAX_IP_LEN];
    char ssid[MAX_SSID_LEN];
} ip_ssid_info_t;

/* url 資料結構 */
typedef struct {
    uint8_t url_len;                   // URL Length (N)
    char url[MAX_URL_LEN];
} url_info_t;

/*=================================================================*/

void set_system_time(uint16_t year, uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t second);

void application_examples_atomic_exec(void);

/*=======================================================================*/

void spi_handler_task(void *pvParameters);
void send_spi_request(uint8_t msg_type, uint8_t cmd_id);


// 串流傳輸控制結構
typedef struct {
    bool     active;        // 是否正在進行串流測試
    uint32_t current_pkt;   // 目前傳到第幾包 (0~19)
    uint32_t total_pkts;    // 總包數 (20)
    uint32_t retry_count;   // 當前封包重試次數
    uint32_t global_crc;    // 累積的 CRC
} stream_ctx_t;

// 宣告這個函數，供 button_task 呼叫
void start_spi_streaming_test(void); 

slave_fsm_state_t spi_protocol_get_status (void);

#endif /* SPI_HANDLER_H_ */

#endif
