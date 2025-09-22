#ifndef _aw933xx_H
#define _aw933xx_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "aw_type.h"
#include "aw_cap_config.h"
#define DATA_MAX_LEN							(512)

#define AW933XX_LITTLE_ENDIAN_CONVERT			(0)

#define AW933XX_PTR_NULL_CHECK(ptr)				((ptr) == NULL)

#define AW933XX_CHIP_ID0					(0xA9331210)
#define AW933XX_CHIP_ID1					(0x00010000)

#define AW933XX_RAW_CH0						(0x250)
#define AW933XX_RAW_CH1						(0x400)
#define AW933XX_VALID_CH0					(0x22C)
#define AW933XX_BASELINE_CH0					(0x230)
#define AW933XX_DIFF_CH0					(0x234)

#define AW_REG_STEP						(AW933XX_RAW_CH1 - AW933XX_RAW_CH0)

#define AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE	(128)
#define AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE	(4)
#define AW933XX_CODERAM_START_ADDR				(0x2000)
#define AW933XX_CODERAM_END_ADDR				(0x4ff4 + 4)
#define AW933XX_CODERAM_DEFAULT_VAL				(0xffffffff)

#define AW933XX_CPU_OSC_CTRL_MASK				(1)
#define AW933XX_SLDX_STEP					(0x5C)

#define AW933XX_PRESS_STAT_IDX					(0)
#define AW933XX_PRESS_VALID_DAT					(0x0f)
#define AW933XX_CLICK_STAT_IDX					(4)
#define AW933XX_CLICK_VALID_DAT					(0x3f)
#define AW933XX_SLIDE_STAT_IDX					(10)
#define AW933XX_SLIDE_VALID_DAT					(0x3f)
#define AW933XX_BTN_POS_X_IDX					(16)
#define AW933XX_BTN_POS_Y_IDX					(24)
#define AW933XX_BTN_POS_MASK					(0xFF)

#define AW933XX_SLIDE_WEAR_STATE_MASK				(0x3)

#define AW933XX_EVENT_SLIDE_DIR_UP				(1)
#define AW933XX_EVENT_SLIDE_DIR_DOWN				(2)
#define AW933XX_EVENT_SLIDE_DIR_LEFT				(4)
#define AW933XX_EVENT_SLIDE_DIR_RIGHT				(8)

#define AW933XX_EVENT_PRESS					(1)
#define AW933XX_EVENT_PRESS_LONG				(3)
#define AW933XX_EVENT_PRESS_SUPER_LONG				(7)

#define AW933XX_STEP_LEN_UNSIGNED_CAP_ROUGH_ADJ			(9900)
#define AW933XX_STEP_LEN_UNSIGNED_CAP_FINE_ADJ			(152)
#define AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE			(10000)

#define AW933XX_DBG_DATA_MAX_LEN				(512)
#define AW933XX_REDUNDANCY_DATA_LEN				(10)
#define AW933XX_REDUNDANCY_DATA_LEN_V1				(11)

#define AW933XX_CHIP_CAP					(3)
#define AW_SAR_REG_LEN						(16) //reg len : 16bits
#define AW_SAR_DATA_LEN						(32) //data len : 32bits
#define AW_CURVE_DATA_LEN					(48) //curve len : 32bits
#define AW_COMM_CYCLE						(90)
#define PACKS_IN_ONE_COMM					(2)
#define SAR_REG_DATA_LEN					(4)
#define SAR_APP_CURVE_CNT					(4)
#define AW_SAR_STATUS_LEN					(12) /*single click,double click,trip click,long press,super press,lift,right,wear,up,down,backup1,backup2*/
#define VIRTUAL_REG_NUM						(10)
#define VIRTUAL_REG_BUF_LEN					(VIRTUAL_REG_NUM * SAR_REG_DATA_LEN) //10 word
#define AW933XX_REPORT_POS_DISTANCE_NUMS	2
#define AW_SAR_SPP_ONE_PACK_LEN				(SAR_APP_CURVE_CNT * AW_CURVE_DATA_LEN + VIRTUAL_REG_BUF_LEN + AW_SAR_STATUS_LEN +\
											(AW933XX_REPORT_POS_DISTANCE_NUMS * 2) * SAR_REG_DATA_LEN)

#define AW_SPP_REG_MAX_NUM					(10)
#define AW933XX_CURVER_LEN					(12)

