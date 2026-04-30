/*
* aw883xx_calib.c aw883xx codec driver
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

#include <aw883xx.h>
#include <aw883xx_base.h>
#include <aw883xx_calib.h>
#include <aw883xx_device.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef AW_CALIB
static unsigned int g_cali_re_time = AW_CALI_RE_DEFAULT_TIMER;

bool aw883xx_cali_check_result(struct aw_cali_desc *cali_desc)
{
	if (cali_desc->cali_check_st &&
		(cali_desc->cali_result == CALI_RESULT_ERROR)) {
		return false;
	} else {
		return true;
	}
}

static int aw_cali_svc_get_dev_re_range(struct aw_device *aw_dev,
						uint32_t *data_buf)
{
	data_buf[RE_MIN_FLAG] = aw_dev->re_range.re_min;
	data_buf[RE_MAX_FLAG] = aw_dev->re_range.re_max;

	return 0;
}

static int aw_cali_svc_get_devs_re_range(struct aw_device *aw_dev,
						uint32_t *data_buf, int num)
{
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;
	uint8_t cnt = 0;
	uint8_t i;
	int dev_cnt = aw883xx_get_dev_num();

	for (i = 0; i < dev_cnt; i++) {
		aw883xx = aw883xx_get_dev(i);
		local_dev = aw883xx->aw_pa;
		if (local_dev->dev < num) {
			data_buf[RE_MIN_FLAG + local_dev->dev * 2] =
				local_dev->re_range.re_min;
			data_buf[RE_MAX_FLAG + local_dev->dev * 2] =
				local_dev->re_range.re_max;
			cnt++;
		} else {
			aw_dev_err(local_dev->dev, "channel num[%d] overflow buf num[%d]",
						local_dev->dev, num);
			return -EINVAL;
		}
	}

	return cnt;
}

static int aw_cali_store_cali_re(struct aw_device *aw_dev, uint32_t re)
{
	if ((re > aw_dev->re_range.re_min) && (re < aw_dev->re_range.re_max)) {
		aw_dev->cali_desc.cali_re = re;
	} else {
		aw_dev_err(aw_dev->dev, "invalid cali re %d!", re);
		return -EINVAL;
	}

	return 0;
}

static int aw_cali_devs_store_cali_re(struct aw_device *aw_dev, uint32_t *set_re)
{
	struct aw_device *local_dev = NULL;
	struct aw883xx *aw883xx = NULL;
	int ret = -1;
	uint8_t cnt;
	int dev_num = 0;
	dev_num = aw883xx_get_dev_num();

	for (cnt = 0; cnt < dev_num; cnt++) {
		aw883xx = aw883xx_get_dev(cnt);
		local_dev = aw883xx->aw_pa;
		if (local_dev->dev < AW_DEV_CH_MAX) {
			ret = aw_cali_store_cali_re(local_dev, set_re[local_dev->dev]);
			if (ret < 0) {
				aw_dev_err(local_dev->dev, "store cali re failed");
				return ret;
			}
		}
	}

	return 0;
}

static int aw_cali_svc_get_cali_re_from_hw(struct aw_device *aw_dev, uint32_t *re)
{
	int ret = -1;
	uint32_t cali_re = 0;
	uint16_t re_hbit = 0;
	uint16_t re_lbit = 0;
	struct aw_hw_cali_re_desc *hw_cali_re_desc = &aw_dev->cali_desc.hw_cali_re_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, hw_cali_re_desc->hbits_reg, &re_hbit);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read re_hbit failed");
		return ret;
	}

	ret = aw_dev->ops.aw_reg_read(aw_dev, hw_cali_re_desc->lbits_reg, &re_lbit);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read re_hbit failed");
		return ret;
	}

	cali_re = (re_hbit << hw_cali_re_desc->hbits_shift) | re_lbit;

	*re = AW_DSP_RE_TO_SHOW_RE(cali_re, hw_cali_re_desc->cali_re_shift) - aw_dev->cali_desc.ra;

	return 0;
}

static int aw_cali_svc_read_cali_re_from_pa(struct aw_device *aw_dev, uint32_t *re)
{
	int ret = -1;

	if (aw_dev->ops.aw_get_cali_re) {
		ret = aw_dev->ops.aw_get_cali_re(aw_dev, re);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read cali re error");
			return ret;
		}
	} else {
		ret = aw_cali_svc_get_cali_re_from_hw(aw_dev, re);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read cali re error");
			return ret;
		}
	}

	aw_dev_info(aw_dev->dev, "get cali re:%d", *re);

	return 0;
}

/*************Calibration base function************/

static int aw_cali_set_cali_re_to_hw(struct aw_device *aw_dev)
{
	int ret;
	uint32_t cali_re = 0;
	uint16_t re_hbits = 0;
	uint16_t re_lbits = 0;

	struct aw_hw_cali_re_desc *hw_cali_re_desc = &aw_dev->cali_desc.hw_cali_re_desc;

	cali_re = AW_SHOW_RE_TO_DSP_RE((aw_dev->cali_desc.cali_re +
		aw_dev->cali_desc.ra), hw_cali_re_desc->cali_re_shift);

	re_hbits = (cali_re & (~hw_cali_re_desc->hbits_mask)) >> hw_cali_re_desc->hbits_shift;

	re_lbits = (cali_re & (~hw_cali_re_desc->lbits_mask)) >> hw_cali_re_desc->lbits_shift;

	ret = aw_dev->ops.aw_reg_write(aw_dev, hw_cali_re_desc->hbits_reg, re_hbits);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write cali re failed");
		return ret;
	}

	ret = aw_dev->ops.aw_reg_write(aw_dev, hw_cali_re_desc->lbits_reg, re_lbits);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write cali re failed");
		return ret;
	}

	return 0;
}

int aw883xx_set_cali_re(void *dev)
{
	int ret = -1;
	struct aw_device *aw_dev = dev;


	if (aw_dev->ops.aw_set_cali_re) {
		ret = aw_dev->ops.aw_set_cali_re(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "set cali re failed");
			return ret;
		}
	} else {
		ret = aw_cali_set_cali_re_to_hw(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "set cali re failed");
			return ret;
		}
	}

	return 0;
}

