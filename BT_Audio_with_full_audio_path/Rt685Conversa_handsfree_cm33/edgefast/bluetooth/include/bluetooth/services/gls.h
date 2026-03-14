/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HTS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HTS_H_

/**
 * @brief Health Thermometer Service (HTS)
 * @defgroup bt_hts Health Thermometer Service (HTS)
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Definitions
******************************************************************************/
/* HTS flag values */
#define status_flag        0x00U /* bit 0 unset */


/* Status Update Structure */
struct status_update {
    uint8_t flags;
    char status[15];
};


/*******************************************************************************
* Prototypes
******************************************************************************/
/** @brief Notify indicate a temperature measurement.
 *
 * This will send a GATT indication to all current subscribers.
 * Awaits an indication response from peer.
 *
 *  @param none.
 *
 *  @return Zero in case of success and error code in case of error.
 */
void bt_status_notify(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_HTS_H_ */
