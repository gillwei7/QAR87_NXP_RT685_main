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
#include "app.h"
#include "fsl_spi.h"

/*======<SPI>=======*/
#define FIXED_BUFFER_SIZE (4)
#define NUM_FRAMES (3)

#define MAX_FRAME_SIZE 64
#define TRIGGER_HEX_VALUE 0x92
#define SHORT_PRESS_HEX_VALUE 0x21
#define LONG_PRESS_HEX_VALUE 0x23

void spi_handler_task(void *pvParameters); // <<< MODIFIED: 新的 SPI 消費者任務
void passive_handler_task(void *pvParameters);
void spi_task(void *pvParameters);


#endif /* SPI_HANDLER_H_ */
