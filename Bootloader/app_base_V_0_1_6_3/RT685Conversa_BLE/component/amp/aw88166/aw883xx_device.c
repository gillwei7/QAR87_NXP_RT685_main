/*
* aw883xx_device.c aw883xx codec driver
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

#include <aw_profile_process.h>
#include <aw883xx_base.h>
#include <aw883xx_device.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define AW_DEV_SYSST_CHECK_MAX		(10)

static unsigned int g_fade_in_time = AW_1_MS;
static unsigned int g_fade_out_time = AW_1_MS;


#ifdef AW_MONITOR
extern int aw883xx_monitor_init(void *dev);
extern void aw883xx_monitor_start(void *dev);
extern void aw883xx_monitor_stop(void *dev);
extern void aw883xx_monitor_deinit(void *dev);
extern int aw883xx_monitor_work_func(void *dev);
extern void aw883xx_monitor_set_handle(void *dev);
extern void aw883xx_monitor_update_st(void *dev, monitor_update_t type);

void aw883xx_dev_get_monitor_func(struct aw_device *aw_dev)
{
	aw_dev->ops.aw_monitor_init = aw883xx_monitor_init;
	aw_dev->ops.aw_monitor_start = aw883xx_monitor_start;
	aw_dev->ops.aw_monitor_stop = aw883xx_monitor_stop;
	aw_dev->ops.aw_monitor_deinit = aw883xx_monitor_deinit;
	aw_dev->ops.aw_monitor_work_func = aw883xx_monitor_work_func;
	aw_dev->ops.aw_monitor_set_handle = aw883xx_monitor_set_handle;
}

#else
void aw883xx_dev_get_monitor_func(struct aw_device *aw_dev)
{
	aw_dev->ops.aw_monitor_init = NULL;
	aw_dev->ops.aw_monitor_start = NULL;
	aw_dev->ops.aw_monitor_stop = NULL;
	aw_dev->ops.aw_monitor_deinit = NULL;
	aw_dev->ops.aw_monitor_work_func = NULL;
	aw_dev->ops.aw_monitor_work_func = NULL;
	aw_dev->ops.aw_monitor_set_handle = NULL;
}
#endif


int aw883xx_dev_check_prof(uint32_t dev, struct aw_prof_info *prof_info)
{
	struct aw_prof_desc *prof_desc = NULL;
	int i = 0;
	int j = 0;

	if (prof_info == NULL) {
		aw_dev_err(dev, "prof_info is NULL");
		return -EINVAL;
	}

	if (prof_info->count <= 0) {
		aw_dev_err(dev, "prof count :%d unsupported", prof_info->count);
		return -EINVAL;
	}

	prof_desc = prof_info->prof_desc;
	for (i = 0; i < prof_info->count; i++) {
		if (prof_desc == NULL) {
			aw_dev_err(dev, "invalid prof_desc");
			return -EINVAL;
		}

		if (prof_desc->sec_desc->len <= 0) {
			aw_dev_err(dev, "prof len:%d unsupported", prof_desc->sec_desc->len);
			return -EINVAL;
		}

		if (prof_desc->sec_desc->data == NULL) {
			aw_dev_err(dev, "prof data is NULL");
			return -EINVAL;
		}
	}


	for (i = 0; i < prof_info->count; i++) {
		for (j = i + 1; j < prof_info->count; j++) {
			if (strncmp(prof_info->prof_desc[i].name, prof_info->prof_desc[j].name, AW_PROF_NAME_MAX) == 0) {
				aw_dev_err(dev, "prof_desc pos[%d] and prof_desc pos[%d] conflict with prof_name[%s]",
						i, j, prof_info->prof_desc[j].name);
				return -EINVAL;
			}
		}
	}

	aw_dev_info(dev, "aw_dev_check_prof success");

	return 0;
}

struct aw_sec_data_desc *aw883xx_dev_get_prof_data_byname(struct aw_device *aw_dev, char *prof_name, int data_type)
{
	struct aw_sec_data_desc *sec_data = NULL;
	struct aw_prof_desc *prof_desc = NULL;
	struct aw_prof_info *prof_info = aw_dev->prof_info;
	int i = 0;

	if (data_type >= AW_DATA_TYPE_MAX) {
		aw_dev_err(aw_dev->dev, "unsupport data type id [%d]", data_type);
		return NULL;
	}

	for (i = 0; i < prof_info->count; i++) {
		if (strncmp(prof_name, prof_info->prof_desc[i].name, AW_PROF_NAME_MAX) == 0) {
			prof_desc = &aw_dev->prof_info->prof_desc[i];
			sec_data = &prof_desc->sec_desc[data_type];
			aw_dev_dbg(aw_dev->dev, "get prof[%s] data len[%d]",
								prof_desc->name, sec_data->len);
			return sec_data;
		}
	}

	aw_dev_err(aw_dev->dev, "not found prof_name[%s]", prof_name);
	return NULL;
}

static int aw_dev_check_profile_name(struct aw_device *aw_dev, const char *prof_name)
{
	int i =0;
	struct aw_prof_info *prof_info = aw_dev->prof_info;

	for (i = 0; i < prof_info->count; i++) {
		if (strncmp(prof_name, prof_info->prof_desc[i].name, AW_PROF_NAME_MAX) == 0){
			return 0;
		}
	}
	aw_dev_err(aw_dev->dev, "not found prof_name[%s]", prof_name);
	return -EINVAL;
}

int aw883xx_dev_set_profile_name(struct aw_device *aw_dev, const char *prof_name)
{
	if (aw_dev_check_profile_name(aw_dev, prof_name)) {
		return -EINVAL;
	} else {
		strncpy(aw_dev->set_prof_name, prof_name, AW_PROF_NAME_MAX - 1);
		aw_dev_info(aw_dev->dev, "set prof_name[%s]", aw_dev->set_prof_name);
	}
	return 0;
}

char *aw883xx_dev_get_profile_name(struct aw_device *aw_dev)
{
	return aw_dev->set_prof_name;
}

static int aw_dev_prof_init(struct aw_device *aw_dev, struct aw_init_info *init_info)
{
	int i =0;
	const char *first_prof_name = NULL;
	/*find profile*/
	for (i = 0; i < init_info->mix_chip_count; i++) {
		if (init_info->prof_info[i].chip_id == aw_dev->chip_id) {
			aw_dev->prof_info = &init_info->prof_info[i];
			first_prof_name = init_info->prof_info[i].prof_desc[0].name;
			strncpy(aw_dev->first_prof_name, first_prof_name, AW_PROF_NAME_MAX);
			aw_dev_info(aw_dev->dev, "first prof_name[%s]", aw_dev->first_prof_name);
			return 0;
		}
	}

	aw_dev_err(aw_dev->dev, "no supported profile");
	return -EINVAL;
}

/*****************************awinic device*************************************/
int aw883xx_dev_reg_dump(struct aw_device *aw_dev)
{
	int reg_num = aw_dev->ops.aw_get_reg_num();
	uint8_t i = 0;
	uint16_t reg_val = 0;

	for (i = 0; i < reg_num; i++) {
		if (aw_dev->ops.aw_check_rd_access(i)) {
			aw_dev->ops.aw_reg_read(aw_dev, i, &reg_val);
			aw_dev_info(aw_dev->dev, "read: reg = 0x%02x, val = 0x%04x",
				i, reg_val);
		}
	}
	return 0;
}

static int aw_dev_set_volume(struct aw_device *aw_dev, uint32_t set_vol)
{
	uint16_t hw_vol = 0;
	int ret = -1;
	struct aw_volume_desc *vol_desc = &aw_dev->volume_desc;

	hw_vol = set_vol + vol_desc->init_volume;

	ret = aw_dev->ops.aw_set_hw_volume(aw_dev, hw_vol);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "set volume failed");
		return ret;
	}

	return 0;
}

static int aw_dev_get_volume(struct aw_device *aw_dev, uint32_t *get_vol)
{
	int ret = -1;
	uint16_t hw_vol = 0;
	struct aw_volume_desc *vol_desc = &aw_dev->volume_desc;

	ret = aw_dev->ops.aw_get_hw_volume(aw_dev, &hw_vol);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read volume failed");
		return ret;
	}

	*get_vol = hw_vol - vol_desc->init_volume;

	return 0;
}

static void aw_dev_fade_in(struct aw_device *aw_dev)
{
	int i = 0;
	struct aw_volume_desc *desc = &aw_dev->volume_desc;
	int fade_step = aw_dev->fade_step;
	int fade_in_vol = desc->ctl_volume;
	if (!aw_dev->fade_en) {
		return;
	}

	if (fade_step == 0 || g_fade_in_time == 0) {
		aw_dev_set_volume(aw_dev, fade_in_vol);
		return;
	}
	/*volume up*/
	for (i = desc->mute_volume; i >= fade_in_vol; i -= fade_step) {
		aw_dev_set_volume(aw_dev, i);
		AW_MS_DELAY(g_fade_in_time);
	}
	if (i != fade_in_vol) {
		aw_dev_set_volume(aw_dev, fade_in_vol);
	}
}

static void aw_dev_fade_out(struct aw_device *aw_dev)
{
	int i = 0;
	struct aw_volume_desc *desc = &aw_dev->volume_desc;
	int fade_step = aw_dev->fade_step;

	if (!aw_dev->fade_en)
		return;

	if (fade_step == 0 || g_fade_out_time == 0) {
		aw_dev_set_volume(aw_dev, desc->mute_volume);
		return;
	}

	for (i = desc->ctl_volume; i <= desc->mute_volume; i += fade_step) {
		aw_dev_set_volume(aw_dev, i);
		AW_MS_DELAY(g_fade_out_time);
	}
	if (i != desc->mute_volume) {
		aw_dev_set_volume(aw_dev, desc->mute_volume);
		AW_MS_DELAY(g_fade_out_time);
	}

}

