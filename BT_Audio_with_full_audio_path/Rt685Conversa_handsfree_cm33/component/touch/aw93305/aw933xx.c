#include <stdlib.h>
#include <string.h>
#include "aw_type.h"
#include "aw_cap_config.h"
#include "aw_cap.h"
#include "aw933xx.h"
#include "aw933xx_param.h"
#include "aw933xx_reg.h"
#include "aw_bin_parse.h"
#define AW933XX_DRIVER_VERSION		"V0.7.0"
volatile struct aw933xx_dev aw933xx;

#if (defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1))
static void aw933xx_dbg_send_curve_dat(void);
#endif

static void aw_cap_delay_ms(AW_U32 ms)
{
	aw933xx.aw_func->delay(ms);
}

static AW_S32 aw_cap_i2c_write(AW_U16 reg_addr, AW_U32 reg_data)
{
	return aw933xx.aw_func->i2c_func->i2c_write(reg_addr, reg_data);
}

static AW_S32 aw_cap_i2c_read(AW_U16 reg_addr, AW_U32 *reg_data)
{
	return aw933xx.aw_func->i2c_func->i2c_read(reg_addr, reg_data);
}

static AW_S32 aw_cap_i2c_write_seq(AW_U16 addr, AW_U8 *data, AW_U32 len)
{
	return aw933xx.aw_func->i2c_func->i2c_write_seq(addr, data, len);
}

static AW_S32 aw_cap_i2c_read_seq(AW_U16 addr, AW_U8 *data, AW_U32 len)
{
	return aw933xx.aw_func->i2c_func->i2c_read_seq(addr, data, len);
}

static AW_S8 aw_cap_i2c_write_bits(AW_U16 reg_addr,
		AW_U32 mask, AW_U32 reg_data)
{
	AW_U32 reg_val;

	aw_cap_i2c_read(reg_addr, &reg_val);
	reg_val &= mask;
	reg_val |= (reg_data & (~mask));
	aw_cap_i2c_write(reg_addr, reg_val);

	return 0;
}

static void aw933xx_init_global_val(void)
{
	memset(aw933xx.last_status, 0x00, sizeof(aw933xx.last_status));
}

static void aw933xx_set_cs_as_irq(int flag)
{
	if (flag == AW933XX_CS2_IRQ) {
		aw_cap_i2c_write(0xfff4, 0x3c00d013);
		aw_cap_i2c_write(0xc100, 0x00000020);
		aw_cap_i2c_write(0xe018, 0x00000004);
	} else if (flag == AW933XX_CS5_IRQ) {
		aw_cap_i2c_write(0xfff4, 0x3c00d013);
		aw_cap_i2c_write(0xc100, 0x00000800);
		aw_cap_i2c_write(0xe018, 0x00000020);
	} else {
		aw_cap_i2c_write(0xfff4, 0x3c00d013);
		aw_cap_i2c_write(0xc100, 0x00000000);
		aw_cap_i2c_write(0xe018, 0x00000000);
	}
}

/******************* debug interface start **********************/
static void aw933xx_enable_clock(void)
{
	aw_cap_i2c_write_bits(REG_CHIPSTAT, ~AW933XX_CPU_OSC_CTRL_MASK, AW933XX_CPU_OSC_CTRL_MASK);
}

static AW_U32 aw933xx_rc_irqscr(void)
{
	AW_U32 val = 0;

	aw_cap_i2c_read(REG_IRQSRC, &val);

	return val;
}

void aw933xx_mode_switch(AW_U8 mode)
{
	if (mode == aw933xx.current_mode)
		return;

	if (mode != AW933XX_ACTIVE_MODE &&
			mode != AW933XX_SLEEP_MODE &&
			mode != AW933XX_DEEPSLEEP_MODE) {
		return;
	}

	if (aw933xx.current_mode == AW933XX_DEEPSLEEP_MODE)
		aw933xx_enable_clock();

	aw933xx_rc_irqscr();

	aw_cap_i2c_write(REG_CMD, mode);

	aw933xx.current_mode = mode;
}

AW_S32 aw933xx_sw_rst(void)
{
	AW_S32 ret = 0;

	ret = aw_cap_i2c_write(0xff0c, 0x0);
//	aw933xx.aw_func->set_rst_cb();
	aw933xx.aw_func->delay(30);
	return ret;
}

AW_S32 aw933xx_read_diff(AW_S32 *diff)
{
	AW_S8 ret = -1;

	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		ret = aw_cap_i2c_read(AW933XX_DIFF_CH0 + (i * AW_REG_STEP),
				(AW_U32 *)diff + i);
		if (ret != AW_OK) {
			AW933XX_ERR("read reg[0x%04X] diff failed: %d",
					AW933XX_DIFF_CH0 + (i * AW_REG_STEP), ret);
			return ret;
		} else {
#ifndef AW_DEBUG_PRINTF
			AW933XX_INF("diff: ch[%d]: %d", i, (*(diff + i) >> 10));
#endif
		}
	}

	return AW_OK;
}

AW_S32 aw933xx_read_base(AW_S32 *base)
{
	AW_S8 ret = -1;

	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		ret = aw_cap_i2c_read(AW933XX_BASELINE_CH0 + (i * AW_REG_STEP),
				(AW_U32 *)base + i);
		if (ret != AW_OK) {
			AW933XX_ERR("read reg[0x%04X] diff failed: %d",
					AW933XX_BASELINE_CH0 + (i * AW_REG_STEP), ret);
			return ret;
		} else {
#ifndef AW_DEBUG_PRINTF
			AW933XX_INF("base: ch[%d]: %d", i, (*(base + i) >> 10));
#endif
		}
	}

	return AW_OK;
}

AW_S32 aw933xx_read_valid(AW_S32 *valid)
{
	AW_S8 ret = -1;

	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		ret = aw_cap_i2c_read(AW933XX_VALID_CH0 + (i * AW_REG_STEP),
				(AW_U32 *)valid + i);
		if (ret != AW_OK) {
			AW933XX_ERR("read reg[0x%04X] valid failed: %d",
					AW933XX_VALID_CH0 + (i * AW_REG_STEP), ret);
			return ret;
		} else {
#ifndef AW_DEBUG_PRINTF
			AW933XX_INF("valid: ch[%d]: %d", i, (*(valid + i) >> 10));
#endif
		}
	}

	return AW_OK;
}

AW_S32 aw933xx_read_raw(AW_S32 *raw)
{
	AW_S8 ret = -1;

	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		ret = aw_cap_i2c_read(AW933XX_RAW_CH0 + (i * AW_REG_STEP),
				(AW_U32 *)raw + i);
		if (ret != AW_OK) {
			AW933XX_ERR("read reg[0x%04X] raw failed: %d",
					AW933XX_RAW_CH0 + (i * AW_REG_STEP), ret);
			return ret;
		} else {
#ifndef AW_DEBUG_PRINTF
			AW933XX_INF("raw: ch[%d]: %d", i, (*(raw + i) >> 10));
#endif
		}
	}

	return AW_OK;
}

AW_S32 aw933xx_read_ch_data(void){
	
	AW_S8 ret = -1;
	AW_S32 valid[AW_CHANNEL_NUM_MAX];
	AW_S32 raw[AW_CHANNEL_NUM_MAX];
	AW_S32 diff[AW_CHANNEL_NUM_MAX];
	AW_S32 base[AW_CHANNEL_NUM_MAX];
	
	ret = aw933xx_read_diff(diff);
	if(ret != AW_OK){
		AW933XX_ERR("read diff error");
	}
	
	ret = aw933xx_read_base(base);
	if(ret != AW_OK){
		AW933XX_ERR("read base error");
	}
	
	ret = aw933xx_read_valid(valid);
	if(ret != AW_OK){
		AW933XX_ERR("read valid error");
	}
	
	ret = aw933xx_read_raw(raw);
	if(ret != AW_OK){
		AW933XX_ERR("read raw error");
	}
#ifdef AW_DEBUG_PRINTF
	for(AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++){
		AW933XX_INF("ch = %d, raw = %d, valid = %d, base = %d, diff = %d", i, raw[i] >> 10, valid[i] >> 10, base[i] >> 10, diff[i] >> 10 );
	}
#endif	
	return ret;
}


void aw933xx_cali_get_offset(AW_U8 aot_flag)
{

	AW_U32 cnt = 0;
	AW_U32 reg = 0;
	AW_U32 offset[AW_CHANNEL_NUM_MAX];

	AW_U32 reg_afe_cfg1[] = {REG_AFECFG1_CH0, REG_AFECFG1_CH1, REG_AFECFG1_CH2,
		REG_AFECFG1_CH3, REG_AFECFG1_CH4, REG_AFECFG1_CH5,
		REG_AFECFG1_CH6, REG_AFECFG1_CH7, REG_AFECFG1_CH8,
		REG_AFECFG1_CH9, REG_AFECFG1_CH10, REG_AFECFG1_CH11,};

	AW_U32 offset_f = 0;
	AW_U32 offset_c = 0;


	//step 2: Waiting 1s for calibration complete flag
	if(aot_flag == 1){
		//step 1: Parasitic capacitance calibration
		aw_cap_i2c_write(REG_SCANCTRL0, 0x00000fff);
		aw_cap_i2c_write(REG_SCANCTRL1, 0x00000fff);
		while (1) {
			if (cnt >= 100) {
				AW933XX_ERR("aw933xx get calibration complete flag error");
				break;
			}
			aw_cap_i2c_read(REG_STAT7, &reg);
			if (reg == 0)
				break;
			cnt++;
			aw_cap_delay_ms(10);
		}
	}
	//step 3: Read offset data and restore
	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		aw_cap_i2c_read( reg_afe_cfg1[i], &offset[i]);
		AW933XX_INF("aw933xx reg = 0x%x  data = 0x%x", reg_afe_cfg1[i], offset[i]);

		offset_c = (offset[i] >> AW_BIT8) & 0xff;
		offset_f = (offset[i] >> AW_BIT16) & 0xff;

		offset_c = offset_c* AW933XX_STEP_LEN_UNSIGNED_CAP_ROUGH_ADJ;
		offset_f = offset_f * AW933XX_STEP_LEN_UNSIGNED_CAP_FINE_ADJ;

		AW933XX_INF("unsigned cap ofst ch%u: %u.%u pf\r\n",
					i,
					(offset_c +  offset_f)/ AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE,
					(offset_c +  offset_f) % AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE);
	}

	
}

