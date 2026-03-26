
#if 0
#include "plat_types.h"
#include "hal_i2c.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_trace.h"
#include "cmsis_os.h"
#include "tgt_hardware.h"
#endif

#include "sx920x.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "hal_common.h"

/*define the value without Phase enable settings for easy changes in driver*/
static const struct smtc_reg_data sx920x_i2c_reg_setup[] =
{
#if 1 // Google A88D

	    {0x4004, 0x00000060},
	    {0x8010, 0x00000000},
	    {0x4008, 0x00000000},
	    {0x42C0, 0x00041000},
	    {0x42C4, 0x000000FF},
	    {0x8014, 0x00000000},
	    {0x8018, 0x00000000},
	    {0x801C, 0x00000000},
	    {0x8020, 0x00000000},
	    {0x8024, 0x0000001E},
	    {0x8204, 0x00000000},
	    {0x802C, 0x0000045F},
	    {0x8038, 0x0000005E},
	    {0x8044, 0x0000005E},
	    {0x8050, 0x0000045D},
	    {0x805C, 0x0000045D},
	    {0x8068, 0x0000045D},
	    {0x8074, 0x0000045D},
	    {0x8080, 0x0000045D},
	    {0x8034, 0x00004DB6},
	    {0x8040, 0x00004DAE},
	    {0x804C, 0x00004F7E},
	    {0x8058, 0x00004FFF},
	    {0x8064, 0x00004FFF},
	    {0x8070, 0x00004FFF},
	    {0x807C, 0x00004FFF},
	    {0x8088, 0x00004FFF},
	    {0x808C, 0x00100000},
	    {0x80AC, 0x02100000},
	    {0x80CC, 0x02100000},
	    {0x80EC, 0x00100000},
	    {0x810C, 0x00100000},
	    {0x812C, 0x00100000},
	    {0x814C, 0x00100000},
	    {0x816C, 0x00100000},
	    {0x809C, 0x40404040},
	    {0x80BC, 0x00DF0019},
	    {0x80DC, 0x008D0016},
	    {0x80FC, 0x40404040},
	    {0x811C, 0x40404040},
	    {0x813C, 0x40404040},
	    {0x815C, 0x40404040},
	    {0x817C, 0x40404040},
	    {0x8098, 0x00000000},
	    {0x80B8, 0x00000000},
	    {0x80D8, 0x00000000},
	    {0x80F8, 0x00000000},
	    {0x8118, 0x00000000},
	    {0x8138, 0x00000000},
	    {0x8158, 0x00000000},
	    {0x8178, 0x00000000},
	    {0x80A4, 0x00000000},
	    {0x80C4, 0x00000000},
	    {0x80E4, 0x00000000},
	    {0x8104, 0x00000000},
	    {0x8124, 0x00000000},
	    {0x8144, 0x00000000},
	    {0x8164, 0x00000000},
	    {0x8184, 0x00000000},
	    {0x80A8, 0x00000000},
	    {0x80C8, 0x00000000},
	    {0x80E8, 0x00000000},
	    {0x8108, 0x00000000},
	    {0x8128, 0x00000000},
	    {0x8148, 0x00000000},
	    {0x8168, 0x00000000},
	    {0x8188, 0x00000000},
	    {0x819C, 0x00000000},
	    {0x81A0, 0x00000000},
	    {0x8090, 0x20608000},
	    {0x80B0, 0x20608519},
	    {0x80D0, 0x20608519},
	    {0x80F0, 0x20601800},
	    {0x8110, 0x20601800},
	    {0x8130, 0x20601800},
	    {0x8150, 0x20601800},
	    {0x8170, 0x20601800},
	    {0x8094, 0x00000000},
	    {0x80B4, 0x7DE0F14F},
	    {0x80D4, 0x7DE0F14F},
	    {0x80F4, 0x00000000},
	    {0x8114, 0x00000000},
	    {0x8134, 0x00000000},
	    {0x8154, 0x00000000},
	    {0x8174, 0x00000000},
	    {0x818C, 0x00000004},
	    {0x8194, 0x00000000},
	    {0x8190, 0x00080200},
	    {0x8198, 0x00000000},
	    {0x81A4, 0x0000130A},
	    {0x81C8, 0x00000000},
	    {0x81A8, 0x000005DC},
	    {0x81CC, 0x00000000},
	    {0x81B8, 0x00001770},
	    {0x81DC, 0x00000000},
	    {0x81AC, 0x00002EE0},
	    {0x81D0, 0x00000000},
	    {0x81BC, 0x00007530},
	    {0x81E0, 0x00000000},
	    {0x81B0, 0x000088B8},
	    {0x81D4, 0x00000000},
	    {0x81C0, 0x00011170},
	    {0x81E4, 0x00000000},
	    {0x81B4, 0x00038270},
	    {0x81D8, 0x00000000},
	    {0x81C4, 0x000493E0},
	    {0x81E8, 0x00000000},
	    {0x81FC, 0x00000414},
	    {0x8200, 0x00000001},
	    {0x8030, 0x00000000},
	    {0x803C, 0x007901E4},
	    {0x8048, 0x00F083C2},
	    {0x8054, 0x00000000},
	    {0x8060, 0x00000000},
	    {0x806C, 0x00000000},
	    {0x8078, 0x00000000},
	    {0x8084, 0x00000000},
	    {0x8290, 0x2F020010},
	    {0x8028, 0x00000707},
	    {0x4280, 0x0000000F}

#else if // SAR EVK
	    {0x4004, 0x00000060},
	    {0x8010, 0x00000000},
		{0x4008, 0x00000100},
	    {0x42C0, 0x00041000},
	    {0x42C4, 0x000000FF},
	    {0x8014, 0x00000000},
	    {0x8018, 0x00000000},
	    {0x801C, 0x00000000},
	    {0x8020, 0x00000000},
	    {0x8024, 0x0000001F},
	    {0x8204, 0x00000000},
	    {0x802C, 0x0000045F},
	    {0x8038, 0x0000005D},
	    {0x8044, 0x0000005D},
	    {0x8050, 0x0000045D},
	    {0x805C, 0x0000045D},
	    {0x8068, 0x0000045D},
	    {0x8074, 0x0000045D},
	    {0x8080, 0x0000045D},
	    {0x8034, 0x00004DB6},
	    {0x8040, 0x00004DAE},
	    {0x804C, 0x00004F7E},
	    {0x8058, 0x00004FFF},
	    {0x8064, 0x00004FFF},
	    {0x8070, 0x00004FFF},
	    {0x807C, 0x00004FFF},
	    {0x8088, 0x00004FFF},
	    {0x808C, 0x00100000},
	    {0x80AC, 0x00300000},
	    {0x80CC, 0x00200000},
	    {0x80EC, 0x00100000},
	    {0x810C, 0x00100000},
	    {0x812C, 0x00100000},
	    {0x814C, 0x00100000},
	    {0x816C, 0x00100000},
	    {0x809C, 0x40404040},
	    {0x80BC, 0x00F5002C},
	    {0x80DC, 0x00F50028},
	    {0x80FC, 0x40404040},
	    {0x811C, 0x40404040},
	    {0x813C, 0x40404040},
	    {0x815C, 0x40404040},
	    {0x817C, 0x40404040},
	    {0x8098, 0x00000000},
	    {0x80B8, 0x00000000},
	    {0x80D8, 0x00000000},
	    {0x80F8, 0x00000000},
	    {0x8118, 0x00000000},
	    {0x8138, 0x00000000},
	    {0x8158, 0x00000000},
	    {0x8178, 0x00000000},
	    {0x80A4, 0x00000000},
	    {0x80C4, 0x00000000},
	    {0x80E4, 0x00000000},
	    {0x8104, 0x00000000},
	    {0x8124, 0x00000000},
	    {0x8144, 0x00000000},
	    {0x8164, 0x00000000},
	    {0x8184, 0x00000000},
	    {0x80A8, 0x00000000},
	    {0x80C8, 0x00000000},
	    {0x80E8, 0x00000000},
	    {0x8108, 0x00000000},
	    {0x8128, 0x00000000},
	    {0x8148, 0x00000000},
	    {0x8168, 0x00000000},
	    {0x8188, 0x00000000},
	    {0x819C, 0x00000000},
	    {0x81A0, 0x00000000},
	    {0x8090, 0x20601800},
	    {0x80B0, 0x206080E6},
	    {0x80D0, 0x20608078},
	    {0x80F0, 0x20601800},
	    {0x8110, 0x20601800},
	    {0x8130, 0x20601800},
	    {0x8150, 0x20601800},
	    {0x8170, 0x20601800},
	    {0x8094, 0x00000000},
	    {0x80B4, 0x3CC49263},
	    {0x80D4, 0x3CC49263},
	    {0x80F4, 0x00000000},
	    {0x8114, 0x00000000},
	    {0x8134, 0x00000000},
	    {0x8154, 0x00000000},
	    {0x8174, 0x00000000},
	    {0x818C, 0x00000004},
	    {0x8194, 0x00000000},
	    {0x8190, 0x00080200},
	    {0x8198, 0x00000000},
	    {0x81A4, 0x0000100A},
	    {0x81C8, 0x00000000},
	    {0x81A8, 0x000005DC},
	    {0x81CC, 0x00000000},
	    {0x81B8, 0x00000640},
	    {0x81DC, 0x00000000},
	    {0x81AC, 0x00002328},
	    {0x81D0, 0x00000000},
	    {0x81BC, 0x00003A98},
	    {0x81E0, 0x00000000},
	    {0x81B0, 0x00004E20},
	    {0x81D4, 0x00000000},
	    {0x81C0, 0x00007918},
	    {0x81E4, 0x00000000},
	    {0x81B4, 0x0000A7F8},
	    {0x81D8, 0x00000000},
	    {0x81C4, 0x00011170},
	    {0x81E8, 0x00000000},
	    {0x81FC, 0x00000414},
	    {0x8200, 0x00000001},
	    {0x8030, 0x00000000},
	    {0x803C, 0x005B016C},
	    {0x8048, 0x00DB436D},
	    {0x8054, 0x00000000},
	    {0x8060, 0x00000000},
	    {0x806C, 0x00000000},
	    {0x8078, 0x00000000},
	    {0x8084, 0x00000000},
	    {0x8290, 0x2F020000},
	    {0x8028, 0x00000606},
	    {0x4280, 0x0000000F}
#endif
};



