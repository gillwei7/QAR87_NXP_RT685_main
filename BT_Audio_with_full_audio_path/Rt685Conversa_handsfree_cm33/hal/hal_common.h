/*
 * hal_common.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
/*******************************************************************************
 * Quanta hal functions
 ******************************************************************************/

#ifndef HAL_DRIVER_H_
#define HAL_DRIVER_H_


/*******************************************************************************
 * Includes
 ******************************************************************************/

/* Include platform header files */
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "board.h"
#include <stdarg.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
// Log levels
#define hal_log_info(has_file, has_func, message,...)  hal_log_message(LOG_INFO,__FILE__,__func__,has_file,has_func,message,## __VA_ARGS__)
#define hal_log_debug(has_file, has_func, message,...) hal_log_message(LOG_DEBUG,__FILE__,__func__,has_file,has_func,message,## __VA_ARGS__)
#define hal_log_error(has_file, has_func, message,...) hal_log_message(LOG_ERROR,__FILE__,__func__,has_file,has_func,message,## __VA_ARGS__)

// SW Version
#define HAL_MCU_APP_RELEASE        0
#define HAL_MCU_APP_VERSION        sw_version
#define HAL_MCU_APP_VERSION_TIME   version_time
#define HAL_MCU_APP_VERSION_SUB    version_sub




/*******************************************************************************
 * Prototypes and Variables
 ******************************************************************************/

// SW Version
/* Rule of Version: [based version]_[commit_date]_[commit_time]_[sub_item] */
static uint8_t sw_version[20] = "0.06";
#if HAL_MCU_APP_RELEASE
static uint8_t version_time[15] = "";
#else
static uint8_t version_time[15] = "_251021_1800";
#endif
static uint8_t version_sub[15] = "";



/*******************************************************************************
 * Functions
 ******************************************************************************/
// Delay
void hal_loop_delay_us(uint32_t val);
void hal_loop_delay_ms(uint32_t val);

// GPIO
void hal_gpio_init(void);
void hal_gpio_write_pin(uint8_t port, uint8_t pin, uint8_t state);
uint8_t hal_gpio_read_pin(uint8_t port, uint8_t pin);

// I2C
void hal_i3c_init(void);
void hal_scan_i2c_devices(I3C_Type *base);

// SPI
void hal_spi_init(void);

// SoC
void hal_soc_enable(void);

// HW Version
uint8_t hal_get_hw_version (void);

// Log levels
void hal_log_message(const int level,const char *file, const char *func, uint8_t has_file, uint8_t has_func, const char *message,...);

// Board Init
void hal_board_init(void);

#endif /* HAL_DRIVER_H_ */
#endif
