#ifndef __AW93312_PARAM_H_
#define __AW93312_PARAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "aw_type.h"
#include <stdint.h>
#include "stddef.h"
#include "aw_cap_config.h"
#define PER_GROUP_PARAM_NUM					(2 * 14)

#define PARAM_REG_BUF_MAX					(8 + PER_GROUP_PARAM_NUM * AW_CHANNEL_NUM_MAX)

#define AFE_CH_ADDR_INTERVAL					(0x01B0)
#define CAP_CTRL_BASE_ADDR					(0x1628)
#define CAP_CH_ADDR_INTERVAL					(0x005C)

// reg data default
#define DEFAULT_REG_LEAVE_TH_X					(0)
#define DEFAULT_REG_LEAVE_TH_Y					(0)

// reg field data default
#define DEFAULT_FIELD_TOUCH_DEB_TH				(3)
#define DEFAULT_FIELD_CHANNEL_NUM_X				(0)
#define DEFAULT_FIELD_CHANNEL_NUM_Y				(0)
#define DEFAULT_FIELD_MOVE_RECIP_TH				(3)
#define DEFAULT_FIELD_SPE_LINEAR_EN				(0)
#define DEFAULT_FIELD_CON_SLD_EN				(0)
#define DEFAULT_FIELD_CON_SLD_MODE				(0)

// reg addr
#define REG_SCAN_CTRL_ADDR					(0x0000)
#define REG_CHL_SEL0_ADDR					(0x0184)
#define REG_CHL_SEL1_ADDR					(0x0188)
#define REG_TOUCH_TH_X_ADDR					(0x0000)
#define REG_LEAVE_TH_X_ADDR					(0x0004)
#define REG_TOUCH_TH_Y_ADDR					(0x0008)
#define REG_LEAVE_TH_Y_ADDR					(0x000C)
#define REG_BTN_CFG0_ADDR					(0x0010)
#define REG_BTN_CFG1_ADDR					(0x0014)
#define REG_LONGPRESS_CFG0_ADDR					(0x0018)
#define REG_LONGPRESS_CFG1_ADDR					(0x001C)
#define REG_POS_CALCU_TH_X_ADDR					(0x0030)
#define REG_POS_CALCU_TH_Y_ADDR					(0x0034)
#define REG_SLD_CFG_ADDR					(0x0038)
#define REG_SLD_LIMIT_TH_ADDR					(0x0040)

// reg field bit position
/* SCANCTRL0 (0x0000) detail */
#define AFE_SCANPERIOD_POS					(16)
/* BUTTON_CFG0_SLD0 (0x0010) detail */
#define CAP_TOUCH_IRQ_EN_SLD0_POS				(28)
#define CAP_CONTROL_FUNC_SLD0_POS				(24)
#define CAP_TOUCH_DEB_TH_SLD0_POS				(16)
#define CAP_CHANNEL_NUM_Y_SLD0_POS				(12)
#define CAP_CHANNEL_NUM_X_SLD0_POS				(8)
#define CAP_START_INDEX_SLD0_POS				(0)
/* BUTTON_CFG1_SLD0 (0x0014) detail */
#define CAP_TOUCH_TIME_PERIOD_SLD0_POS			(16)
#define CAP_CLICK_INTERVAL_TIME_TH_SLD0_POS		(0)
/* LONGPRESS_CFG0_SLD0 (0x0018) detail */
#define CAP_LONG_LONG_PRESS_TH_SLD0_POS			(16)
#define CAP_LONG_PRESS_TH_SLD0_POS				(0)
/* default value of LONGPRESS_CFG0_SLD0 (0x0018) */
#define CAP_SUPER_LONG_PRESS_TH_SLD0_POS		(0)
/* SLD_CFG_SLD0 (0x0038) detail */
#define CAP_MOVE_RECIP_TH_SLD0_POS				(8)
#define CAP_SPE_LINEAR_EN_SLD0_POS				(7)
#define CAP_CONTINUE_SLD_EN_SLD0_POS			(6)
#define CAP_CONTINUE_SLD_MODE_SLD0_POS			(4)
#define CAP_SLD_TYPE_SLD0_POS					(0)
/* SLD_LIMIT_TH_SLD0 (0x0040) detail */
#define CAP_SLD_LIMIT_TH_Y_SLD0_POS				(16)
#define CAP_SLD_LIMIT_TH_X_SLD0_POS				(0)

