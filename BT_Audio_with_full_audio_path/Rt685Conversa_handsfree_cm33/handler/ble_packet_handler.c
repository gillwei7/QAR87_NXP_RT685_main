#include <ble_event_handler.h>
#include <ble_packet_handler.h>
/*
 * ble_gatt_parse.c
 *
 *  Created on: 2026年4月14日
 *      Author: 11301026
 */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno/errno.h>
#include <toolchain.h>
#include <porting.h>

#include "fsl_debug_console.h"
#include "fsl_crc.h"
#include <string.h>
#include "spi_command_set.h"
#include "system_status.h"


static uint8_t video_call_url[128] = {0};


// 標籤位遮罩
#define FLAG_FIRST_PACKET 0x01
#define FLAG_LAST_PACKET  0x02

// 內部狀態結構
typedef struct {
    uint8_t  buffer[MAX_MSG_BUFFER_SIZE];
    uint32_t current_len;
    uint32_t expected_total_len;
    uint32_t expected_crc32;
    uint32_t last_update_ms;
    ble_msg_type_t active_type;
    bool     is_assembling;
} ble_parse_ctx_t;

static ble_parse_ctx_t g_ctx;

// --- NXP 硬體 CRC 初始化與計算 ---

static void InitCrc32(CRC_Type *base, uint32_t seed)
{
    crc_config_t config;

    /* 標準 CRC-32 設定 (與 Java/Android java.util.zip.CRC32 相容) */
    config.polynomial    = kCRC_Polynomial_CRC_32;
    config.reverseIn     = false;//true;
    config.complementIn  = false;
    config.reverseOut    = false;//true;
    config.complementOut = true;
    config.seed          = seed;

    CRC_Init(base, &config);
}

static uint32_t ble_calculate_crc32(const uint8_t *data, size_t length)
{
    CRC_Type *base = CRC_ENGINE;

    /* 1. 初始化 (Seed = 0xFFFFFFFF) */
    InitCrc32(base, 0xFFFFFFFFU);

    /* 2. 寫入資料 */
    CRC_WriteData(base, (const uint8_t *)data, length);

    /* 3. 回傳結果 */
    return CRC_Get32bitResult(base);

}