static AW_S32 aw933xx_pow2(AW_U32 cnt)
{
	AW_U32 i = 0;
	AW_U32 sum = 1;

	if (cnt == 0) {
		sum = 1;
	} else {
		for (i = 0; i < cnt; i++)
			sum *= 2;
	}

	return sum;
}

static AW_S32 aw933xx_get_signed_cap(AW_U16 reg_addr)
{
	AW_U32 reg_data = 0;
	AW_S32 off_f  = 0;
	AW_U32 off_c = 0;
	AW_U32 off_m = 0;
	AW_U32 off_m_bit = 0;
	AW_U32 off_c_bit = 0;
	AW_S32 s_ofst_c = 0;
	AW_U32 i = 0;

	aw_cap_i2c_read(reg_addr, &reg_data);

	off_f = ((reg_data >> 16) & 0xff) * AW933XX_STEP_LEN_UNSIGNED_CAP_FINE_ADJ;
	off_c = (reg_data >> 8) & 0xff;
	off_m = reg_data & 0xff;

	for (i = 0; i < 8; i++) {
		off_m_bit = (off_m >> i) & 0x01;
		off_c_bit = (off_c >> i) & 0x01;
		s_ofst_c += ((1 - 2 * off_m_bit) * off_c_bit * aw933xx_pow2(i)) *
					AW933XX_STEP_LEN_UNSIGNED_CAP_ROUGH_ADJ;
	}

	return (s_ofst_c + off_f);
}

static AW_S32 aw933xx_get_unsigned_cap(AW_U16 reg_addr)
{
	AW_U32 reg_data = 0;
	AW_U32 rough = 0;
	AW_U32 fine = 0;

	aw_cap_i2c_read(reg_addr, &reg_data);

	rough = ((reg_data >> 8) & 0xff) * AW933XX_STEP_LEN_UNSIGNED_CAP_ROUGH_ADJ;
	fine = ((reg_data >> 16) & 0xff) * AW933XX_STEP_LEN_UNSIGNED_CAP_FINE_ADJ;

	return (rough + fine);
}

void aw933xx_get_cap_offset(void)
{
	AW_U32 reg_data = 0;
	AW_U32 mode = 0xff;
	AW_U32 i = 0;
	AW_U32 cap_ofst = 0;
	AW_S32 signed_cap_ofst = 0;
	AW_U32 tmp = 0;

	for (i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		aw_cap_i2c_read(REG_AFESOFTCFG0_CH0 +
						i * (REG_AFESOFTCFG0_CH1 - REG_AFESOFTCFG0_CH0),
						&reg_data);
		mode = reg_data & 0x0ff;
		switch (mode) {
		case AW933XX_UNSIGNED_CAP:	//self-capacitance mode unsigned cail
			cap_ofst = aw933xx_get_unsigned_cap(REG_AFECFG1_CH0 +
				i * (REG_AFECFG1_CH1 - REG_AFECFG1_CH0));
			AW933XX_INF("cap_ofst = %d", cap_ofst);
			AW933XX_INF("unsigned cap ofst ch%d: %d.%dpf\r\n",
						i,
						cap_ofst / AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE,
						cap_ofst % AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE);
			break;
		case AW933XX_SIGNED_CAP:	//self-capacitance mode signed cail
			signed_cap_ofst = aw933xx_get_signed_cap(REG_AFECFG1_CH0 +
					i * (REG_AFECFG1_CH1 - REG_AFECFG1_CH0));
			AW933XX_INF("cap_ofst1 = 0x%x", signed_cap_ofst);
			if (signed_cap_ofst < 0) {
				tmp = -signed_cap_ofst;
				AW933XX_INF("cap_ofst2 = 0x%x", signed_cap_ofst);
				AW933XX_INF("signed cap ofst ch%d: -%d.%dpf\n",
						i,
						tmp / AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE,
						tmp % AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE);
			} else {
				AW933XX_INF("cap_ofst2 = 0x%x", signed_cap_ofst);
				AW933XX_INF("signed cap ofst ch%d: %d.%dpf\n",
					i,
					signed_cap_ofst / AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE,
					signed_cap_ofst % AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE);
			}
			break;
		case AW933XX_MUTUAL_CAP:	//mutual-capacitance mode
			signed_cap_ofst = aw933xx_get_signed_cap(REG_AFECFG1_CH0 +
						i * (REG_AFECFG1_CH1 - REG_AFECFG1_CH0));
			AW933XX_INF("cap_ofst1 = 0x%x", signed_cap_ofst);
			if (signed_cap_ofst < 0) {
				tmp = -signed_cap_ofst;
				AW933XX_INF("cap_ofst2 = 0x%x", signed_cap_ofst);
				AW933XX_INF("mutual cap ofst ch%d: -%d.%dpf\r\n",
					i,
					tmp / AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE,
					tmp % AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE);
			} else {
				AW933XX_INF("cap_ofst2 = 0x%x", signed_cap_ofst);
				AW933XX_INF("mutual cap ofst ch%d: %d.%dpf\r\n",
					i,
					signed_cap_ofst / AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE,
					signed_cap_ofst % AW933XX_STEP_LEN_UNSIGNED_CAP_ENLARGE);
			}
			break;
		default:
			AW933XX_INF("aw933xx ofst error 0x%x", reg_data & 0x0f);
			break;
		}
	}
}

void aw933xx_aot(void)
{
	aw_cap_i2c_write_bits(REG_SCANCTRL1, ~0xfff, 0xfff);
}
/******************* debug interface end **********************/

/**
 * @brief this func detect if any channel connected to vdd or gnd.
 *
 *@param gnd_stat Pin short circuited to GND
 *@param vcc_stat Pin short circuited to VCC
 *return If 0, it is normal, otherwise it is abnormal
 */
AW_S8 aw933xx_short_circuit_detect_get_stat(AW_U32 *p_gnd_stat, AW_U32 *p_vcc_stat)
{
	AW_U32 stat = 0;
	AW_U32 apb_store = 0;
	AW_U32 apb_val = 0;

	aw_cap_i2c_write(REG_CMD, AW933XX_SLEEP_MODE);

	aw_cap_i2c_read(REG_ACCESS_APB_EN, &apb_store);
	apb_val = apb_store | 0x3C00D103;
	aw_cap_i2c_write(REG_ACCESS_APB_EN, apb_val);

	aw_cap_i2c_write(REG_MFP00, 0x555555);
	aw_cap_i2c_write(REG_GPIODIR, 0);
	aw_cap_i2c_write(REG_GPIOIBE, AW933XX_GPIO_DIRIN_EN);

	aw_cap_i2c_write(REG_GPIOPU, AW933XX_GPIO_PU_EN);
	aw_cap_i2c_write(REG_GPIOPD, AW933XX_GPIO_PD_DIS);
	aw_cap_i2c_read(REG_GPIORDATA, &stat);
	*p_gnd_stat = stat;
	if (stat != 0x0fff) {
		for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
			if (((stat >> i) & 0x01) == 0) {
				AW933XX_ERR("aw933xx channel %d connect the ground.", i);
				return -1;
			}
		}
	}

	aw_cap_i2c_write(REG_GPIOPU, AW933XX_GPIO_PU_DIS);
	aw_cap_i2c_write(REG_GPIOPD, AW933XX_GPIO_PD_EN);
	aw_cap_i2c_read(REG_GPIORDATA, &stat);
	*p_vcc_stat = stat;
	if (stat != 0) {
		for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
			if (((stat >> i) & 0x01) == 1) {
				AW933XX_ERR("aw933xx channel %d connect the power supply.", i);
				return -1;
			}
		}
	}

	// reset reg to pre-state.
	aw_cap_i2c_write(REG_GPIOPU, 0x0);
	aw_cap_i2c_write(REG_GPIOPD, 0x0);
	aw_cap_i2c_write(REG_MFP00, 0x0);
	// aw_cap_i2c_write(REG_IEB,  0x40);

	apb_val = apb_store | 0x3C000000;
	aw_cap_i2c_write(REG_ACCESS_APB_EN, apb_val);

	aw_cap_i2c_write(REG_CMD, AW933XX_ACTIVE_MODE);

	return 0;
}

static AW_S32 aw933xx_read_chipid(void)
{
	AW_S32 ret = 0;
	AW_U32 reg_val = 0;

	ret = aw_cap_i2c_read(REG_CHIP_ID0, &reg_val);
	if (ret < 0) {
		AWLOGE("read CHIP ID failed: %d", ret);
		return -AW_ERR;
	}

	switch (reg_val) {
	case AW93303_CHIP_ID:
		AWLOGD("AW93303FDR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	case AW93305_CHIP_ID:
		AWLOGD("AW93305DNR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	case AW93307_CHIP_ID:
		AWLOGD("AW93307QNR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	case AW93310_CHIP_ID:
		AWLOGD("AW93310QNR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	case AW93312_CHIP_ID:
		AWLOGD("AW93312QNR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	case AW93305BFOR_CHIP_ID:
		AWLOGD("AW93305BFOR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	case AW93303BFOR_CHIP_ID:
		AWLOGD("AW93303BFOR detected, 0x%08x", reg_val);
		ret = AW_OK;
		break;
	default:
		AWLOGD("chip id error, 0x%08x", reg_val);
		ret =  -AW_ERR;
		break;
	}
	aw933xx.chip_id = reg_val;

	return ret;
}

static AW_S32 aw933xx_read_init_over_irq(void)
{
	AW_U32 cnt = 1000;
	AW_U32 reg = 0;
	AW_S32 ret = 0;

	while (cnt--) {
		ret = aw_cap_i2c_read(REG_IRQSRC, &reg);
		if (ret != 0) {
			AW933XX_ERR("i2c error %d", ret);
			return ret;
		}
		if ((reg & 0x01) == 0x01) {
			AW933XX_INF("read init irq success!");
			aw_cap_i2c_read(REG_FWVER, &reg);
			AW933XX_INF("firmware version = 0x%08x", reg);
			return AW_OK;
		}
		aw_cap_delay_ms(10);
	}
	AW933XX_ERR("read init over irq error");

	return -AW_ERROR_CODERAM_RD_FWVER_ERROR;
}

/******** coderam update start here ********/
#if (defined(AW933XX_LITTLE_ENDIAN_CONVERT) && (AW933XX_LITTLE_ENDIAN_CONVERT == 1))
/**
 * @brief convert fw data from little endian to big endian
 *
 * @param aw_bin fw data
 */
static void aw933xx_convert_little_endian_2_big_endian(AW_U8 *dat, AW_U32 len)
{
	AW_U32 i = 0;
	AW_U32 fw_len = len;
	AW_U32 uints = fw_len / AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE;

	for (i = 0; i < uints; i++) {
		AW_U8 tmp1 = dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE + 3];
		AW_U8 tmp2 = dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE + 2];
		AW_U8 tmp3 = dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE + 1];
		AW_U8 tmp4 = dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE];
		memcpy((void *)&dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE], &tmp1, 1);
		memcpy((void *)&dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE + 1], &tmp2, 1);
		memcpy((void *)&dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE + 2], &tmp3, 1);
		memcpy((void *)&dat[i * AW933XX_CODERAM_UPDATE_ONE_UINT_SIZE + 3], &tmp4, 1);
	}
}
#endif

