#ifndef _AW_CAP_H_
#define _AW_CAP_H_

#ifdef AW_DEBUG_LOG

#include "fsl_debug_console.h"

#if (defined(AW_STM32H7_P) && (AW_STM32H7_P == 1))
#define AW933XX_INF(fmt, ...) PRINTF("[%s:%d] INF: "fmt" \r\n", __func__, __LINE__, ##__VA_ARGS__)
#define AW933XX_DBG(fmt, ...) PRINTF("[%s:%d] DBG: "fmt" \r\n", __func__, __LINE__, ##__VA_ARGS__)
#define AW933XX_ERR(fmt, ...) PRINTF("[%s:%d] ERR: "fmt" \r\n", __func__, __LINE__, ##__VA_ARGS__)
#define AWLOGI(fmt, ...) PRINTF("[%s:%d] INF: "fmt" \r\n", __func__, __LINE__, ##__VA_ARGS__)
#define AWLOGD(fmt, ...) PRINTF("[%s:%d] DBG: "fmt" \r\n", __func__, __LINE__, ##__VA_ARGS__)
#define AWLOGE(fmt, ...) PRINTF("[%s:%d] ERR: "fmt" \r\n", __func__, __LINE__, ##__VA_ARGS__)
#endif

#if (defined(AW_BES2500_P) && (AW_BES2500_P == 1))
#include <hal_trace.h>
#include "example_bes2500.h"

#define AW933XX_INF(fmt, ...) TRACE(5, "[%s:%d] INF: "fmt" \n", __func__, __LINE__, ##__VA_ARGS__)
#define AW933XX_DBG(fmt, ...) TRACE(5, "[%s:%d] DBG: "fmt" \n", __func__, __LINE__, ##__VA_ARGS__)
#define AW933XX_ERR(fmt, ...) TRACE(5, "[%s:%d] ERR: "fmt" \n", __func__, __LINE__, ##__VA_ARGS__)
#define AWLOGI(fmt, ...) TRACE(5, "[%s:%d] INF: "fmt" \n", __func__, __LINE__, ##__VA_ARGS__)
#define AWLOGD(fmt, ...) TRACE(5, "[%s:%d] DBG: "fmt" \n", __func__, __LINE__, ##__VA_ARGS__)
#define AWLOGE(fmt, ...) TRACE(5, "[%s:%d] ERR: "fmt" \n", __func__, __LINE__, ##__VA_ARGS__)
#endif

#endif

#define AW_PRINT_PARAM(param)			AWLOGE(#param" is NULL")
#define AW_PTR_NULL_CHECK(ptr)		\
	do {	\
		if ((ptr) == NULL) {		\
			AW_PRINT_PARAM(ptr);	\
			return -1;	\
		}	\
	} while (0)

typedef AW_S32 (*aw_i2c_read_t)(AW_U16 reg_addr, AW_U32 *reg_data);
typedef AW_S32 (*aw_i2c_write_t)(AW_U16 reg_addr, AW_U32 reg_data);
typedef AW_S32 (*aw_i2c_read_seq_t)(AW_U16 addr, AW_U8 *data, AW_U32 len);
typedef AW_S32 (*aw_i2c_write_seq_t)(AW_U16 addr, AW_U8 *data, AW_U32 len);
struct aw_i2c_func_t {
	aw_i2c_read_t i2c_read;
	aw_i2c_write_t i2c_write;
	aw_i2c_read_seq_t i2c_read_seq;
	aw_i2c_write_seq_t i2c_write_seq;
};

typedef void (*aw_delay_t)(AW_U32 ms);
typedef void (*aw_set_irq_cb_t)(void (*)(void));
typedef void (*aw_reset_t)(void);
struct aw_func_t {
	const struct aw_i2c_func_t *i2c_func;
	const aw_set_irq_cb_t set_irq_cb;
	const aw_reset_t reset;
	const aw_delay_t delay;
#if (defined(AW_BES2500_P) && (AW_BES2500_P == 1))
#if defined(AW933XX_OS_USED) && (AW933XX_OS_USED == 1)
	const struct aw933xx_os_func *os_func;
#endif
#if defined(AW933XX_SPP_USED) && (AW933XX_SPP_USED == 1)
	const struct aw933xx_dbg_func *dgb_func;
#endif
#if defined(AW_FLASH_USED) && (AW_FLASH_USED == 1)
	const struct aw_flash_func *p_flash_func;
#endif

#endif
};

#endif /* _AW_CAP_H_ */
