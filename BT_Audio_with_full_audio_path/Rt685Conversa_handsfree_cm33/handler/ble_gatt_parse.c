/*
 * ble_gatt_parse.c
 *
 *  Created on: 2026年4月14日
 *      Author: 11301026
 */

#include "ble_gatt_parse.h"
#include "fsl_debug_console.h"
#include "fsl_crc.h"
#include <string.h>

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
int ble_packet_parse(const uint8_t *buf, uint16_t len) {
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
    	ble_message_parse(g_ctx.active_type, g_ctx.buffer, g_ctx.current_len);

        // 重置狀態迎接下一則訊息
        g_ctx.is_assembling = false;
        BLE_GATT_PARSE_DEBUG("Message process completed. Context reset.\n");
    }

    return 0;
}

void ble_message_parse(ble_msg_type_t type, const uint8_t *data, size_t len)
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
    }


}
