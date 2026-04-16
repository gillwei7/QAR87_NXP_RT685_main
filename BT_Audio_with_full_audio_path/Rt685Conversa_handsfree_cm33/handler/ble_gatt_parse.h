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

// 解析結果的回撥函式原型
typedef void (*ble_msg_callback_t)(ble_msg_type_t type, const uint8_t *data, size_t len);

// 解析函式
int ble_parse_handle_packet(const uint8_t *buf, uint16_t len, ble_msg_callback_t callback);
void ble_app_handler(ble_msg_type_t type, const uint8_t *data, size_t len);


#endif /* BLE_GATT_PARSE_H_ */
