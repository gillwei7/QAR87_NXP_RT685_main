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

// SW Version (Git Tag)
/*******************************************************************************
 * Release/Test: [R/T][Version]
 * Project Name: [QAR87/QAR88n]
 * SKU ID: 01: Dev Board, 02: Actual Board, 03: xMEMS(Actual Board)
 * User/UserDebug Build: 01: UserDebug, 02: User
 * Build Date: YYMMDD
 * Purposes: Power / Demo
 * CommitTime: Commit Date and Time
 *
 * Example:
 *   R01.QAR87.01.01.251122
 *   T01.QAR87.01.01.251122.Power
 ******************************************************************************/
#define HAL_MCU_APP_RELEASE        0
#define HAL_MCU_APP_VERSION        sw_version


#define SITUATION_ENABLE           1
#define SITUATION_DISENABLE        0

/*******************************************************************************
 * Prototypes and Variables
 ******************************************************************************/

// SW Version
#if HAL_MCU_APP_RELEASE
#if UsingQAR87BoardHwVersion == 0 // Dev Board
static uint8_t sw_version[40] = "R01.QAR88n.01.01.251227.Standalone";
#endif
#if UsingQAR87BoardHwVersion == 1 // Actual Board
static uint8_t sw_version[40] = "R01.QAR88n.02.01.251227.Standalone";
#endif

#else
#if UsingQAR87BoardHwVersion == 0 // Dev Board
static uint8_t sw_version[60] = "T03.QAR88n.01.01.251223.Standalone.2512271827";
#endif
#if UsingQAR87BoardHwVersion == 1 // Actual Board
static uint8_t sw_version[60] = "T03.QAR88n.02.01.251223.Standalone.2512271827";
#endif

#endif



/*******************************************************************************
 * Functions
 ******************************************************************************/
// Delay
void hal_delay_ms(uint32_t val);

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
