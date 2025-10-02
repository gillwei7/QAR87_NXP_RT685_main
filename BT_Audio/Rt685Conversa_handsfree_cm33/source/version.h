/*
 * version.h
 *
 *  Created on: 2025年10月2日
 *      Author: 11306067
 */

#ifndef VERSION_H_
#define VERSION_H_

#define MCU_APP_RELEASE        0

#define MCU_APP_VERSION        version
#define MCU_APP_VERSION_TIME   version_time
//#define MCU_APP_VERSION_SUB    version_sub

/* Rule of Version: [based version]_[commit_date]_[commit_time]_[sub_item] */
static uint8_t version[20] = "0.01";

#if MCU_APP_RELEASE
static uint8_t version_time[15] = "";
#else
static uint8_t version_time[15] = "_251002_1101";
#endif


#endif /* VERSION_H_ */