/* default value of registers S2103 */
/* reg 0x0030 */
#define AFE_AFECFG0_CH0_DEFAULT					(0x09500000)


typedef enum {
	CH_DISEN = 0, // channel dis en
	CH_EN = 1, // channel en
	EN_MAX
} CHANNEL_EN;

typedef enum {
	R_OK = 0, // Return ok
	R_REPEAT = 1, // cs pin select repeat
	R_OVER_RANGE = 2, // cs pin select over range
	RETURN_MAX
} RESULT_R;

typedef enum {
	AW_CS0 = 1,
	AW_CS1,
	AW_CS2,
	AW_CS3,
	AW_CS4,
	AW_CS5,
	AW_CS6,
	AW_CS7,
	AW_CS8,
	AW_CS9,
	AW_CS10,
	AW_CS11,
	AW_CS_MAX
} Cs_t;

typedef enum {
	MS_10 = 10,
	MS_20 = 20,
	MS_30 = 30,
	MS_40 = 40,
	MS_50 = 50,
	MS_60 = 60,
	MS_70 = 70,
	MS_80 = 80,
	MS_90 = 90,
	MS_100 = 100,
	MS_MAX
} ScanPeriod_t;

typedef enum {
	NO_TYPE = 0,
	BUTTON,
	ONE_D_SLIDER,
	TWO_D_SLIDER,
	CIRCLE_SLIDER,
	WEAR_SLIDER,
	CV_FORCE,
	WIDGET_MAX
} Widget_t;

typedef enum {
	HZ = 0,
	MEASURED_INPUT,
	SHIELD,
	GND,
	CS_TYPE_MAX
} CS_SEL;

struct aw_widget_type_def {
	AW_U8 widget_type; // btn,1d,2d,cycle
	AW_U8 x_pad_cs_sel[AW_CHANNEL_NUM_MAX]; // cs0-cs11
	AW_U8 y_pad_cs_sel[AW_CHANNEL_NUM_MAX]; // cs0-cs11
	AW_U16 x_Mini_distance_TH; // 15:0 unit %
	AW_U16 y_Mini_distance_TH; // 31:16 unit %
	AW_U16 click_interval; // unit ms
	AW_U16 long_press_TH1; // 0-65535 unit %
	AW_U16 long_press_TH2; // 0-65535 unit %
	AW_U16 long_press_TH3; // 0-65535 unit %
	AW_U32 x_touch_TH; // 0~100%, unit %
	AW_U32 y_touch_TH; // 0~100%, unit %
	AW_U32 x_pos_calu_TH; // 31:0 unit %
	AW_U32 y_pos_calu_TH; // 31:0 unit %
};
typedef struct aw_widget_type_def AW_WIDGET_TYPE_DEF;


struct aw_param_init_cfg {
	AW_U32 scan_period; // unit ms
	AW_WIDGET_TYPE_DEF widget_cfg[AW_CHANNEL_NUM_MAX];
};
typedef struct aw_param_init_cfg AW_PARRAM_INIT_CFG;

struct aw_cs_ch_cfg {
	AW_U8 ch_en;
	AW_U8 ch_num;
	AW_U8 ch_type;
	AW_U8 cs_num;
	AW_U8 cs_pin[AW_CHANNEL_NUM_MAX];
};
typedef struct aw_cs_ch_cfg AW_CS_CH_CFG;

struct aw_start_ch_cfg {
	AW_U8 slider_en;
	AW_U8 slider_num;
	AW_U8 cs_num_x; //  Slide several channels were used
	AW_U8 cs_num_y; //  Slide several channels were used
	AW_U8 start_ch;
};
typedef struct aw_start_ch_cfg AW_START_CH_CFG;


struct aw_cs_ch_struct {
	AW_CS_CH_CFG cs_select[AW_CHANNEL_NUM_MAX];
};
typedef struct aw_cs_ch_struct AW_CS_CH_S;


