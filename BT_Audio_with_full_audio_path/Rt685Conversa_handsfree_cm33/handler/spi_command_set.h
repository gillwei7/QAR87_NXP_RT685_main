/*
 * spi_command_set.h
 *
 *  Created on: 2026年4月8日
 *      Author: 11301026
 */

#ifndef SPI_COMMAND_SET_H_
#define SPI_COMMAND_SET_H_

#include "spi_handler.h"


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



typedef enum {
	SPI_COMMAND_UI_PAGE_HOME = 0,
	SPI_COMMAND_UI_PAGE_LAUNCHER,
	SPI_COMMAND_UI_PAGE_NOTIFICATION,
	SPI_COMMAND_UI_PAGE_ABOUT,
	SPI_COMMAND_UI_PAGE_SNAPSHOT,
	SPI_COMMAND_UI_PAGE_RTSP,
	SPI_COMMAND_UI_PAGE_MEDIA,
	SPI_COMMAND_UI_PAGE_RECORDING,
	SPI_COMMAND_UI_PAGE_HEARING_AID,
	SPI_COMMAND_UI_PAGE_AI_CHAT,
	SPI_COMMAND_UI_PAGE_TRANSLATION,
	SPI_COMMAND_UI_PAGE_MUSIC_PLAYER,
} spi_command_ui_page_t;


/**
 * @brief Media Play/Pause control command (0x00~0x02).
 *
 * 0x00: Toggle       - 若播放中則暫停，若暫停則播放。
 * 0x01: Force Play   - 確保進入播放狀態。
 * 0x02: Force Pause  - 確保進入暫停狀態。
 */
typedef enum {
	SPI_COMMAND_MEDIA_PLAY_TOGGLE = 0x00,  ///< Toggle (切換)
	SPI_COMMAND_MEDIA_FORCE_PLAY  = 0x01,  ///< Force Play (播放)
	SPI_COMMAND_MEDIA_FORCE_PAUSE = 0x02,  ///< Force Pause (暫停)
} spi_command_media_play_pause_t;


/* 定義系統時間結構體 */
typedef struct {
    uint16_t year;   // 年份 (例如 2026)
    uint8_t  month;  // 月份 (1-12)
    uint8_t  day;    // 日期 (1-31)
    uint8_t  hour;   // 小時 (0-23)
    uint8_t  minute; // 分鐘 (0-59)
    uint8_t  second; // 秒數 (0-59)
} spi_command_time_info_t;


void spi_command_atomic_exec_open_oe(void);
void spi_command_atomic_exec_close_oe(void);
void spi_command_atomic_exec_soft_power_off(void);
void spi_command_atomic_exec_unload_wifi_driver(void);

void spi_command_atomic_exec_enter(void);
void spi_command_atomic_exec_exit(void);
void spi_command_atomic_exec_menu_left(void);
void spi_command_atomic_exec_menu_right(void);

void spi_command_atomic_exec_media_start(void);
void spi_command_atomic_exec_media_stop(void);
void spi_command_atomic_exec_next_media(void);
void spi_command_atomic_exec_previous_media(void);
void spi_command_atomic_exec_media_play_pause(uint8_t media_state);

void spi_command_atomic_exec_take_picture(void);
void spi_command_atomic_exec_stop_recording(void);
void spi_command_atomic_exec_start_recording(void);

void spi_command_atomic_exec_switch_ui_page(spi_command_ui_page_t state);

void spi_command_atomic_exec_start_video_call(const char *url);
void spi_command_atomic_exec_stop_video_call(void);
void spi_command_atomic_exec_start_video_ai(void);
void spi_command_atomic_exec_stop_video_ai(void);
void spi_command_atomic_exec_start_translation(void);
void spi_command_atomic_exec_stop_translation(void);

void spi_command_atomic_exec_start_wifi_ap(void);
void spi_command_atomic_exec_stop_wifi_ap(void);

void spi_command_atomic_status_version_info(void);
void spi_command_atomic_status_sys_status(void);
void spi_command_atomic_status_message_notification(app_msg_type_t app_type, const char *title, const char *body);
void spi_command_atomic_status_time_sync(   uint16_t year,
											uint8_t  month,
											uint8_t  day,
											uint8_t  hour,
											uint8_t  minute,
											uint8_t  second );

void spi_command_atomic_event_parser (uint8_t event_id, const uint8_t *args);

uint8_t spi_command_get_args_and_len (uint8_t msg_type, uint8_t cmd_id, char *pArgs);


#endif /* SPI_COMMAND_SET_H_ */