#define __SMTC_DBG
#define __SMTC_INF
#define __SMTC_ERR

#ifdef __SMTC_DBG
// 利用字串拼接將 fmt 和 "\r\n" 組合，確保每次 log 都會正確換行
#define SMTC_DBG(fmt, ...) PRINTF("SMTC_DBG %s(%d): " fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define SMTC_DBG(...) (void)0
#endif

#ifdef __SMTC_INF
#define SMTC_INF(fmt, ...) PRINTF("SMTC_INF %s(%d): " fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define SMTC_INF(...) (void)0
#endif

#ifdef __SMTC_ERR
#define SMTC_ERR(fmt, ...) PRINTF("SMTC_ERR %s(%d): " fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define SMTC_ERR(...) (void)0
#endif

#define DRV_TAG "sx920x"
static u32 l_prox_state = 0;


/* 記住上一次是否判定為「人體」 */
static bool s_body_prev = false;

typedef enum {
    OBJ_TYPE_NONE = 0,
    OBJ_TYPE_BODY,
    OBJ_TYPE_TABLE,
    OBJ_TYPE_OTHER
} obj_type_t;

static obj_type_t s_last_obj_type = OBJ_TYPE_NONE;

static int sx920x_i2c_write(u16 reg, u32 val)
{
/*
	static u8 buf[6];
	u32 ret = 0;
	
	buf[0] = (u8)(reg>>8);
	buf[1] = (u8)(reg & 0xFF);
	buf[2] = (u8)(val>>24);
	buf[3] = (u8)(val>>16);
	buf[4] = (u8)(val>>8);
	buf[5] = (u8)(val & 0xFF);

    ret = hal_i2c_send(HAL_I2C_ID_0, SEMTECH_ADDRESSS, buf, 2, 4, 0, NULL);
	if (ret != 0)
	{
		SMTC_ERR("ret= %ld reg= 0x%x val= 0x%lx", ret, reg, val);
		return -1;
	}
	return 0;	
*/
    status_t ret;
    uint8_t txBuff[4];

    // SX920X 的 Data 為 32-bit，且走 Big-Endian，由高位元組先送
    txBuff[0] = (uint8_t)(val >> 24);
    txBuff[1] = (uint8_t)(val >> 16);
    txBuff[2] = (uint8_t)(val >> 8);
    txBuff[3] = (uint8_t)(val & 0xFF);

    /*
     * NXP API 說明:
     * subAddress: 填入暫存器地址 (reg)
     * subaddressSize: 填入 2 (16-bit address)
     * txBuffSize: 填入 4 (32-bit data)
     */
    ret = BOARD_I3C_Send(BOARD_PMIC_I3C_BASEADDR, SEMTECH_ADDRESSS, (uint32_t)reg, 2, txBuff, 4);

    // kStatus_Success 通常等於 0
    if (ret != kStatus_Success)
    {
        SMTC_ERR("ret= %ld reg= 0x%x val= 0x%lx", (u32)ret, reg, val);
        return -1;
    }
    return 0;

}