static int aw_cali_set_mute_status(struct aw_device *aw_dev, int32_t mute)
{
	int ret = -1;
	uint32_t msg_len = sizeof(uint32_t);

	ret = aw883xx_device_params(aw_dev, AW_DEV_HMUTE_PARAMS,
			(void *)&mute, msg_len, AW_SET_DEV_PARAMS);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write_params failed");
		return ret;
	}

	return 0;
}

static void aw_cali_svc_run_mute(struct aw_device *aw_dev, int8_t cali_result)
{
	aw_dev_dbg(aw_dev->dev, "enter");

	if (aw_dev->cali_desc.cali_check_st) {
		if (cali_result == CALI_RESULT_ERROR) {
			aw_cali_set_mute_status(aw_dev, AW_DEV_HMUTE_ENABLE);
		} else if (cali_result == CALI_RESULT_NORMAL) {
			aw_cali_set_mute_status(aw_dev, AW_DEV_HMUTE_DISABLE);
		}
	} else {
		aw_dev_info(aw_dev->dev, "cali check disable");
	}

	aw_dev_info(aw_dev->dev, "done");
}

int aw883xx_cali_init_re_update(void *dev, backup_sec_t flag)
{
	int ret = -1;
	struct aw_device *aw_dev = dev;
	struct aw_hw_cali_re_desc *hw_cali_re_desc = &aw_dev->cali_desc.hw_cali_re_desc;

	if (aw_dev->ops.aw_init_re_update) {
		ret = aw_dev->ops.aw_init_re_update(aw_dev, flag);
		if (ret < 0)
			aw_dev_err(aw_dev->dev, "read or write init re failed");
		return ret;
	}

	if (flag == AW_RECOVERY_SEC_DATA) {
		ret = aw_dev->ops.aw_reg_write(aw_dev, hw_cali_re_desc->hbits_reg, hw_cali_re_desc->re_hbits);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write cali re failed");
			return ret;
		}

		ret = aw_dev->ops.aw_reg_write(aw_dev, hw_cali_re_desc->lbits_reg, hw_cali_re_desc->re_lbits);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write cali re failed");
			return ret;
		}
	} else if (flag == AW_RECORD_SEC_DATA) {
		ret = aw_dev->ops.aw_reg_read(aw_dev, hw_cali_re_desc->hbits_reg, &hw_cali_re_desc->re_hbits);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read cali re failed");
			return ret;
		}

		ret = aw_dev->ops.aw_reg_read(aw_dev, hw_cali_re_desc->lbits_reg, &hw_cali_re_desc->re_lbits);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read cali re failed");
			return ret;
		}
	} else {
		aw_dev_err(aw_dev->dev, "unsupported type:%d", flag);
		return -EINVAL;
	}

	return 0;
}

int aw883xx_cali_get_ra(void *dev)
{
	int ret = -1;
	uint32_t dsp_ra;
	struct aw_device *aw_dev = dev;

	struct aw_ra_desc *desc = &aw_dev->cali_desc.ra_desc;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, desc->dsp_reg,
				&dsp_ra, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read ra error");
		return ret;
	}

	aw_dev->cali_desc.ra = AW_DSP_RE_TO_SHOW_RE(dsp_ra,
					desc->shift);
	aw_dev_info(aw_dev->dev, "get ra:%d", aw_dev->cali_desc.ra);
	return 0;
}

static int aw_cali_get_r0_from_dsp(struct aw_device *aw_dev,
					uint32_t *r0)
{
	int ret = -1;
	uint32_t dsp_re = 0;
	struct aw_r0_desc *r0_desc = &aw_dev->cali_desc.r0_desc;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, r0_desc->dsp_reg, &dsp_re, r0_desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get_dsp_realtime_re failed");
		return ret;
	}

	*r0 = AW_DSP_RE_TO_SHOW_RE(dsp_re, r0_desc->shift);

	return 0;
}

static int aw_cali_svc_get_dev_r0(struct aw_device *aw_dev,
					uint32_t *re)
{
	int ret = -1;

	if (aw_dev->ops.aw_get_r0) {
		ret = aw_dev->ops.aw_get_r0(aw_dev, re);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get_r0 failed");
			return ret;
		}
	} else {
		ret = aw_cali_get_r0_from_dsp(aw_dev, re);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get_r0 failed");
			return ret;
		}
	}

	return 0;
}

static int aw_cali_svc_get_dev_re(struct aw_device *aw_dev,
					uint32_t *re)
{
	int ret;
	struct aw_dsp_cali_re_desc *desc = &aw_dev->cali_desc.dsp_re_desc;
	uint32_t dsp_re = 0;
	uint32_t show_re = 0;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, desc->dsp_reg, &dsp_re, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read re error");
		return ret;
	}

	show_re = AW_DSP_RE_TO_SHOW_RE(dsp_re, desc->shift);

	*re = show_re - aw_dev->cali_desc.ra;
	aw_dev_dbg(aw_dev->dev, "real_r0:[%d]", *re);

	return 0;
}


static int aw_cali_svc_get_devs_r0(struct aw_device *aw_dev, uint32_t *r0_buf, int num)
{
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;
	int ret = -1;
	uint8_t	cnt = 0;
	uint8_t i;

	int dev_cnt = aw883xx_get_dev_num();

	for (i = 0; i < dev_cnt; i++) {
		aw883xx = aw883xx_get_dev(i);
		local_dev = aw883xx->aw_pa;
		if (local_dev->dev < num) {
			ret = aw_cali_svc_get_dev_r0(local_dev, &r0_buf[local_dev->dev]);
			if (ret) {
				aw_dev_err(local_dev->dev, "get r0 failed!");
				return ret;
			}
			cnt++;
		} else {
				aw_dev_err(aw_dev->dev, "channel num[%d] overflow buf num[%d] ",
							 local_dev->dev, num);
		}
	}
	return cnt;
}

static int aw_cali_svc_get_dev_f0(struct aw_device *aw_dev,
					uint32_t *f0)
{
	struct aw_f0_desc *f0_desc = &aw_dev->cali_desc.f0_desc;
	uint32_t dsp_val = 0;
	int ret = -1;

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
				f0_desc->dsp_reg, &dsp_val, f0_desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read f0 failed");
		return ret;
	}

	*f0 = dsp_val >> f0_desc->shift;
	aw_dev_dbg(aw_dev->dev, "real_f0:[%d]", *f0);

	return 0;
}