/**
 * @brief convert raw coderam firmware array datas to the format of "struct aw_bin"
 *
 * @param bin the struct to storage the information parsed
 * @param dat the raw datas of firmware
 * @param len the lengthe of raw data
 * @return AW_S32 success(0) or not
 */
static AW_S32 aw933xx_coderam_data_parse(struct aw_bin *bin, const AW_U8 *dat, AW_U32 len)
{
	return aw_parsing_bin_file(bin, dat, len);
}

/**
 * @brief write data to coderam and read to check if wrote is success or not
 *
 * @param addr start to write
 * @param dat data to write
 * @param len of dat
 * @return AW_S32 success(0) or not
 */
static AW_S32 aw933xx_coderam_write_check(AW_U16 addr, AW_U8 *dat, AW_U32 len,AW_U8 ram_flag)
{
	AW_U8 ret = 0;
	AW_U8 *r_buf = NULL;

	if (len > AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE) {
		AW933XX_ERR("coderam write len error, max = %d, read = %d",
						AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE, len);
		return -AW_ERROR_CODERAM_WRITE_LEN_ERROR;
	}

	if(ram_flag == AW933XX_RAM_UPDATE){
		ret = aw_cap_i2c_write_seq(addr, dat, len);

		if (ret != AW_OK) {
			AW933XX_ERR(" 0x%x, write_seq error(%d)!", addr, ret);
			return ret;
		}
	}

	r_buf = malloc(AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE);
	if (r_buf == NULL) {
		AW933XX_ERR("aw933xx_coderam_write_check malloc error");
		return -AW_ERR;
	}
	memset(r_buf, 0, AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE);

	ret = aw_cap_i2c_read_seq(addr, r_buf, len);
	if (ret != AW_OK) {
		AW933XX_ERR("0x%x, read_seq error!", addr);
		goto err_out;
	}

	if (memcmp(dat, r_buf, len) != 0) {
		AW933XX_ERR("read is not equal to write 0x%x\n", addr);
		for(AW_U16 i = 0;i<len;i++){
			AW933XX_ERR("read is not equal to write,r_buf 0x%x,dat 0x%x\n", r_buf[i],dat[i]);
		}
		ret = -AW_ERROR_CODERAM_CHECK_ERROR;
	}

err_out:
	free(r_buf);

	return ret;
}

/**
 * @brief  |----------------code ram-----------------|
 *         |--- app filled here---|--fill with 0xff--|
 *       0x2000                                    0x4fff
 *
 *         if the size of app is less than the size of coderam, the rest of
 *         ram is filled with 0xff.
 * @param offset the rear addr of app
 * @return AW_S32
 */
static AW_S32 aw933xx_sram_fill_not_wrote_area(AW_U32 offset,AW_U8 ram_flag)
{
	AW_U8 *buf = NULL;
	AW_S32 ret = 0;
	AW_U32 i = 0;
	AW_U32 write_len = 0;
	AW_U32 download_addr_with_ofst = 0;
	AW_U32 last_pack_len = (AW933XX_CODERAM_END_ADDR - offset) %
							AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE;
	AW_U32 pack_cnt = last_pack_len == 0 ?
		((AW933XX_CODERAM_END_ADDR - offset) / AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE) :
		((AW933XX_CODERAM_END_ADDR - offset) / AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE) + 1;

	AW933XX_INF("last_pack_len = %d", last_pack_len);
	AW933XX_INF("pack_cnt = %d", pack_cnt);
	AW933XX_INF("offset = 0x%x", offset);

	buf = malloc(AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE);
	if (buf == NULL) {
		AW933XX_ERR("aw933xx_sram_fill_not_wrote_area malloc error");
		return -AW_ERR;
	}
	memset(buf, 0xff, AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE);

	for (i = 0; i < pack_cnt; i++) {
		download_addr_with_ofst = offset + i * AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE;
		write_len = (i == (pack_cnt - 1) && last_pack_len) ?
			last_pack_len : AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE;
		ret = aw933xx_coderam_write_check(download_addr_with_ofst, buf,
						write_len,ram_flag);
		if (ret != AW_OK)
			break;
	}

	free(buf);

	return ret;
}

/**
 * @brief implementation of writing data to coderam
 *
 * @param bin data to write
 * @return AW_S32 success(0) or not
 */

static AW_S32 aw933xx_coderam_data_write(struct aw_bin *bin,AW_U8 ram_flag)
{
	AW_S32 ret = 0;
	AW_U32 i = 0, j = 0;
	AW_U32 write_len = 0;
	AW_U32 pack_cnt = 0;
	AW_U32 download_addr = AW933XX_CODERAM_START_ADDR;
	AW_U32 fw_len = bin->bin_soc_app_head_info->soc_app_size;
	AW_U32 last_pack_len = fw_len % AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE;
	AW_U32 download_addr_with_ofst = 0;

	pack_cnt = ((fw_len % AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE) == 0) ?
			(fw_len / AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE) :
			(fw_len / AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE) + 1;
	AWLOGD(" fw_len = %d,pack_cnt = %d", fw_len, pack_cnt);

	AW_U8 *buf = malloc(AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE);
	if (buf == NULL) {
		AW933XX_ERR("aw933xx_coderam_data_write malloc error");
		return -AW_ERR;
	}

	for (i = 0; i < pack_cnt; i++) {
		memset(buf, 0, AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE);
		download_addr_with_ofst = download_addr + i * AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE;
		write_len = (i == (pack_cnt - 1) && last_pack_len) ?
			last_pack_len : AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE;
		for (j = 0 ; j < write_len; j++)
			buf[j] = bin->data[j + i * AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE];
		//memcpy(buf, &bin->data[i * AW933XX_CODERAM_UPDATE_ONE_PACK_SIZE], write_len);

#if (defined(AW933XX_LITTLE_ENDIAN_CONVERT) && (AW933XX_LITTLE_ENDIAN_CONVERT == 1))
		aw933xx_convert_little_endian_2_big_endian(buf, write_len);
#endif
		ret = aw933xx_coderam_write_check(download_addr_with_ofst, buf, write_len,ram_flag);
		if (ret != AW_OK) {
			free(buf);
			return ret;
		}
	}
	if (download_addr_with_ofst + write_len < AW933XX_CODERAM_END_ADDR) {
		/* fill 0xff in the area that not worte. */
		ret = aw933xx_sram_fill_not_wrote_area(download_addr_with_ofst + write_len,ram_flag);
		if (ret != AW_OK) {
			printf("cnt%d, sram_fill_not_wrote_area error!\n", i);
			free(buf);
			return ret;
		}
	}
	free(buf);

	return AW_OK;
}

/**
 * @brief start the process that update coderam
 *
 * @return AW_S32 success(0) or not
 */
static AW_S32 aw933xx_coderam_update(void)
{
	AW_S32 ret = 0;
	struct aw_bin bin;

	ret = aw933xx_coderam_data_parse(&bin, aw933xx_coderam_dat, sizeof(aw933xx_coderam_dat) / sizeof(aw933xx_coderam_dat[0]));
	if (ret) {
		AW933XX_ERR("coderam parse error");
		return -AW_ERROR_CODERAM_PARSE_ERROR;
	}
	// step1: close coderam shutdown mode
	aw_cap_i2c_write(0xfff4, 0x3c00d11f);
	aw_cap_i2c_write(0xc400, 0x21660000);

	// step 2: reset mcu only and set boot mode to 1. (0xf800 0x00010100)
	aw_cap_i2c_write(0xf800, 0x00010100);

	// step 3: enable data ram. (0xFFE4 0x3C000000)
	aw_cap_i2c_write(0xFFE4, 0x3C000000);

	// step 4: write ram data.
	ret = aw933xx_coderam_data_write(&bin,AW933XX_RAM_UPDATE);
	if (ret != AW_OK)
		return ret;
	aw_cap_delay_ms(100);
	// step 5: exit reset mcu and boot cpu in ram. (0xf800 0x00000100)
	aw_cap_i2c_write(0xf800, 0x00000100);
	// step 6: reset cpu (0xFF0C 0x0)
	aw_cap_i2c_write(0xFF0C, 0x0);
	// step 7: Wait for chip initialization to complete
	aw_cap_delay_ms(500);

	return aw933xx_read_init_over_irq();
}

/**
 * @brief start the process that check coderam
 *
 * @return AW_S32 success(0) or not
 */
static AW_S32 aw933xx_coderam_check(void)
{
	AW_S32 ret = 0;
	struct aw_bin bin;

    ret = aw933xx_coderam_data_parse(&bin, aw933xx_coderam_dat, sizeof(aw933xx_coderam_dat) / sizeof(aw933xx_coderam_dat[0]));
    if (ret)
    {
        AW933XX_ERR("coderam parse error");
        return -AW_ERROR_CODERAM_PARSE_ERROR;
    }
    // step1: Decipher coderam
    aw_cap_i2c_write(0xfff4, 0x3c00d11f);
   
	aw_cap_i2c_write(0xFFE4, 0x3C000000);
    // step2: check ram data.
    ret = aw933xx_coderam_data_write(&bin,AW933XX_RAM_CHECK);
    if (ret != AW_OK)
        return ret;
    aw_cap_delay_ms(100);
		
		// step3:close Decipher coderam
    aw_cap_i2c_write(0xfff4, 0x3c00d103);
		
	 AW933XX_INF("coderam check success");
	return ret;
}

/**
 * @brief load basic params
 *
 * @return AW_S32 success(0) or not
 */