static int sx920x_i2c_read(u16 reg, u32 *val)
{
/*
	u32 ret = 0;
	static u8 buf[6];

	buf[0] = (u8)(reg>>8);
	buf[1] = (u8)(reg & 0xFF);

	ret = hal_i2c_recv(HAL_I2C_ID_0, SEMTECH_ADDRESSS, buf, 2, 4, 1, 0, 0);
	if(ret != 0) 
	{
		SMTC_ERR("ret= %ld reg= 0x%x", ret, reg);
		return -1;
	}

    *val = ((u32)buf[2]<<24) | ((u32)buf[3]<<16) | ((u32)buf[4]<<8) | ((u32)buf[5]);
	return 0;
*/

    status_t ret;
    uint8_t rxBuff[4];

    /*
     * NXP API 說明:
     * subAddress: 填入暫存器地址 (reg)
     * subaddressSize: 填入 2 (16-bit address)
     * rxBuffSize: 填入 4 (預期讀取 32-bit data)
     */
    ret = BOARD_I3C_Receive(BOARD_PMIC_I3C_BASEADDR, SEMTECH_ADDRESSS, (uint32_t)reg, 2, rxBuff, 4);

    if (ret != kStatus_Success)
    {
        SMTC_ERR("ret= %ld reg= 0x%x", (u32)ret, reg);
        return -1;
    }

    // 將收到的 4 個 bytes 以 Big-Endian 格式組合成 32-bit 數值
    *val = ((u32)rxBuff[0] << 24) |
           ((u32)rxBuff[1] << 16) |
           ((u32)rxBuff[2] << 8)  |
           ((u32)rxBuff[3]);

    return 0;
}

