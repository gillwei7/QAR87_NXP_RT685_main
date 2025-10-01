#ifndef __AW_BIN_PARSE_H__
#define __AW_BIN_PARSE_H__
/********** awinic bin parse start ***********/
struct aw_bin_comm_head_info {
	unsigned int check_sum; /* Frame header information-Checksum */
	unsigned int header_ver; /* Frame header information-Frame header version */
	unsigned int bin_data_type; /* Frame header information-Data type */
	unsigned int bin_data_ver; /* Frame header information-Data version */
	unsigned int bin_data_len; /* Frame header information-Data length */
	unsigned int ui_ver; /* Frame header information-ui version */
	unsigned char chip_type[8]; /* Frame header information-chip type */
	unsigned int reg_byte_len; /* Frame header information-reg byte len */
	unsigned int data_byte_len; /* Frame header information-data byte len */
	unsigned int device_addr; /* Frame header information-device addr */
	unsigned char reserved[16];
};

struct aw_bin_soc_app_head_info {
	unsigned int soc_app_version;
	unsigned int soc_app_download_addr;
	unsigned int soc_app_size;
};

struct aw_bin_reg_head_info {
	unsigned int reg_num;
};

struct aw_bin {
	struct aw_bin_comm_head_info *bin_head_info;
	struct aw_bin_soc_app_head_info *bin_soc_app_head_info;
	struct aw_bin_reg_head_info *bin_reg_head_info;
	const unsigned char *data;
};

int aw_parsing_bin_file(struct aw_bin *bin, const unsigned char *firmware_data, unsigned int arr_len);

/********** awinic bin parse end ***********/
#endif