static int aw_cali_svc_get_devs_f0(struct aw_device *aw_dev, uint32_t *f0_buf, int num)
{
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;
	int ret = -1;
	uint8_t cnt = 0;
	uint8_t i;
	int dev_cnt = aw883xx_get_dev_num();

	for (i = 0; i < dev_cnt; i++) {
		aw883xx = aw883xx_get_dev(i);
		local_dev = aw883xx->aw_pa;
		if (local_dev->dev < num) {
			ret = aw_cali_svc_get_dev_f0(local_dev, &f0_buf[local_dev->dev]);
			if (ret < 0) {
				aw_dev_err(local_dev->dev, "get f0 failed!");
				return ret;
			}
			cnt++;
		} else {
			aw_dev_err(aw_dev->dev, "channel num[%d] overflow buf num[%d] ",
							local_dev->dev, num);
		}
	}
	return cnt;
}

static int aw_cali_svc_get_dev_q(struct aw_device *aw_dev,
					uint32_t *q)
{
	struct aw_q_desc *q_desc = &aw_dev->cali_desc.q_desc;
	uint32_t dsp_val = 0;
	int ret = -1;

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			q_desc->dsp_reg, &dsp_val, q_desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read q failed");
		return ret;
	}

	*q = ((dsp_val * 1000) >> q_desc->shift);

	return 0;
}

int aw_cali_svc_get_dev_te(struct aw_device *aw_dev, struct aw_cali_desc *cali_desc, int32_t *te)
{
	uint16_t reg_val = 0;
	int ret = -1;

	ret = aw_dev->ops.aw_reg_read(aw_dev, cali_desc->spkr_temp_desc.reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read temperature failed");
		return ret;
	}

	*te = (int32_t)((int16_t)reg_val);
	aw_dev_info(aw_dev->dev, "real_te:[%d]", *te);

	return 0;
}

static int aw_cali_svc_get_devs_te(struct aw_device *aw_dev, int32_t *te_buf, int num)
{
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;
	int ret = -1;
	uint8_t i;
	uint8_t cnt = 0;
	int dev_cnt = aw883xx_get_dev_num();

	for (i = 0; i < dev_cnt; i++) {
		aw883xx = aw883xx_get_dev(i);
		local_dev = aw883xx->aw_pa;
		if (local_dev->dev < num) {
			ret = aw_cali_svc_get_dev_te(local_dev, &local_dev->cali_desc, &te_buf[local_dev->dev]);
			if (ret) {
				aw_dev_err(local_dev->dev, "get temperature failed!");
				return ret;
			}
			cnt++;
		} else {
			aw_dev_err(aw_dev->dev, "channel num[%d] overflow buf num[%d]",
						 local_dev->dev, num);
		}
	}
	return cnt;
}

static int aw_cali_svc_get_iv_st(struct aw_device *aw_dev)
{
	struct aw_cali_iv_desc *iv_desc = &aw_dev->cali_desc.iv_desc;
	int ret;
	uint16_t reg_data = 0;
	int i;

	aw_dev_info(aw_dev->dev, "enter");

	for (i = 0; i < AW_GET_IV_CNT_MAX; i++) {
		ret = aw_dev->ops.aw_reg_read(aw_dev, iv_desc->reg, &reg_data);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev,
				"read 0x%x failed", iv_desc->reg);
			return ret;
		}

		reg_data &= (~iv_desc->reabs_mask);
		if (!reg_data) {
			return 0;
		}
		AW_MS_DELAY(30);
	}

	aw_dev_err(aw_dev->dev, "IV data abnormal, please check");

	return -EINVAL;
}

static void aw_cali_svc_bubble_sort(uint32_t *data, int data_size)
{
	int loop_num = data_size - 1;
	uint16_t temp_store = 0;
	int i;
	int j;

	if (data == NULL) {
		aw_pr_err("data is NULL");
		return;
	}

	for (i = 0; i < loop_num; i++) {
		for (j = 0; j < loop_num - i; j++) {
			if (data[j] > data[j + 1]) {
				temp_store = data[j];
				data[j] = data[j + 1];
				data[j + 1] = temp_store;
			}
		}
	}
}

static int aw_cali_svc_del_max_min_ave_algo(uint32_t *data, int data_size)
{
	int sum = 0;
	int ave = 0;
	int i = 0;

	aw_cali_svc_bubble_sort(data, data_size);
	for (i = 1; i < data_size - 1; i++)
		sum += data[i];

	if ((data_size - AW_CALI_DATA_SUM_RM) == 0) {
		aw_pr_err("data_size id :%d less than 2", data_size);
		return -EINVAL;
	}

	ave = sum / (data_size - AW_CALI_DATA_SUM_RM);

	return ave;
}

static void aw_cali_svc_set_cali_status(struct aw_device *aw_dev, bool status)
{

	aw_dev->cali_desc.status = status;
	aw_dev_info(aw_dev->dev, "cali %s",
		(status == 0) ? ("disable") : ("enable"));
}

bool aw883xx_cali_get_cali_status(struct aw_cali_desc *cali_desc)
{
	return cali_desc->status;
}

static int aw_cali_svc_cali_init_check(struct aw_device *aw_dev)
{
	int ret = -1;
	bool pa_status =false;
	uint32_t hmute_status = AW_DEV_HMUTE_DISABLE;

	aw_dev_dbg(aw_dev->dev, "enter");

	pa_status = aw883xx_device_status(aw_dev, AW_DEV_PLL_WDT_STATUS,
				AW_GET_DEV_STATUS);
	if (!pa_status) {
		aw_dev_err(aw_dev->dev, "pa status abnormal");
		return -EINVAL;
	}

	ret = aw883xx_device_params(aw_dev, AW_DEV_HMUTE_PARAMS,
			(void *)&hmute_status, sizeof(hmute_status), AW_GET_DEV_PARAMS);
	if ((ret < 0) || (hmute_status == AW_DEV_HMUTE_ENABLE)) {
		aw_dev_err(aw_dev->dev, "hmute status abnormal:%d, ret = %d", hmute_status, ret);
		return -EINVAL;
	}

	return 0;
}