#ifdef AW_FADE
static int aw_dev_get_fade_vol_step(struct aw_device *aw_dev, uint32_t *fade_step)
{
	*fade_step = aw_dev->fade_step;

	return 0;
}

static int aw_dev_set_fade_vol_step(struct aw_device *aw_dev, unsigned int step)
{
	aw_dev->fade_step = step;

	return 0;
}

static int aw_dev_get_fade_time(unsigned int *time, fade_time_t fade_type)
{
	if (fade_type == AW_FADE_IN_TIME) {
		*time = g_fade_in_time;
	} else if (fade_type == AW_FADE_OUT_TIME) {
		*time = g_fade_out_time;
	} else {
		aw_pr_err("unsupported fade type:%d", fade_type);
		return -EINVAL;
	}

	return 0;
}

static int aw_dev_set_fade_time(unsigned int time, fade_time_t fade_type)
{
	if (fade_type == AW_FADE_IN_TIME) {
		g_fade_in_time = time;
	} else if (fade_type == AW_FADE_OUT_TIME) {
		g_fade_out_time = time;
	} else {
		aw_pr_err("unsupported fade type:%d", fade_type);
		return -EINVAL;
	}

	return 0;
}
#endif

#ifdef AW_IRQ
static void aw_dev_interrupt_clear(struct aw_device *aw_dev)
{
	uint16_t reg_val;

	aw_dev->ops.aw_i2c_read(aw_dev, aw_dev->sysst_desc.reg, &reg_val);
	aw_dev_info(aw_dev->dev,"reg SYSST=0x%x", reg_val);
	aw_dev->ops.aw_i2c_read(aw_dev, aw_dev->int_desc.st_reg, &reg_val);
	aw_dev_info(aw_dev->dev,"reg SYSINT=0x%x", reg_val);
	aw_dev->ops.aw_i2c_read(aw_dev, aw_dev->int_desc.mask_reg, &reg_val);
	aw_dev_info(aw_dev->dev,"reg SYSINTM=0x%x", reg_val);
}
#endif

static void aw_dev_soft_reset(struct aw_device *aw_dev)
{
	struct aw_soft_rst *reset = &aw_dev->soft_rst;

	aw_dev->ops.aw_i2c_write(aw_dev, reset->reg, reset->reg_value);
	aw_dev_info(aw_dev->dev, "soft reset done");
}

static void aw_dev_pwd(struct aw_device *aw_dev, bool pwd)
{
	struct aw_pwd_desc *pwd_desc = &aw_dev->pwd_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (pwd) {
		aw_dev->ops.aw_reg_write_bits(aw_dev, pwd_desc->reg,
				pwd_desc->mask,
				pwd_desc->enable);
	} else {
		aw_dev->ops.aw_reg_write_bits(aw_dev, pwd_desc->reg,
				pwd_desc->mask,
				pwd_desc->disable);
	}
	aw_dev_info(aw_dev->dev, "done");
}

static void aw_dev_amppd(struct aw_device *aw_dev, bool amppd)
{
	struct aw_amppd_desc *amppd_desc = &aw_dev->amppd_desc;

	aw_dev_dbg(aw_dev->dev, "enter");
	if (amppd) {
		aw_dev->ops.aw_reg_write_bits(aw_dev, amppd_desc->reg,
				amppd_desc->mask,
				amppd_desc->enable);
	} else {
		aw_dev->ops.aw_reg_write_bits(aw_dev, amppd_desc->reg,
				amppd_desc->mask,
				amppd_desc->disable);
	}
	aw_dev_info(aw_dev->dev, "done");
}

static int aw_dev_set_hmute(struct aw_device *aw_dev, uint32_t mute)
{
	int ret = -1;
	struct aw_mute_desc *mute_desc = &aw_dev->mute_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (mute == AW_DEV_HMUTE_ENABLE) {
		aw_dev_fade_out(aw_dev);
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, mute_desc->reg,
				mute_desc->mask, mute_desc->enable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write mute enable failed");
			return ret;
		}
	} else if (mute == AW_DEV_HMUTE_DISABLE) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, mute_desc->reg,
				mute_desc->mask, mute_desc->disable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write mute disable failed");
			return ret;
		}
		aw_dev_fade_in(aw_dev);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported mute state: %d", mute);
		return -EINVAL;
	}

	aw_dev_info(aw_dev->dev, "done");
	return 0;
}

static int aw_dev_get_hmute(struct aw_device *aw_dev, uint32_t *status)
{
	uint16_t reg_val = 0;
	int ret = -1;
	struct aw_mute_desc *desc = &aw_dev->mute_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if (ret < 0) {
		return ret;
	}

	if (reg_val & (~desc->mask)) {
		*status = AW_DEV_HMUTE_ENABLE;
	} else {
		*status = AW_DEV_HMUTE_DISABLE;
	}

	return 0;
}

static int aw_dev_get_icalk(struct aw_device * aw_dev, int16_t *icalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t icalkh_val = 0;
	uint16_t icalkl_val = 0;
	uint16_t icalk_val = 0;
	struct aw_efuse_check_desc *efuse_check_desc = &aw_dev->efuse_check_desc;
	struct aw_vcalb_desc *vcalb_desc = &aw_dev->vcalb_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->icalkh_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read icalkh_reg failed");
		return ret;
	}
	icalkh_val = reg_val & (~vcalb_desc->icalkh_reg_mask);

	ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->icalkl_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read icalkl_reg failed");
		return ret;
	}
	icalkl_val = reg_val & (~vcalb_desc->icalkl_reg_mask);

	if (efuse_check_desc->check_val == AW_EF_AND_CHECK) {
		icalk_val = (icalkh_val >> vcalb_desc->icalkh_shift) & (icalkl_val >> vcalb_desc->icalkl_shift);
	} else if (efuse_check_desc->check_val == AW_EF_OR_CHECK) {
		icalk_val = (icalkh_val >> vcalb_desc->icalkh_shift) | (icalkl_val >> vcalb_desc->icalkl_shift);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported check type:%d", efuse_check_desc->check_val);
		return -EINVAL;
	}

	if (icalk_val & (~vcalb_desc->icalk_sign_mask)) {
		icalk_val = icalk_val | vcalb_desc->icalk_neg_mask;
	}
	*icalk = (int16_t)icalk_val;
	return 0;
}

static int aw_dev_get_vcalk(struct aw_device * aw_dev, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t vcalkh_val = 0;
	uint16_t vcalkl_val = 0;
	uint16_t vcalk_val = 0;
	struct aw_efuse_check_desc *efuse_check_desc = &aw_dev->efuse_check_desc;
	struct aw_vcalb_desc *vcalb_desc = &aw_dev->vcalb_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->vcalkh_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vcalkh_reg failed");
		return ret;
	}
	vcalkh_val = reg_val & (~vcalb_desc->vcalkh_reg_mask);

	ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->vcalkl_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vcalkl_reg failed");
		return ret;
	}
	vcalkl_val = reg_val & (~vcalb_desc->vcalkl_reg_mask);

	if (efuse_check_desc->check_val == AW_EF_AND_CHECK) {
		vcalk_val = (vcalkh_val >> vcalb_desc->vcalkh_shift) & (vcalkl_val >> vcalb_desc->vcalkl_shift);
	} else if (efuse_check_desc->check_val == AW_EF_OR_CHECK) {
		vcalk_val = (vcalkh_val >> vcalb_desc->vcalkh_shift) | (vcalkl_val >> vcalb_desc->vcalkl_shift);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported check type:%d", efuse_check_desc->check_val);
		return -EINVAL;
	}

	if (vcalk_val & (~vcalb_desc->vcalk_sign_mask)) {
		vcalk_val = vcalk_val | vcalb_desc->vcalk_neg_mask;
	}
	*vcalk = (int16_t)vcalk_val;
	return 0;
}

static int aw_dev_get_internal_vcalk(struct aw_device *aw_dev, int16_t *vcalk)
{
	int ret = -1;
	uint16_t reg_val = 0;
	uint16_t vcalkh_val = 0;
	uint16_t vcalkl_val = 0;
	uint16_t vcalk_val = 0;
	struct aw_efuse_check_desc *efuse_check_desc = &aw_dev->efuse_check_desc;
	struct aw_vcalb_desc *vcalb_desc = &aw_dev->vcalb_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->vcalkh_dac_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vcalkh_dac_reg failed");
		return ret;
	}
	vcalkh_val = reg_val & (~vcalb_desc->vcalkh_dac_reg_mask);

	ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->vcalkl_dac_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vcalkl_dac_reg failed");
		return ret;
	}
	vcalkl_val = reg_val & (~vcalb_desc->vcalkl_dac_reg_mask);

	if (efuse_check_desc->check_val == AW_EF_AND_CHECK) {
		vcalk_val = (vcalkh_val >> vcalb_desc->vcalkh_dac_shift) & (vcalkl_val >> vcalb_desc->vcalkl_dac_shift);
	} else if (efuse_check_desc->check_val == AW_EF_OR_CHECK) {
		vcalk_val = (vcalkh_val >> vcalb_desc->vcalkh_dac_shift) | (vcalkl_val >> vcalb_desc->vcalkl_dac_shift);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported check type:%d", efuse_check_desc->check_val);
		return -EINVAL;
	}

	if (vcalk_val & (~vcalb_desc->vcalk_dac_sign_mask)) {
		vcalk_val = vcalk_val | vcalb_desc->vcalk_dac_neg_mask;
	}
	*vcalk = (int16_t)vcalk_val;
	return 0;
}

