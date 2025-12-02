/*
 * system_status.h
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#ifndef SYSTEM_STATUS_H_
#define SYSTEM_STATUS_H_

#include <stdint.h>
#include <stdbool.h>

#define BATTERY_FULL_VOLTAGE   4450//(mV)
#define BATTERY_EMPTY_VOLTAGE  3300//(mV)

#define COMPONENT_OFF          0
#define COMPONENT_ON           1
#define COMPONENT_START        2
#define COMPONENT_END          3



typedef enum {
    BATTERY_STATE_NORMAL = 0,       // 一般狀態
    BATTERY_STATE_LOW,              // 電量過低
    BATTERY_STATE_FULL              // 已充飽
} battery_state_t;

typedef enum {
	USAGE_STATE_HOME = 0,          // 使用 Home
	USAGE_STATE_MENU,              // 使用 Menu
	USAGE_STATE_ABOUT,             // 使用 About
	USAGE_STATE_MUSIC_PLAYER,      // 使用 Music Player
	USAGE_STATE_MEDIA_PLAYER,      // 使用 Media Player
	USAGE_STATE_VIDEO_RECORDING,   // 使用 Video Recording
	USAGE_STATE_TAKE_PHOTO,        // 使用拍照
	USAGE_STATE_VIDEO_AI,          // 使用 Video AI
	USAGE_STATE_TRANSLATION        // 使用翻譯
} usage_state_t;

typedef struct __attribute__((packed)) {
    uint8_t flags;   // byte0: [RSVD3:3][RSVD1:1][MIC:1][BT:1][HA:1][BLE:1]
    uint8_t layer;   // byte1: [Layer:8]
    uint8_t batt;    // byte2: [Charger:1][BatteryLevel:7]
} SystemStatus;

enum {
    // flags byte (byte0)
    SS_BLE_BIT   = 1u << 0, // bit0
    SS_HA_BIT    = 1u << 1, // bit1
    SS_BT_BIT    = 1u << 2, // bit2
    SS_MIC_BIT   = 1u << 3, // bit3
    SS_RSVD1_BIT = 1u << 4, // bit4
    SS_RSVD3_MASK= 0xE0u,   // bits7..5

    // batt byte (byte2)
    SS_CHARGER_BIT   = 1u << 7, // bit7
    SS_LEVEL_SHIFT 	 = 0u,      // BatteryLevel 在 bits6..0，不需要位移
    SS_LEVEL_MASK    = 0x7Fu    // 低 7 位（bits6..0）

};

void ss_set_bt_addr_0 (uint8_t addr);
void ss_set_bt_addr_1 (uint8_t addr);
void ss_set_bt_addr_2 (uint8_t addr);
void ss_set_bt_addr_3 (uint8_t addr);
void ss_set_bt_addr_4 (uint8_t addr);
void ss_set_bt_addr_5 (uint8_t addr);
void ss_print_bt_addr (void);

/* ====== BLE/HA/BT/MIC：開關與讀取 ====== */
void ss_ble_on();
void ss_ble_off();
bool ss_ble_is_on();

void ss_ha_on();
void ss_ha_off();
bool ss_ha_is_on();

void ss_bt_on();
void ss_bt_off();
bool ss_bt_is_on();

void ss_mic_on();
void ss_mic_off();
bool ss_mic_is_on();


/* ====== Layer：設定與讀取 ====== */
void     ss_set_layer(uint8_t layer);
uint8_t  ss_get_layer();

/* ====== 充電與電量：設定與讀取 ====== */
void ss_set_charging(bool on);
bool ss_is_charging();
void ss_set_battery(uint8_t percent);
uint8_t ss_get_battery();

uint8_t ss_get_state(void);
void ss_set_state(uint8_t state);//Change the usage state and send a notification back to Novatek.
void send_state_to_soc(void);
void ss_set_music_status(uint8_t status);
void send_music_status_to_soc(void);

/*

SystemStatus ss = {0};

// 開 BLE/BT，關 HA/MIC
ss_ble_on(&ss);
ss_bt_on(&ss);
ss_ha_off(&ss);
ss_mic_off(&ss);

// Layer 設 3
ss_set_layer(&ss, 3);

// 充電中、電量 87%
ss_set_charging(&ss, true);
ss_set_battery(&ss, 87);

// 讀取
bool ble   = ss_ble_is_on(&ss);
bool bt    = ss_bt_is_on(&ss);
bool mic   = ss_mic_is_on(&ss);
bool ha    = ss_ha_is_on(&ss);
bool chg   = ss_is_charging(&ss);
uint8_t lv = ss_get_battery(&ss);  // 87

PRINTF("[Sysetm status] 0x%02X 0x%02X 0x%02X \n ",ss.flags,ss.layer,ss.batt);
 */

#endif /* SYSTEM_STATUS_H_ */