struct aw_slider_ch_struct {
	AW_START_CH_CFG slider_cfg[AW_CHANNEL_NUM_MAX];
};
typedef struct aw_slider_ch_struct AW_SLIDER_CH_S;


// 0000h
struct reg_scan_ctrl0 {
	AW_U8 field_chen_ch[AW_CHANNEL_NUM_MAX];
	AW_U16 field_scan_period;
};
typedef struct reg_scan_ctrl0 REG_SCAN_CTRL;

// 0030h
struct reg_afe_cfg0 {
	AW_U8 field_cs_sel[AW_CHANNEL_NUM_MAX];
};
typedef struct reg_afe_cfg0 REG_AFE_CFG0;


// 0000h
struct reg_touth_th_x {
	AW_U32 field_touch_th_x; // 0~100%, unit %
};
typedef struct reg_touth_th_x REG_TOUCH_TH_X;

// 0004h
struct reg_leave_th_x {
	AW_U32 field_leave_th_x; // 0~100%, unit %
};
typedef struct reg_leave_th_x REG_LEAVE_TH_X;

// 0008h
struct reg_touth_th_y {
	AW_U32 field_touch_th_y; // 0~100%, unit %
};
typedef struct reg_touth_th_y REG_TOUCH_TH_Y;
// 000ch
struct reg_leave_th_y {
	AW_U32 field_leave_th_y; // 0~100%, unit %
};
typedef struct reg_leave_th_y REG_LEAVE_TH_Y;

// 0010h
struct reg_button_cfg0 {
	AW_U8 field_irq_en;
	AW_U8 field_ctl_func;
	AW_U8 field_touch_deb_th;
	AW_U8 field_ch_num_y;
	AW_U8 field_ch_num_x;
	AW_U8 field_start_index;
};
typedef struct reg_button_cfg0 REG_BTN_CFG0;

// 0014h
struct reg_button_cfg1 {
	AW_U16 field_touch_time_period;
	AW_U16 field_click_interval;
};
typedef struct reg_button_cfg1 REG_BTN_CFG1;

// 0018h
struct reg_longpresss_cfg0 {
	AW_U16 field_longpress_th1;
	AW_U16 field_longpress_th2;
};
typedef struct reg_longpresss_cfg0 REG_LONGPRESS_CFG0;

// 001ch
struct reg_longpresss_cfg1 {
	AW_U16 field_longpress_th3;
};
typedef struct reg_longpresss_cfg1 REG_LONGPRESS_CFG1;

// 0030h
struct reg_pos_cal_x {
	AW_U32 field_pos_cal_th_x; // 0~100%, unit %
};
typedef struct reg_pos_cal_x REG_POS_CAL_X;

// 0034h
struct reg_pos_cal_y {
	AW_U32 field_pos_cal_th_y; // 0~100%, unit %
};
typedef struct reg_pos_cal_y REG_POS_CAL_Y;

// 0038h
struct reg_sld_cfg {
	AW_U8 field_move_recip_th;
	AW_U8 field_spe_linear_en;
	AW_U8 field_con_sld_en;
	AW_U8 field_con_sld_mode;
	AW_U8 field_sld_type;
};
typedef struct reg_sld_cfg REG_SLD_CFG;

// 0040h
struct reg_sld_limit_th {
	AW_U16 field_sld_limit_th_x;
	AW_U16 field_sld_limit_th_y;
};
typedef struct reg_sld_limit_th REG_SLD_LIMIT_TH;