static AW_S32 aw933xx_load_params(void)
{
	AW_U32 cnt = (sizeof(aw933xx_default_reg) / sizeof(aw933xx_default_reg[1]));
	AW_S32 ret = 0;

	for (int i = 0; i < cnt; i += 2) {
		ret = aw_cap_i2c_write(aw933xx_default_reg[i], aw933xx_default_reg[i + 1]);
		if (ret != AW_OK)
			return ret;
		AW933XX_DBG("addr = 0x%x, val = 0x%x",
					aw933xx_default_reg[i], aw933xx_default_reg[i + 1]);
	}

	return AW_OK;
}
/******** coderam update end ***************/

static AW_S32 aw933xx_cap_param_loaded(void)
{
	AW_S32 i = 0;
	AW_S32 ret = 0;
	AW_S32 len = sizeof(aw933xx_reg_data) / sizeof(aw933xx_reg_data[0]);

	AW933XX_INF("start to .c param download!");

	for (i = 0; i < len; i = i + 2) {
		ret = aw_cap_i2c_write((AW_U16)aw933xx_reg_data[i], aw933xx_reg_data[i + 1]);
		AW933XX_INF("reg_addr = 0x%04x, reg_data = 0x%08x",
						aw933xx_reg_data[i],
						aw933xx_reg_data[i + 1]);
		if (ret != AW_OK) {
			AW933XX_ERR("i2c write failed");
			return ret;
		}
	}
	AW933XX_INF(".c param writen completely");
	AW933XX_INF("cap reg buf len = %d", len);

	return AW_OK;
}

/**
 * @brief load the config generated by Awinic UI
 *
 * @return AW_S32 success(0) or not
 */
static AW_S32 aw933xx_load_cfg(void)
{
	return AW_OK;
	aw_widget_cfg();

	return aw933xx_cap_param_loaded();
}

static void aw933xx_SLDx_parse(AW_U32 x, AW_U32 reg)
{
	AW_U32 btn_event = 0;
	AW_U32 prs_event = 0;
	AW_U32 sld_event = 0;

	AW933XX_INF("sld %d", x);
	prs_event = (reg >> AW933XX_PRESS_STAT_IDX) & AW933XX_PRESS_VALID_DAT;
	btn_event = (reg >> AW933XX_CLICK_STAT_IDX) & AW933XX_CLICK_VALID_DAT;
	sld_event = (reg >> AW933XX_SLIDE_STAT_IDX) & AW933XX_SLIDE_VALID_DAT;
	aw933xx.event.btn_pos_x = (reg >> AW933XX_BTN_POS_X_IDX) & AW933XX_BTN_POS_MASK;
	aw933xx.event.btn_pos_y = (reg >> AW933XX_BTN_POS_Y_IDX) & AW933XX_BTN_POS_MASK;

	AW933XX_DBG("prs_event = 0x%x", prs_event);
	AW933XX_DBG("btn_event = 0x%x", btn_event);
	AW933XX_DBG("sld_event = 0x%x", sld_event);
	if (aw933xx.event.btn_pos_x)
		AW933XX_DBG("btn_pos_x = ch%d", aw933xx.event.btn_pos_x - 1);
	if (aw933xx.event.btn_pos_y)
		AW933XX_DBG("btn_pos_y = ch%d", aw933xx.event.btn_pos_y - 1);

	if (btn_event) {
		AW933XX_INF("AW933XX_EVENT_CLICK = %d", btn_event);
		aw933xx.event.click = btn_event;
	}

	switch (prs_event) {
	case AW933XX_EVENT_PRESS:
		AW933XX_INF("AW933XX_EVENT_PRESS");
		aw933xx.event.press = 1;
		break;
	case AW933XX_EVENT_PRESS_LONG:
		AW933XX_INF("AW933XX_EVENT_PRESS_LONG");
		aw933xx.event.long_press = 1;
		break;
	case AW933XX_EVENT_PRESS_SUPER_LONG:
		AW933XX_INF("AW933XX_EVENT_PRESS_SUPER_LONG");
		aw933xx.event.super_long_press = 1;
		break;
	default:
		break;
	}

	switch (sld_event) {
	case AW933XX_EVENT_SLIDE_DIR_UP:
		AW933XX_INF("AW933XX_EVENT_SLIDE_DIR_UP");
		aw933xx.event.up_wareds = 1;
		break;
	case AW933XX_EVENT_SLIDE_DIR_DOWN:
		AW933XX_INF("AW933XX_EVENT_SLIDE_DIR_DOWN");
		aw933xx.event.down_wareds = 1;
		break;
	case AW933XX_EVENT_SLIDE_DIR_LEFT:
		AW933XX_INF("AW933XX_EVENT_SLIDE_DIR_LEFT");
		aw933xx.event.left_wareds = 1;
		break;
	case AW933XX_EVENT_SLIDE_DIR_RIGHT:
		AW933XX_INF("AW933XX_EVENT_SLIDE_DIR_RIGHT");
		aw933xx.event.right_wareds = 1;
		break;
	default:
		break;
	}
}

static AW_S32 aw933xx_SLDx_read(AW_U32 x, AW_U32 *reg)
{
	AW_U32 ret = 0;

	ret = aw_cap_i2c_read(REG_BAR_REPORT_SLD0 + x * AW933XX_SLDX_STEP, reg);
	if (ret != AW_OK) {
		AW933XX_ERR("read REG_BAR_REPORT_SLD%d error(%d)", x, ret);
		return ret;
	}

	return AW_OK;
}

static void aw933xx_wera_state_check(void)
{
	AW_U32 ret = 0;
	AW_U32 reg = 0;

	ret = aw_cap_i2c_read(REG_WEAR_STATE, &reg);
	if (ret != AW_OK)
		AW933XX_ERR("read REG_WEAR_STATE error(%d)", ret);
	aw933xx.event.wear = reg  & AW933XX_SLIDE_WEAR_STATE_MASK;
	AW933XX_INF("AW933XX_EVENT_WEAR = %d", aw933xx.event.wear);
	if (aw933xx.event.wear)
		AW933XX_INF("AW933XX_EVENT_WEAR");
	else
		AW933XX_INF("AW933XX_EVENT_UNWEAR");
}

static void aw933xx_approch_status_check(void)
{
	AW_U8 new_status[AW_CHANNEL_NUM_MAX] = {0};
	AW_U32 status0 = 0, status1 = 0;

	aw_cap_i2c_read(REG_STAT0, &status0);
	aw_cap_i2c_read(REG_STAT1, &status1);
	for (int ch = 0; ch < AW_CHANNEL_NUM_MAX; ch++) {
		new_status[ch] = (AW_U8)(((status0 >> ch) & 0x1) | (((status1 >> ch) & 0x1) << 1));
		if (aw933xx.last_status[ch] != new_status[ch]) {
			switch (new_status[ch]) {
			case 0:
				AW933XX_INF("aw933xx ch%d far", ch);
				break;
			case 1:
				AW933XX_INF("aw933xx ch%d prox0 near", ch);
				break;
			case 3:
				AW933XX_INF("aw933xx ch%d prox1 & prox0 near", ch);
				break;

			default:
				AW933XX_INF("aw933xx err");
				break;
			}
		}
		aw933xx.last_status[ch] = new_status[ch];
	}
}

void aw933xx_irq_process_impl(void)
{
	AW_U32 val = 0;
	AW_U32 sld = 0;
	AW_U32 i = 0;

	aw_cap_i2c_read(REG_IRQSRC, &val);
	memset(&aw933xx.event, 0, sizeof(aw933xx.event));
	if (val & ((1 << REG_IRQSRC_CLOSEANYIRQ_BIT) |
				(1 << REG_IRQSRC_FARANYIRQ_BIT) |
				(1 << REG_IRQSRC_TOUCHANYIRQ_BIT) |
				(1 << REG_IRQSRC_EXITTOUCHANYIRQ_BIT)))
		aw933xx_approch_status_check();

	if (val & (1 << REG_IRQSRC_WEARIRQ_BIT))
		aw933xx_wera_state_check();

	for (i = REG_IRQSRC_SLD0IRQ_BIT; i <= REG_IRQSRC_SLD11IRQ_BIT; i++) {
		if ((val >> i) & 0x01) {
			aw933xx_SLDx_read(i - REG_IRQSRC_SLD0IRQ_BIT, &sld);
			aw933xx_SLDx_parse(i - REG_IRQSRC_SLD0IRQ_BIT, sld);
			break;
		}
	}
	aw_cap_i2c_write(REG_CMD, 0x0c);
}

#if defined(AW933XX_OS_USED) && (AW933XX_OS_USED == 1)
static void aw933xx_thread_cb(void)
{
	AW_S32 ret = 0;
	struct aw933xx_msg *msg = NULL;

	while (1) {
		ret = aw933xx.aw_func->os_func->fifo_get((void *)&msg);
		if (msg == NULL || ret != AW_OK) {
			AW933XX_INF("msg get error!");
			continue;
		}
		switch (msg->evt_id) {
		case AW933XX_EVT_ID_INTR: //external interrupt
			aw933xx_irq_process_impl();
			break;
#if defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1)
		case AW933XX_EVT_ID_GET_DBG_CURVE_DAT:
			aw933xx_dbg_send_curve_dat();
			break;
#endif
		default:
			break;
		}
		aw933xx.aw_func->os_func->fifo_free(msg);
	}
}
#endif

/**
 * @brief callback of interrupt
 *
 */
static void aw933xx_irq_process_cb(void)
{
#if defined(AW933XX_OS_USED) && (AW933XX_OS_USED == 1)
	struct aw933xx_msg msg;
	msg.evt_id = AW933XX_EVT_ID_INTR;
	aw933xx.aw_func->os_func->fifo_put(&msg);
#else
	aw933xx_irq_process_impl();
#endif
}

#if defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1)
/**
 * @brief this func used to parse data reviced via spp.
 *
 * @param in data recived
 * @param in_len revived data len.
 * @param out data stored in type 'struct aw933xx_dbg_dat'.
 * @return AW_S32 0 if unpack successed. others if unpack error.
 */
