/*
 * version.h
 *
 *  Created on: Dev 2, 2027
 *      Author: Gill
 */

#ifndef INC_VERSION_H_
#define INC_VERSION_H_

#define MCU_APP_RELEASE        0

#define MCU_APP_VERSION        version
#define MCU_APP_VERSION_TIME   version_time
#define MCU_APP_VERSION_SUB    version_sub

/* Rule of Version: [based version]_[commit_date]_[commit_time]_[sub_item] */
static uint8_t version[20] = "0.01";

#if MCU_APP_RELEASE
static uint8_t version_time[15] = "";
#else
static uint8_t version_time[15] = "_251202_0900";
#endif

#if MCU_APP_RELEASE
static uint8_t version_time[15] = "";
#else
static uint8_t version_sub[15] = "V0.1.6.2";
#endif

#endif /* INC_VERSION_H_ */