#define AW933XX_OFFSET_MAX						(1000)
#define AW933XX_OFFSET_MIN						(100)
#define AW933XX_CAP_MIN							(-6.5f)
#define AW933XX_CAP_MAX							(6.5f)
#define AW933XX_DIFF_PROX_MIN					(0)
#define AW933XX_DIFF_PROX_MAX					(1000000)

#define AW933XX_GPIO_DIRIN_EN				(0xFFF)
#define AW933XX_GPIO_PU_EN					(0xFFF)
#define AW933XX_GPIO_PU_DIS					(0)
#define AW933XX_GPIO_PD_EN					(0xFFF)
#define AW933XX_GPIO_PD_DIS					(0)
#define AW933XX_BYTE_TO_INT(p)   (AW_U32)(((((AW_U32)(*p)) << AW_BIT24) | (((AW_U32)*(p - 1))) << AW_BIT16) | (((AW_U32)*(p - 2)) << AW_BIT8) | *(p - 3));


enum AW933XX_IRQSRC {
	REG_IRQSRC_INITOVERIRQ_BIT,
	REG_IRQSRC_CLOSEANYIRQ_BIT,
	REG_IRQSRC_FARANYIRQ_BIT,
	REG_IRQSRC_AOTDONEIRQ_BIT,
	REG_IRQSRC_WDTIRQ_BIT,
	REG_IRQSRC_SLD0IRQ_BIT,
	REG_IRQSRC_SLD1IRQ_BIT,
	REG_IRQSRC_SLD2IRQ_BIT,
	REG_IRQSRC_SLD3IRQ_BIT,
	REG_IRQSRC_SLD4IRQ_BIT,
	REG_IRQSRC_SLD5IRQ_BIT,
	REG_IRQSRC_SLD6IRQ_BIT,
	REG_IRQSRC_SLD7IRQ_BIT,
	REG_IRQSRC_SLD8IRQ_BIT,
	REG_IRQSRC_SLD9IRQ_BIT,
	REG_IRQSRC_SLD10IRQ_BIT,
	REG_IRQSRC_SLD11IRQ_BIT,
	REG_IRQSRC_WEARIRQ_BIT,
	REG_IRQSRC_TOUCHANYIRQ_BIT,
	REG_IRQSRC_EXITTOUCHANYIRQ_BIT,
};

enum aw933xx_chip_id {
	AW93303_CHIP_ID = 0xA9630340,
	AW93305_CHIP_ID = 0xA9630520,
	AW93307_CHIP_ID = 0xA9330710,
	AW93310_CHIP_ID = 0xA9331010,
	AW93312_CHIP_ID = 0xA9331210,
};

enum aw933xx_cs_2_irq {
	AW933XX_CS2_IRQ = 2,
	AW933XX_CS5_IRQ = 5,
};

enum AW933XX_ERROR_LIST {
	AW_OK = 0,
	AW_ERROR_NULL_PTR,
	AW_ERROR_MEMORY,
	AW_ERR,
	AW_ERROR_CODERAM_PARSE_ERROR,
	AW_ERROR_CODERAM_CHECK_ERROR,
	AW_ERROR_CODERAM_WRITE_LEN_ERROR,
	AW_ERROR_CODERAM_RD_FWVER_ERROR,
	AW_ERROR_DBG_FRAME_HEADER,
	AW_ERROR_DBG_FRAME_END,
	AW_ERROR_DBG_FRAME_SUM,
	AW_ERROR_UNKNOWN,
};

enum AW933XX_MODE_LIST {
	AW933XX_ACTIVE_MODE = 1,
	AW933XX_SLEEP_MODE,
	AW933XX_DEEPSLEEP_MODE,
};

enum AW_RAM_OPERATION_LIST {
	AW933XX_RAM_UPDATE = 1,
	AW933XX_RAM_CHECK
};

enum AW933XX_CAP_OFFSET {
	AW933XX_UNSIGNED_CAP = 0,
	AW933XX_SIGNED_CAP = 4,
	AW933XX_MUTUAL_CAP = 5,
};

enum AW933XX_EVENT_LIST {
	SINGLE_CLICK = 0,
	DOUBLE_CLICK,
	TRIPLE_CLICK,
	LONG_PRESS,
	SUPER_LONG_PRESS,
	LEFT_WAREDS,
	RIGHT_WAREDS,
	WEAR,
	UP_WAREDS,
	DOWN_WAREDS,
	BTN_POS_X,
	BTN_POS_Y,
};