static AW_S32 aw933xx_data_unpack(AW_U8 *in, AW_U8 in_len, struct aw933xx_dbg_dat *out)
{
	AW_U32 valid_dat_len = 0;
	AW_U64 sum_r = 0;
	AW_U64 sum_v = 0;

	if (in[0] != APK_HEADER) {
		AW933XX_ERR("frame header error, 0x%x.", in[0]);
		return -AW_ERROR_DBG_FRAME_HEADER;
	}

	if (in[in_len - 2] != APK_END0 || in[in_len - 1] != APK_END1) {
		AW933XX_ERR("frame end error, 0x%x 0x%x", in[in_len - 2], in[in_len - 1]);
		return -AW_ERROR_DBG_FRAME_END;
	}

	valid_dat_len = ((AW_U32)in[3] << 8) | in[2];

	for (AW_U8 i = 0; i < valid_dat_len; i++)
		sum_v += (long)in[i + 4];

	sum_r = (AW_U64)(((AW_U64)in[APK_VALID_DATA_HEADER_V1 + valid_dat_len]) |
			((AW_U64)in[APK_VALID_DATA_HEADER_V1 + valid_dat_len + 1]) << 8 |
			((AW_U64)in[APK_VALID_DATA_HEADER_V1 + valid_dat_len + 2]) << 16 |
			((AW_U64)in[APK_VALID_DATA_HEADER_V1 + valid_dat_len + 3]) << 24 |
			((AW_U64)in[APK_VALID_DATA_HEADER_V1 + valid_dat_len + 4]) << 32);
	if (sum_r != sum_v) {
		AW933XX_ERR("sum_r = 0x%lld  sum_v = 0x%lld.", sum_r, sum_v);
		return -AW_ERROR_DBG_FRAME_SUM;
	}
	out->cmd = in[1];
	out->len = valid_dat_len;
	memcpy((void *)out->dat, (void *)&in[APK_VALID_DATA_HEADER_V1], valid_dat_len);

	return AW_OK;
}

/**
 * @brief this func pack data.
 *
 * @param cmd the purpose this package of data.
 * @param in the data to be sent.
 * @param in_len length of buf.
 * @param out data after package.
 */
static void aw933xx_data_pack_v1(AW_U8 cmd, AW_U8 *in, AW_U16 in_len, AW_U8 *out, AW_U16 *out_len)
{
	AW_U64 sum = 0;

	out[0] = APK_HEADER;
	out[1] = cmd;
	out[2] = (AW_U8)(in_len & 0xff);
	out[3] = (AW_U8)(in_len >> 8) & 0Xff;

	for (AW_U16 i = 0; i < in_len; i++) {
		sum += in[i];
		out[APK_VALID_DATA_HEADER_V1 + i] = in[i];
	}

	out[APK_VALID_DATA_HEADER_V1 + in_len] = (AW_U8)(sum & 0xff);
	out[APK_VALID_DATA_HEADER_V1 + in_len + 1] = (AW_U8)((sum >> 8) & 0xff);
	out[APK_VALID_DATA_HEADER_V1 + in_len + 2] = (AW_U8)((sum >> 16) & 0xff);
	out[APK_VALID_DATA_HEADER_V1 + in_len + 3] = (AW_U8)((sum >> 24) & 0xff);
	out[APK_VALID_DATA_HEADER_V1 + in_len + 4] = (AW_U8)((sum >> 32) & 0xff);
	out[APK_VALID_DATA_HEADER_V1 + in_len + 5] = APK_END0;
	out[APK_VALID_DATA_HEADER_V1 + in_len + 6] = APK_END1;

	*out_len = in_len + AW933XX_REDUNDANCY_DATA_LEN_V1;
}

/**
 * @brief this func pack valid data and send. frame head/end, cmd,
 * data len, sum check is added in this func.
 *
 * @param cmd the purpose this package of data.
 * @param buf the data to be sent.
 * @param length of buf.
 */
static void aw933xx_dbg_dat_send(AW_U8 cmd, AW_U8 *buf, AW_U32 length)
{
	AW_U8 send_total_buf[800] = { 0 };
	AW_U16 send_len = 0;

	aw933xx_data_pack_v1(cmd, buf, length, send_total_buf, &send_len);

	aw933xx.aw_func->dgb_func->data_send(send_total_buf, (AW_U16)(send_len));
}
/**
 * @brief this func is used to send dev(aw ic) info,
 *        inlcude sensoor type(byte 0), reg data width(byte 1), reg addr
 *        width(byte 2), interval between communication(byte 3) how many
 *        curve packs contained in one communication(byte 4), curve data
 *        width(byte 5, some ic store one data in multiple regs.)
 *
 */
static void aw933xx_dbg_send_dev_info(void)
{
	AW_U8 buf[6];

	buf[0] = AW933XX_CHIP_CAP;
	buf[1] = AW_SAR_REG_LEN;
	buf[2] = AW_SAR_DATA_LEN;
	buf[3] = AW_COMM_CYCLE;
	buf[4] = PACKS_IN_ONE_COMM;
	buf[5] = AW_CURVE_DATA_LEN;
	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_GET_DEVICE_INFO, buf, sizeof(buf));
}

/**
 * @brief get register value.
 *
 * @param app_data data recived via spp.
 */
static void aw933xx_dbg_get_reg(struct aw933xx_dbg_dat app_data)
{
	AW_U16 addr = 0;
	AW_U8 sbuf[AW_SPP_REG_MAX_NUM * SAR_REG_DATA_LEN] = { 0 };

	memcpy(&addr, app_data.dat, sizeof(AW_U16));
	if (app_data.dat[2] > AW_SPP_REG_MAX_NUM)
		app_data.dat[2] = AW_SPP_REG_MAX_NUM;

	aw_cap_i2c_read_seq(addr, sbuf, app_data.dat[2] * SAR_REG_DATA_LEN);
	for (AW_S32 i = 0; i < app_data.dat[2]; i++) {
		AW_U8 tmp = sbuf[(i * SAR_REG_DATA_LEN) + 3];
		AW_U8 tmp1 = sbuf[(i * SAR_REG_DATA_LEN) + 2];
		sbuf[(i * SAR_REG_DATA_LEN) + 3] = sbuf[(i * SAR_REG_DATA_LEN)];
		sbuf[(i * SAR_REG_DATA_LEN) + 2] = sbuf[(i * SAR_REG_DATA_LEN) + 1];
		sbuf[(i * SAR_REG_DATA_LEN) + 1] = tmp1;
		sbuf[(i * SAR_REG_DATA_LEN)] = tmp;
	}

	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_READ_REG, sbuf, app_data.dat[2] * SAR_REG_DATA_LEN);

}

/**
 * @brief set register value.
 *
 * @param dbg_dat data recived via spp.
 */
static void aw933xx_dbg_set_reg(struct aw933xx_dbg_dat dbg_dat)
{
	AW_U8 sbuf = 0;
	AW_U16 addr = 0;
	AW_U32 data = 0;

	memcpy(&addr, dbg_dat.dat, sizeof(AW_U16));
	memcpy(&data, &dbg_dat.dat[2], sizeof(AW_U32));
	AW933XX_INF(" addr = 0x%x, data = 0x%x", addr, data);
	aw_cap_i2c_write(addr, data);

	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_WRITE_REG, &sbuf, sizeof(AW_U8));
}

static AW_U8 aw933xx_factory_cali_diff_read_set(AW_U8 mask)
{
	AW_U8 data[AW_CHANNEL_NUM_MAX * sizeof(AW_S32) + AW933XX_REDUNDANCY_DATA_LEN_V1] = {0};
	AW_S32 diff_th[AW_CHANNEL_NUM_MAX];
	struct aw933xx_dbg_dat data_valid;

#ifdef AW_FLASH_USED
	aw933xx.aw_func->p_flash_func->flash_read(0, data, sizeof(data));
#endif
	if (aw933xx_data_unpack(data, sizeof(data), &data_valid) != AW_OK) {
		AW933XX_ERR("aw933xx unpack err");
		return -AW_ERR;
	}

	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		if ((mask >> i) & 0x01) {
			diff_th[i] = AW933XX_BYTE_TO_INT(&data_valid.dat[i * sizeof(AW_S32) + 3]);
			// set threshold only when diff kept in flash within seted range.
			if (diff_th[i] < AW933XX_DIFF_PROX_MIN || diff_th[i] > AW933XX_DIFF_PROX_MAX) {
				AW933XX_ERR("aw933xx diff_th = %d, out of rangne %d ~ %d",
						diff_th[i], AW933XX_DIFF_PROX_MIN, AW933XX_DIFF_PROX_MAX);
				return -AW_ERR;
			} else {
				AW933XX_INF("aw933xx channel %d threshold is %d", i, diff_th[i]);
				aw_cap_i2c_write(AW933XX_DIFF_CH0 + (i * AW_REG_STEP), diff_th[i]);
			}
		}
	}

	return AW_OK;
}

/**
 * @brief this func pack and wirte approach data to flash.
 *
 * @param in data need to write.
 * @param len data len
 */
static AW_U32 aw933xx_factory_cali_diff_write(AW_U8 *in, AW_U32 len)
{
	AW_U8 out[DATA_MAX_LEN] = { 0 };
	AW_U16 out_len = 0;

	aw933xx_data_pack_v1(AW933XX_DBG_CMD_DIFF_APPROACH, in, len, out, &out_len);

#ifdef AW_FLASH_USED
	aw933xx.aw_func->p_flash_func->flash_write(0, out, out_len);
#endif

	return AW_OK;
}

static void aw933xx_short_circuit_state_send(AW_U32 gnd, AW_U32 vcc)
{
#ifdef AW933XX_SPP_USED
	AW_U8 sbuf[8] = {0};

	AW933XX_INF("short circuit detect gnd=[0x%08X], vcc=[0x%08X]", gnd, vcc);
	sbuf[0] = (uint8_t)gnd;
	sbuf[1] = (uint8_t)(gnd >> AW_BIT8);
	sbuf[2] = (uint8_t)(gnd >> AW_BIT16);
	sbuf[3] = (uint8_t)(gnd >> AW_BIT24);
	sbuf[4] = (uint8_t)vcc;
	sbuf[5] = (uint8_t)(vcc >> AW_BIT8);
	sbuf[6] = (uint8_t)(vcc >> AW_BIT16);
	sbuf[7] = (uint8_t)(vcc >> AW_BIT24);
	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_SHORT_CIRCUIT, sbuf, sizeof(sbuf));
#endif
}

/**
 * @brief this func detect if any channel connected to vdd or gnd.
 *
 */
static void aw933xx_spp_short_circuit_detect(void)
{
	AW_U32 gnd_stat = 0;
	AW_U32 vcc_stat = 0;

	aw933xx_short_circuit_detect_get_stat(&gnd_stat, &vcc_stat);
	aw933xx_short_circuit_state_send(gnd_stat, vcc_stat);
}

/**
 * @brief send diff-prox data.
 *
 * @param diff_avg diff avg in prox state
 * @param avg_state diff avg in prox state
 * @param snr_state
 */