/* 轉出中文描述，方便印 log */
const char* sx920x_event_to_str_zh(sx920x_event_t evt)
{
    switch (evt) {
    case SX920X_EVT_BODY_CLOSE: return "人體接近";
    case SX920X_EVT_BODY_FAR:   return "人體遠離";
		case SX920X_EVT_TABLE_CLOSE: return "非人體接近";
		case SX920X_EVT_TABLE_FAR:   return "非人體遠離";
    case SX920X_EVT_CLOSE:      return "接近";
    case SX920X_EVT_FAR:        return "遠離";
    default:                    return "無事件";
    }
}

/* 輪詢一次事件並產生直觀語意 */
int sx920x_poll_event(sx920x_event_t *evt)
{
    if (!evt) return -1;

    /* 1) 讀 IRQ 來源：讀它也會清除 IRQ（datasheet 建議流程） */
    u32 irq_src = 0;
    if (sx920x_i2c_read(SX920X_IRQ_SOURCE_REG, &irq_src) != 0) {
        SMTC_ERR("read IRQ src failed");
        return -1;
    }

    bool close_any = (irq_src & SX920X_IRQSRC_CLOSEANY) != 0;  // bit6
    bool far_any   = (irq_src & SX920X_IRQSRC_FARANY)   != 0;  // bit5

    /* 若沒有 close/far 邊緣，直接回無事件 */
    if (!close_any && !far_any) {
        *evt = SX920X_EVT_NONE;
        return 0;
    }

    /* 2) 讀 RegStatB 了解 Smart Human 狀態（A/B 引擎 + Startup） */
    u32 statb = 0;
    if (sx920x_i2c_read(SX920X_DEVICE_STATUS_B_REG, &statb) != 0) {
        SMTC_ERR("read RegStatB failed");
        return -1;
    }

    uint8_t a = (uint8_t)((statb >> SX920X_SMARTH_A_SHIFT) & SX920X_SMARTH_MASK); // [12:11]
    uint8_t b = (uint8_t)((statb >> SX920X_SMARTH_B_SHIFT) & SX920X_SMARTH_MASK); // [14:13]
    bool startup_any = (statb & SX920X_STARTUP_ANY_BIT) ? true : false;           // [15]

    /* 判斷當前物體類型 */
    bool body_now = (!startup_any) && ((a == SX920X_SMARTH_BODY) || (b == SX920X_SMARTH_BODY));
    bool table_now = (!startup_any) && ((a == SX920X_SMARTH_TABLE) || (b == SX920X_SMARTH_TABLE));

    /* 3) 決策：優先處理 close，再處理 far（理論上同時出現機率低） */
    sx920x_event_t out = SX920X_EVT_NONE;

    if (close_any) {
        if (body_now) {
            out = SX920X_EVT_BODY_CLOSE;
            s_last_obj_type = OBJ_TYPE_BODY;
        } else if (table_now) {
            out = SX920X_EVT_TABLE_CLOSE;
            s_last_obj_type = OBJ_TYPE_TABLE;
        } else {
            out = SX920X_EVT_CLOSE;
            s_last_obj_type = OBJ_TYPE_OTHER;
        }
    }
    else if (far_any) {
        /* 根據「接近時」的物體類型發送對應的遠離事件 */
        if (s_last_obj_type == OBJ_TYPE_BODY) {
            out = SX920X_EVT_BODY_FAR;
        } else if (s_last_obj_type == OBJ_TYPE_TABLE) {
            out = SX920X_EVT_TABLE_FAR;
        } else {
            out = SX920X_EVT_FAR;
        }
        s_last_obj_type = OBJ_TYPE_NONE; // 清除狀態
    }

    SMTC_DBG("IRQ_SRC=0x%lX (CLOSE=%d FAR=%d), RegStatB=0x%lX (A=%u B=%u startup=%d) -> %s",
             irq_src, close_any, far_any, statb, a, b, startup_any, sx920x_event_to_str_zh(out));

    *evt = out;
    return 0;
}