// cap reg
struct reg_cfg_struct {
	REG_SCAN_CTRL reg_scan_ctrl0;
	REG_SCAN_CTRL reg_scan_ctrl1;
	REG_SCAN_CTRL reg_scan_ctrl2;
	REG_SCAN_CTRL reg_scan_ctrl3;
	REG_AFE_CFG0 reg_chl_sel0[AW_CHANNEL_NUM_MAX];
	REG_AFE_CFG0 reg_chl_sel1[AW_CHANNEL_NUM_MAX];
	REG_TOUCH_TH_X reg_touch_th_x[AW_CHANNEL_NUM_MAX];
	REG_LEAVE_TH_X reg_leave_th_x[AW_CHANNEL_NUM_MAX];
	REG_TOUCH_TH_Y reg_touch_th_y[AW_CHANNEL_NUM_MAX];
	REG_LEAVE_TH_Y reg_leave_th_y[AW_CHANNEL_NUM_MAX];
	REG_BTN_CFG0 reg_button_cfg0[AW_CHANNEL_NUM_MAX];
	REG_BTN_CFG1 reg_button_cfg1[AW_CHANNEL_NUM_MAX];
	REG_LONGPRESS_CFG0 reg_longpresss_cfg0[AW_CHANNEL_NUM_MAX];
	REG_LONGPRESS_CFG1 reg_longpresss_cfg1[AW_CHANNEL_NUM_MAX];
	REG_POS_CAL_X reg_pos_cal_x[AW_CHANNEL_NUM_MAX];
	REG_POS_CAL_Y reg_pos_cal_y[AW_CHANNEL_NUM_MAX];
	REG_SLD_CFG reg_sld_cfg[AW_CHANNEL_NUM_MAX];
	REG_SLD_LIMIT_TH reg_sld_limit_th[AW_CHANNEL_NUM_MAX];
};
typedef struct reg_cfg_struct REG_CFG_STRUCT;


// cap reg addr
struct reg_cfg_addr_struct {
	AW_U16 addr_scan_ctrl0;
	AW_U32 addr_scan_ctrl1;
	AW_U32 addr_scan_ctrl2;
	AW_U32 addr_scan_ctrl3;
	AW_U16 addr_chl_sel0[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_chl_sel1[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_touch_th_x[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_leave_th_x[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_touch_th_y[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_leave_th_y[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_button_cfg0[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_button_cfg1[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_longpresss_cfg0[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_longpresss_cfg1[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_pos_cal_x[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_pos_cal_y[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_sld_cfg[AW_CHANNEL_NUM_MAX];
	AW_U16 addr_sld_limit_th[AW_CHANNEL_NUM_MAX];
};
typedef struct reg_cfg_addr_struct REG_CFG_ADDR_S;

// cap reg data
struct reg_cfg_data_struct {
	AW_U32 data_scan_ctrl0;
	AW_U32 data_scan_ctrl1;
	AW_U32 data_scan_ctrl2;
	AW_U32 data_scan_ctrl3;
	AW_U32 data_chl_sel0[AW_CHANNEL_NUM_MAX];
	AW_U32 data_chl_sel1[AW_CHANNEL_NUM_MAX];
	AW_U32 data_touch_th_x[AW_CHANNEL_NUM_MAX];
	AW_U32 data_leave_th_x[AW_CHANNEL_NUM_MAX];
	AW_U32 data_touch_th_y[AW_CHANNEL_NUM_MAX];
	AW_U32 data_leave_th_y[AW_CHANNEL_NUM_MAX];
	AW_U32 data_button_cfg0[AW_CHANNEL_NUM_MAX];
	AW_U32 data_button_cfg1[AW_CHANNEL_NUM_MAX];
	AW_U32 data_longpresss_cfg0[AW_CHANNEL_NUM_MAX];
	AW_U32 data_longpresss_cfg1[AW_CHANNEL_NUM_MAX];
	AW_U32 data_pos_cal_x[AW_CHANNEL_NUM_MAX];
	AW_U32 data_pos_cal_y[AW_CHANNEL_NUM_MAX];
	AW_U32 data_sld_cfg[AW_CHANNEL_NUM_MAX];
	AW_U32 data_sld_limit_th[AW_CHANNEL_NUM_MAX];
};
typedef struct reg_cfg_data_struct REG_CFG_DATA_S;

extern AW_U32 aw933xx_reg_data[PARAM_REG_BUF_MAX];

AW_CS_CH_S *aw_get_cs_ch_func(void);
AW_SLIDER_CH_S *aw_get_slider_ch_func(void);
void aw_widget_cfg(void);

#ifdef __cplusplus
}
#endif
#endif