static void aw933xx_diff_approach_send(AW_S32 *diff_avg, AW_U8 avg_state, AW_U8 snr_state)
{
#ifdef AW933XX_SPP_USED
	AW_U8 sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + 2] = { 0 };

	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		sbuf[i * sizeof(AW_S32) + 0] = (uint8_t)diff_avg[i];
		sbuf[i * sizeof(AW_S32) + 1] = (uint8_t)(diff_avg[i] >> AW_BIT8);
		sbuf[i * sizeof(AW_S32) + 2] = (uint8_t)(diff_avg[i] >> AW_BIT16);
		sbuf[i * sizeof(AW_S32) + 3] = (uint8_t)(diff_avg[i] >> AW_BIT24);
	}
	sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX] = avg_state;
	sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + 1] = snr_state;
	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_DIFF_APPROACH, sbuf, sizeof(sbuf));
#endif
}

/**
 * @brief this func sample diff data for 1s with press and calculate average value.
 *
 */
void aw933xx_diff_approach(void)
{
	AW_U8 diff_avg_save[AW_CHANNEL_NUM_MAX * sizeof(AW_S32)];
	AW_U32 ret = 0;
	AW_U32 sample_cnt = 100;
	AW_S32 diff[AW_CHANNEL_NUM_MAX];
	AW_S32 diff_avg[AW_CHANNEL_NUM_MAX];
	AW_S64 diff_sum[AW_CHANNEL_NUM_MAX];

	memset(diff_avg_save, 0, sizeof(diff_avg_save));
	memset(diff_avg, 0, sizeof(diff_avg));
	memset(diff_sum, 0, sizeof(diff_sum));
	memset(diff, 0, sizeof(diff));

	while (1) {
		sample_cnt--;
		if (sample_cnt <= 0) {
			sample_cnt = 100;
			break;
		}
		aw933xx_read_diff(diff);
		for (int i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
			diff[i] >>= 10;
			diff_sum[i] += diff[i];
		}
		aw_cap_delay_ms(10);
	}

	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		diff_avg[i] = diff_sum[i] / sample_cnt;
		AW933XX_INF("aw933xx diff_avg[%d] = 0x%x", i, diff_avg[i]);
		if (diff_avg[i] < AW933XX_DIFF_PROX_MIN || diff_avg[i] > AW933XX_DIFF_PROX_MAX)
			AW933XX_ERR("aw933xx diff_avg[%d] = 0x%x", i, diff_avg[i]);
		diff_avg_save[i * 4] = (AW_U8)(diff_avg[i] & AW_ONE_BYTE_1);
		diff_avg_save[i * 4 + 1] = (AW_U8)(diff_avg[i] >> AW_BIT8 & AW_ONE_BYTE_1);
		diff_avg_save[i * 4 + 2] = (AW_U8)(diff_avg[i] >> AW_BIT16 & AW_ONE_BYTE_1);
		diff_avg_save[i * 4 + 3] = (AW_U8)(diff_avg[i] >> AW_BIT24 & AW_ONE_BYTE_1);
	}
	ret = aw933xx_factory_cali_diff_write(diff_avg_save, sizeof(diff_avg_save));
	if (ret)
		AW933XX_ERR("aw933xx data write error");
	aw933xx_diff_approach_send(diff_avg, 0, 0);
}


static void aw933xx_diff_to_air_send(AW_S32 *diff_max, AW_S32 *diff_min)
{
#ifdef AW933XX_SPP_USED
	AW_U8 sbuf[2 * sizeof(AW_S32) * AW_CHANNEL_NUM_MAX] = { 0 };

	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		sbuf[i * sizeof(AW_S32) + 0] = (uint8_t)diff_max[i];
		sbuf[i * sizeof(AW_S32) + 1] = (uint8_t)(diff_max[i] >> AW_BIT8);
		sbuf[i * sizeof(AW_S32) + 2] = (uint8_t)(diff_max[i] >> AW_BIT16);
		sbuf[i * sizeof(AW_S32) + 3] = (uint8_t)(diff_max[i] >> AW_BIT24);
	}
	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 0] = (uint8_t)diff_min[i];
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 1] = (uint8_t)(diff_min[i] >> AW_BIT8);
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 2] = (uint8_t)(diff_min[i] >> AW_BIT16);
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 3] = (uint8_t)(diff_min[i] >> AW_BIT24);
	}
	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_DIFF_TO_AIR, sbuf, sizeof(sbuf));
#endif
}

/**
 * @brief this func sample diff data for 5s without press and peak to peak value.
 *
 */
void aw933xx_diff_to_air(void)
{
	AW_U32 sample_cnt = 500;
	AW_S32 diff[AW_CHANNEL_NUM_MAX];
	AW_S32 diff_max[AW_CHANNEL_NUM_MAX];
	AW_S32 diff_min[AW_CHANNEL_NUM_MAX];
	AW_S32 diff_to_air_vpp[AW_CHANNEL_NUM_MAX];
	AW_S64 diff_sum[AW_CHANNEL_NUM_MAX];

	memset(diff, 0, sizeof(diff));
	memset(diff_max, 0, sizeof(diff_max));
	memset(diff_min, 0, sizeof(diff_min));
	memset(diff_sum, 0, sizeof(diff_sum));
	memset(diff_to_air_vpp, 0, sizeof(diff_to_air_vpp));

	aw933xx_read_diff(diff);
	for (int i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		diff[i] >>= 10;
		diff_max[i] = diff[i];
		diff_min[i] = diff[i];
	}

	while (1) {
		sample_cnt--;
		if (sample_cnt <= 0) {
			break;
		}
		aw933xx_read_diff(diff);
		for (int i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
			diff[i] >>= 10;
			diff_sum[i] += diff[i];
			if (diff_max[i] < diff[i])
				diff_max[i] = diff[i];
			if (diff_min[i] > diff[i])
				diff_min[i] = diff[i];
		}
		aw_cap_delay_ms(10);
	}
	for (int i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		diff_to_air_vpp[i] = diff_max[i] - diff_min[i];
		AW933XX_INF("aw933xx diff_to_air_vpp[%d] = 0x%x", i, diff_to_air_vpp[i]);
		AW933XX_INF("aw933xx diff_max[%d] = 0x%x", i, diff_max[i]);
		AW933XX_INF("aw933xx diff_min[%d] = 0x%x", i, diff_min[i]);
	}
	aw933xx_diff_to_air_send(diff_max, diff_min);
	AW933XX_INF("leave");
}

void aw933xx_offset_cali_send(AW_U32 *offset, AW_U32 *init, AW_U8 offset_state, AW_U8 init_state)
{
#ifdef AW933XX_SPP_USED
	AW_U8 sbuf[2 * sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + 2] = { 0 };

	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		sbuf[i * sizeof(AW_S32) + 0] = (uint8_t)offset[i];
		sbuf[i * sizeof(AW_S32) + 1] = (uint8_t)(offset[i] >> AW_BIT8);
		sbuf[i * sizeof(AW_S32) + 2] = (uint8_t)(offset[i] >> AW_BIT16);
		sbuf[i * sizeof(AW_S32) + 3] = (uint8_t)(offset[i] >> AW_BIT24);
	}
	for (AW_U8 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 0] = (uint8_t)init[i];
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 1] = (uint8_t)(init[i] >> AW_BIT8);
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 2] = (uint8_t)(init[i] >> AW_BIT16);
		sbuf[sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + i * sizeof(AW_S32) + 3] = (uint8_t)(init[i] >> AW_BIT24);
	}
	sbuf[2 * sizeof(AW_S32) * AW_CHANNEL_NUM_MAX] = offset_state;
	sbuf[2 * sizeof(AW_S32) * AW_CHANNEL_NUM_MAX + 1] = init_state;
	aw933xx_dbg_dat_send(AW933XX_DBG_CMD_OFFSET_CALI, sbuf, sizeof(sbuf));
#endif
}

/**
 * @brief this func is used for parasitic capacitance calibration.
 *
 */
