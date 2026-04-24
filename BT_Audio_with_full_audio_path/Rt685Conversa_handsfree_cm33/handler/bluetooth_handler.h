/*
 * bluetooth_handler.h
 *
 *  Created on: Apr 18, 2026
 *      Author: Lydia
 */

#ifndef BLUETOOTH_HANDLER_H_
#define BLUETOOTH_HANDLER_H_

#include "hal_common.h"

void bluetooth_reconnect_timer_init (void);
void bluetooth_reconnect_timer_start(void);
void bluetooth_reconnect (void);

#endif /* BLUETOOTH_HANDLER_H_ */