static int aw_cali_svc_get_cali_cfg(struct aw_device *aw_dev)
{
	int ret = -1;
	struct aw_cali_cfg_desc *desc = &aw_dev->cali_desc.cali_cfg_desc;
	struct cali_cfg *cali_cfg = &aw_dev->cali_desc.cali_cfg;

	aw_dev_dbg(aw_dev->dev, "enter");

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			desc->actampth_reg, &cali_cfg->data[0], desc->actampth_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read reg0x%x error", desc->actampth_reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			desc->noiseampth_reg, &cali_cfg->data[1], desc->noiseampth_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read reg0x%x error", desc->noiseampth_reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			desc->ustepn_reg, &cali_cfg->data[2], desc->ustepn_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read reg0x%x error", desc->ustepn_reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			desc->alphan_reg, &cali_cfg->data[3], desc->alphan_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp read reg0x%x error", desc->alphan_reg);
		return ret;
	}

	return 0;
}

static int aw_cali_svc_set_cali_cfg(struct aw_device *aw_dev,
				struct cali_cfg cali_cfg)
{
	int ret = -1;
	struct aw_cali_cfg_desc *desc = &aw_dev->cali_desc.cali_cfg_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	ret = aw_dev->ops.aw_dsp_write(aw_dev,
			desc->actampth_reg, cali_cfg.data[0], desc->actampth_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp write reg0x%x error", desc->actampth_reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_write(aw_dev,
			desc->noiseampth_reg, cali_cfg.data[1], desc->noiseampth_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp write reg0x%x error", desc->noiseampth_reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_write(aw_dev,
			desc->ustepn_reg, cali_cfg.data[2], desc->ustepn_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp write reg0x%x error", desc->ustepn_reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_write(aw_dev,
			desc->alphan_reg, cali_cfg.data[3], desc->alphan_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp write reg0x%x error", desc->alphan_reg);
		return ret;
	}

	return 0;
}

static int aw_cali_svc_set_dsp_status(struct aw_device *aw_dev, uint32_t status)
{
	int ret = -1;
	uint32_t msg_len = sizeof(uint32_t);

	ret = aw883xx_device_params(aw_dev, AW_DEV_DSP_PARAMS,
			(void *)&status, msg_len, AW_SET_DEV_PARAMS);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write_params failed");
		return ret;
	}

	return 0;
}

static int aw_cali_svc_get_smooth_cali_re(struct aw_device *aw_dev)
{
	int ret = -1;
	int i = 0;
	uint32_t re_temp[AW_CALI_READ_CNT_MAX] = { 0 };
	uint32_t dsp_re;

	aw_dev_dbg(aw_dev->dev, "enter");

	for (i = 0; i < AW_CALI_READ_CNT_MAX; i++) {
		ret = aw_cali_svc_get_dev_re(aw_dev, &re_temp[i]);
		if (ret < 0) {
			goto cali_re_fail;
		}

		AW_MS_DELAY(30);/*delay 30 ms*/
	}

	dsp_re = aw_cali_svc_del_max_min_ave_algo(re_temp,
					AW_CALI_READ_CNT_MAX);

	ret = aw_cali_svc_get_iv_st(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev,
			"get iv data failed");
		goto cali_re_fail;
	}

	if (dsp_re < aw_dev->re_range.re_min || dsp_re > aw_dev->re_range.re_max) {
		aw_dev_err(aw_dev->dev,
				"out range re value: %d", dsp_re);
		aw_dev->cali_desc.cali_re = dsp_re;
		if (aw_dev->cali_desc.cali_check_st)
			aw_dev->cali_desc.cali_result = CALI_RESULT_ERROR;
		aw_cali_svc_run_mute(aw_dev, aw_dev->cali_desc.cali_result);
		return 0;
	}

	if (aw_dev->cali_desc.cali_check_st)
		aw_dev->cali_desc.cali_result = CALI_RESULT_NORMAL;

	aw_dev->cali_desc.cali_re = dsp_re;
	aw_dev_info(aw_dev->dev, "re[%d]mohm", aw_dev->cali_desc.cali_re);

	aw_cali_svc_set_dsp_status(aw_dev, AW_DEV_DSP_BYPASS);
	aw883xx_set_cali_re((void *)aw_dev);
	aw_cali_svc_set_dsp_status(aw_dev, AW_DEV_DSP_WORK);

	return 0;
cali_re_fail:
	if (aw_dev->cali_desc.cali_check_st)
		aw_dev->cali_desc.cali_result = CALI_RESULT_ERROR;
	aw_cali_svc_run_mute(aw_dev, aw_dev->cali_desc.cali_result);
	return -EINVAL;

}

static int aw_cali_svc_cali_en(struct aw_device *aw_dev, bool cali_en)
{
	int ret = -1;
	struct cali_cfg set_cfg;

	aw_dev_info(aw_dev->dev, "cali_en:%d", cali_en);

	aw_cali_svc_set_dsp_status(aw_dev, AW_DEV_DSP_BYPASS);
	if (cali_en) {
		ret = aw_cali_svc_get_cali_cfg(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get cali cfg failed");
			aw_cali_svc_set_dsp_status(aw_dev, AW_DEV_DSP_WORK);
			return ret;
		}
		set_cfg.data[0] = 0;
		set_cfg.data[1] = 0;
		set_cfg.data[2] = 0xffffffff;
		set_cfg.data[3] = 1;

		ret = aw_cali_svc_set_cali_cfg(aw_dev, set_cfg);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "set cali cfg failed");
			aw_cali_svc_set_cali_cfg(aw_dev, aw_dev->cali_desc.cali_cfg);
			aw_cali_svc_set_dsp_status(aw_dev, AW_DEV_DSP_WORK);
			return ret;
		}
	} else {
		aw_cali_svc_set_cali_cfg(aw_dev, aw_dev->cali_desc.cali_cfg);
	}
	aw_cali_svc_set_dsp_status(aw_dev, AW_DEV_DSP_WORK);

	return 0;
}