//Read Uint16 Big-Endian
static uint16_t read_u16_big_endian(const uint8_t *p) { return (p[0] << 8) | p[1]; }
//Read Uint32 Big-Endian
static uint32_t read_u32_big_endian(const uint8_t *p) {
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

// --- 核心解析函式 ---
int ble_packet_parser (const uint8_t *buf, uint16_t len) {
    // Stage 0: 接收基礎檢查
	BLE_GATT_PARSE_DEBUG("Received packet, len = %d\n", len);

    if (len < BLE_PROTO_HDR_SIZE) {
    	BLE_GATT_PARSE_ERROR("Packet too short (%d bytes), drop it.\n", len);
        return -1;
    }
    if (buf[0] != BLE_PROTO_VERSION) {
    	BLE_GATT_PARSE_ERROR("Version mismatch! Expected: 0x01, Got: 0x%02X\n", buf[0]);
        return -2;
    }

    uint8_t type = buf[1];
    uint8_t flags = buf[2];
    uint16_t seq = read_u16_big_endian(&buf[4]);
    uint16_t total_pkts = read_u16_big_endian(&buf[6]);
    uint32_t payload_offset = BLE_PROTO_HDR_SIZE;

    BLE_GATT_PARSE_DEBUG(" Header -> Type: 0x%02X, Flags: 0x%02X, Seq: %d/%d\n", type, flags, seq, total_pkts);

    // Stage 1: 處理起始包 (First Packet)
    if (flags & FLAG_FIRST_PACKET) {
        if (len < (BLE_PROTO_HDR_SIZE + BLE_PROTO_EXTRA_HDR)) {
        	BLE_GATT_PARSE_ERROR("First packet too short for extra header.\n");
            return -3;
        }

        g_ctx.expected_total_len = read_u32_big_endian(&buf[8]);
        g_ctx.expected_crc32 = read_u32_big_endian(&buf[12]);
        g_ctx.current_len = 0;
        g_ctx.active_type = (ble_msg_type_t)type;
        g_ctx.is_assembling = true;

        payload_offset += BLE_PROTO_EXTRA_HDR;

        BLE_GATT_PARSE_DEBUG(" >>> First Packet detected! Total Len: %lu bytes, Expected CRC: 0x%08lX\n",
                g_ctx.expected_total_len, g_ctx.expected_crc32);
    }

    // 防護：檢查當前是否處於重組狀態
    if (!g_ctx.is_assembling) {
    	BLE_GATT_PARSE_WARN("Received sequence without First Packet. Dropping.\n");
        return -4;
    }

    // Stage 2: 累加 Payload 到緩衝區
    uint32_t payload_len = len - payload_offset;
    if (g_ctx.current_len + payload_len > MAX_MSG_BUFFER_SIZE) {
    	BLE_GATT_PARSE_ERROR("Buffer overflow! Attempted to exceed %d bytes.\n", MAX_MSG_BUFFER_SIZE);
        g_ctx.is_assembling = false; // 溢出重置
        return -5;
    }

    memcpy(&g_ctx.buffer[g_ctx.current_len], &buf[payload_offset], payload_len);

    BLE_GATT_PARSE_DEBUG("Appended %lu bytes. Current accumulated size: %lu/%lu\n",
            payload_len, (g_ctx.current_len + payload_len), g_ctx.expected_total_len);

    g_ctx.current_len += payload_len;

    // Stage 3: 處理結束包 (Last Packet)
    if (flags & FLAG_LAST_PACKET) {
    	BLE_GATT_PARSE_DEBUG(" <<< Last Packet detected. Verifying full message...\n");

        // 校驗 CRC32
        if (g_ctx.expected_crc32 != 0) {
        	BLE_GATT_PARSE_DEBUG("Calculating CRC32 for %lu bytes...\n", g_ctx.current_len);
            uint32_t actual_crc = ble_calculate_crc32(g_ctx.buffer, g_ctx.current_len);

            if (actual_crc != g_ctx.expected_crc32) {
            	BLE_GATT_PARSE_ERROR("CRC Mismatch! Expected: 0x%08lX, Got: 0x%08lX\n",
                        g_ctx.expected_crc32, actual_crc);
                g_ctx.is_assembling = false;
                return -6;
            }
            BLE_GATT_PARSE_DEBUG("CRC Check Passed (0x%08lX)!\n", actual_crc);
        } else {
        	BLE_GATT_PARSE_WARN("Expected CRC was 0. Skipping CRC check.\n");
        }

        //解析訊息
    	BLE_GATT_PARSE_DEBUG("Dispatching message type 0x%02X to message parse...\n", g_ctx.active_type);
    	ble_message_parser(g_ctx.active_type, g_ctx.buffer, g_ctx.current_len);

        // 重置狀態迎接下一則訊息
        g_ctx.is_assembling = false;
        BLE_GATT_PARSE_DEBUG("Message process completed. Context reset.\n");
    }

    return 0;
}

void ble_message_parser (ble_msg_type_t type, const uint8_t *data, size_t len)
{
    if (type == BLE_MSG_TYPE_AI_RESPONSE) {
    	PRINTF("[BLE][App] BLE_MSG_TYPE_AI_RESPONSE \r\n");
    	PRINTF("[BLE][App][AI] Content: %.*s\r\n", (int)len, (const char *)data);
    } else if (type == BLE_MSG_TYPE_TRANSLATION) {
    	PRINTF("[BLE][App] BLE_MSG_TYPE_TRANSLATION \r\n");
    	PRINTF("[BLE][App][Trans] Content: %.*s\r\n", (int)len, (const char *)data);
    }else if (type == BLE_MSG_TYPE_COMMAND) {
    	PRINTF("[BLE][App] BLE_MSG_TYPE_COMMAND \r\n");
        if (strcmp(data, "Start_AP") == 0)
        {
        	PRINTF("[BLE][App][CMD] Start_AP \r\n");
        }
        else if(strcmp(data, "Start_AP_IP") == 0)
        {
        	PRINTF("[BLE][App][CMD] Start_AP_IP \r\n");
        }
        else if (strcmp(data, "WIFI_CONNECTED") == 0)
        {
            PRINTF("[BLE][App][CMD] WIFI_CONNECTED \r\n");
        }
        else if (strcmp(data, "STOP_VIDEOCHAT") == 0)
        {
        	PRINTF("[BLE][App][CMD] STOP_VIDEOCHAT \r\n");
        }
        else if (strcmp(data, "stopTranslation") == 0)
        {
        	PRINTF("[BLE][App][CMD] stopTranslation \r\n");
        }
        else if (strcmp(data, "TAKE_PICTURE") == 0)
        {
        	PRINTF("[BLE][App][CMD] TAKE_PICTURE \r\n");
        }
        else if (strcmp(data, "START_RECORDING") == 0)
        {
        	PRINTF("[BLE][App][CMD] START_RECORDING \r\n");
        }
        else if (strcmp(data, "startFileSync") == 0)
        {
        	PRINTF("[BLE][App][CMD] startFileSync \r\n");
        }
        else if (strcmp(data, "NEW_MEDIA") == 0)
        {
        	PRINTF("[BLE][App][CMD] NEW_MEDIA \r\n");
        }
        else if (strcmp(data, "STOP_RECORDING") == 0)
        {
        	PRINTF("[BLE][App][CMD] STOP_RECORDING \r\n");
        }
        else if (strcmp(data, "RTSP_AUDIO_ONLY_ON") == 0)
        {
        	PRINTF("[BLE][App][CMD] RTSP_AUDIO_ONLY_ON \r\n");
        }
        else if (strcmp(data, "RTSP_AV") == 0)
        {
        	PRINTF("[BLE][App][CMD] RTSP_AV \r\n");
        }
        else if (strcmp(data, "FileSync") == 0)
        {
        	PRINTF("[BLE][App][CMD] FileSync \r\n");
        }
        else if (strncmp(data, "RECV:", 5) == 0)
        {
        	PRINTF("[BLE][App][CMD] RECV \r\n");
        }
        else if (strcmp(data, "RTSP_RECV_STOP") == 0)
        {
        	PRINTF("[BLE][App][CMD] RTSP_RECV_STOP \r\n");
        }
        else if (strcmp(data, "Stop_AP") == 0)
        {
        	PRINTF("[BLE][App][CMD] RTStop_APSP_AV \r\n");
        }
        else if (strcmp(data, "PHONE_PREVIEW_SHOW") == 0)
        {
        	PRINTF("[BLE][App][CMD] PHONE_PREVIEW_SHOW \r\n");
        }
        else if (strcmp(data, "PHONE_PREVIEW_HIDE") == 0)
        {
        	PRINTF("[BLE][App][CMD] PHONE_PREVIEW_HIDE \r\n");
        }
        else if (strcmp(data, "ACTION_GLASS_BLACK_PREVIEW") == 0)
        {
        	PRINTF("[BLE][App][CMD] ACTION_GLASS_BLACK_PREVIEW \r\n");
        }
        else if (strcmp(data, "ACTION_GLASS_CLEAR_BLACK_PREVIEW") == 0)
        {
        	PRINTF("[BLE][App][CMD] ACTION_GLASS_CLEAR_BLACK_PREVIEW \r\n");
        }
        else {
            PRINTF("[BLE][App][CMD] Unknown command: %s\n", data);
        }

    }
}

static uint8_t peripheral_ble_get_cmd_id(uint8_t * ble_data)
{
	if (strcmp(ble_data, "Start_AP") == 0)
	{
		return BLE_CMD_ID_START_AP;
	}
	else if (strcmp(ble_data, "Start_AP_IP") == 0)
	{
		return BLE_CMD_ID_START_AP_IP;
	}
	else if (strcmp(ble_data, "WIFI_CONNECTED") == 0)
	{
		return BLE_CMD_ID_WIFI_CONNECTED;
	}
	else if (strcmp(ble_data, "RTSP_AV") == 0)
	{
		return BLE_CMD_ID_RTSP_AV;
	}
	else if(strstr(ble_data,"Start_Video_Call_URL:") != NULL)
	{
		return BLE_CMD_ID_START_VIDEO_CALL_URL ;
	}
	else if(strcmp(ble_data, "Stop_Video_Call") == 0)
	{
		return BLE_CMD_ID_STOP_VIDEO_CALL;
	}
	else if (strcmp(ble_data, "Enter_Video_Call") == 0)
	{
		return BLE_CMD_ID_ENTER_VIDEO_CALL;
	}
	else if (strcmp(ble_data, "Leave_Video_Call") == 0)
	{
		return BLE_CMD_ID_LEAVE_VIDEO_CALL;
	}
    else if (strcmp(ble_data, "STOP_VIDEOCHAT") == 0) {
		return BLE_CMD_ID_LEAVE_VIDEO_CALL;
    }
    else if (strcmp(ble_data, "stopTranslation") == 0) {
		return BLE_CMD_ID_STOP_TRANSLATION;
    }
    else if (strcmp(ble_data, "TAKE_PHOTO") == 0) {
		return BLE_CMD_ID_TAKE_PHOTO;
    }
    else if (strcmp(ble_data, "START_RECORDING") == 0) {
		return BLE_CMD_ID_START_RECORDING;
    }
    else if (strcmp(ble_data, "startFileSync") == 0) {
		return BLE_CMD_ID_START_FILE_SYNC;
    }
    else if (strcmp(ble_data, "NEW_MEDIA") == 0) {
		return BLE_CMD_ID_NEW_MEDIA;
    }
    else if (strcmp(ble_data, "STOP_RECORDING") == 0) {
		return BLE_CMD_ID_STOP_RECORDING;
    }
    else if (strcmp(ble_data, "RTSP_AUDIO_ONLY_ON") == 0) {
		return BLE_CMD_ID_RTSP_AUDIO_ONLY_ON;
    }

	return BLE_CMD_ID_UNKNOWN ;
}

void peripheral_ble_cmd_parser(uint8_t * ble_data, uint16_t data_len)
{
	uint8_t cmd_id = peripheral_ble_get_cmd_id(ble_data);

	switch(cmd_id)
	{
		case BLE_CMD_ID_ENTER_VIDEO_CALL :
			PRINTF("[BLE Parser] ENTER_VIDEO_CALL_URL\n");
			/*NXP 更改 Audio Path 設定 */
			break;

		case BLE_CMD_ID_START_AP :
			PRINTF("[BLE Parser] Start_AP\n");
			spi_command_atomic_exec_start_wifi_ap();
			break;

		case BLE_CMD_ID_START_AP_IP :
			PRINTF("[BLE Parser] Start_AP_IP\n");
			ble_send_event_ip_ssid();
			break;

		case BLE_CMD_ID_WIFI_CONNECTED :
			PRINTF("[BLE Parser] WIFI_CONNECTED\n");
			break;

		case BLE_CMD_ID_RTSP_AV :
			PRINTF("[BLE Parser] RTSP_AV\n");
			break;

		case BLE_CMD_ID_START_VIDEO_CALL_URL :
			size_t url_len = strlen("Start_Video_Call_URL:");
			snprintf((char *)video_call_url, sizeof(video_call_url), "%s", &ble_data[url_len]);
			PRINTF("[BLE] URL: %s\n", video_call_url);

			spi_command_atomic_exec_start_video_call(video_call_url);
			break;

		case BLE_CMD_ID_STOP_VIDEO_CALL :
			PRINTF("[BLE Parser] STOP_VIDEO_CALL\n");
			spi_command_atomic_exec_stop_video_call();
			break;

		case BLE_CMD_ID_LEAVE_VIDEO_CALL :
			PRINTF("[BLE Parser] LEAVE_VIDEO_CALL\n");
			/*NXP 恢復 Audio Path 設定 */
			spi_command_atomic_exec_stop_wifi_ap();
			break;

		case BLE_CMD_ID_STOP_VIDEOCHAT :
			PRINTF("[BLE Parser] STOP_VIDEOCHAT\n");

			break;

		case BLE_CMD_ID_STOP_TRANSLATION :
			PRINTF("[BLE Parser] STOP_TRANSLATION\n");

			break;

		case BLE_CMD_ID_TAKE_PHOTO :
			PRINTF("[BLE Parser] TAKE_PHOTO\n");

			break;

		case BLE_CMD_ID_START_RECORDING :
			PRINTF("[BLE Parser] START_RECORDING\n");

			break;

		case BLE_CMD_ID_STOP_RECORDING :
			PRINTF("[BLE Parser] STOP_RECORDING\n");

			break;

		case BLE_CMD_ID_START_FILE_SYNC :
			PRINTF("[BLE Parser] START_FILE_SYNC\n");

			break;

		case BLE_CMD_ID_NEW_MEDIA :
			PRINTF("[BLE Parser] NEW_MEDIA\n");

			break;

		case BLE_CMD_ID_RTSP_AUDIO_ONLY_ON :
			PRINTF("[BLE Parser] RTSP_AUDIO_ONLY_ON\n");

			break;

		case BLE_CMD_ID_UNKNOWN :
			PRINTF("[BLE Parser] Unknown BLE Command\n");
			break;
		default:
			break;
	}
}

