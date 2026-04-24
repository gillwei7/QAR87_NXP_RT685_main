/*
 * ble_gatt_parse.h
 *
 *  Created on: 2026年4月14日
 *      Author: 11301026
 */

#ifndef BLE_GATT_PARSE_H_
#define BLE_GATT_PARSE_H_

#include <stdint.h>
#include <stddef.h>

#define BLE_LOG_ERROR_ENABLE  1
#define BLE_LOG_WARN_ENABLE   1
#define BLE_LOG_DEBUG_ENABLE  0  // 預設關閉 Debug，只看 Error 和 Warn

// 巨集定義
#if BLE_LOG_ERROR_ENABLE
    #define BLE_GATT_PARSE_ERROR(...) do { PRINTF("[BLE_PARSE][Error]  "); PRINTF(__VA_ARGS__); } while(0)
#else
    #define BLE_GATT_PARSE_ERROR(...)
#endif

#if BLE_LOG_WARN_ENABLE
    #define BLE_GATT_PARSE_WARN(...)  do { PRINTF("[BLE_PARSE][Warning] "); PRINTF(__VA_ARGS__); } while(0)
#else
    #define BLE_GATT_PARSE_WARN(...)
#endif

#if BLE_LOG_DEBUG_ENABLE
    #define BLE_GATT_PARSE_DEBUG(...) do { PRINTF("[BLE_PARSE][Debug] "); PRINTF(__VA_ARGS__); } while(0)
#else
    #define BLE_GATT_PARSE_DEBUG(...)
#endif


// 協定常數
#define BLE_PROTO_VERSION    0x01
#define BLE_PROTO_HDR_SIZE   8
#define BLE_PROTO_EXTRA_HDR  8
#define MAX_MSG_BUFFER_SIZE  4096 // 根據眼鏡端記憶體調整

// 訊息類型
typedef enum {
    BLE_MSG_TYPE_TEXT        = 0x01,
    BLE_MSG_TYPE_AI_RESPONSE = 0x10,
    BLE_MSG_TYPE_TRANSLATION = 0x11,
    BLE_MSG_TYPE_COMMAND     = 0x20,
    BLE_MSG_TYPE_ACK         = 0x30,
    BLE_MSG_TYPE_HEARTBEAT   = 0x40
} ble_msg_type_t;

typedef enum {
	BLE_CMD_ID_START_AP,
	BLE_CMD_ID_START_AP_IP,
	BLE_CMD_ID_WIFI_CONNECTED,
	BLE_CMD_ID_RTSP_AV,

	BLE_CMD_ID_START_VIDEO_CALL_URL,
	BLE_CMD_ID_STOP_VIDEO_CALL,

	BLE_CMD_ID_ENTER_VIDEO_CALL,
	BLE_CMD_ID_LEAVE_VIDEO_CALL,

	BLE_CMD_ID_STOP_VIDEOCHAT,
	BLE_CMD_ID_STOP_TRANSLATION,

	BLE_CMD_ID_TAKE_PHOTO,
	BLE_CMD_ID_START_RECORDING,
	BLE_CMD_ID_STOP_RECORDING,

	BLE_CMD_ID_START_FILE_SYNC,
	BLE_CMD_ID_NEW_MEDIA,

	BLE_CMD_ID_RTSP_AUDIO_ONLY_ON,

	BLE_CMD_ID_UNKNOWN = 0xFF,
} ble_cmd_id_t;


// 解析函式
int ble_packet_parser (const uint8_t *buf, uint16_t len);
void ble_message_parser (ble_msg_type_t type, const uint8_t *data, size_t len);


#endif /* BLE_GATT_PARSE_H_ */
