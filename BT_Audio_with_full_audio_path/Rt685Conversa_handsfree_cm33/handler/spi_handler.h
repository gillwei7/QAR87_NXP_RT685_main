/*
 * spi_handler.h
 *
 *  Created on: 2025年10月15日
 *      Author: 11301026
 */

#ifndef SPI_HANDLER_H_
#define SPI_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_spi.h"

/*======<SPI>=======*/
#define FIXED_BUFFER_SIZE (4)
#define STATUS_BUFFER_SIZE (7)
#define NUM_FRAMES (3)

#define MAX_FRAME_SIZE 64
#define TRIGGER_HEX_VALUE 0x92
#define SYSTEM_STATUS_HEX_VALUE 0x93
#define SHORT_PRESS_HEX_VALUE 0x21
#define LONG_PRESS_HEX_VALUE 0x23
#define ONE_TOUCH_HEX_VALUE 0x11
#define DOUBLE_TOUCH_HEX_VALUE 0x12
#define PRESS_TOUCH_HEX_VALUE 0x13
#define FORWARD_SLIDE_HEX_VALUE 0x14
#define BACK_SLIDE_HEX_VALUE 0x15

void spi_handler_task(void *pvParameters);


#endif /* SPI_HANDLER_H_ */