static int aw_dev_vsense_select(struct aw_device *aw_dev, int *vsense_select)
{
	int ret = -1;
	struct aw_vsense_desc *desc = &aw_dev->vsense_desc;
	uint16_t vsense_reg_val;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->vcalb_vsense_reg, &vsense_reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read vsense_reg_val failed");
		return ret;
	}
	aw_dev_dbg(aw_dev->dev, "vsense_reg = 0x%x", vsense_reg_val);

	if (vsense_reg_val & (~desc->vcalk_vdsel_mask)) {
		*vsense_select = AW_DEV_VDSEL_VSENSE;
		aw_dev_info(aw_dev->dev, "vsense external");
		return 0;
	}

	*vsense_select = AW_DEV_VDSEL_DAC;
	aw_dev_info(aw_dev->dev, "vsense internal");
	return 0;
}

static int aw_dev_set_vcalb_to_hw(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val = 0;
	int16_t icalk = 0;
	int16_t vcalk = 0;
	int32_t ical_k = 0;
	int32_t vcal_k = 0;
	int32_t vcalb = 0;
	int vsense_select = -1;
	struct aw_vcalb_desc *vcalb_desc = &aw_dev->vcalb_desc;

	if (aw_dev->vsense_desc.vcalb_vsense_reg) {
		ret = aw_dev_vsense_select(aw_dev, &vsense_select);
		if (ret < 0) {
			return ret;
		}
		aw_dev_dbg(aw_dev->dev, "vsense_select = %d", vsense_select);
	} else {
		vsense_select = aw_dev->vsense_desc.vcalb_vsense_default;
	}

	ret = aw_dev_get_icalk(aw_dev, &icalk);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get icalk failed");
		return ret;
	}
	aw_dev_info(aw_dev->dev, "icalk = %d", icalk);

	ical_k = icalk * vcalb_desc->icalk_value_factor + vcalb_desc->cabl_base_value;

	if (vsense_select == AW_DEV_VDSEL_VSENSE) {
		ret = aw_dev_get_vcalk(aw_dev, &vcalk);
		if (ret < 0) {
			return ret;
		}
		vcal_k = vcalk * vcalb_desc->vcalk_value_factor + vcalb_desc->cabl_base_value;
		vcalb = vcalb_desc->vcalb_accuracy *
			vcalb_desc->vscal_factor / vcalb_desc->iscal_factor * ical_k / vcal_k * vcalb_desc->init_value;

		aw_dev_info(aw_dev->dev, "icalk=%d, vcalk=%d", icalk, vcalk);
	} else if (vsense_select == AW_DEV_VDSEL_DAC) {
		ret = aw_dev_get_internal_vcalk(aw_dev, &vcalk);
		if (ret < 0) {
			return ret;
		}
		vcal_k = vcalk * vcalb_desc->vcalk_dac_value_factor + vcalb_desc->cabl_base_value;
		vcalb = vcalb_desc->vcalb_accuracy *
			vcalb_desc->vscal_dac_factor / vcalb_desc->iscal_dac_factor * ical_k / vcal_k * vcalb_desc->init_value;

		aw_dev_info(aw_dev->dev, "icalk=%d, vcalk=%d", icalk, vcalk);
	} else {
		aw_dev_err(aw_dev->dev, "unsupport vsense status");
		return -EINVAL;
	}

	vcalb = vcalb >> vcalb_desc->vcalb_adj_shift;
	reg_val = (uint32_t)vcalb;

	aw_dev_info(aw_dev->dev, "vcalb = %d, vcalb_adj = %d",
			reg_val, vcalb_desc->init_value >> vcalb_desc->vcalb_adj_shift);

	ret = aw_dev->ops.aw_reg_write(aw_dev, vcalb_desc->vcalb_reg, reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write vcalb_reg failed");
		return ret;
	}
	return 0;
}

static int aw_dev_set_vcalb(struct aw_device *aw_dev)
{
	int ret = -1;

	if (aw_dev->ops.aw_set_vcalb) {
		ret = aw_dev->ops.aw_set_vcalb(aw_dev);
	} else {
		ret = aw_dev_set_vcalb_to_hw(aw_dev);
	}
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "set_vcalb failed");
		return ret;
	}

	return 0;
}

static int aw_dev_get_int_status(struct aw_device *aw_dev, uint32_t *int_val)
{
	int ret = -1;
	uint16_t reg_val = 0;

	ret = aw_dev->ops.aw_reg_read(aw_dev, aw_dev->int_desc.st_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read interrupt reg fail, ret=%d", ret);
		return ret;
	}

	*int_val = reg_val;
	aw_dev_dbg(aw_dev->dev, "read interrupt reg = 0x%04x", reg_val);

	return 0;
}

static int aw_dev_clear_int_status(struct aw_device *aw_dev)
{
	uint32_t reg_val = 0;

	/*read int status and clear*/
	aw_dev_get_int_status(aw_dev, &reg_val);
	/*make sure int status is clear*/
	aw_dev_get_int_status(aw_dev, &reg_val);
	aw_dev_info(aw_dev->dev, "done");

	return 0;
}

static int aw_dev_get_sysst_value(struct aw_device *aw_dev, uint32_t *value)
{
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if(ret < 0) {
		aw_dev_err(aw_dev->dev, "read sysst failed");
		return ret;
	}

	*value = reg_val;

	return 0;
}

static bool aw_dev_get_iis_status(struct aw_device *aw_dev)
{
	int ret = -1;
	bool status = false;
	uint16_t reg_val = 0;
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;

	aw_dev_dbg(aw_dev->dev, "enter");

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
	if(ret < 0) {
		aw_dev_err(aw_dev->dev, "read pll failed");
		return false;
	}

	if ((reg_val & desc->pll_check) == desc->pll_check) {
		status = true;
	} else {
		status = false;
		aw_dev_err(aw_dev->dev, "check pll lock fail,reg_val:0x%04x", reg_val);
	}

	return status;
}

static int aw_dev_mode1_pll_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t i = 0;

	for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
		if (!(aw_dev_get_iis_status(aw_dev))) {
			aw_dev_err(aw_dev->dev, "mode1 iis signal check error");
			AW_MS_DELAY(AW_2_MS);
		} else {
			return 0;
		}
	}

	return ret;
}

static int aw_dev_mode2_pll_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t i = 0;
	uint16_t reg_val = 0;
	struct aw_cco_mux_desc *cco_mux_desc = &aw_dev->cco_mux_desc;

	aw_dev->ops.aw_reg_read(aw_dev, cco_mux_desc->reg, &reg_val);
	reg_val &= (~cco_mux_desc->mask);
	if (reg_val == cco_mux_desc->divider) {
		aw_dev_dbg(aw_dev->dev, "CCO_MUX is already divider");
		return ret;
	}

	/* change mode2 */
	aw_dev->ops.aw_reg_write_bits(aw_dev, cco_mux_desc->reg,
		cco_mux_desc->mask, cco_mux_desc->divider);

	for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
		if (!(aw_dev_get_iis_status(aw_dev))) {
			aw_dev_err(aw_dev->dev, "mode2 iis signal check error");
			AW_MS_DELAY(AW_2_MS);
			ret = -EINVAL;
		} else {
			ret = 0;
			break;
		}
	}

	/* change mode1*/
	aw_dev->ops.aw_reg_write_bits(aw_dev, cco_mux_desc->reg,
		cco_mux_desc->mask, cco_mux_desc->bypass);

	if (ret == 0) {
		AW_MS_DELAY(AW_2_MS);
		for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
			ret = aw_dev_mode1_pll_check(aw_dev);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "mode2 switch to mode1, iis signal check error");
				AW_MS_DELAY(AW_2_MS);
			} else {
				break;
			}
		}
	}

	return ret;
}

static int aw_dev_syspll_check(struct aw_device *aw_dev)
{
	int ret = -1;

	ret = aw_dev_mode1_pll_check(aw_dev);
	if (ret < 0) {
		aw_dev_info(aw_dev->dev, "mode1 check iis failed try switch to mode2 check");
		ret = aw_dev_mode2_pll_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "mode2 check iis failed");
			return ret;
		}
	}

	return ret;
}

int aw883xx_dev_sysst_check(struct aw_device *aw_dev)
{
	int ret = -1;
	unsigned char i = 0;
	uint16_t reg_val = 0;
	unsigned int check_value = 0;
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;
	struct aw_noise_gate_en *noise_gate_desc = &aw_dev->noise_gate_en;

	check_value = desc->st_check;

	if (noise_gate_desc->reg != AW_REG_NONE) {
		aw_dev->ops.aw_reg_read(aw_dev, noise_gate_desc->reg, &reg_val);
		if (reg_val & (~noise_gate_desc->noise_gate_mask)) {
			check_value = desc->st_check;
		} else {
			check_value = desc->st_sws_check;
		}
	}

	for (i = 0; i < AW_DEV_SYSST_CHECK_MAX; i++) {
		aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
		if (((reg_val & (~desc->st_mask)) & check_value) == check_value) {
			ret = 0;
			break;
		} else {
			aw_dev_dbg(aw_dev->dev, "check fail, cnt=%d, reg_val=0x%04x",
				i, reg_val);
			AW_MS_DELAY(AW_2_MS);
		}
	}
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "check fail");
	}
	return ret;
}

static int aw_dev_get_monitor_sysint_st(struct aw_device *aw_dev)
{
	int ret = 0;
	struct aw_int_desc *desc = &aw_dev->int_desc;

	if ((desc->intst_mask) & (desc->sysint_st)) {
		aw_dev_err(aw_dev->dev,
			"monitor check fail:0x%04x", desc->sysint_st);
		ret = -EINVAL;
	}
	desc->sysint_st = 0;

	return ret;
}

static int aw_dev_sysint_check(struct aw_device *aw_dev)
{
	int ret = 0;
	uint32_t reg_val = 0;
	struct aw_int_desc *desc = &aw_dev->int_desc;

	ret = aw_dev_get_int_status(aw_dev, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get int status fail ret:%d", ret);
		return ret;
	}

	if (reg_val & (desc->intst_mask)) {
		aw_dev_err(aw_dev->dev, "int check fail:0x%04x", reg_val);
		ret = -EINVAL;
	}

	return ret;
}

