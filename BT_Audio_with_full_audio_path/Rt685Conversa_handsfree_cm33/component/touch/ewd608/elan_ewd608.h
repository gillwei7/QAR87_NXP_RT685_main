/*
 * elan_ewd608.h
 *
 *  Created on: 2025年8月26日
 *      Author: 11301026
 */

#ifndef TOUCH_ELAN_EWD608_H_
#define TOUCH_ELAN_EWD608_H_

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define EKTF_I2C_ADDR_7BIT   (0x50U)

/* 設定最大支援點數與封包長度 (非必要勿改動)*/
#define MAX_SUPPORT_FINGERS    2
#define EWD_FRAME_MAX_LEN      10   /* 例：10 bytes；若協定更新請同步 .c 內讀取長度策略 */

/* ==== 常用巨集 ==== (非必要勿改動)*/
#define BYTES_TO_U16(h,l)      ( ((uint16_t)(h) << 8) | (uint16_t)(l) )
#define PWR_STATE_MASK         0x03   // 範例：低 2 bits 控制電源狀態

/* ==== 型別定義 ==== */
/* 座標 */
typedef struct {
    bool     is_valid;
    uint16_t x;
    uint16_t y;
} elan_touch_point_t;

/* 手勢 */
typedef struct {
    uint8_t            touch_count;                          /* 有效觸控點數 */
    elan_touch_point_t points[MAX_SUPPORT_FINGERS];
    uint8_t            gesture_id;                           /* 0 表無 */
} elan_touch_data_t;

/* 手勢定義 */
typedef enum {
    GEST_LEFT         = 0x01,
    GEST_RIGHT        = 0x02,
    GEST_PRESS        = 0x1A,
    GEST_DOUBLE_CLICK = 0x1B,
    GEST_CLICK        = 0x1C,
    GEST_TRIPLE_CLICK = 0x1D,
} elan_touch_gesture_t;


typedef enum
{
    GESTURE_SINGLE_FORWARD   = 0x01, // 單指前滑
    GESTURE_SINGLE_BACKWARD  = 0x02, // 單指後滑
    GESTURE_SINGLE_PRESS     = 0x1A, // 單指按壓
    GESTURE_SINGLE_TAP       = 0x1C, // 單指單擊
    GESTURE_SINGLE_DOUBLE    = 0x1B, // 單指雙擊
    GESTURE_SINGLE_TRIPLE    = 0x1D, // 單指三擊

    GESTURE_TWO_TAP          = 0x21, // 雙指單擊
    GESTURE_TWO_FORWARD      = 0x24, // 雙指前滑
    GESTURE_TWO_BACKWARD     = 0x25, // 雙指後滑
    GESTURE_TWO_PRESS        = 0x26, // 雙指按壓
} elan_touch_gesture_type_t;


/* 事件回傳：把解析好的資料交給上層 */
typedef void (*elan_event_cb_t)(const elan_touch_data_t *data);

/* HAL 抽象層 */
typedef struct {
    int  (*i2c_write)(uint16_t addr, const uint8_t *buf, uint16_t len);
    int  (*i2c_read) (uint16_t addr, uint8_t *buf, uint16_t len);
    void (*delay_ms) (uint32_t ms);
} elan_hal_t;

/* ==== 實例API ==== */
/* I2C 寫入 */
int  elan_i2c_write_cmd(uint8_t reg, uint8_t val);
/* I2C 讀取 */
int  elan_i2c_read_reg (uint8_t reg, uint8_t *buf, uint8_t len);
/* 軟體重置 */
void elan_sw_reset(void);
/* MCU 模式設定 */
int  elan_set_power_state(int state);
/* 獲取FW checksum */
int  elan_touch_get_fw_version(uint16_t *version);
/* 解析手勢及座標 */
void elan_parse_and_report_data(uint8_t *buf, uint16_t len);
/* 韌體更新 */
int  elan_touch_update_firmware(const uint8_t *fw_data, uint32_t fw_size);

int hal_i2c_mem_read_impl(uint16_t addr8, uint8_t reg, uint8_t *buf, uint16_t len);

#endif /* TOUCH_ELAN_EWD608_H_ */
