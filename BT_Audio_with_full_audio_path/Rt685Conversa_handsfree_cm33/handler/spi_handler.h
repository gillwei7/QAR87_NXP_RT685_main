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

/* ------------------------------------------------------------------------- *
 * [0x20] CMD_ATOMIC_EXEC (執行指令：NXP -> Novatek)
 * 適用場景: NXP 請求 Novatek 執行特定動作 (Action)
 * ------------------------------------------------------------------------- */
#define CMD_ATOMIC_EXEC_OPEN_OE                 0x01  // 開啟光機/螢幕
#define CMD_ATOMIC_EXEC_CLOSE_OE                0x02  // 關閉光機/螢幕
#define CMD_ATOMIC_EXEC_SOFT_POWER_OFF          0x03  // 通知軟關機
#define CMD_ATOMIC_EXEC_UNLOAD_WIFI_DRIVER      0x04  // 解除/卸載 Wi-Fi 驅動

#define CMD_ATOMIC_EXEC_ENTER                   0x11  // UI：確認/進入
#define CMD_ATOMIC_EXEC_EXIT                    0x15  // UI：退出/返回
#define CMD_ATOMIC_EXEC_MENU_LEFT               0x17  // UI：選單左移
#define CMD_ATOMIC_EXEC_MENU_RIGHT              0x19  // UI：選單右移

#define CMD_ATOMIC_EXEC_MEDIA_START             0x12  // 啟動播放器
#define CMD_ATOMIC_EXEC_MEDIA_STOP              0x13  // 停止播放
#define CMD_ATOMIC_EXEC_NEXT_MEDIA              0x14  // 下一首/下一部
#define CMD_ATOMIC_EXEC_PREVIOUS_MEDIA          0x1C  // 上一首
#define CMD_ATOMIC_EXEC_MEDIA_PLAY_PAUSE        0x1D  // 播放/暫停(含強制設定)

#define CMD_ATOMIC_EXEC_TAKE_PICTURE            0x21  // 拍照
#define CMD_ATOMIC_EXEC_STOP_RECORDING          0x22  // 停止錄影
#define CMD_ATOMIC_EXEC_START_RECORDING         0x23  // 開始錄影

#define CMD_ATOMIC_EXEC_START_VIDEO_CALL        0x28  // 視訊會議開始 (包含傳送URL)
#define CMD_ATOMIC_EXEC_STOP_VIDEO_CALL         0x29  // 視訊會議停止
#define CMD_ATOMIC_EXEC_START_WIFI_AP           0x2A  // 開啟WIFI AP (包含傳送IP、SSID)
#define CMD_ATOMIC_EXEC_STOP_WIFI_AP            0x2B  // 關閉WIFI AP

#define CMD_ATOMIC_EXEC_SWITCH_UI_PAGE          0x30  // 要求切換至指定 UI 頁面

#define CMD_ATOMIC_EXEC_START_VIDEO_AI          0x31
#define CMD_ATOMIC_EXEC_STOP_VIDEO_AI           0x32
#define CMD_ATOMIC_EXEC_START_TRANSLATION       0x33
#define CMD_ATOMIC_EXEC_STOP_TRANSLATION        0x34

/* ------------------------------------------------------------------------- *
 * [0x21] CMD_ATOMIC_STATUS (資訊同步：NXP -> Novatek)
 * 適用場景: NXP 將自身的狀態數據推送給 Novatek
 * ------------------------------------------------------------------------- */
#define CMD_ATOMIC_STATUS_TIME_SYNC             0x91  // 同步系統時間
#define CMD_ATOMIC_STATUS_MSG_NOTIFICATION      0x92  // 推送手機通知訊息
#define CMD_ATOMIC_STATUS_SYS_STATUS            0x93  // 同步電量/充電/藍牙連線狀態
#define CMD_ATOMIC_STATUS_VERSION_INFO_SYNC     0x94  // 同步 NXP 版本號
#define CMD_ATOMIC_STATUS_VOLUME_INFO_SYNC      0x95  // 同步當前音量值與狀態
#define CMD_ATOMIC_STATUS_NXP_OTA_STATUS_SYNC   0x96  // 同步 NXP OTA 更新狀態

/* ------------------------------------------------------------------------- *
 * [0x22] CMD_ATOMIC_EVENT (事件回報：Novatek -> NXP)
 * 適用場景: Novatek 主動回報硬體、UI 或系統發生的事件
 * ------------------------------------------------------------------------- */
#define CMD_ATOMIC_EVENT_SYSTEM_BOOT_DONE       0x11  // 回報：已開機完成

#define CMD_ATOMIC_EVENT_CAMERA_ACTIVATED       0x01  // 回報：相機已經啟動
#define CMD_ATOMIC_EVENT_PHOTO_CAPTURED         0x02  // 回報：拍照完成
#define CMD_ATOMIC_EVENT_CAMERA_CLOSED          0x03  // 回報：相機已關閉
#define CMD_ATOMIC_EVENT_RECORDING_STARTED      0x04  // 回報：錄影開始
#define CMD_ATOMIC_EVENT_RECORDING_STOPPED      0x05  // 回報：錄影停止

#define CMD_ATOMIC_EVENT_VIDEO_PLAY_STARTED     0x06  // 回報：影片開始播放
#define CMD_ATOMIC_EVENT_VIDEO_PLAY_PAUSED      0x07  // 回報：影片已暫停

#define CMD_ATOMIC_EVENT_WIFI_CONNECTED         0x08  // 回報：Wi-Fi 連線成功
#define CMD_ATOMIC_EVENT_WIFI_DISCONNECTED      0x09  // 回報：Wi-Fi 連線中斷
#define CMD_ATOMIC_EVENT_MSG_NOTIFIED           0x0A  // 回報：訊息已顯示/提示

#define CMD_ATOMIC_EVENT_UI_PAGE_CHANGED        0x30  // 回報：UI 頁面已切換
#define CMD_ATOMIC_EVENT_OTA_STARTED            0x38  // 回報：韌體更新開始
#define CMD_ATOMIC_EVENT_OTA_FINISHED           0x39  // 回報：韌體更新結果

#define CMD_ATOMIC_EVENT_WIFI_AP_OPEN           0x40  // 回報: WIFI AP開啟
#define CMD_ATOMIC_EVENT_WIFI_AP_CLOSE          0x41  // 回報: WIFI AP關閉

#define CMD_ATOMIC_EVENT_UNKNOWN_CMD_ERROR      0xFF  // 錯誤回報：未知的指令


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
