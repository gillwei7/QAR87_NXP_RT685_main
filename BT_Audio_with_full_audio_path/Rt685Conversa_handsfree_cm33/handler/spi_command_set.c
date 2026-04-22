/*
 * spi_command_set.c
 *
 *  Created on: 2026年4月8日
 *      Author: 11301026
 */


#include "spi_command_set.h"
#include "scenario_state.h"
#include "system_status.h"
#include "hal_common.h"
#include "ringtone_handler.h"
#include "scenario_state.h"
#include "i2c_component_handler.h"
#include "app_handsfree.h"
#include "hal_led.h"

static spi_command_ui_page_t s_ui_page_id = SPI_COMMAND_UI_PAGE_HOME;
static spi_command_media_play_pause_t s_media_play_pause = SPI_COMMAND_MEDIA_PLAY_TOGGLE;
static msg_notification_info_t g_msg_info = {0};

static ip_ssid_info_t g_ip_ssid_info = {0};
static url_info_t g_url_info = {0};

static uint8_t args_buff[BUFFER_SIZE];
uint8_t Novatek_boot_completed = 0;

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

static void url_processing(const char *url)
{
    // 1. 清空緩衝區
    memset(g_url_info.url, 0, MAX_URL_LEN);

    // 2. 處理 URL 字串
    if (url != NULL && url[0] != '\0') {
        size_t u_len = strlen(url);

        // 確保長度不超過 Buffer，預留 1 byte 給 \0
        if (u_len >= MAX_URL_LEN) {
            u_len = MAX_URL_LEN - 1;
        }

        // 拷貝字串內容
        memcpy(g_url_info.url, url, u_len);

        // 確保結尾為 \0 (雖然 memset 已經處理，但手動指定更安全)
        g_url_info.url[u_len] = '\0';

        // 儲存實際長度
        g_url_info.url_len = (uint8_t)u_len;
    } else {
        g_url_info.url_len = 0;
    }
}

static void ip_ssid_processing(const char *ip, const char *ssid)
{
    // 1. 清空緩衝區
    memset(g_ip_ssid_info.ip, 0, MAX_IP_LEN);
    memset(g_ip_ssid_info.ssid, 0, MAX_SSID_LEN);

    // 2. 處理 IP
    if (ip != NULL && ip[0] != '\0') {
        size_t i_len = strlen(ip);

        // 如果長度大於等於 MAX_IP_LEN，截斷並預留 1 byte 給 \0
        if (i_len >= MAX_IP_LEN) {
            i_len = MAX_IP_LEN - 1;
        }

        memcpy(g_ip_ssid_info.ip, ip, i_len);
        g_ip_ssid_info.ip[i_len] = '\0'; // 補上字串結束符
        g_ip_ssid_info.ip_len = (uint8_t)i_len;
    } else {
        g_ip_ssid_info.ip_len = 0;
    }

    // 3. 處理 SSID
    if (ssid != NULL && ssid[0] != '\0') {
        size_t s_len = strlen(ssid);

        // 如果長度大於等於 MAX_SSID_LEN，截斷並預留 1 byte 給 \0
        if (s_len >= MAX_SSID_LEN) {
            s_len = MAX_SSID_LEN - 1;
        }

        memcpy(g_ip_ssid_info.ssid, ssid, s_len);
        g_ip_ssid_info.ssid[s_len] = '\0'; // 補上字串結束符
        g_ip_ssid_info.ssid_len = (uint8_t)s_len;
    } else {
        g_ip_ssid_info.ssid_len = 0;
    }
}

static void message_processing(app_msg_type_t app_type, const char *title, const char *body)
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


void spi_command_atomic_exec_start_video_call(const char *url)
{
	url_processing(url);
	send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_START_VIDEO_CALL);
}
void spi_command_atomic_exec_stop_video_call(void)
{
	send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_STOP_VIDEO_CALL);
}
void spi_command_atomic_exec_start_wifi_ap(const char *ip, const char *ssid)
{
	ip_ssid_processing(ip,ssid);
	send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_START_WIFI_AP);
}
void spi_command_atomic_exec_stop_wifi_ap(void)
{
	send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_STOP_WIFI_AP);
}

void spi_command_atomic_exec_open_oe(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_OPEN_OE);
}

void spi_command_atomic_exec_close_oe(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_CLOSE_OE);
}

void spi_command_atomic_exec_soft_power_off(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_SOFT_POWER_OFF);
}

void spi_command_atomic_exec_unload_wifi_driver(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_UNLOAD_WIFI_DRIVER);
}

void spi_command_atomic_exec_enter(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_ENTER);
}

void spi_command_atomic_exec_exit(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_EXIT);
}

void spi_command_atomic_exec_menu_left(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MENU_LEFT);
}

