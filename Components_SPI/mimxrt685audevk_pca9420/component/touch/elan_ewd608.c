/*
 * elan_ewd608.c
 *
 *  Created on: 2025年8月26日
 *      Author: 11301026
 */


#include "elan_ewd608.h"
#include "fsl_debug_console.h"
#include "fsl_i3c.h"
#include "board.h"

/* ==== 靜態/全域 ==== */
static const elan_hal_t *g_hal = NULL;
static elan_event_cb_t   g_event_callback = NULL;

#if 1
static int hal_i2c_write_impl(uint16_t addr, const uint8_t *buf, uint16_t len) {
    status_t status = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR, (uint8_t)addr, 0, 0, (uint8_t *)buf, (uint8_t)len);
    return (status == kStatus_Success) ? 0 : -1;
}

static int hal_i2c_read_impl(uint16_t addr, uint8_t *buf, uint16_t len) {
    status_t status = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR, (uint8_t)addr, 0, 0, buf, (uint8_t)len);
    return (status == kStatus_Success) ? 0 : -1;
}

/* 記憶體讀（Repeated-START） */
int hal_i2c_mem_read_impl(uint16_t addr8, uint8_t reg, uint8_t *buf, uint16_t len) {
    status_t status = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR, (uint8_t)addr8, reg, 1, buf, (uint8_t)len);
    return (status == kStatus_Success) ? 0 : -1;
}
#endif

static const char* elan_gesture_to_string(uint8_t gesture_id)
{
    switch (gesture_id) {
        case GEST_LEFT:         return "Swipe Left";
        case GEST_RIGHT:        return "Swipe Right";
        case GEST_PRESS:        return "Press";
        case GEST_DOUBLE_CLICK: return "Double Click";
        case GEST_CLICK:        return "Click";
        case GEST_TRIPLE_CLICK: return "Triple Click";
        default:                return "Unknown Gesture";
    }
}


static void print_elan_touch_data(const elan_touch_data_t *data)
{
    PRINTF("Touch Count: %u\n", data->touch_count);
    for (int i = 0; i < data->touch_count && i < MAX_SUPPORT_FINGERS; ++i) {
        const elan_touch_point_t *pt = &data->points[i];
        if (pt->is_valid) {
            PRINTF("  Finger %d: X = %u, Y = %u\n", i, pt->x, pt->y);
        } else {
            PRINTF("  Finger %d: Invalid\n", i);
        }
    }

    if (data->gesture_id != 0) {
        PRINTF("Gesture: %s (0x%02X)\n", elan_gesture_to_string(data->gesture_id), data->gesture_id);
    } else {
        PRINTF("No Gesture\n");
    }
    PRINTF("\r\n");
}


int  elan_i2c_write_cmd(uint8_t reg, uint8_t val)
{
	status_t ret;

    ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR,
    					EKTF_I2C_ADDR_7BIT,
                         reg,
                         1,
                         &val,
                         1);
    PRINTF("[Debug][tp_i2c_write] BOARD_I3C_Send ret:%d \r\n",ret);

    return (int)ret;
}

int  elan_i2c_read_reg (uint8_t reg, uint8_t *buf, uint8_t len)
{

	status_t ret;

    ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR,
    						EKTF_I2C_ADDR_7BIT,
                            reg,               /* subAddress */
                            1,                 /* subaddressSize = 1 byte */
                            buf,
                            len);
    PRINTF("[Debug][tp_i2c_read] BOARD_I3C_Receive ret:%d \r\n",ret);

	return (int)ret;
}

int elan_touch_get_fw_version(uint16_t *version)
{

    if (!version) return -2;
    uint8_t buf_recv[2] = {0};
    int rc = elan_i2c_read_reg(0x08, buf_recv, sizeof(buf_recv));
    if (rc != 0) return -1;
    *version = BYTES_TO_U16(buf_recv[0], buf_recv[1]);
    return 0;
}

void elan_parse_and_report_data(uint8_t *buf, uint16_t len)
{

    //if (!g_event_callback || !buf) return;

    elan_touch_data_t report_data;
    memset(&report_data, 0, sizeof(report_data));

    if (len < 1) return;

    if (buf[0] & 0x10) {
        if (len > 9) {
            report_data.gesture_id = buf[9];
        }
    } else if (buf[0] & 0x80) {
        if (len >= 5) {
            uint8_t finger_state = buf[0] & 0x03;
            if ((finger_state & 0x01) && len >= 5) {
                report_data.points[0].is_valid = true;
                report_data.points[0].x = BYTES_TO_U16(buf[2], buf[1]);
                report_data.points[0].y = BYTES_TO_U16(buf[4], buf[3]);
                report_data.touch_count++;
            }
            if ((finger_state & 0x02) && len >= 9) {
                report_data.points[1].is_valid = true;
                report_data.points[1].x = BYTES_TO_U16(buf[6], buf[5]);
                report_data.points[1].y = BYTES_TO_U16(buf[8], buf[7]);
                report_data.touch_count++;
            }
        }
    }
    print_elan_touch_data(&report_data);
    //g_event_callback(&report_data);
}

