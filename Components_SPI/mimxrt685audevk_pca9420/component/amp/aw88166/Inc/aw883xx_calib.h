/*
* aw883xx_calib.h aw883xx codec driver
*
* Copyright (c) 2021 AWINIC Technology CO., LTD
*
* Author: <zhaolei@awinic.com>
*
* This program is free software; you can redistribute  it and/or modify it
* under  the terms of  the GNU General	Public License as published by the
* Free Software Foundation;  either version 2 of the  License, or (at your
* option) any later version.
*/

#ifndef __AWINIC_CALIBRATION_H__
#define __AWINIC_CALIBRATION_H__

#include <aw883xx_base.h>
#include "stdint.h"

#define AW_CALI_STORE_EXAMPLE
#define AW_ERRO_CALI_RE_VALUE (0)
#define AW_ERRO_CALI_F0_VALUE (2600)

#define AW_CALI_RE_DEFAULT_TIMER (3000)
#define MSGS_SIZE (512)
#define RESERVED_SIZE (252)


#define AW_CALI_ALL_DEV (0xFFFFFFFF)

#define AW_CALI_RE_MAX (15000)
#define AW_CALI_RE_MIN (4000)
#define AW_CALI_CFG_NUM (4)
#define AW_CALI_F0_DATA_NUM (4)
#define AW_CALI_READ_CNT_MAX (8)
#define AW_CALI_DATA_SUM_RM (2)
#define AW_DSP_RE_TO_SHOW_RE(re, shift) (((re) * (1000)) >> (shift))
#define AW_SHOW_RE_TO_DSP_RE(re, shift)  (((re) << shift) / (1000))
#define AW_CALI_F0_TIME (5 * 1000)
#define F0_READ_CNT_MAX (5)
#define AW_FS_CFG_MAX	(11)
#define AW_DEV_CH_MAX	(16)
#define AW_DEV_RE_RANGE	(RE_RANGE_NUM * AW_DEV_CH_MAX)
#define AW_TE_CACL_VALUE(te, coil_alpha) (int32_t)(((int32_t)te << 18) / (coil_alpha))
#define AW_RE_REALTIME_VALUE(re_cacl, te_cacl) ((re_cacl) + (int32_t)((int64_t)((te_cacl) * (re_cacl)) >> 14))

enum {
	CALI_CHECK_DISABLE = 0,
	CALI_CHECK_ENABLE = 1,
};

enum {
	CALI_RESULT_NONE = 0,
	CALI_RESULT_NORMAL = 1,
	CALI_RESULT_ERROR = -1,
};

enum {
	CALI_OPS_HMUTE = 0X0001,
	CALI_OPS_NOISE = 0X0002,
};

enum {
	CALI_TYPE_RE = 0,
	CALI_TYPE_F0,
};

enum {
	GET_RE_TYPE = 0,
	GET_F0_TYPE,
	GET_Q_TYPE,
};

enum {
	RE_MIN_FLAG = 0,
	RE_MAX_FLAG = 1,
	RE_RANGE_NUM = 2,
};

enum{
	AW_CALI_MODE_NONE = 0,
	AW_CALI_MODE_ALL,
	AW_CALI_MODE_MAX,
};

struct re_data {
	uint32_t re_range[2];
};

struct cali_cfg {
	uint32_t data[AW_CALI_CFG_NUM];
};

struct aw_cali_cfg_desc {
	unsigned int actampth_reg;
	unsigned char actampth_data_type;

	unsigned int noiseampth_reg;
	unsigned char noiseampth_data_type;

	unsigned int ustepn_reg;
	unsigned char ustepn_data_type;

	unsigned int alphan_reg;
	unsigned int alphan_data_type;
};

struct aw_noise_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int mask;
};

struct aw_f0_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int shift;
};

struct aw_q_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int shift;
};

struct aw_dsp_cali_re_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int shift;
};

struct aw_r0_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int shift;
	unsigned int init_value;
};

struct aw_ra_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int shift;
};

struct aw_hw_cali_re_desc {
	unsigned int hbits_reg;
	unsigned int lbits_reg;
	unsigned int hbits_mask;
	unsigned int hbits_shift;
	unsigned int lbits_mask;
	unsigned int lbits_shift;
	unsigned int cali_re_shift;
	uint16_t re_hbits;
	uint16_t re_lbits;
};

struct aw_spkr_temp_desc {
	unsigned int reg;
};

struct aw_cali_delay_desc {
	unsigned int dsp_reg;
	unsigned char data_type;
	unsigned int delay;
};

struct aw_cali_iv_desc {
	unsigned int reg;
	unsigned int reabs_mask;
};

struct aw_cali_desc {
	bool status;
	struct cali_cfg cali_cfg;
	uint16_t store_vol;
	uint32_t cali_re;	/*cali value*/
	uint32_t re;
	uint32_t f0;
	uint32_t q;
	uint32_t ra;
	int8_t cali_result;
	unsigned char cali_check_st;

	struct aw_cali_cfg_desc cali_cfg_desc;
	struct aw_ra_desc ra_desc;
	struct aw_noise_desc noise_desc;
	struct aw_f0_desc f0_desc;
	struct aw_q_desc q_desc;
	struct aw_dsp_cali_re_desc dsp_re_desc;
	struct aw_r0_desc r0_desc;
	struct aw_spkr_temp_desc spkr_temp_desc;
	struct aw_hw_cali_re_desc hw_cali_re_desc;
	struct aw_cali_delay_desc cali_delay_desc;
	struct aw_cali_iv_desc iv_desc;
};

bool aw883xx_cali_get_cali_status(struct aw_cali_desc *cali_desc);
bool aw883xx_cali_check_result(struct aw_cali_desc *cali_desc);

int aw883xx_set_cali_re(void *dev);
int aw883xx_cali_get_ra(void *dev);

int aw883xx_cail_re_cali(void *dev, aw_single_t is_single);
int aw883xx_cali_f0_q_cali(void *dev, aw_single_t is_single);
int aw883xx_cali_re_f0_cali(void *dev, aw_single_t is_single);

int aw883xx_cali_get_re_range(void *dev, aw_single_t is_single, uint32_t *range_buf);
int aw883xx_cali_get_r0(void *dev, aw_single_t is_single, uint32_t *r0_data);
int aw883xx_cali_get_f0(void *dev, aw_single_t is_single, uint32_t *f0);
int aw883xx_cali_get_te(void *dev, aw_single_t is_single, int32_t *te_data);

int aw883xx_cali_f0_show(void *dev, aw_single_t is_single, uint32_t *f0);
int aw883xx_cali_q_show(void *dev, aw_single_t is_single, uint32_t *q);
int aw883xx_cali_re_show(void *dev, aw_single_t is_single, uint32_t *re);
int aw883xx_cali_store_cali_re(void *dev, aw_single_t is_single, uint32_t *set_re);
int aw883xx_cali_read_re_from_dsp(void *dev, uint32_t *re);

int aw883xx_cali_store_cali_time(uint32_t time);
int aw883xx_cali_show_cali_time(uint32_t *time);

int aw883xx_cali_init_re_update(void *dev, backup_sec_t flag);

#endif