enum AW933XX_DBG_DATA_PARSE {
	APK_VALID_DATA_HEADER = 3,
	APK_VALID_DATA_HEADER_V1 = 4,
	APK_HEADER = 0x3A,
	OFFSET = 0x82,
	NOISE,
	SIGNAL_RAW_0,
	SIGNAL_RAW_1,
	VERIFY_0,
	VERIFY_1,
	DYNAMIC_CALI = 0x8C,
	APK_END0 = 0x0D,
	APK_END1 = 0x0A,
};

enum AW933XX_DBG_CMD {
	AW933XX_DBG_CMD_GET_DEVICE_INFO = 0x00,
	AW933XX_DBG_CMD_GET_CURVE_DATA = 0x01,
	AW933XX_DBG_CMD_READ_REG = 0x02,
	AW933XX_DBG_CMD_WRITE_REG = 0x03,
	AW933XX_DBG_CMD_GET_ALGO_PARA = 0x04,
	AW933XX_DBG_CMD_SET_ALGO_PARA = 0x05,
	AW933XX_DBG_CMD_GET_INTN_DATA = 0x06,
	AW933XX_DBG_CMD_GET_INTN_EVENT = 0x07,
	AW933XX_DBG_CMD_STOP_GET_CURVE_DATA = 0x08,
	AW933XX_DBG_CMD_START_GET_CURVE_DATA = 0x09,
	AW933XX_DBG_CMD_OFFSET_CALI = 0xa0,
	AW933XX_DBG_CMD_DIFF_TO_AIR = 0xa1,
	AW933XX_DBG_CMD_DIFF_APPROACH = 0xa2,
	AW933XX_DBG_CMD_SHORT_CIRCUIT = 0xa3,
	AW933XX_DBG_CMD_SET_DIFF_TH = 0xa4,
};

#define AW_REG_DATA_LEN         (4)
#define AW_REG_ADDR_LEN         (2)
#define AW_TX_BUF_LEN           (AW_REG_DATA_LEN +  AW_REG_ADDR_LEN)



struct aw933xx_event {
	AW_U8 click;
	AW_U8 press;
	AW_U8 long_press;
	AW_U8 super_long_press;
	AW_U8 wear;
	AW_U8 right_wareds;
	AW_U8 left_wareds;
	AW_U8 up_wareds;
	AW_U8 down_wareds;
	AW_U8 in_ear;
	AW_U8 btn_pos_y;
	AW_U8 btn_pos_x;
};

struct aw933xx_dbg_dat {
	AW_U8 cmd;
	AW_U8 dat[AW933XX_DBG_DATA_MAX_LEN];
	AW_U8 len;
};

struct aw933xx_dev {
	AW_U8 current_mode;
	AW_U32 irq_reg_val;
	AW_U32 chip_id;
	AW_U8 last_status[AW_CHANNEL_NUM_MAX];
	const struct aw_func_t *aw_func;
	struct aw933xx_event event;
};

/**
 * @brief call this function to init driver
 *
 * @param aw_func the functions related to platform and need to inital
 * @return AW_S32
 */
AW_S32 aw933xx_init(const struct aw_func_t *aw_func);

/**
 * @brief set current mode
 *
 * @param mode need to set
 */
void aw933xx_mode_switch(AW_U8 mode);

/**
 * @brief software reset
 *
 * @return AW_S32 success(0) or not
 */
AW_S32 aw933xx_sw_rst(void);

/**
 * @brief read all channel's diff
 *
 * @param diff buffer to store diff
 * @return AW_S32 success(0) or not
 */
AW_S32 aw933xx_read_diff(AW_S32 *diff);

/**
 * @brief read all channel's base
 *
 * @param diff buffer to store diff
 * @return AW_S32 success(0) or not
 */
AW_S32 aw933xx_read_base(AW_S32 *base);

/**
 * @brief get the offset of each channel, the offset will show by log
 *
 */
void aw933xx_get_cap_offset(void);

/**
 * @brief auto calibrate all channel
 *
 */
void aw933xx_aot(void);

void aw933xx_irq_process_impl(void);

#ifdef __cplusplus
}
#endif

#endif /* _aw933xx_H */
