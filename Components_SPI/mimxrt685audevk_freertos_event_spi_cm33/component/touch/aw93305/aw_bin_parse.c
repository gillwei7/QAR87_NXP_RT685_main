#include <stdio.h>

/********** awinic bin parse start ***********/
#include "aw_bin_parse.h"

#define AW_BIN_COMM_HEAD_SIZE		(60)
#define AW_BIN_HEAD_SIZE_MAX		(72) //soc app data head size is 12

enum {
	AW_OK,
	AW_PARA_ERR,
	AW_CHECK_SUM_ERR,
	AW_SOC_APP_SIZE_CHECK_ERR,
	AW_REG_SIZE_CHECK_ERR,
};

enum {
	DATA_TYPE_REGISTER = 0x00000000,
	DATA_TYPE_SOC_APP = 0x00000021,
};

#define AW_GET_32_DATA(w, x, y, z)              ((unsigned int)(((w) << 24) | ((x) << 16) | ((y) << 8) | (z)))

static int aw_check_sum(const unsigned char *firmware_data, unsigned int arr_len)
{
	unsigned int i = 0;
	unsigned int arr_data_sum = 0;
	unsigned int check_sum = AW_GET_32_DATA(firmware_data[3], firmware_data[2], firmware_data[1], firmware_data[0]);

	for (i = 4; i < arr_len; i++)
		arr_data_sum += firmware_data[i];

	if (check_sum != arr_data_sum)
		return -AW_CHECK_SUM_ERR;

	return AW_OK;
}

int aw_parsing_bin_file(struct aw_bin *bin, const unsigned char *firmware_data, unsigned int arr_len)
{
	int ret = 0;
	int check_register_num = 0;

	if (bin == NULL || firmware_data == NULL || arr_len < AW_BIN_COMM_HEAD_SIZE)
		return -AW_PARA_ERR;

	ret = aw_check_sum(firmware_data, arr_len);
	if (ret != AW_OK)
		return ret;

	bin->bin_head_info = (struct aw_bin_comm_head_info *)firmware_data;

	if (bin->bin_head_info->bin_data_type == DATA_TYPE_REGISTER) { //reg parse
		bin->bin_reg_head_info = (struct aw_bin_reg_head_info *)&firmware_data[AW_BIN_COMM_HEAD_SIZE];
		check_register_num =
			(bin->bin_head_info->bin_data_len - 4) / (bin->bin_head_info->reg_byte_len + bin->bin_head_info->data_byte_len);
		if (check_register_num != bin->bin_reg_head_info->reg_num)
			return -AW_REG_SIZE_CHECK_ERR;

		bin->data = &firmware_data[AW_BIN_COMM_HEAD_SIZE + 4];
	} else if (bin->bin_head_info->bin_data_type == DATA_TYPE_SOC_APP) { //soc app parse
		bin->bin_soc_app_head_info = (struct aw_bin_soc_app_head_info *)&firmware_data[AW_BIN_COMM_HEAD_SIZE];

		if (bin->bin_soc_app_head_info->soc_app_size != bin->bin_head_info->bin_data_len - 12)
			return -AW_SOC_APP_SIZE_CHECK_ERR;
		bin->data = &firmware_data[AW_BIN_HEAD_SIZE_MAX];
	} else {
		bin->data = &firmware_data[AW_BIN_COMM_HEAD_SIZE];
	}
	return AW_OK;
}
/********** awinic bin parse end ***********/