void aw933xx_offset_cali(void)
{
	AW_U8 offset_state = 0;
	AW_U8 init_state = 0;
	AW_U32 cnt = 0;
	AW_U32 reg = 0;
	AW_U32 crange[AW_CHANNEL_NUM_MAX];
	AW_U32 offset[AW_CHANNEL_NUM_MAX];
	AW_S32 initial_data_after_cali[AW_CHANNEL_NUM_MAX];
	AW_U32 reg_afe_cfg1[] = {REG_AFECFG1_CH0, REG_AFECFG1_CH1, REG_AFECFG1_CH2,
		REG_AFECFG1_CH3, REG_AFECFG1_CH4, REG_AFECFG1_CH5,
		REG_AFECFG1_CH6, REG_AFECFG1_CH7, REG_AFECFG1_CH8,
		REG_AFECFG1_CH9, REG_AFECFG1_CH10, REG_AFECFG1_CH11,};
	AW_U32 reg_afe_cfg0[] = {REG_AFECFG0_CH0, REG_AFECFG0_CH1, REG_AFECFG0_CH2,
		REG_AFECFG0_CH3, REG_AFECFG0_CH4, REG_AFECFG0_CH5,
		REG_AFECFG0_CH6, REG_AFECFG0_CH7, REG_AFECFG0_CH8,
		REG_AFECFG0_CH9, REG_AFECFG0_CH10, REG_AFECFG0_CH11};
	float cap[AW_CHANNEL_NUM_MAX];
	float aw933xx_cap_map[16][2] = { {0, 1.1}, {1, 2.2}, {2, 3.3}, {3, 4.4}, {4, 6.6}, {5, 7.7}, {6, 8.8}, {7, 9.9},
					{8, 11}, {9, 12.1}, {10, 13.2}, {11, 14.3}, {12, 16.5}, {13, 17.6}, {14, 18.7}, {15, 19.8} };

	AW_U32 offset_f = 0;
	AW_U32 offset_c = 0;

	//step 1: Parasitic capacitance calibration
	aw_cap_i2c_write(REG_SCANCTRL0, 0x00000fff);
	aw_cap_i2c_write(REG_SCANCTRL1, 0x00000fff);

	//step 2: Waiting 1s for calibration complete flag
	while (1) {
		if (cnt >= 100) {
			AW933XX_ERR("aw933xx get calibration complete flag error");
			break;
		}
		aw_cap_i2c_read(REG_STAT7, &reg);
		if (reg == 0)
			break;
		cnt++;
		aw_cap_delay_ms(10);
	}

	//step 3: Read offset data and restore
	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		aw_cap_i2c_read(reg_afe_cfg1[i], &offset[i]);
		offset[i] >>= AW_BIT8;
		offset[i] &= 0x0000ffff;
		AW933XX_INF("aw933xx reg = 0x%x  data = 0x%x", reg_afe_cfg1[i], offset[i]);
		offset_c = offset[i] & 0xff;
		offset_f = (offset[i] >> 8) & 0xff;
		offset[i] = (offset_c << 8) | offset_f;
	}

	//step 4: Read the initial data after calibration and restore
	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		aw_cap_i2c_read(REG_PSCBD_CH0 + i * AW_REG_STEP, (AW_U32 *)&initial_data_after_cali[i]);
		AW933XX_INF("aw933xx reg = 0x%x, initial_data_after_cali[%d] = 0x%x", reg_afe_cfg0[i], i, initial_data_after_cali[i]);
	}

	//step 5: Judge whether the offset meets the expectation
	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		if ((offset[i] > AW933XX_OFFSET_MAX) || (offset[i] < AW933XX_OFFSET_MIN)) {
			offset_state &= ~(1 << i);
			AW933XX_ERR("offset doesn't meets the expectation, offset[%d] = 0x%x", i, offset[i]);
		} else {
			offset_state |= 1 << i;
			AW933XX_INF("aw933xx offset[%d] = 0x%x", i, offset[i]);
		}
	}

	//step 6: Judge whether the initial data meets the expectation
	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		aw_cap_i2c_read(reg_afe_cfg0[i], &crange[i]);
		crange[i] >>= AW_BIT12;
		crange[i] &= 0x0000000f;
		AW933XX_INF("aw933xx crange[%d] = 0x%2x", i, crange[i]);
		for (AW_U32 j = 0; j < 16; j++) {
			if (crange[i] == aw933xx_cap_map[j][0]) {
				cap[i] = aw933xx_cap_map[j][1];
				AW933XX_INF("aw933xx cap[%d] = %d / 1000", i, (AW_U32)(cap[i] * 1000));
				break;
			}
		}
	}
	for (AW_U32 i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
		AW933XX_INF("aw933xx initial_data_after_cali[%d] = 0x%x", i, (AW_U32)(initial_data_after_cali[i]));
		AW_S32 result = (AW_S32)((initial_data_after_cali[i] >> 21) * cap[i]);
		if ((result < AW933XX_CAP_MIN) || (result > AW933XX_CAP_MAX)) {
			init_state &= ~(1 << i);
			AW933XX_ERR("aw933xx initial data doesn't meet the expection, result[%d] = (%d / 1000)", i, (AW_U32)(result * 1000));
		} else {
			init_state |= 1 << i;
			AW933XX_INF("aw933xx result[%d] = (%d / 1000)", i, (AW_U32)(result * 1000));
		}
	}
	aw933xx_offset_cali_send(offset, (AW_U32 *)initial_data_after_cali, offset_state, init_state);
}

/**
 * @brief this fun is used for dbg cmd handler. base on protocol v0.4
 *
 * @param dat data recived via spp.
 * @param len recived data len.
 */
static void aw933xx_dbg_cmd_handler(AW_U8 *dat, AW_U16 len)
{
	struct aw933xx_dbg_dat dbg_dat;

	if (aw933xx_data_unpack(dat, len, &dbg_dat)) {
		AW933XX_ERR(" unpack dbg data error.");
		return;
	}

	switch (dbg_dat.cmd) {
	case AW933XX_DBG_CMD_GET_DEVICE_INFO:
		AW933XX_INF("get dev info");
		aw933xx_dbg_send_dev_info();
		break;
	case AW933XX_DBG_CMD_GET_CURVE_DATA:
		AW933XX_INF("get curve info");
#if defined(AW933XX_OS_USED) && (AW933XX_OS_USED == 1)
		AW933XX_INF("get curve info  os");
		struct aw933xx_msg msg;
		msg.evt_id = AW933XX_EVT_ID_GET_DBG_CURVE_DAT;
		aw933xx.aw_func->os_func->fifo_put(&msg);
#else
		AW933XX_INF("get curve info  no os");
		aw933xx_dbg_send_curve_dat();
#endif
		break;
	case AW933XX_DBG_CMD_STOP_GET_CURVE_DATA:
		AW933XX_INF("stop get curve data");
		aw_cap_i2c_write(REG_IRQEN, aw933xx.irq_reg_val); // rewrite irq
		break;
	case AW933XX_DBG_CMD_START_GET_CURVE_DATA:
		AW933XX_INF("start get curve data");
		aw_cap_i2c_write(REG_IRQEN, 0); // disable irq
		break;
	case AW933XX_DBG_CMD_READ_REG:
		AW933XX_INF("get reg");
		aw933xx_dbg_get_reg(dbg_dat);
		break;
	case AW933XX_DBG_CMD_WRITE_REG:
		AW933XX_INF("set reg");
		aw933xx_dbg_set_reg(dbg_dat);
		break;
	case AW933XX_DBG_CMD_OFFSET_CALI:
		AW933XX_INF("AW933XX_DBG_CMD_OFFSET_CALI");
		aw933xx_offset_cali();
		break;
	case AW933XX_DBG_CMD_DIFF_TO_AIR:
		AW933XX_INF("AW933XX_DBG_CMD_DIFF_TO_AIR");
		aw933xx_diff_to_air();
		break;
	case AW933XX_DBG_CMD_DIFF_APPROACH:
		AW933XX_INF("AW933XX_DBG_CMD_DIFF_APPROACH");
		aw933xx_diff_approach();
		break;
	case AW933XX_DBG_CMD_SHORT_CIRCUIT:
		AW933XX_INF("AW933XX_DBG_CMD_SHORT_CIRCUIT");
		aw933xx_spp_short_circuit_detect();
		break;
	case AW933XX_DBG_CMD_SET_DIFF_TH:
		AW933XX_INF("AW933XX_DBG_CMD_SET_DIFF_TH");
		aw933xx_factory_cali_diff_write(dbg_dat.dat, dbg_dat.len);
		break;

	default:
		AW933XX_ERR("no such commnd(%d).", dbg_dat.cmd);
		break;
	}
}
#endif

#if	(defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1))

/**
 * @brief this func send data to dbg to draw curve.
 *
 */