static int aw_cali_svc_cali_run_dsp_vol(struct aw_device *aw_dev,
						int type, bool enable)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t set_vol = 0;
	struct aw_dsp_vol_desc *desc = &aw_dev->dsp_vol_desc;

	aw_dev_info(aw_dev->dev, "type:%d, enable:%d", type, enable);

	if (enable) {
		/*set dsp vol*/
		if (type == CALI_TYPE_RE) {
			set_vol = desc->mute_st;
		} else if (type == CALI_TYPE_F0) {
			set_vol = desc->noise_st;
		} else {
			aw_dev_err(aw_dev->dev, "type:%d unsupported", type);
			return -EINVAL;
		}

		ret = aw_dev->ops.aw_reg_read(aw_dev,
					desc->reg, &reg_val);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read reg 0x%x failed", desc->reg);
			return ret;
		}

		aw_dev->cali_desc.store_vol = reg_val & (~desc->mask);
		reg_val &= desc->mask;
		reg_val |= set_vol;

		ret = aw_dev->ops.aw_reg_write(aw_dev,
					desc->reg, reg_val);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write reg 0x%x failed", desc->reg);
			return ret;
		}
	} else {
		/*reset dsp vol*/
		ret = aw_dev->ops.aw_reg_read(aw_dev,
						desc->reg, &reg_val);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "read reg 0x%x failed", desc->reg);
			return ret;
		}

		reg_val &= desc->mask;
		reg_val |= aw_dev->cali_desc.store_vol;

		ret = aw_dev->ops.aw_reg_write(aw_dev,
						desc->reg, reg_val);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write reg 0x%x failed", desc->reg);
			return ret;
		}
	}

	return 0;
}

static int aw_cali_svc_set_white_noise(struct aw_device *aw_dev,
					bool noise_enable)
{
	int ret = -1;
	uint32_t reg_val;
	struct aw_noise_desc *desc = &aw_dev->cali_desc.noise_desc;

	aw_dev_info(aw_dev->dev, "set noise %s",
			(noise_enable == 0) ? ("disable") : ("enable"));

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			desc->dsp_reg, &reg_val, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read dsp reg 0x%x failed", desc->dsp_reg);
		return ret;
	}

	if (noise_enable) {
		reg_val |= (~desc->mask);
	} else {
		reg_val &= desc->mask;
	}

	ret = aw_dev->ops.aw_dsp_write(aw_dev,
			desc->dsp_reg, reg_val, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write dsp reg 0x%x failed", desc->dsp_reg);
		return ret;
	}

	return 0;
}

static int aw_cali_svc_cali_f0_en(struct aw_device *aw_dev, bool f0_enable)
{
	int ret = -1;
	struct aw_cali_delay_desc *desc = &aw_dev->cali_desc.cali_delay_desc;

	aw_dev_info(aw_dev->dev, "cali f0 %s",
			(f0_enable == 0) ? ("disable") : ("enable"));

	if (f0_enable) {
		ret = aw_cali_svc_cali_run_dsp_vol(aw_dev, CALI_TYPE_F0, true);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "run dsp volume error, ret=%d", ret);
			return ret;
		}
		AW_MS_DELAY(desc->delay);

		ret = aw_cali_svc_set_white_noise(aw_dev, true);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write white noise error, ret=%d", ret);
			aw_cali_svc_cali_run_dsp_vol(aw_dev, CALI_TYPE_F0, false);
			return ret;
		}
	} else {
		aw_cali_svc_set_white_noise(aw_dev, false);
		aw_cali_svc_cali_run_dsp_vol(aw_dev, CALI_TYPE_F0, false);
	}

	return 0;
}

static int aw_cali_svc_get_cali_f0_q(struct aw_device *aw_dev)
{
	int ret = -1;
	int cnt = 0;
	uint32_t f0 = 0;
	uint32_t q = 0;
	uint32_t f0_sum = 0;
	uint32_t q_sum = 0;
	struct aw_cali_desc *cali_desc = &aw_dev->cali_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	for (cnt = 0; cnt < F0_READ_CNT_MAX; cnt++) {
		/*f0*/
		ret = aw_cali_svc_get_dev_f0(aw_dev, &f0);
		if (ret < 0) {
			return ret;
		}
		f0_sum += f0;

		/*q*/
		ret = aw_cali_svc_get_dev_q(aw_dev, &q);
		if (ret < 0) {
			return ret;
		}
		q_sum += q;
		AW_MS_DELAY(30);
	}

	cali_desc->f0 = f0_sum / cnt;
	cali_desc->q = q_sum / cnt;

	ret = aw_cali_svc_get_iv_st(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev,
			"get iv data failed, set default f0: 2600 q: 2600");
		cali_desc->f0 = 2600;
		cali_desc->q = 2600;
	}

	aw_dev_info(aw_dev->dev, "f0[%d] q[%d]", cali_desc->f0, cali_desc->q);
	return 0;
}

static int aw_cali_svc_cali_mode_enable(struct aw_device *aw_dev,
					int type, unsigned int flag, bool is_enable)
{
	int ret = -1;
	int32_t crc_switch_set = -1;

	aw_dev_info(aw_dev->dev, "type:%d, flag:0x%x, is_enable:%d",
				type, flag, is_enable);

	if (is_enable) {
		if (aw_dev->crc_check_realtime_desc.init_switch) {
			crc_switch_set = aw_dev->crc_check_realtime_desc.disable;
			ret = aw883xx_device_params(aw_dev, AW_DEV_REALTIME_CRC_SET_PARAMS,
				(void *)&crc_switch_set, sizeof(crc_switch_set), AW_SET_DEV_PARAMS);
		}

		ret = aw_cali_svc_cali_init_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "init check failed");
			return ret;
		}

		aw_cali_svc_set_cali_status(aw_dev, true);

		ret = aw_cali_svc_cali_en(aw_dev, true);
		if (ret < 0) {
			aw_cali_svc_set_cali_status(aw_dev, false);
			return ret;
		}

		if ((type == CALI_TYPE_RE) && (flag & CALI_OPS_HMUTE)) {
			ret = aw_cali_svc_cali_run_dsp_vol(aw_dev, CALI_TYPE_RE, true);
			if (ret < 0) {
				aw_cali_svc_cali_en(aw_dev, false);
				aw_cali_svc_set_cali_status(aw_dev, false);
				return ret;
			}
		} else if ((type == CALI_TYPE_F0) && (flag & CALI_OPS_NOISE)) {
			ret = aw_cali_svc_cali_f0_en(aw_dev, true);
			if (ret < 0) {
				aw_cali_svc_cali_en(aw_dev, false);
				aw_cali_svc_set_cali_status(aw_dev, false);
				return ret;
			}
		}
	} else {

		if ((type == CALI_TYPE_RE) && (flag & CALI_OPS_HMUTE)) {
			aw_cali_svc_cali_run_dsp_vol(aw_dev, CALI_TYPE_RE, false);
		} else if ((type == CALI_TYPE_F0) && (flag & CALI_OPS_NOISE)) {
			aw_cali_svc_cali_f0_en(aw_dev, false);
		}
		aw_cali_svc_cali_en(aw_dev, false);
		aw883xx_device_status(aw_dev, AW_DEV_CLEAR_INT_STATUS, AW_SET_DEV_STATUS);
		aw_cali_svc_set_cali_status(aw_dev, false);

		if (aw_dev->crc_check_realtime_desc.init_switch) {
			crc_switch_set = aw_dev->crc_check_realtime_desc.enable;
			ret = aw883xx_device_params(aw_dev, AW_DEV_REALTIME_CRC_SET_PARAMS,
				(void *)&crc_switch_set, sizeof(crc_switch_set), AW_SET_DEV_PARAMS);
		}
	}

	return 0;
}

