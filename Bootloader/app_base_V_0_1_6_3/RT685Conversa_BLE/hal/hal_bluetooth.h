/*
 * hal_bluetooth.h
 *
 *  Created on: Oct 21, 2025
 *      Author: Lydia
 */
#if UsingQAR87Board == 1
#ifndef HAL_BLUETOOTH_H_
#define HAL_BLUETOOTH_H_

#include "hal_common.h"


void hal_bluetooth_set_connected(void);
void hal_bluetooth_set_disconnected(void);
void hal_bluetooth_get_connection_status(void);


#endif /* HAL_BLUETOOTH_H_ */

#endif