static void aw_dev_get_cur_mode_st(struct aw_device *aw_dev)
{
	uint16_t reg_val;
	struct aw_profctrl_desc *profctrl_desc = &aw_dev->profctrl_desc;

	aw_dev->ops.aw_reg_read(aw_dev, aw_dev->pwd_desc.reg, &reg_val);

	if ((reg_val & (~profctrl_desc->mask)) == profctrl_desc->rcv_mode_val) {
		profctrl_desc->cur_mode = AW_RCV_MODE;
	} else {
		profctrl_desc->cur_mode = AW_NOT_RCV_MODE;
	}
}

static int aw_dev_set_intmask(struct aw_device *aw_dev, uint32_t flag)
{
	int ret = -1;
	struct aw_int_desc *desc = &aw_dev->int_desc;

	if (flag == AW_DEV_UNMASK_INT_VAL) {
		ret = aw_dev->ops.aw_reg_write(aw_dev, desc->mask_reg,
					desc->int_mask);
	} else if (flag == AW_DEV_MASK_INT_VAL) {
		ret = aw_dev->ops.aw_reg_write(aw_dev, desc->mask_reg,
					desc->mask_default);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported flag : %d", flag);
		return -EINVAL;
	}

	aw_dev_info(aw_dev->dev, "done");
	return ret;
}

static int aw_dev_dsp_enable(struct aw_device *aw_dev, uint32_t dsp)
{
	int ret = -1;
	struct aw_dsp_en_desc *desc = &aw_dev->dsp_en_desc;

	if (dsp == AW_DEV_DSP_WORK) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->enable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "enable dsp failed");
			return ret;
		}
	} else if (dsp == AW_DEV_DSP_BYPASS) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->disable);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "disable dsp failed");
			return ret;
		}
	} else {
		aw_dev_err(aw_dev->dev, "unsupported flag : %d", dsp);
		return -EINVAL;
	}

	aw_dev_info(aw_dev->dev, "done");
	return 0;
}

static void aw_dev_memclk_select(struct aw_device *aw_dev, unsigned char flag)
{
	struct aw_memclk_desc *desc = &aw_dev->memclk_desc;
	int ret = -1;

	if (flag == AW_DEV_MEMCLK_PLL) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->mcu_hclk);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "memclk select pll failed");
		}
	} else if (flag == AW_DEV_MEMCLK_OSC) {
		ret = aw_dev->ops.aw_reg_write_bits(aw_dev, desc->reg,
					desc->mask, desc->osc_clk);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "memclk select OSC failed");
		}
	} else {
		aw_dev_err(aw_dev->dev, "unknown memclk config, flag=0x%x", flag);
	}

	aw_dev_info(aw_dev->dev, "done");
}

static int aw_dev_get_dsp_status(struct aw_device *aw_dev)
{
	int i = -1;
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_watch_dog_desc *desc = &aw_dev->watch_dog_desc;

	aw_dev_info(aw_dev->dev, "enter");

	for (i = 0; i < AW_DEV_DSP_CHECK_MAX; i++) {
		aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &reg_val);
		if (reg_val & (~desc->mask)) {
			return 0;
		}
		AW_MS_DELAY(AW_1_MS);
	}

	return ret;
}

static bool aw_dev_get_pll_wdt_status(struct aw_device *aw_dev)
{
	int ret = -1;

	ret = aw_dev_syspll_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "pll abnormal");
		return false;
	}

	ret = aw_dev_get_dsp_status(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dsp not work");
		return false;
	}
	return true;
}

static int aw_dev_get_cali_f0_delay(struct aw_device *aw_dev)
{
	struct aw_cali_delay_desc *desc = &aw_dev->cali_desc.cali_delay_desc;
	uint32_t cali_delay = 0;
	int ret = -1;

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			desc->dsp_reg, &cali_delay, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read cali delay failed, ret=%d", ret);
		return ret;
	}

	desc->delay = AW_CALI_DELAY_CACL(cali_delay);
	aw_dev_info(aw_dev->dev, "read cali delay: %d ms", desc->delay);

	return 0;
}

/******************************************************
 *
 * aw_dev update cfg
 *
 ******************************************************/

static int aw_dev_get_crc_flag(struct aw_device *aw_dev)
{
	return aw_dev->dsp_crc_st;

}

static int aw_dev_set_crc_flag(struct aw_device *aw_dev, uint32_t status)
{
	aw_dev->dsp_crc_st = status;
	return 0;
}

static int aw_dev_cfg_crc_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val;
	uint16_t check_val;
	uint16_t cfg_len_val;
	struct aw_crc_check_desc *crc_check_desc = &aw_dev->crc_check_desc;

	/*calculate cfg_end_addr*/
	cfg_len_val = ((aw_dev->dsp_cfg_len / AW_FW_ADDR_LEN) - 1) +
			crc_check_desc->crc_cfg_base_addr;
	aw_dev_info(aw_dev->dev, "cfg_end_addr 0x%x", cfg_len_val);

	/*write cfg_end_addr to crc_end_addr*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->crc_ctrl_reg,
		crc_check_desc->crc_end_addr_mask, cfg_len_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write cfg_len_val failed");
		return ret;
	}

	/*enable cfg crc check*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->crc_ctrl_reg,
		crc_check_desc->crc_cfg_check_en_mask, crc_check_desc->crc_cfgcheck_enable);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write crc_check_enable failed");
		return ret;
	}

	AW_MS_DELAY(AW_1_MS);
	/*read crc check result*/
	ret = aw_dev->ops.aw_reg_read(aw_dev, crc_check_desc->crc_check_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read crc_check_reg failed");
		return ret;
	}

	check_val = (reg_val & (~crc_check_desc->crc_check_mask)) >> crc_check_desc->crc_check_bits_shift;

	/*disable cfg crc check*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->crc_ctrl_reg,
		crc_check_desc->crc_cfg_check_en_mask, crc_check_desc->crc_cfgcheck_disable);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write crc_check_disable failed");
		return ret;
	}

	/*compare crc check result and pass value*/
	if (check_val != crc_check_desc->crc_check_pass) {
		aw_dev_err(aw_dev->dev, "crc_check failed,check val %x != %x", check_val,
			crc_check_desc->crc_check_pass);
		return -EINVAL;
	}

	return 0;
}

static int aw_dev_fw_crc_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val;
	uint16_t check_val;
	uint16_t fw_len_val;
	struct aw_crc_check_desc *crc_check_desc = &aw_dev->crc_check_desc;

	/*calculate fw_end_addr*/
	fw_len_val = ((aw_dev->dsp_fw_len / AW_FW_ADDR_LEN) - 1) +
			crc_check_desc->crc_fw_base_addr;

	aw_dev_info(aw_dev->dev, "fw_end_addr 0x%x", fw_len_val);

	/*write fw_end_addr to crc_end_addr*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->crc_ctrl_reg,
		crc_check_desc->crc_end_addr_mask, fw_len_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write fw_len_val failed");
		return ret;
	}
	/*enable fw crc check*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->crc_ctrl_reg,
		crc_check_desc->crc_fw_check_en_mask, crc_check_desc->crc_fwcheck_enable);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write crc_check_enable failed");
		return ret;
	}

	AW_MS_DELAY(AW_2_MS);
	/*read crc check result*/
	ret = aw_dev->ops.aw_reg_read(aw_dev, crc_check_desc->crc_check_reg, &reg_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read crc_check_reg failed");
		return ret;
	}

	check_val = (reg_val & (~crc_check_desc->crc_check_mask)) >> crc_check_desc->crc_check_bits_shift;

	/*disable fw crc check*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->crc_ctrl_reg,
		crc_check_desc->crc_fw_check_en_mask, crc_check_desc->crc_fwcheck_disable);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write crc_check_disable failed");
		return ret;
	}

	/*compare crc check result and pass value*/
	if (check_val != crc_check_desc->crc_check_pass) {
		aw_dev_err(aw_dev->dev, "crc_check failed,check val %x != %x", check_val,
			crc_check_desc->crc_check_pass);
		return -EINVAL;
	}

	return 0;
}

static int aw_dev_hw_crc_check(struct aw_device *aw_dev)
{
	int ret = -1;
	struct aw_crc_check_desc *crc_check_desc = &aw_dev->crc_check_desc;

	/*force RAM clock on*/
	ret = aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->ram_clk_reg,
		crc_check_desc->ram_clk_mask, crc_check_desc->ram_clk_on);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write ram_clk_on failed");
		return ret;
	}

	ret = aw_dev_fw_crc_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, " fw_crc_check failed");
		goto ram_clk_off;
	}

	ret = aw_dev_cfg_crc_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, " cfg_crc_check failed");
		goto ram_clk_off;
	}

	/*write bin value to crc reg when fw & cfg crc check pass*/
	ret = aw_dev->ops.aw_reg_write(aw_dev, crc_check_desc->crc_ctrl_reg, crc_check_desc->crc_init_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "write crc_init_val failed");
		goto ram_clk_off;
	}

	/*disable force RAM clock*/
	aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->ram_clk_reg,
		crc_check_desc->ram_clk_mask, crc_check_desc->ram_clk_off);

	return 0;
ram_clk_off:
	aw_dev->ops.aw_reg_write_bits(aw_dev, crc_check_desc->ram_clk_reg,
		crc_check_desc->ram_clk_mask, crc_check_desc->ram_clk_off);
	return ret;
}

static int aw_dev_crc_check(struct aw_device *aw_dev)
{
	int ret = -1;

	if (aw_dev_get_crc_flag(aw_dev) == AW_DSP_CRC_BYPASS) {
		aw_dev_info(aw_dev->dev, "CRC Bypass in driver debug process");
		return 0;
	}
	if (aw_dev->crc_type == AW_SW_CRC_CHECK) {
		if (aw_dev->ops.aw_sw_crc_check) {
			ret = aw_dev->ops.aw_sw_crc_check(aw_dev);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "dsp crc check failed");
				return ret;
			}
		} else {
			aw_dev_err(aw_dev->dev, "sw_crc_check ops is NULL");
			return ret;
		}
	} else if (aw_dev->crc_type == AW_HW_CRC_CHECK) {
		ret = aw_dev_hw_crc_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "hw_crc_check failed");
			return ret;
		}
	}
	return 0;
}