static int aw_cali_svc_devs_cali_mode_enable(int dev_cnt,
						int type, unsigned int flag,
						bool is_enable)
{
	int ret;
	uint8_t cnt;
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;

	for (cnt = 0; cnt < dev_cnt; cnt++) {
		aw_pr_info("dev[%d]", cnt);
		aw883xx = aw883xx_get_dev(cnt);
		local_dev = aw883xx->aw_pa;
		if (is_enable) {
			aw_cali_svc_run_mute(local_dev, CALI_RESULT_NORMAL);
		}
		ret = aw_cali_svc_cali_mode_enable(local_dev, type,
					flag, is_enable);
		if (ret < 0) {
			return ret;
		}
		if (!is_enable && (type == CALI_TYPE_F0)) {
			aw_cali_svc_run_mute(local_dev, local_dev->cali_desc.cali_result);
		}
	}

	return ret;
}

static int aw_cali_svc_dev_cali_re(struct aw_device *aw_dev, unsigned int flag)
{
	int ret = -1;

	aw_dev_info(aw_dev->dev, "enter");

	aw_cali_svc_run_mute(aw_dev, CALI_RESULT_NORMAL);

	ret = aw_cali_svc_cali_mode_enable(aw_dev,
				CALI_TYPE_RE, flag, true);
	if (ret < 0) {
		return ret;
	}
	AW_MS_DELAY(g_cali_re_time);

	ret = aw_cali_svc_get_smooth_cali_re(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "cali re failed");
	}

	aw_cali_svc_cali_mode_enable(aw_dev,
				CALI_TYPE_RE, flag, false);

	return ret;
}

static int aw_cali_svc_devs_get_cali_re(int dev_cnt)
{
	int ret;
	uint8_t cnt;
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;

	for (cnt = 0; cnt < dev_cnt; cnt++) {
		aw883xx = aw883xx_get_dev(cnt);
		local_dev = aw883xx->aw_pa;
		ret = aw_cali_svc_get_smooth_cali_re(local_dev);
		if (ret < 0) {
			aw_dev_err(local_dev->dev, "get re failed");
			return ret;
		}
	}

	return ret;
}

static int aw_cali_svc_devs_cali_re(struct aw_device *aw_dev, unsigned int flag)
{
	int ret = -1;
	int dev_cnt = aw883xx_get_dev_num();

	aw_dev_info(aw_dev->dev, "enter");

	ret = aw_cali_svc_devs_cali_mode_enable(dev_cnt,CALI_TYPE_RE,
					flag, true);
	if (ret < 0)
		goto error;

	AW_MS_DELAY(g_cali_re_time);

	ret = aw_cali_svc_devs_get_cali_re(dev_cnt);
	if (ret < 0) {
		goto error;
	}
	aw_cali_svc_devs_cali_mode_enable(dev_cnt,CALI_TYPE_RE,
						flag, false);

	return 0;
error:
	aw_cali_svc_devs_cali_mode_enable(dev_cnt,CALI_TYPE_RE,
						flag, false);
	return ret;
}

static int aw_cali_svc_cali_re(struct aw_device *aw_dev, aw_single_t is_single, unsigned int flag)
{
	if (is_single) {
		return aw_cali_svc_dev_cali_re(aw_dev, flag);
	} else {
		return aw_cali_svc_devs_cali_re(aw_dev, flag);
	}
}

static int aw_cali_svc_dev_cali_f0_q(struct aw_device *aw_dev, unsigned int flag)
{
	int ret = -1;

	aw_cali_svc_run_mute(aw_dev, CALI_RESULT_NORMAL);

	ret = aw_cali_svc_cali_mode_enable(aw_dev, CALI_TYPE_F0, flag, true);
	if (ret < 0) {
		return ret;
	}
	AW_MS_DELAY(AW_CALI_F0_TIME);

	ret = aw_cali_svc_get_cali_f0_q(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get f0 q failed");
	}
	aw_cali_svc_cali_mode_enable(aw_dev, CALI_TYPE_F0, flag, false);

	aw_cali_svc_run_mute(aw_dev, aw_dev->cali_desc.cali_result);

	return ret;
}

static int aw_cali_svc_devs_get_cali_f0_q(int dev_cnt)
{
	int ret = -1;
	uint8_t cnt;
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;

	for (cnt = 0; cnt < dev_cnt; cnt++) {
		aw883xx = aw883xx_get_dev(cnt);
		local_dev = aw883xx->aw_pa;
		ret = aw_cali_svc_get_cali_f0_q(local_dev);
		if (ret < 0) {
			aw_dev_err(local_dev->dev, "get f0 q failed");
			return ret;
		}
	}

	return ret;
}

static int aw_cali_svc_devs_cali_f0_q(struct aw_device *aw_dev, unsigned int flag)
{
	int ret = -1;
	int dev_cnt = aw883xx_get_dev_num();

	ret = aw_cali_svc_devs_cali_mode_enable(dev_cnt,CALI_TYPE_F0,
						flag, true);
	if (ret < 0) {
		goto error;
	}
	AW_MS_DELAY(AW_CALI_F0_TIME);

	ret = aw_cali_svc_devs_get_cali_f0_q(dev_cnt);
	if (ret < 0) {
		goto error;
	}
	aw_cali_svc_devs_cali_mode_enable(dev_cnt,CALI_TYPE_F0,
							flag, false);

	return 0;
error:
	aw_cali_svc_devs_cali_mode_enable(dev_cnt,CALI_TYPE_F0,
					flag, false);
	return ret;
}