void spi_command_atomic_exec_menu_right(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MENU_RIGHT);
}

void spi_command_atomic_exec_media_start(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_START);
}

void spi_command_atomic_exec_media_stop(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_STOP);
}

void spi_command_atomic_exec_next_media(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_NEXT_MEDIA);
}

void spi_command_atomic_exec_previous_media(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_PREVIOUS_MEDIA);
}

void spi_command_atomic_exec_media_play_pause(uint8_t media_state)
{
	s_media_play_pause = media_state;
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_MEDIA_PLAY_PAUSE);
}

void spi_command_atomic_exec_take_picture(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_TAKE_PICTURE);
}

void spi_command_atomic_exec_stop_recording(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_STOP_RECORDING);
}

void spi_command_atomic_exec_start_recording(void)
{
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_START_RECORDING);
}


void spi_command_atomic_exec_switch_ui_page(spi_command_ui_page_t state)
{
    s_ui_page_id = state;
    send_spi_request(CMD_ATOMIC_EXEC, CMD_ATOMIC_EXEC_SWITCH_UI_PAGE);
}

void spi_command_atomic_status_version_info(void)
{
    send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_VERSION_INFO_SYNC);
}

void spi_command_atomic_status_sys_status(void)
{
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_SYS_STATUS);
}

void spi_command_atomic_status_time_sync(   uint16_t year,
											uint8_t  month,
											uint8_t  day,
											uint8_t  hour,
											uint8_t  minute,
											uint8_t  second )
{
	set_system_time(year, month, day, hour, minute, second);
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_TIME_SYNC);

}

void spi_command_atomic_status_message_notification(app_msg_type_t app_type, const char *title, const char *body)
{
	message_processing(app_type, title, body);
	send_spi_request(CMD_ATOMIC_STATUS, CMD_ATOMIC_STATUS_MSG_NOTIFICATION);
}

void spi_command_atomic_event_parser (uint8_t event_id, const uint8_t *args)
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

		case CMD_ATOMIC_EVENT_WIFI_AP_OPEN:
			PRINTF("[SPI][Event] WIFI_AP_OPEN \r\n ");
			break;

		case CMD_ATOMIC_EVENT_WIFI_AP_CLOSE:
			PRINTF("[SPI][Event] WIFI_AP_CLOSE \r\n ");
			break;

		case CMD_ATOMIC_EVENT_UNKNOWN_CMD_ERROR:
			PRINTF("[SPI][Event] UNKNOWN_CMD_ERROR \r\n ");
			break;

	}
}

uint8_t spi_command_get_args_and_len (uint8_t msg_type, uint8_t cmd_id, char *pArgs)
{
	uint8_t arg_len = 0;

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

             case CMD_ATOMIC_EXEC_START_VIDEO_CALL: // START VIDEO_CALL

                 /* arg_len = 1 (URL_Len) + N */
                 arg_len = 1 + g_url_info.url_len;

                 /* Args[0]: URL Length (N) */
                 pArgs[0] = g_url_info.url_len;

                 /* 複製 URL 內容 */
                 if (g_url_info.url_len > 0) {
                     memcpy(&pArgs[1], g_url_info.url, g_url_info.url_len);
                 }

                 break;

             case CMD_ATOMIC_EXEC_START_WIFI_AP: // START_WIFI_AP
                 /* arg_len = 2 (IP_Len + SSID_Len) + N + M */
                 arg_len = 2 + g_ip_ssid_info.ip_len + g_ip_ssid_info.ssid_len;

                 /* Args[0]: IP Length (N) */
                 pArgs[0] = g_ip_ssid_info.ip_len;

                 /* Args[1]: SSID Length (M) */
                 pArgs[1] = g_ip_ssid_info.ssid_len;

                 /* 複製 IP 字串內容 */
                 if (g_ip_ssid_info.ip_len > 0) {
                     memcpy(&pArgs[2], g_ip_ssid_info.ip, g_ip_ssid_info.ip_len);
                 }

                 /* 複製 SSID 字串內容，起始位置在 pArgs[2 + IP長度] */
                 if (g_ip_ssid_info.ssid_len > 0) {
                     memcpy(&pArgs[2 + g_ip_ssid_info.ip_len], g_ip_ssid_info.ssid, g_ip_ssid_info.ssid_len);
                 }
                  break;

             default:
                 // 其他控制指令如 OPEN OE(0x01), TAKE PICTURE(0x21) 等，無參數
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
                 break;
         }
     }
     return arg_len;
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

spi_command_ui_page_t get_ui_page_id(void)
{
	return s_ui_page_id;
}

spi_command_media_play_pause_t get_media_play_pause_cmd(void)
{
	return s_media_play_pause;
}