#ifdef AW_MONITOR
static void aw_dev_init_vmax_update(struct aw_device *aw_dev, backup_sec_t flag)
{
	if (flag == AW_RECOVERY_SEC_DATA) {
		aw883xx_monitor_update_st(aw_dev, AW_SET_INIT_VMAX);
	} else if (flag == AW_RECORD_SEC_DATA) {
		aw883xx_monitor_update_st(aw_dev, AW_GET_INIT_VMAX);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported type:%d", flag);
		return;
	}
}
#endif

static int aw_dev_init_vcalb_update(struct aw_device *aw_dev, backup_sec_t flag)
{
	int ret = -1;
	struct aw_vcalb_desc *vcalb_desc = &aw_dev->vcalb_desc;

	if (aw_dev->ops.aw_init_vcalb_update) {
		ret = aw_dev->ops.aw_init_vcalb_update(aw_dev, flag);
		if (ret < 0)
			aw_dev_err(aw_dev->dev, "write vcalb_reg failed");
		return ret;
	}

	if (flag == AW_RECOVERY_SEC_DATA) {
		ret = aw_dev->ops.aw_reg_write(aw_dev, vcalb_desc->vcalb_reg, vcalb_desc->init_value);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write vcalb_reg failed");
			return ret;
		}
	} else if (flag == AW_RECORD_SEC_DATA) {
		ret = aw_dev->ops.aw_reg_read(aw_dev, vcalb_desc->vcalb_reg, &vcalb_desc->init_value);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "write vcalb_reg failed");
			return ret;
		}
	} else {
		aw_dev_err(aw_dev->dev, "unsupported type:%d", flag);
		return -EINVAL;
	}

	return 0;
}

static int aw_dev_get_firmware_ver(struct aw_device *aw_dev)
{
	int ret = -1;
	struct aw_fw_ver_desc *fw_ver_desc = &aw_dev->fw_ver_desc;

	ret = aw_dev->ops.aw_dsp_read(aw_dev,
			fw_ver_desc->version_reg, &aw_dev->fw_ver, fw_ver_desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read firmware version failed, ret=%d", ret);
		return ret;
	}

	return 0;
}

/*pwd enable update reg*/

static int aw_dev_reg_container_update(struct aw_device *aw_dev,
				const uint8_t *data, uint32_t len)
{
	int i = 0;
	int ret = -1;
	uint8_t reg_addr = 0;
	uint16_t reg_val = 0;
	uint16_t read_vol = 0;
	struct aw_int_desc *int_desc = &aw_dev->int_desc;
	struct aw_volume_desc *vol_desc = &aw_dev->volume_desc;
	uint16_t *reg_data = NULL;
	int data_len;

	aw_dev_dbg(aw_dev->dev, "enter");
	reg_data = (uint16_t *)data;
	data_len = len >> 1;

	if (data_len % 2 != 0) {
		aw_dev_err(aw_dev->dev, "data len:%d unsupported",
				data_len);
		return -EINVAL;
	}

	for (i = 0; i < data_len; i += 2) {
		reg_addr = reg_data[i];
		reg_val = reg_data[i + 1];
		aw_dev_dbg(aw_dev->dev, "reg = 0x%02x, val = 0x%04x",
				reg_addr, reg_val);
		if (reg_addr == int_desc->mask_reg) {
			int_desc->int_mask = reg_val;
			reg_val = int_desc->mask_default;
		}
		if (reg_addr == aw_dev->mute_desc.reg) {
			reg_val &= aw_dev->mute_desc.mask;
			reg_val |= aw_dev->mute_desc.enable;
		}

		if (reg_addr == aw_dev->pwd_desc.reg) {
			reg_val &= aw_dev->pwd_desc.mask;
			reg_val |= aw_dev->pwd_desc.enable;
		}

		if (reg_addr == aw_dev->txen_desc.reg) {
			/*get bin value*/
			aw_dev->txen_st = reg_val & (~aw_dev->txen_desc.mask);
			aw_dev_dbg(aw_dev->dev, "txen_st=0x%04x", aw_dev->txen_st);

			reg_val &= aw_dev->txen_desc.mask;
			reg_val |= aw_dev->txen_desc.disable;
		}

		if (reg_addr == aw_dev->dsp_crc_desc.ctl_reg) {
			reg_val &= aw_dev->dsp_crc_desc.ctl_mask;
		}

		if (reg_addr == aw_dev->volume_desc.reg) {
			read_vol = (reg_val & (~aw_dev->volume_desc.mask)) >>
				aw_dev->volume_desc.shift;
			aw_dev->volume_desc.init_volume =
				aw_dev->ops.aw_reg_val_to_db(read_vol);
		}

		if (reg_addr == aw_dev->efuse_check_desc.reg) {
			if (reg_val & (~aw_dev->efuse_check_desc.mask) ==
						aw_dev->efuse_check_desc.or_val)
				aw_dev->efuse_check_desc.check_val = AW_EF_OR_CHECK;
			else
				aw_dev->efuse_check_desc.check_val = AW_EF_AND_CHECK;
		}

		if (reg_addr == aw_dev->crc_check_desc.crc_ctrl_reg)
			aw_dev->crc_check_desc.crc_init_val = reg_val;

		if (reg_addr == aw_dev->vcalb_desc.vcalb_reg) {
			aw_dev->vcalb_desc.init_value = reg_val;
			continue;
		}

		if (reg_addr == aw_dev->cali_desc.hw_cali_re_desc.hbits_reg) {
			aw_dev->cali_desc.hw_cali_re_desc.re_hbits = reg_val;
			continue;
		}

		if (reg_addr == aw_dev->cali_desc.hw_cali_re_desc.lbits_reg) {
			aw_dev->cali_desc.hw_cali_re_desc.re_lbits = reg_val;
			continue;
		}

		if (reg_addr == aw_dev->dsp_en_desc.reg) {
			if (reg_val & (~aw_dev->dsp_en_desc.mask))
				aw_dev->dsp_cfg = AW_DEV_DSP_BYPASS;
			else
				aw_dev->dsp_cfg = AW_DEV_DSP_WORK;
			reg_val &= aw_dev->dsp_en_desc.mask;
			reg_val |= aw_dev->dsp_en_desc.disable;
		}

		if(reg_addr == aw_dev->dither_desc.reg) {
			aw_dev->dither_st = reg_val & (~aw_dev->dither_desc.mask);
			aw_dev_info(aw_dev->dev, "dither_st=0x%04x", aw_dev->dither_st);
		}

		ret = aw_dev->ops.aw_reg_write(aw_dev, reg_addr, reg_val);
		if (ret < 0) {
			break;
		}
	}

	aw_dev_pwd(aw_dev, false);
	AW_MS_DELAY(AW_1_MS);;

	aw_dev_get_cur_mode_st(aw_dev);

	if ((strncmp(aw_dev->cur_prof_name, aw_dev->set_prof_name, AW_PROF_NAME_MAX) != 0)) {
		/*clear control volume when PA change profile*/
		vol_desc->ctl_volume = 0;
	} else {
		/*keep control volume when PA start with sync mode*/
		aw_dev_set_volume(aw_dev, vol_desc->ctl_volume);
	}

	/*keep min volume*/
	if (aw_dev->fade_en) {
		aw_dev_set_volume(aw_dev, vol_desc->mute_volume);
	}

#ifdef AW_MONITOR
	aw883xx_monitor_update_st(aw_dev, AW_HW_MONITOR_ST);
#endif

	aw_dev_dbg(aw_dev->dev, "exit");

	return ret;
}


static int aw_dev_reg_update(struct aw_device *aw_dev)
{
	struct aw_sec_data_desc *reg_data = NULL;
	reg_data = aw883xx_dev_get_prof_data_byname(aw_dev, aw_dev->set_prof_name, AW_DATA_TYPE_REG);

	if (reg_data == NULL){
		aw_dev_err(aw_dev->dev, "reg data is null");
		return -EINVAL;
	}

	aw_dev_reg_container_update(aw_dev, reg_data->data, reg_data->len);

	aw_dev_dbg(aw_dev->dev, "reg len:%d", reg_data->len);

	return 0;
}

static int aw_dev_dsp_container_update(struct aw_device *aw_dev,
			const uint8_t *dsp_data, uint32_t len, uint16_t base)
{
	int i;
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;
	uint8_t *data = NULL;
	data = (uint8_t *)dsp_data;

#ifdef AW_DSP_I2C_WRITES
	uint32_t tmp_len = 0;
#else
	uint16_t reg_val = 0;
#endif

	aw_dev_dbg(aw_dev->dev, "enter");
	aw_mutex_lock();
#ifdef AW_DSP_I2C_WRITES
	/* i2c writes */
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg, base);

	for (i = 0; i < len; i += AW_MAX_RAM_WRITE_BYTE_SIZE) {
		if ((len - i) < AW_MAX_RAM_WRITE_BYTE_SIZE) {
			tmp_len = len - i;
		} else {
			tmp_len = AW_MAX_RAM_WRITE_BYTE_SIZE;
		}
		aw_dev->ops.aw_i2c_writes(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					&data[i], tmp_len);
	}

#else
	/* i2c write */
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg, base);

	for (i = 0; i < len; i += 2) {
		reg_val = (data[i] << 8) + data[i + 1];
		aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					reg_val);
	}
#endif

	aw_mutex_unlock();
	aw_dev_dbg(aw_dev->dev, "exit");

	return 0;
}

