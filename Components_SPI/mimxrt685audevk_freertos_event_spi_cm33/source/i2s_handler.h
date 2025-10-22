/*
 * i2s_handler.h
 *
 *  Created on: 2025年10月22日
 *      Author: 11301026
 */

#ifndef I2S_HANDLER_H_
#define I2S_HANDLER_H_

#include "fsl_debug_console.h"
#include "board.h"
#include "app.h"
#include "fsl_dma.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"

void StopSoundPlayback(void);
void StartSoundPlayback(void);

void Init_I2S(void);

#endif /* I2S_HANDLER_H_ */