static int aw_cali_svc_cali_f0_q(struct aw_device *aw_dev, aw_single_t is_single, unsigned int flag)
{
	if (is_single) {
		return aw_cali_svc_dev_cali_f0_q(aw_dev, flag);
	} else {
		return aw_cali_svc_devs_cali_f0_q(aw_dev, flag);
	}
}

static int aw_cali_svc_get_dev_cali_val(struct aw_device *aw_dev, int type, uint32_t *data_buf)
{
	switch (type) {
	case GET_RE_TYPE:
		*data_buf = aw_dev->cali_desc.cali_re;
		break;
	case GET_F0_TYPE:
		*data_buf = aw_dev->cali_desc.f0;
		break;
	case GET_Q_TYPE:
		*data_buf = aw_dev->cali_desc.q;
		break;
	default:
		aw_dev_err(aw_dev->dev, "type:%d not support", type);
		return -EINVAL;
	}

	return 0;
}

static int aw_cali_svc_get_devs_cali_val(struct aw_device *aw_dev, int type, uint32_t *data_buf, int num)
{
	struct aw883xx *aw883xx = NULL;
	struct aw_device *local_dev = NULL;
	uint8_t cnt = 0;
	uint8_t i;
	int dev_cnt = aw883xx_get_dev_num();

	for (i = 0; i < dev_cnt; i++) {
		aw883xx = aw883xx_get_dev(i);
		local_dev = aw883xx->aw_pa;
		if (local_dev->dev < num) {
			switch (type) {
			case GET_RE_TYPE:
				data_buf[local_dev->dev] = local_dev->cali_desc.cali_re;
				break;
			case GET_F0_TYPE:
				data_buf[local_dev->dev] = local_dev->cali_desc.f0;
				break;
			case GET_Q_TYPE:
				data_buf[local_dev->dev] = local_dev->cali_desc.q;
				break;
			default:
				aw_dev_err(local_dev->dev, "type:%d not support", type);
				return -EINVAL;
			}
			cnt++;
		} else {
			aw_dev_err(local_dev->dev, "channel num[%d] overflow buf num[%d]",
						local_dev->dev, num);
			return -EINVAL;
		}
	}
	return cnt;
}



static int aw_cali_svc_cali_re_f0_q(struct aw_device *aw_dev, aw_single_t is_single, unsigned int flag)
{
	int ret = -1;

	ret = aw_cali_svc_cali_re(aw_dev, is_single, flag);
	if (ret < 0) {
		return ret;
	}
	ret = aw_cali_svc_cali_f0_q(aw_dev, is_single, flag);
	if (ret < 0) {
		return ret;
	}
	return 0;
}



/*****************************Calibration Interface******************************************************/

int aw883xx_cail_re_cali(void *dev, aw_single_t is_single)
{
	int ret = -1;
	struct aw_device *aw_dev = dev;

	aw_dev_info(aw_dev->dev, "enter");

	ret = aw_cali_svc_cali_re(aw_dev, is_single, CALI_OPS_HMUTE);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "cali re failed");
		return -EINVAL;
	}

	return 0;
}

int aw883xx_cali_f0_q_cali(void *dev, aw_single_t is_single)
{
	int ret = -1;
	struct aw_device *aw_dev = dev;

	aw_dev_info(aw_dev->dev, "enter");

	ret = aw_cali_svc_cali_f0_q(aw_dev, is_single, CALI_OPS_NOISE);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "cali f0 failed");
		return -EINVAL;
	}

	return 0;
}

int aw883xx_cali_re_f0_cali(void *dev, aw_single_t is_single)
{
	int ret = -1;
	struct aw_device *aw_dev = dev;

	aw_dev_info(aw_dev->dev, "enter");

	ret = aw_cali_svc_cali_re_f0_q(aw_dev, is_single, CALI_OPS_HMUTE|CALI_OPS_NOISE);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "cali re or f0 failed");
		return -EINVAL;
	}

	return 0;
}


int aw883xx_cali_get_re_range(void *dev, aw_single_t is_single, uint32_t *range_buf)
{
	struct aw_device *aw_dev = dev;
	int ret = -1;
	uint8_t cnt;

	int dev_cnt = aw883xx_get_dev_num();

	if (is_single) {
		aw_cali_svc_get_dev_re_range(aw_dev, range_buf);

		aw_dev_info(aw_dev->dev, "re_min value: [%d]", range_buf[RE_MIN_FLAG]);
		aw_dev_info(aw_dev->dev, "re_max value: [%d]", range_buf[RE_MAX_FLAG]);
	} else {
		ret = aw_cali_svc_get_devs_re_range(aw_dev, range_buf, AW_DEV_CH_MAX);
		if (ret <= 0) {
			aw_dev_err(aw_dev->dev, "get re_range failed");
			return -EINVAL;
		}
		for (cnt = 0; cnt < dev_cnt; cnt++) {
			aw_pr_info("dev[%d]:re_min:%d re_max:%d", cnt,
					range_buf[RE_MIN_FLAG + cnt * RE_RANGE_NUM], range_buf[RE_MAX_FLAG + cnt * RE_RANGE_NUM]);
		}
	}
	return 0;
}


int aw883xx_cali_get_r0(void *dev, aw_single_t is_single, uint32_t *r0_data)
{
	struct aw_device *aw_dev = dev;
	int ret = -1;
	uint8_t cnt;

	int dev_cnt = aw883xx_get_dev_num();

	if (is_single) {
		ret = aw_cali_svc_get_dev_r0(aw_dev, r0_data);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get r0 failed");
			return -EINVAL;
		}
		aw_dev_info(aw_dev->dev, "r0:%d mOhms", *r0_data);
	} else {
		ret = aw_cali_svc_get_devs_r0(aw_dev, r0_data, AW_DEV_CH_MAX);
		if (ret <= 0) {
			aw_dev_err(aw_dev->dev, "get r0 failed");
			return -EINVAL;
		}
		for (cnt = 0; cnt < dev_cnt; cnt++) {
			aw_pr_info("dev[%d]r0:%d mOhms", cnt, r0_data[cnt]);
		}
	}
	return 0;
}