int sx920x_clear_irq(void)
{
	u32 irq_state = 0;

	//clear irq to release irq pin
	if (sx920x_i2c_read(SX920X_IRQ_SOURCE_REG, &irq_state) != 0) 
	{
		SMTC_ERR("");
		return -1;
	}	
	SMTC_DBG("SX920X_IRQ_SOURCE_REG = 0x%lx", irq_state);	

	return 0;
}

int sx920x_init_reg(void)
{
	int i = 0;
	u32 temp_val = 0;

	for (i = 0; i < ARRAY_SIZE(sx920x_i2c_reg_setup); i++) 
	{
		temp_val = sx920x_i2c_reg_setup[i].val;
		if(sx920x_i2c_reg_setup[i].reg == SX920X_IRQ_MASK_REG)
        {
            temp_val &= 0xEF;
            SMTC_INF("diable COMPDONEIRQEN");
        }

		SMTC_DBG("reg= 0x%x val= 0x%lx", sx920x_i2c_reg_setup[i].reg, temp_val);
		if (sx920x_i2c_write(sx920x_i2c_reg_setup[i].reg, temp_val) != 0) 
		{
			SMTC_ERR("");
			return -1;
		}

        if(sx920x_i2c_reg_setup[i].reg == SX920X_DEVICE_RESET_REG)
        {
            hal_delay_ms(10);
            SMTC_INF("sx920x reset, delay 10ms");
        }
	}

	return 0;
}