static void aw933xx_dbg_send_curve_dat(void)
{
	AW_U32 i = 0;
	AW_U32 cnt = 0;
	AW_S32 spp_offset = 0;
	AW_S32 raw_data[AW933XX_CURVER_LEN] = { 0 };
	AW_S32 valid[AW933XX_CURVER_LEN] = { 0 };
	AW_S32 baseline[AW933XX_CURVER_LEN] = { 0 };
	AW_S32 diff[AW933XX_CURVER_LEN] = { 0 };
	AW_U32 now_pos[AW933XX_REPORT_POS_DISTANCE_NUMS] = { 0 };
	AW_U32 move_distance[AW933XX_REPORT_POS_DISTANCE_NUMS] = { 0 };
	AW_U8 *buf = malloc(AW_SAR_SPP_ONE_PACK_LEN * PACKS_IN_ONE_COMM);
	AW_U16 pos_regs[AW933XX_REPORT_POS_DISTANCE_NUMS] = { REG_NOW_POS_SLD0,
												 REG_NOW_POS_SLD1, };
	AW_U16 distance_regs[AW933XX_REPORT_POS_DISTANCE_NUMS] =
					{ REG_MOVE_DISTANCE_SLD0, REG_MOVE_DISTANCE_SLD1 };

	if (buf == NULL) {
		AWLOGE("malloc apply error!");
		return;
	}
	memset(buf, 0, AW_SAR_SPP_ONE_PACK_LEN * PACKS_IN_ONE_COMM);
	AWLOGI("packs size=%d", AW_SAR_SPP_ONE_PACK_LEN * PACKS_IN_ONE_COMM);

	while (AW_TRUE) {
		for (i = 0; i < AW_CHANNEL_NUM_MAX; i++) {
			aw_cap_i2c_read(AW933XX_RAW_CH0 + (i * AW_REG_STEP), (AW_U32 *)&raw_data[i]);
			aw_cap_i2c_read(AW933XX_VALID_CH0 + (i * AW_REG_STEP), (AW_U32 *)&valid[i]);
			aw_cap_i2c_read(AW933XX_BASELINE_CH0 + (i * AW_REG_STEP), (AW_U32 *)&baseline[i]);
			aw_cap_i2c_read(AW933XX_DIFF_CH0 + (i * AW_REG_STEP), (AW_U32 *)&diff[i]);
		}
		for (i = 0; i < AW933XX_REPORT_POS_DISTANCE_NUMS; i++) {
			aw_cap_i2c_read(pos_regs[i], &now_pos[i]);
			aw_cap_i2c_read(distance_regs[i], &move_distance[i]);
		}

		for (i = 0; i < AW933XX_CURVER_LEN; i++) {
			raw_data[i] >>= 10;
			spp_offset = i * 4 + cnt * AW_SAR_SPP_ONE_PACK_LEN;
			buf[spp_offset] = (AW_U8)(raw_data[i] & 0xff);
			buf[spp_offset + 1] = (AW_U8)(raw_data[i] >> 8 & 0xff);
			buf[spp_offset + 2] = (AW_U8)(raw_data[i] >> 16 & 0xff);
			buf[spp_offset + 3] = (AW_U8)(raw_data[i] >> 24 & 0xff);
		}

		for (i = 0; i < AW933XX_CURVER_LEN; i++) {
			spp_offset = AW_CURVE_DATA_LEN + i * 4 + cnt * AW_SAR_SPP_ONE_PACK_LEN;
			valid[i] >>= 10;
			buf[spp_offset] = (AW_U8)(valid[i] & 0xff);
			buf[spp_offset + 1] = (AW_U8)(valid[i] >> 8 & 0xff);
			buf[spp_offset + 2] = (AW_U8)(valid[i] >> 16 & 0xff);
			buf[spp_offset + 3] = (AW_U8)(valid[i] >> 24 & 0xff);
		}

		for (i = 0; i < AW933XX_CURVER_LEN; i++) {
			spp_offset = AW_CURVE_DATA_LEN * 2 + i * 4 + cnt * AW_SAR_SPP_ONE_PACK_LEN;
			baseline[i] >>= 10;
			buf[spp_offset] = (AW_U8)(baseline[i] & 0xff);
			buf[spp_offset + 1] = (AW_U8)(baseline[i] >> 8 & 0xff);
			buf[spp_offset + 2] = (AW_U8)(baseline[i] >> 16 & 0xff);
			buf[spp_offset + 3] = (AW_U8)(baseline[i] >> 24 & 0xff);
		}

		for (AW_U32 i = 0; i < AW933XX_CURVER_LEN; i++) {
			spp_offset = AW_CURVE_DATA_LEN * 3 + i * 4 + cnt * AW_SAR_SPP_ONE_PACK_LEN;
			diff[i] >>= 10;
			buf[spp_offset] = (AW_U8)(diff[i] & 0xff);
			buf[spp_offset + 1] = (AW_U8)(diff[i] >> 8 & 0xff);
			buf[spp_offset + 2] = (AW_U8)(diff[i] >> 16 & 0xff);
			buf[spp_offset + 3] = (AW_U8)(diff[i] >> 24 & 0xff);
		}

		for (i = 0; i < VIRTUAL_REG_NUM; i++) {
			spp_offset = AW_CURVE_DATA_LEN * 4 + i * 4 + cnt * AW_SAR_SPP_ONE_PACK_LEN;
			//reserve
			buf[spp_offset] = 0x04;
			buf[spp_offset + 1] = 0x03;
			buf[spp_offset + 2] = 0x02;
			buf[spp_offset + 3] = 0x01;
		}

		aw933xx_irq_process_impl(); /*analyse EVENT*/
		spp_offset = AW_CURVE_DATA_LEN * 4 +
			VIRTUAL_REG_BUF_LEN + cnt * AW_SAR_SPP_ONE_PACK_LEN;
		buf[spp_offset + SINGLE_CLICK] = (aw933xx.event.click == 1) ? 1 : 0;
		buf[spp_offset + DOUBLE_CLICK] = (aw933xx.event.click == 2) ? 1 : 0;
		buf[spp_offset + TRIPLE_CLICK] = (aw933xx.event.click == 3) ? 1 : 0;
		buf[spp_offset + LONG_PRESS] = (aw933xx.event.press == 1) ? 1 : 0;
		buf[spp_offset + SUPER_LONG_PRESS] = (aw933xx.event.long_press == 1) ? 1 : 0;
		buf[spp_offset + LEFT_WAREDS] = (aw933xx.event.left_wareds == 1) ? 1 : 0;
		buf[spp_offset + RIGHT_WAREDS] = (aw933xx.event.right_wareds == 1) ? 1 : 0;
		buf[spp_offset + WEAR] = (aw933xx.event.wear == 1) ? 1 : 2;
		buf[spp_offset + UP_WAREDS] = (aw933xx.event.up_wareds == 1) ? 1 : 0;
		buf[spp_offset + DOWN_WAREDS] = (aw933xx.event.down_wareds == 1) ? 1 : 0;
		buf[spp_offset + BTN_POS_X] = aw933xx.event.btn_pos_x;
		buf[spp_offset + BTN_POS_Y] = aw933xx.event.btn_pos_y;

		AWLOGD("buf[%d]=single_click:0x%x", spp_offset + SINGLE_CLICK,
				buf[spp_offset + SINGLE_CLICK]);
		AWLOGD("buf[%d]=double_click:0x%x", spp_offset + DOUBLE_CLICK,
				buf[spp_offset + DOUBLE_CLICK]);
		AWLOGD("buf[%d]=triple_click:0x%x", spp_offset + TRIPLE_CLICK,
				buf[spp_offset + TRIPLE_CLICK]);
		AWLOGD("buf[%d]=long_press:0x%x", spp_offset + LONG_PRESS,
				buf[spp_offset + LONG_PRESS]);
		AWLOGD("buf[%d]=super_perss:0x%x", spp_offset + SUPER_LONG_PRESS,
				buf[spp_offset + SUPER_LONG_PRESS]);
		AWLOGD("buf[%d]=left_wareds:0x%x", spp_offset + LEFT_WAREDS,
				buf[spp_offset + LEFT_WAREDS]);
		AWLOGD("buf[%d]=right_wareds:0x%x", spp_offset + RIGHT_WAREDS,
				buf[spp_offset + RIGHT_WAREDS]);
		AWLOGD("buf[%d]=in_ear:0x%x", spp_offset + WEAR, buf[spp_offset + WEAR]);
		AWLOGD("buf[%d]=up_wareds:0x%x", spp_offset + UP_WAREDS,
				buf[spp_offset + UP_WAREDS]);
		AWLOGD("buf[%d]=down_wareds:0x%x", spp_offset + DOWN_WAREDS,
				buf[spp_offset + DOWN_WAREDS]);
		AWLOGD("buf[%d]=btn_pos_x:0x%x", spp_offset + BTN_POS_X,
				buf[spp_offset + BTN_POS_X]);
		AWLOGD("buf[%d]=btn_pos_y:0x%x", spp_offset + BTN_POS_Y,
				buf[spp_offset + BTN_POS_Y]);

		spp_offset = AW_CURVE_DATA_LEN * 4 + VIRTUAL_REG_BUF_LEN +
						cnt * AW_SAR_SPP_ONE_PACK_LEN + AW_SAR_STATUS_LEN;
		for (i = 0; i < AW933XX_REPORT_POS_DISTANCE_NUMS; i ++) {
			*(AW_U32 *)(buf + spp_offset + i * SAR_REG_DATA_LEN * 2) = now_pos[i];
			AWLOGD("pos = 0x%08X, x = 0x%04X, y = 0x%04X",
				now_pos[i], now_pos[i] & 0xFFFF, now_pos[i] >> OFFSET_BIT_16);

			*(AW_U32 *)(buf + spp_offset + i * SAR_REG_DATA_LEN * 2 + 1) = move_distance[i];
			AWLOGD("distance = 0x%08X, x = 0x%04X, y = 0x%04X",
				move_distance[i], move_distance[i] & 0xFF, move_distance[i] >> OFFSET_BIT_16);
		}

		cnt++;
		if (cnt >= PACKS_IN_ONE_COMM) {
			aw933xx_dbg_dat_send(AW933XX_DBG_CMD_GET_CURVE_DATA, buf, AW_SAR_SPP_ONE_PACK_LEN * PACKS_IN_ONE_COMM);
			cnt = 0;
			break;
		}

		aw_cap_delay_ms(AW_COMM_CYCLE / PACKS_IN_ONE_COMM);
	}
	free(buf);
}
#endif

/**
 * @brief call this func to init driver
 *
 * @param aw_func the funcs init by platform
 * @return AW_S32 success(0) or not
 */
AW_S32 aw933xx_init(const struct aw_func_t *aw_func)
{
	AW_S32 ret = 0;
	AW_U32 val = 0;

	AW933XX_INF("AW933XX driver version %s", AW933XX_DRIVER_VERSION);
	AW_PTR_NULL_CHECK(aw_func);
//	AW_PTR_NULL_CHECK(aw_func->reset);
	AW_PTR_NULL_CHECK(aw_func->i2c_func);
	AW_PTR_NULL_CHECK(aw_func->i2c_func->i2c_read);
	AW_PTR_NULL_CHECK(aw_func->i2c_func->i2c_write);
	AW_PTR_NULL_CHECK(aw_func->i2c_func->i2c_read_seq);
	AW_PTR_NULL_CHECK(aw_func->i2c_func->i2c_write_seq);
	AW_PTR_NULL_CHECK(aw_func->delay);
	AW_PTR_NULL_CHECK(aw_func->set_irq_cb);
#if defined(AW933XX_OS_USED) && (AW933XX_OS_USED == 1)
	AW_PTR_NULL_CHECK(aw_func->os_func);
	AW_PTR_NULL_CHECK(aw_func->os_func->set_thread_cb);
	AW_PTR_NULL_CHECK(aw_func->os_func->fifo_init);
	AW_PTR_NULL_CHECK(aw_func->os_func->fifo_put);
	AW_PTR_NULL_CHECK(aw_func->os_func->fifo_get);
	AW_PTR_NULL_CHECK(aw_func->os_func->fifo_free);
#endif
#if defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1)
	AW_PTR_NULL_CHECK(aw_func->dgb_func);
	AW_PTR_NULL_CHECK(aw_func->dgb_func->data_send);
	AW_PTR_NULL_CHECK(aw_func->dgb_func->set_rd_cb);
#endif

	aw933xx.aw_func = aw_func;
	aw933xx_init_global_val();
	//aw_func->set_rst_cb();
	aw_cap_delay_ms(50);
	ret = aw933xx_read_chipid();
	if (ret != AW_OK) {
		AW933XX_ERR("read chip id error");
		return ret;
	}

	ret = aw933xx_sw_rst();
	if (ret != AW_OK) {
		AW933XX_ERR("software reset error");
		return ret;
	}

	ret = aw933xx_read_init_over_irq();
	if (ret != AW_OK) {
		ret = aw_cap_i2c_read(0xF800, &val);
		if (ret != AW_OK) {
			AW933XX_ERR("read init over irq error");
			return ret;
		}
		AW933XX_ERR("0xF800=0x%04X", val);
		if ((val >> 16) == 0) {
			AW933XX_ERR("check 0xF800 error");
			return ret;
		}
	}

	ret = aw933xx_coderam_update();
	if (ret != AW_OK) {
		AW933XX_ERR("cdoeram update error");
		return ret;
	}

	ret = aw933xx_load_params();
	if (ret != AW_OK) {
		AW933XX_ERR("load paras error");
		return ret;
	}
	ret = aw933xx_load_cfg();
	if (ret != AW_OK) {
		AW933XX_ERR("load cfg error");
		return ret;
	}

//	if (aw933xx.chip_id == AW93307_CHIP_ID)
		aw933xx_set_cs_as_irq(AW933XX_CSX_TO_IRQ);

#if defined(AW933XX_OS_USED) && (AW933XX_OS_USED == 1)
	aw933xx.aw_func->os_func->fifo_init();
	aw933xx.aw_func->os_func->set_thread_cb(aw933xx_thread_cb);
#endif
#if defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1)
	aw933xx.aw_func->dgb_func->set_rd_cb(aw933xx_dbg_cmd_handler);
#endif

	aw933xx.aw_func->set_irq_cb(aw933xx_irq_process_cb);

#ifdef AW_FLASH_USED
	if (aw933xx_factory_cali_diff_read_set(0x03))
		AW933XX_INF("aw933xx didn't set data in flash as threshold.");
#endif

	aw933xx_mode_switch(AW933XX_ACTIVE_MODE);
	// storage irqen stage
	aw_cap_i2c_read(REG_IRQEN, &aw933xx.irq_reg_val);

	AW933XX_INF("init success");

	return AW_OK;
}