static int aw_dev_dsp_fw_update(struct aw_device *aw_dev)
{
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;
	struct aw_sec_data_desc *dsp_fw_data = NULL;

	dsp_fw_data = aw883xx_dev_get_prof_data_byname(aw_dev, aw_dev->set_prof_name, AW_DATA_TYPE_DSP_FW);

	if (dsp_fw_data == NULL) {
		aw_dev_err(aw_dev->dev, "dsp firmware data is null");
		return -EINVAL;
	}

	aw_dev_dsp_container_update(aw_dev, dsp_fw_data->data,
					dsp_fw_data->len, dsp_mem_desc->dsp_fw_base_addr);
	aw_dev->dsp_fw_len = dsp_fw_data->len;
	aw_dev_dbg(aw_dev->dev, "dsp firmware len:%d", dsp_fw_data->len);

	return 0;
}

static int aw_dev_crc_realtime_get(struct aw_device *aw_dev, uint32_t *status)
{
	int ret = -1;
	struct aw_crc_check_realtime_desc *desc =&aw_dev->crc_check_realtime_desc;
	uint32_t crc_realtime_val = 0;

	ret = aw_dev->ops.aw_dsp_read(aw_dev, desc->addr,
			&crc_realtime_val, desc->data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get crc_realtime failed");
		return ret;
	}

	if (crc_realtime_val & (~desc->mask))
		*status = aw_dev->crc_check_realtime_desc.enable;
	else
		*status = aw_dev->crc_check_realtime_desc.disable;

	aw_dev_dbg(aw_dev->dev, "get %d", *status);

	return 0;
}

static int aw_dev_crc_realtime_set(struct aw_device *aw_dev, uint32_t enable)
{
	int ret = -1;
	struct aw_crc_check_realtime_desc *crc_desc =&aw_dev->crc_check_realtime_desc;

	aw_dev_dbg(aw_dev->dev, "set %d", enable);

	if (enable) {
		ret = aw_dev->ops.aw_dsp_write_bits(aw_dev, crc_desc->addr,
				crc_desc->mask, crc_desc->enable, crc_desc->data_type);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "set enable failed");
			return ret;
		}
	} else {
		ret = aw_dev->ops.aw_dsp_write_bits(aw_dev, crc_desc->addr,
				crc_desc->mask, crc_desc->disable, crc_desc->data_type);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "set disable failed");
			return ret;
		}
	}

	return 0;
}

int aw883xx_crc_realtime_check(struct aw_device *aw_dev, uint32_t *status)
{
	int ret = -1;
	uint16_t sysst_val;
	uint32_t crc_check_val;
	struct aw_sysst_desc *desc = &aw_dev->sysst_desc;
	struct aw_int_desc *int_desc = &aw_dev->int_desc;
	struct aw_crc_check_realtime_desc *crc_desc =&aw_dev->crc_check_realtime_desc;

	if (aw_dev_get_crc_flag(aw_dev) == AW_DSP_CRC_BYPASS) {
		int_desc->int_mask = int_desc->mask_default;
		aw_dev_info(aw_dev->dev, "CRC Bypass in driver debug process");
		return 0;
	}

	ret = aw_dev->ops.aw_reg_read(aw_dev, desc->reg, &sysst_val);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "read reg0x%x failed", desc->reg);
		return ret;
	}

	ret = aw_dev->ops.aw_dsp_read(aw_dev, crc_desc->check_addr,
			&crc_check_val, crc_desc->check_data_type);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "get crc_check_val failed");
		return ret;
	}

	if (((sysst_val & desc->dsp_mask) != desc->dsp_check) &&
			(crc_check_val & (~crc_desc->check_mask)) ==
			aw_dev->crc_check_realtime_desc.check_abnormal) {
		*status = AW_REALTIME_CRC_CHECK_ABNORMAL;
	} else {
		*status = AW_REALTIME_CRC_CHECK_NORMAL;
	}

	aw_dev_dbg(aw_dev->dev, "check status %d", *status);

	return 0;
}

static void aw_dev_backup_sec_record(struct aw_device *aw_dev)
{
	aw_dev_init_vcalb_update(aw_dev, AW_RECORD_SEC_DATA);
	aw_dev_crc_realtime_get(aw_dev, &aw_dev->crc_check_realtime_desc.init_switch);
#ifdef AW_CALIB
	aw883xx_cali_init_re_update(aw_dev, AW_RECORD_SEC_DATA);
#endif
#ifdef AW_MONITOR
	aw_dev_init_vmax_update(aw_dev, AW_RECORD_SEC_DATA);
#endif
}

static void aw_dev_backup_sec_recovery(struct aw_device *aw_dev)
{
	aw_dev_init_vcalb_update(aw_dev, AW_RECOVERY_SEC_DATA);
#ifdef AW_CALIB
	aw883xx_cali_init_re_update(aw_dev, AW_RECOVERY_SEC_DATA);
#endif
#ifdef AW_MONITOR
	aw_dev_init_vmax_update(aw_dev, AW_RECOVERY_SEC_DATA);
#endif

}

static int aw_dev_dsp_cfg_update(struct aw_device *aw_dev)
{
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;
	struct aw_sec_data_desc *dsp_cfg_data = NULL;

	dsp_cfg_data = aw883xx_dev_get_prof_data_byname(aw_dev, aw_dev->set_prof_name, AW_DATA_TYPE_DSP_CFG);

	if (dsp_cfg_data == NULL) {
		aw_dev_err(aw_dev->dev, "dsp config data is null");
		return -EINVAL;
	}

	aw_dev_dsp_container_update(aw_dev,
			dsp_cfg_data->data, dsp_cfg_data->len, dsp_mem_desc->dsp_cfg_base_addr);
	aw_dev->dsp_cfg_len = dsp_cfg_data->len;
	aw_dev_dbg(aw_dev->dev, "dsp config len:%d", dsp_cfg_data->len);

	aw_dev_backup_sec_record(aw_dev);

#ifdef AW_CALIB
	aw883xx_cali_get_ra((void *)aw_dev);
	aw_dev_get_cali_f0_delay(aw_dev);
#endif

#ifdef AW_MONITOR
	aw883xx_monitor_update_st(aw_dev, AW_DSP_MONITOR_ST);
#endif

	aw_dev_get_firmware_ver(aw_dev);
	aw_dev_info(aw_dev->dev, "fw_ver: [%04x]", aw_dev->fw_ver);

	return 0;
}

static int aw_dev_sram_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t reg_val = 0;
	struct aw_dsp_mem_desc *dsp_mem_desc = &aw_dev->dsp_mem_desc;

	aw_mutex_lock();

	/*read dsp_rom_check_reg*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_rom_check_reg);
	aw_dev->ops.aw_i2c_read(aw_dev, dsp_mem_desc->dsp_mdat_reg, &reg_val);
	if (dsp_mem_desc->dsp_rom_check_data != reg_val) {
		aw_dev_err(aw_dev->dev, "check reg 0x40 failed, read[0x%x] does not match write[0x%x]",
				reg_val, dsp_mem_desc->dsp_rom_check_data);
		goto error;
	}

	/*write dsp_cfg_base_addr*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_cfg_base_addr);
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_mdat_reg,
					AW_DSP_ODD_NUM_BIT_TEST);

	/*read dsp_cfg_base_addr*/
	aw_dev->ops.aw_i2c_write(aw_dev, dsp_mem_desc->dsp_madd_reg,
					dsp_mem_desc->dsp_cfg_base_addr);
	aw_dev->ops.aw_i2c_read(aw_dev, dsp_mem_desc->dsp_mdat_reg, &reg_val);
	if (AW_DSP_ODD_NUM_BIT_TEST != reg_val) {
		aw_dev_err(aw_dev->dev, "check dsp cfg failed, read[0x%x] does not match write[0x%x]",
						reg_val, AW_DSP_ODD_NUM_BIT_TEST);
		goto error;
	}
	aw_mutex_unlock();
	return 0;

error:
	aw_mutex_unlock();
	return ret;
}

int aw883xx_device_fw_update(struct aw_device *aw_dev, bool up_dsp_fw_en, bool force_up_en)
{
	int ret = -1;

	aw_dev_info(aw_dev->dev, "enter ");

	if ((strncmp(aw_dev->cur_prof_name, aw_dev->set_prof_name, AW_PROF_NAME_MAX) == 0) &&
			(force_up_en == AW_FORCE_UPDATE_OFF)) {
		aw_dev_info(aw_dev->dev, "scene no change, not update");
		return 0;
	}

	/*update reg*/
	ret = aw_dev_reg_update(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "update reg failed");
		return ret;
	}

	aw_dev_set_hmute(aw_dev, AW_DEV_HMUTE_ENABLE);

	if (aw_dev->dsp_cfg == AW_DEV_DSP_WORK) {
		aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_BYPASS);
	}

	aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_OSC);

	ret = aw_dev_sram_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "check sram failed");
		goto error;
	}

	aw_dev_backup_sec_recovery(aw_dev);

	if (up_dsp_fw_en) {
		/*update dsp firmware*/
		ret = aw_dev_dsp_fw_update(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "update dsp fw failed");
			goto error;
		}
	}

	/*update dsp config*/
	ret = aw_dev_dsp_cfg_update(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "update dsp cfg failed");
		goto error;
	}

	aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_PLL);

	strncpy(aw_dev->cur_prof_name, aw_dev->set_prof_name, AW_PROF_NAME_MAX);

	aw_dev_info(aw_dev->dev, "load [%s] done", aw_dev->cur_prof_name);
	return 0;

error:
	aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_PLL);

	return ret;
}