int sx920x_init(void)
{
	int ret = 0;
	u32 irq_src;
	SMTC_INF(DRV_TAG);

/*
    // I2C init
    hal_iomux_set_i2c0();

    static struct HAL_I2C_CONFIG_T adux_i2c_cfg;
#ifdef APP_I2C_USER_TASK
    adux_i2c_cfg.mode = HAL_I2C_API_MODE_TASK;
#else
    adux_i2c_cfg.mode = HAL_I2C_API_MODE_SIMPLE;
#endif
    adux_i2c_cfg.use_dma  = 0;
    adux_i2c_cfg.use_sync = 1;
    adux_i2c_cfg.speed = 100000;
    adux_i2c_cfg.as_master = 1;
    ret = hal_i2c_open(HAL_I2C_ID_0, &adux_i2c_cfg);
    SMTC_INF("hal_i2c_open ret= %ld", ret);
*/
    ret = sx920x_init_reg();
	if(ret != 0)
		SMTC_ERR("sx920x_init_reg ret= %ld", ret);

	//clear reset done irq
	if (sx920x_i2c_read(SX920X_IRQ_SOURCE_REG, &irq_src) != 0) 
		SMTC_ERR("clear reset done irq");

	
	//start sensing
	sx920x_i2c_write(SX920X_COMMAND_REG, SX920X_PHASE_ENABLE);
	
    return ret;
}

/*
void sx920x_enable_int(HAL_GPIO_PIN_IRQ_HANDLER func)
{
	
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&proxsensor_int_pin, 1);
    hal_gpio_pin_set_dir(proxsensor_int_pin.pin, HAL_GPIO_DIR_IN, 1);

    if (func){
        gpiocfg.irq_enable = 1;
        gpiocfg.irq_debounce = 1;
        gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
        gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
        gpiocfg.irq_handler = func;
        hal_gpio_setup_irq(proxsensor_int_pin.pin, &gpiocfg);
    }
	
}


void sx920x_disable_int(void)
{
    struct HAL_GPIO_IRQ_CFG_T gpiocfg;
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&proxsensor_int_pin, 1);
    hal_gpio_pin_set_dir(proxsensor_int_pin.pin, HAL_GPIO_DIR_IN, 0);

    gpiocfg.irq_enable = 0;
    gpiocfg.irq_debounce = 0;
    gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
    gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
    gpiocfg.irq_handler = NULL;
    hal_gpio_setup_irq(proxsensor_int_pin.pin, &gpiocfg);
}
*/

#if 1 
static void print_dbg_reg(void)
{
	u16 addr;
	u32 uData, ph_sel;
	s32 ant_use, ant_raw;
	s32 use_flt_dlt_var;
	s32 ph0_use;

	sx920x_i2c_read(SX920X_DEBUG_PHASESEL_REG, &ph_sel);
	ph_sel = (ph_sel >> 3) & 0x7;
	if (ph_sel > 5)
	{
		SMTC_ERR("invalid phase selection= 0x%lx\n", ph_sel);
		return;
	}

	sx920x_i2c_read(SX920X_DEBUG_RAW_REG, &uData);
	ant_raw = (s32)uData>>10;
	sx920x_i2c_read(SX920X_DEBUG_DELTA_REG, &uData);
	use_flt_dlt_var = (s32)uData>>4;

	sx920x_i2c_read(SX920X_USEFUL_PH0_REG, &uData);
	ph0_use = (s32)uData >> 10;

	
	addr = SX920X_USEFUL_PH0_REG + ph_sel*4;
	sx920x_i2c_read(addr, &uData);
	ant_use = (s32)uData>>10;
	
	//some platform doesn't support too long log
	//TRACE(0,"SDBG PH=%ld USE=%ld RAW=%ld PH0=%ld DLT=%ld END\n",ph_sel, ant_use, ant_raw, ph0_use, use_flt_dlt_var);
	PRINTF("SDBG PH=%ld USE=%ld RAW=%ld PH0=%ld DLT=%ld END\r\n", ph_sel, ant_use, ant_raw, ph0_use, use_flt_dlt_var);
}