int aw883xx_cali_get_f0(void *dev, aw_single_t is_single, uint32_t *f0)
{
	uint8_t cnt;
	int ret = -1;
	struct aw_device *aw_dev = dev;

	int dev_cnt = aw883xx_get_dev_num();

	aw_dev_info(aw_dev->dev, "enter");

	if (is_single) {
		aw_cali_svc_get_dev_f0(aw_dev, f0);
		aw_dev_info(aw_dev->dev, "f0:%u Hz", *f0);
	} else {
		ret = aw_cali_svc_get_devs_f0(aw_dev, f0, AW_DEV_CH_MAX);
		if (ret <= 0) {
			aw_pr_err("dev[%d] get f0 failed", cnt);
			return -EINVAL;
		}
		for (cnt = 0; cnt < dev_cnt; cnt++) {
			aw_pr_info("dev[%d]f0:%u Hz", cnt, f0[cnt]);
		}
	}
	return 0;
}

int aw883xx_cali_get_te(void *dev, aw_single_t is_single, int32_t *te_data)
{
	struct aw_device *aw_dev = dev;
	int ret = -1;
	uint8_t cnt;

	int dev_cnt = aw883xx_get_dev_num();

	if (is_single) {
		ret = aw_cali_svc_get_dev_te(aw_dev, &aw_dev->cali_desc, te_data);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get te failed");
			return -EINVAL;
		}
		aw_dev_info(aw_dev->dev, "te:%d", *te_data);
	} else {
		ret = aw_cali_svc_get_devs_te(aw_dev, te_data, AW_DEV_CH_MAX);
		if (ret <= 0) {
			aw_dev_err(aw_dev->dev, "get te failed");
			return -EINVAL;
		} else {
			for (cnt = 0; cnt < dev_cnt; cnt++) {
				aw_pr_info("dev[%d]te:%d", cnt, te_data[cnt]);
			}
		}
	}
	return 0;
}

int aw883xx_cali_f0_show(void *dev, aw_single_t is_single, uint32_t *f0)
{
	uint8_t cnt;
	int ret = -1;
	struct aw_device *aw_dev = dev;
	int dev_cnt = aw883xx_get_dev_num();

	aw_dev_info(aw_dev->dev, "enter");

	if (is_single) {
		ret = aw_cali_svc_get_dev_cali_val(aw_dev, GET_F0_TYPE, f0);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get q failed");
			return -EINVAL;
		}
		aw_dev_info(aw_dev->dev, "f0:%u Hz", *f0);
	} else {
		for (cnt = 0; cnt < dev_cnt; cnt++) {
			ret = aw_cali_svc_get_devs_cali_val(aw_dev, GET_F0_TYPE, f0, AW_DEV_CH_MAX);
			if (ret <= 0) {
				aw_pr_err("dev[%d] get f0 failed", cnt);
				return -EINVAL;
			}
			aw_pr_info("dev[%d]f0:%u Hz", cnt, f0[cnt]);
		}
	}
	return 0;
}

int aw883xx_cali_q_show(void *dev, aw_single_t is_single, uint32_t *q)
{
	int ret = -1;
	uint8_t cnt;
	struct aw_device *aw_dev = dev;

	int dev_cnt = aw883xx_get_dev_num();

	aw_dev_info(aw_dev->dev, "enter");

	if (is_single) {
		ret = aw_cali_svc_get_dev_cali_val(aw_dev, GET_Q_TYPE, q);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get q failed");
			return -EINVAL;
		}
		aw_dev_info(aw_dev->dev, "q:%u", *q);
	} else {
		for (cnt = 0; cnt < dev_cnt; cnt++) {
			ret = aw_cali_svc_get_devs_cali_val(aw_dev, GET_Q_TYPE, q, AW_DEV_CH_MAX);
			if (ret <= 0) {
				aw_pr_err("dev[%d] get q failed", cnt);
				return -EINVAL;
			}
			aw_pr_info("dev[%d]q:%u", cnt, q[cnt]);
		}
	}
	return 0;
}

int aw883xx_cali_re_show(void *dev, aw_single_t is_single, uint32_t *re)
{
	int ret = -1;
	uint8_t cnt;
	struct aw_device *aw_dev = dev;
	int dev_cnt = aw883xx_get_dev_num();

	aw_dev_info(aw_dev->dev, "enter");

	if (is_single) {
		ret = aw_cali_svc_get_dev_cali_val(aw_dev, GET_RE_TYPE, re);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "get re failed");
			return -EINVAL;
		}
		aw_dev_info(aw_dev->dev, "cali_re:%umOhms", *re);
	} else {
		for (cnt = 0; cnt < dev_cnt; cnt++) {
			ret = aw_cali_svc_get_devs_cali_val(aw_dev, GET_RE_TYPE, re, AW_DEV_CH_MAX);
			if (ret <= 0) {
				aw_pr_err("dev[%d] get re failed", cnt);
				return -EINVAL;
			}
			aw_pr_info("dev[%d]cali_re:%umOhms", cnt, re[cnt]);
		}
	}
	return 0;
}

int aw883xx_cali_store_cali_re(void *dev, aw_single_t is_single, uint32_t *set_re)
{
	struct aw_device *aw_dev = dev;
	uint32_t re;
	int ret = -1;

	if (is_single) {
		re = set_re[aw_dev->dev];
		ret = aw_cali_store_cali_re(aw_dev, re);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "store cali re failed!");
			return -EINVAL;
		}
	} else {
		ret = aw_cali_devs_store_cali_re(aw_dev, set_re);
		if (ret < 0) {
			aw_pr_err("set devs_re failed");
			return -EINVAL;
		}
	}
	return 0;
}

int aw883xx_cali_read_re_from_dsp(void *dev, uint32_t *re)
{
	struct aw_device *aw_dev = dev;
	int ret = -1;

	ret = aw_cali_svc_read_cali_re_from_pa(aw_dev, re);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read re from dsp failed");
		return -EINVAL;
	}
	aw_dev_info(aw_dev->dev, "dsp_re:%d", *re);
	return 0;
}

int aw883xx_cali_show_cali_time(uint32_t *time)
{
	*time = g_cali_re_time;
	aw_pr_info("time:%u", *time);
	return 0;
}


int aw883xx_cali_store_cali_time(uint32_t time)
{
	if (time < 1000) {
		aw_pr_err( "time:%d is too short, no set", time);
		return -EINVAL;
	}

	g_cali_re_time = time;
	aw_pr_info("time:%u", g_cali_re_time);
	return 0;
}

#endif