static int aw883xx_dev_dsp_check(struct aw_device *aw_dev)
{
	int ret = -1;
	uint16_t i = 0;

	aw_dev_dbg(aw_dev->dev, "enter");

	if (aw_dev->dsp_cfg == AW_DEV_DSP_BYPASS) {
		aw_dev_dbg(aw_dev->dev, "dsp bypass");
		return 0;
	} else if (aw_dev->dsp_cfg == AW_DEV_DSP_WORK) {
		aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_BYPASS);
		aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_WORK);
		AW_MS_DELAY(AW_1_MS);
		for (i = 0; i < AW_DEV_DSP_CHECK_MAX; i++) {
			ret = aw_dev_get_dsp_status(aw_dev);
			if (ret < 0) {
				aw_dev_err(aw_dev->dev, "dsp wdt status error=%d", ret);
				AW_MS_DELAY(AW_2_MS);
			} else {
				return 0;
			}
		}
	} else {
		aw_dev_err(aw_dev->dev, "unknown dsp cfg=%d", aw_dev->dsp_cfg);
		return -EINVAL;
	}

	return -EINVAL;
}

static void aw_dev_set_dither(struct aw_device *aw_dev, bool dither)
{
	struct aw_dither_desc *dither_desc = &aw_dev->dither_desc;

	aw_dev_dbg(aw_dev->dev, "enter, dither: %d", dither);

	if (dither_desc->reg == AW_REG_NONE)
		return;

	if (dither) {
		aw_dev->ops.aw_reg_write_bits(aw_dev, dither_desc->reg,
				dither_desc->mask,
				dither_desc->enable);
	} else {
		aw_dev->ops.aw_reg_write_bits(aw_dev, dither_desc->reg,
				dither_desc->mask,
				dither_desc->disable);
	}

	aw_dev_info(aw_dev->dev, "done");
}

#ifdef AW_CALIB
static void aw_dev_cali_re_update(struct aw_device *aw_dev)
{
	if (aw_dev->cali_desc.cali_re < aw_dev->re_range.re_max &&
		aw_dev->cali_desc.cali_re > aw_dev->re_range.re_min) {
		aw883xx_set_cali_re((void *)aw_dev);
	} else {
		aw_dev_err(aw_dev->dev, "cali_re:%d is error, no set",
				aw_dev->cali_desc.cali_re);
	}
}

static int aw_dev_get_cali_msg(struct aw_device *aw_dev, void *cali_data, dev_params_t type)
{
	int ret = -1;
	struct cali_msg_hdr *msg_hdr = NULL;
	uint32_t cali_params[AW_DEV_MAX] = { 0 };
	aw_single_t is_single = IS_SINGLE;

	if (cali_data == NULL) {
		return -EINVAL;
	}
	msg_hdr = (struct cali_msg_hdr *)cali_data;
	is_single = (aw_single_t)msg_hdr->opcode_id;

	switch (type) {
	case AW_DEV_TE_PARAMS:
		ret = aw883xx_cali_get_te(aw_dev, is_single, (int32_t *)cali_params);
		break;
	case AW_DEV_F0_PARAMS:
		ret = aw883xx_cali_get_f0(aw_dev, is_single, cali_params);
		break;
	case AW_DEV_R0_PARAMS:
		ret = aw883xx_cali_get_r0(aw_dev, is_single, cali_params);
		break;
	case AW_DEV_RE_RANGE_PARAMS:
		ret = aw883xx_cali_get_re_range(aw_dev, is_single, cali_params);
		break;
	case AW_DEV_CALI_RE_PARAMS:
		ret = aw883xx_cali_re_show(aw_dev, is_single, cali_params);
		break;
	case AW_DEV_CALI_F0_PARAMS:
		ret = aw883xx_cali_f0_show(aw_dev, is_single, cali_params);
		break;
	case AW_DEV_CALI_Q_PARAMS:
		ret = aw883xx_cali_q_show(aw_dev, is_single, cali_params);
		break;
	case AW_DEV_CALI_TIME_PARAMS:
		return aw883xx_cali_show_cali_time((uint32_t *)cali_data);
	case AW_DEV_DSP_RE_PARAMS:
		return aw883xx_cali_read_re_from_dsp(aw_dev, (uint32_t *)cali_data);
	default:
		aw_dev_err(aw_dev->dev, "unsupported type:%d", type);
		return -EINVAL;
	}

	if (ret < 0) {
		return ret;
	}

	memcpy(msg_hdr->msg, cali_params, sizeof(cali_params));

	return 0;
}

static int aw_dev_set_cali_msg(struct aw_device *aw_dev, void *cali_data, dev_params_t type)
{
	struct cali_msg_hdr *msg_hdr = NULL;
	uint32_t *set_params = NULL;
	int ret = -1;

	if (cali_data == NULL) {
		return -EINVAL;
	}
	set_params = (uint32_t *)cali_data;

	switch (type) {
	case AW_DEV_CALI_RE_PARAMS:
		ret = aw883xx_cail_re_cali(aw_dev, (aw_single_t)set_params[0]);
		break;
	case AW_DEV_CALI_RE_F0_PARAMS:
		ret = aw883xx_cali_re_f0_cali(aw_dev, (aw_single_t)set_params[0]);
		break;
	case AW_DEV_CALI_F0_Q_PARAMS:
		ret = aw883xx_cali_f0_q_cali(aw_dev, (aw_single_t)set_params[0]);
		break;
	case AW_DEV_CALI_TIME_PARAMS:
		ret = aw883xx_cali_store_cali_time(set_params[0]);
		break;
	case AW_DEV_STORE_CALI_RE_PARAMS:
		msg_hdr = (struct cali_msg_hdr *)cali_data;
		ret = aw883xx_cali_store_cali_re(aw_dev, (aw_single_t)msg_hdr->opcode_id, &msg_hdr->msg[0]);
		break;
	default:
		aw_dev_err(aw_dev->dev, "unsupported type:%d", type);
		return -EINVAL;
	}

	return ret;
}
#endif

static int aw_dev_set_params(struct aw_device *aw_dev, dev_params_t type, void *params, uint8_t len)
{
	uint32_t *set_params = NULL;
	if ((params == NULL) || (len == 0)) {
		aw_dev_err(aw_dev->dev, "input params error");
		return -EINVAL;
	}

	set_params = (uint32_t *)params;

	switch (type) {
	case AW_DEV_VOLUME_PARAMS:
		return aw_dev_set_volume(aw_dev, set_params[0]);
#ifdef AW_FADE
	case AW_DEV_FADE_STEP_PARAMS:
		return aw_dev_set_fade_vol_step(aw_dev, set_params[0]);
	case AW_DEV_FADE_IN_TIME_PARAMS:
		return aw_dev_set_fade_time(set_params[0], AW_FADE_IN_TIME);
	case AW_DEV_FADE_OUT_TIME_PARAMS:
		return aw_dev_set_fade_time(set_params[0], AW_FADE_OUT_TIME);
#endif
	case AW_DEV_DSP_PARAMS:
		return aw_dev_dsp_enable(aw_dev, set_params[0]);
	case AW_DEV_HMUTE_PARAMS:
		return aw_dev_set_hmute(aw_dev, set_params[0]);
	case AW_DEV_INT_PARAMS:
		return aw_dev_set_intmask(aw_dev, set_params[0]);
	case AW_DEV_CRC_FLAG_PARAMS:
		return aw_dev_set_crc_flag(aw_dev, set_params[0]);
	case AW_DEV_REALTIME_CRC_SET_PARAMS:
		return aw_dev_crc_realtime_set(aw_dev, set_params[0]);
#ifdef AW_CALIB
	case AW_DEV_CALI_RE_PARAMS:
	case AW_DEV_CALI_RE_F0_PARAMS:
	case AW_DEV_CALI_F0_Q_PARAMS:
	case AW_DEV_CALI_TIME_PARAMS:
	case AW_DEV_STORE_CALI_RE_PARAMS:
		return aw_dev_set_cali_msg(aw_dev, params, type);
#endif
	default:
		aw_dev_err(aw_dev->dev, "unsupported type:%d", type);
		return -EINVAL;
	}
}

static int aw_dev_get_params(struct aw_device *aw_dev, dev_params_t type, void *params, uint8_t len)
{
	int ret = -1;
	if ((params == NULL) || (len == 0)) {
		aw_dev_err(aw_dev->dev, "input params error");
		return -EINVAL;
	}

	switch (type) {
	case AW_DEV_VOLUME_PARAMS:
		ret = aw_dev_get_volume(aw_dev, params);
		break;
#ifdef AW_FADE
	case AW_DEV_FADE_STEP_PARAMS:
		ret = aw_dev_get_fade_vol_step(aw_dev, params);
		break;
	case AW_DEV_FADE_IN_TIME_PARAMS:
		ret = aw_dev_get_fade_time(params, AW_FADE_IN_TIME);
		break;
	case AW_DEV_FADE_OUT_TIME_PARAMS:
		ret = aw_dev_get_fade_time(params, AW_FADE_OUT_TIME);
		break;
#endif
	case AW_DEV_INT_PARAMS:
		ret = aw_dev_get_int_status(aw_dev, params);
		break;
	case AW_DEV_HMUTE_PARAMS:
		ret = aw_dev_get_hmute(aw_dev, params);
		break;
	case AW_DEV_SYSST_PARAMS:
		ret = aw_dev_get_sysst_value(aw_dev, params);
		break;
	case AW_DEV_REALTIME_CRC_GET_PARAMS:
		ret = aw_dev_crc_realtime_get(aw_dev, params);
		break;
#ifdef AW_CALIB
	case AW_DEV_TE_PARAMS:
	case AW_DEV_F0_PARAMS:
	case AW_DEV_R0_PARAMS:
	case AW_DEV_RE_RANGE_PARAMS:
	case AW_DEV_CALI_RE_PARAMS:
	case AW_DEV_CALI_F0_PARAMS:
	case AW_DEV_CALI_Q_PARAMS:
	case AW_DEV_CALI_TIME_PARAMS:
	case AW_DEV_DSP_RE_PARAMS:
		ret = aw_dev_get_cali_msg(aw_dev, params, type);
		break;
#endif
	default:
		aw_dev_err(aw_dev->dev, "unsupported type:%d", type);
		return -EINVAL;
	}

	return ret;
}