#endif

static void print_dbg_log(void)
{
	u8 csx, index;
	s32 useful, average, diff;
	s32 ph0_use;
	u32 uData;
	u16 offset;
	int state;

	sx920x_i2c_read(SX920X_DEVICE_STATUS_A_REG, &uData);
	SMTC_DBG("SX920X_DEVICE_STATUS_A_REG[0x8000] = 0x%lx\n", uData);

	for(csx = 0; csx < 8; csx++)
	{
		index = csx*4;
		sx920x_i2c_read(SX920X_USEFUL_PH0_REG + index, &uData);
		useful = (s32)uData>>10;
		sx920x_i2c_read(SX920X_AVERAGE_PH0_REG + index, &uData);
		average = (s32)uData>>10;
		sx920x_i2c_read(SX920X_DIFF_PH0_REG + index, &uData);
		diff = (s32)uData>>10;
		sx920x_i2c_read(SX920X_OFFSET_PH0_REG + index*3, &uData);
		offset = (u16)(uData & 0x3FFF);

		state = (l_prox_state & (1<<csx)) == 0 ? 0 : 1; 

		//TRACE(0,"SDAT PH=%d  USE=%ld AVG=%ld DIF=%ld OFF=%d STAT=%d END\n", csx, useful, average,diff,offset,state);
		PRINTF("SDAT PH=%d  USE=%ld AVG=%ld DIF=%ld OFF=%d STAT=%d END\r\n", csx, useful, average, diff, offset, state);
	}

	//print_dbg_reg();
}

// This function is to read cap sensing state of all channels
int sx920x_read_state(u8 *state)
{	
	int ret = 0;
	u32 comp_state;
	*state = 0;

	sx920x_clear_irq();

	//wait for the completion of compensation
	if ( sx920x_i2c_read(SX920X_DEVICE_STATUS_B_REG, &comp_state) != 0) 
	{
		SMTC_ERR("read comp state");
		return -1;
	}
	comp_state &= 0xFF;
	SMTC_DBG("comp_state=0x%lX", comp_state);
	if(comp_state != 0)
	{
		SMTC_ERR("compensation is on going");
		return -2;
	}
	
	//Read cap sensing state
	if (sx920x_i2c_read(SX920X_DEVICE_STATUS_A_REG, &l_prox_state) != 0) 
	{
		SMTC_ERR("SX920X_DEVICE_STATUS_A_REG");
		return -1;
	}
	
	l_prox_state = l_prox_state >> 24;
	l_prox_state &= 0xFF;
	*state = l_prox_state;
	SMTC_DBG("prox state reg= 0x%X val= 0x%lx", SX920X_DEVICE_STATUS_A_REG, l_prox_state);

	print_dbg_log();
	SMTC_INF("state=0x%X", *state);
	
	return 0;	
}

int sx920x_compensate(void)
{
	int retry=100;
	u32 comp_state;

    SMTC_DBG("start compensate");
	//compensate chip
	if(sx920x_i2c_write(SX920X_COMMAND_REG, SX920X_COMPENSATION_CONTROL) != 0)
	{
		SMTC_ERR("compensate");
		return -1;		
	}
	//usually, the compensation time is more than 500 milliseconds
	hal_delay_ms(500);

	while(1)
	{		
		if ( sx920x_i2c_read(SX920X_DEVICE_STATUS_B_REG, &comp_state) != 0) 
		{
			SMTC_ERR("read comp state");
			return -1;
		}

		if(retry-- <= 0)
		{			
			SMTC_ERR("compasate timedout");
			return -1;
		}
		
		comp_state &= 0xFF;

		if(comp_state == 0)
			break;
		
		hal_delay_ms(100);
	}

    return 0;
}