int aw883xx_device_params(struct aw_device *aw_dev, dev_params_t params_type,
						void *data, uint8_t size, params_option_t params_ops)
{
	if (params_ops == AW_SET_DEV_PARAMS) {
		return aw_dev_set_params(aw_dev, params_type, data, size);
	} else if (params_ops == AW_GET_DEV_PARAMS) {
		return aw_dev_get_params(aw_dev, params_type, data, size);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported params_ops:%d", params_ops);
		return -EINVAL;
	}
}

static bool aw_dev_get_status(struct aw_device *aw_dev, dev_status_t type)
{
	switch (type) {
	case AW_DEV_PLL_WDT_STATUS:
		return aw_dev_get_pll_wdt_status(aw_dev);
	default:
		aw_dev_err(aw_dev->dev, "unsupported type:%d", type);
		return false;
	}
}

static void aw_dev_set_status(struct aw_device *aw_dev, dev_status_t type)
{
	switch (type) {
#ifdef AW_IRQ
	case AW_DEV_INTERRUPT_CLEAR_STATUS:
		aw_dev_interrupt_clear(aw_dev);
		break;
#endif
	case AW_DEV_CLEAR_INT_STATUS:
		aw_dev_clear_int_status(aw_dev);
		break;
	case AW_DEV_SOFT_RESET_STATUS:
		aw_dev_soft_reset(aw_dev);
		break;
	case AW_DEV_REG_DUMP_STATUS:
		aw883xx_dev_reg_dump(aw_dev);
		break;
	default:
		aw_dev_err(aw_dev->dev, "unsupported type:%d", type);
		return;
	}
}

bool aw883xx_device_status(struct aw_device *aw_dev, dev_status_t type, status_option_t status_ops)
{
	if (status_ops == AW_SET_DEV_STATUS) {
		aw_dev_set_status(aw_dev, type);
		return true;
	} else if (status_ops == AW_GET_DEV_STATUS) {
		return aw_dev_get_status(aw_dev, type);
	} else {
		aw_dev_err(aw_dev->dev, "unsupported status_ops:%d", status_ops);
		return false;
	}
}

int aw883xx_device_start(struct aw_device *aw_dev)
{
	int ret = -1;
	struct aw_dither_desc *dither_desc = &aw_dev->dither_desc;

	aw_dev_info(aw_dev->dev, "enter");

	if (aw_dev->status == AW_DEV_PW_ON) {
		aw_dev_info(aw_dev->dev, "already power on");
		return 0;
	}

	aw_dev_set_dither(aw_dev, false);

	/*power on*/
	aw_dev_pwd(aw_dev, false);
	AW_MS_DELAY(AW_2_MS);

	ret = aw_dev_syspll_check(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "pll check failed cannot start");
		aw883xx_dev_reg_dump(aw_dev);
		goto pll_check_fail;
	}

	/*amppd on*/
	aw_dev_amppd(aw_dev, false);
	AW_MS_DELAY(AW_1_MS);

	/*check i2s status*/
	ret = aw883xx_dev_sysst_check(aw_dev);
	if (ret < 0) {
		/*check failed*/
		aw883xx_dev_reg_dump(aw_dev);
		goto sysst_check_fail;
	}

	if (aw_dev->dsp_cfg == AW_DEV_DSP_WORK) {
		aw_dev_backup_sec_recovery(aw_dev);
		ret = aw_dev_crc_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "crc check failed");
			aw883xx_dev_reg_dump(aw_dev);
			goto crc_check_fail;
		}

		/*dsp bypass*/
		aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_BYPASS);

		aw_dev_set_vcalb(aw_dev);
#ifdef AW_CALIB
		aw_dev_cali_re_update(aw_dev);
#endif

		ret = aw883xx_dev_dsp_check(aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "check dsp status failed");
			aw883xx_dev_reg_dump(aw_dev);
			goto dsp_check_fail;
		}
	} else {
		aw_dev_dbg(aw_dev->dev, "start pa with dsp bypass");
	}

	/*enable tx feedback*/
	if (aw_dev->ops.aw_i2s_tx_enable) {
		if (aw_dev->txen_st) {
			aw_dev->ops.aw_i2s_tx_enable(aw_dev, true);
		}
	}

	if(aw_dev->dither_st == dither_desc->enable) {
		aw_dev_set_dither(aw_dev, true);
	}

	/*close mute*/
	if (aw883xx_cali_check_result(&aw_dev->cali_desc)) {
		aw_dev_set_hmute(aw_dev, AW_DEV_HMUTE_DISABLE);
	} else {
		aw_dev_set_hmute(aw_dev, AW_DEV_HMUTE_ENABLE);
	}
	/*clear inturrupt*/
	aw_dev_clear_int_status(aw_dev);
	/*set inturrupt mask*/
	aw_dev_set_intmask(aw_dev, AW_DEV_UNMASK_INT_VAL);

	if (aw_dev->ops.aw_monitor_start) {
		aw_dev->ops.aw_monitor_start((void *)aw_dev);
	}

	aw_dev->status = AW_DEV_PW_ON;

	aw_dev_info(aw_dev->dev, "done");

	return 0;

dsp_check_fail:
crc_check_fail:
	aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_BYPASS);
sysst_check_fail:
	/*clear interrupt*/
	aw_dev_clear_int_status(aw_dev);
	aw_dev_amppd(aw_dev, true);
pll_check_fail:
	aw_dev_pwd(aw_dev, true);
	aw_dev->status = AW_DEV_PW_OFF;
	return ret;
}

int aw883xx_device_stop(struct aw_device *aw_dev)
{
	int int_st = 0;
	int monitor_int_st = 0;
	uint16_t reg_data = 0;

	aw_dev_info(aw_dev->dev, "enter");

	aw_dev->ops.aw_reg_read(aw_dev, aw_dev->pwd_desc.reg, &reg_data);

	if ((aw_dev->status == AW_DEV_PW_OFF) &&
		(reg_data & aw_dev->pwd_desc.enable)) {
		aw_dev_info(aw_dev->dev, "already power off");
		return 0;
	}

	aw_dev->status = AW_DEV_PW_OFF;

	if (aw_dev->ops.aw_monitor_stop) {
		aw_dev->ops.aw_monitor_stop((void *)aw_dev);
	}

	/*set mute*/
	aw_dev_set_hmute(aw_dev, AW_DEV_HMUTE_ENABLE);
	AW_MS_DELAY(AW_4_MS);

	/*close tx feedback*/
	if (aw_dev->ops.aw_i2s_tx_enable) {
		aw_dev->ops.aw_i2s_tx_enable(aw_dev, false);
	}
	AW_MS_DELAY(AW_1_MS);

	/*set defaut int mask*/
	aw_dev_set_intmask(aw_dev, AW_DEV_MASK_INT_VAL);

	/*check sysint state*/
	int_st = aw_dev_sysint_check(aw_dev);

	/*close dsp*/
	aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_BYPASS);

	/*enable amppd*/
	aw_dev_amppd(aw_dev, true);

	/*check monitor process sysint state*/
	monitor_int_st = aw_dev_get_monitor_sysint_st(aw_dev);

	if (int_st < 0 || monitor_int_st < 0) {
		/*system status anomaly*/
		aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_OSC);
		aw_dev_sram_check(aw_dev);
		aw_dev_dsp_fw_update(aw_dev);
		aw_dev_dsp_cfg_update(aw_dev);
		aw_dev_memclk_select(aw_dev, AW_DEV_MEMCLK_PLL);
	}

	/*set power down*/
	aw_dev_pwd(aw_dev, true);

	aw_dev_info(aw_dev->dev, "done");
	return 0;
}

static int aw_device_init(struct aw_device *aw_dev)
{
	int ret = -1;

	if (aw_dev == NULL) {
		aw_pr_err("aw_dev is NULL");
		return -ENOMEM;
	}

	strncpy(aw_dev->cur_prof_name, aw_dev->first_prof_name, AW_PROF_NAME_MAX);
	strncpy(aw_dev->set_prof_name, aw_dev->first_prof_name, AW_PROF_NAME_MAX);

	ret = aw883xx_device_fw_update(aw_dev, AW_FORCE_UPDATE_ON,
			AW_DSP_FW_UPDATE_ON);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "fw update failed");
		return ret;
	}

	aw_dev_set_intmask(aw_dev, AW_DEV_MASK_INT_VAL);

	/*set mute*/
	aw_dev_set_hmute(aw_dev, AW_DEV_HMUTE_ENABLE);

	/*close tx feedback*/
	if (aw_dev->ops.aw_i2s_tx_enable) {
		aw_dev->ops.aw_i2s_tx_enable(aw_dev, false);
	}
	AW_MS_DELAY(AW_1_MS);

	/*enable amppd*/
	aw_dev_amppd(aw_dev, true);
	/*close dsp*/
	aw_dev_dsp_enable(aw_dev, AW_DEV_DSP_BYPASS);
	/*set power down*/
	aw_dev_pwd(aw_dev, true);

	aw_dev_info(aw_dev->dev, "init done");

	return 0;
}

int aw883xx_device_probe(struct aw_device *aw_dev, struct aw_init_info *init_info)
{
	int ret = -1;

	ret = aw_dev_prof_init(aw_dev, init_info);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "prof init failed");
		return ret;
	}

	aw883xx_dev_get_monitor_func(aw_dev);

	if(aw_dev->ops.aw_monitor_init) {
		ret = aw_dev->ops.aw_monitor_init((void *)aw_dev);
		if (ret < 0) {
			aw_dev_err(aw_dev->dev, "monitor init failed");
			return ret;
		}
	}

	ret = aw_device_init(aw_dev);
	if (ret < 0) {
		aw_dev_err(aw_dev->dev, "dev init failed");
		return ret;
	}

	return 0;
}

