/*
* aw883xx_base.h aw883xx codec driver
*
* Copyright (c) 2021 AWINIC Technology CO., LTD
*
* Author: <zhaolei@awinic.com>
*
* This program is free software; you can redistribute  it and/or modify it
* under  the terms of  the GNU General	Public License as published by the
* Free Software Foundation;  either version 2 of the  License, or (at your
* option) any later version.
*/

#ifndef __AWINIC_BASE_H__
#define __AWINIC_BASE_H__

#include <stdbool.h>
#include <stdint.h>
/*awinic:add and delete main.h according to the actual needs*/

#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
/********************************************
 * function control of each module
 *******************************************/

//#define AW_DEBUG
//#define AW_FADE
//#define AW_VOLUME
//#define AW_IRQ
//#define AW_MONITOR
#define AW_CALIB

/********************************************
 * delay
 *******************************************/
// Keep SDK_DelayAtLeastUs for AMP to avoid SPI transmission failures
//#define AW_MS_DELAY(time) SDK_DelayAtLeastUs((time) * 1000U, CLOCK_GetFreq(kCLOCK_CoreSysClk))
#define AW_MS_DELAY(time) vTaskDelay(pdMS_TO_TICKS(time))

/********************************************
 * print information control
 *******************************************/
//#define AWINIC_DEBUG_LOG
//#define AWINIC_INFO_LOG
#define AWINIC_ERR_LOG


#ifdef AWINIC_ERR_LOG
#define aw_dev_err(dev_index, format, ...); \
	do { \
		PRINTF("[Awinic][dev%d]%s: " format "\r\n", dev_index, __func__, ##__VA_ARGS__);  \
	} while (0)
#else
#define aw_dev_err(dev_index, format, ...)
#endif

#ifdef AWINIC_INFO_LOG
#define aw_dev_info(dev_index, format, ...) \
	do { \
		PRINTF("[Awinic][dev%d]%s: " format "\r\n", dev_index, __func__, ##__VA_ARGS__);  \
	} while (0)
#else
#define aw_dev_info(dev_index, format, ...)
#endif

#ifdef AWINIC_DEBUG_LOG
#define aw_dev_dbg(dev_index, format, ...); \
	do { \
		PRINTF("[Awinic][dev%d]%s: " format "\r\n", dev_index, __func__, ##__VA_ARGS__);  \
	} while (0)
#else
#define aw_dev_dbg(dev_index, format, ...)
#endif


#ifdef AWINIC_ERR_LOG
#define aw_pr_err(format, ...); \
	do { \
		PRINTF("[Awinic]%s: " format "\r\n", __func__, ##__VA_ARGS__);  \
	} while (0)
#else
#define aw_pr_err(format, ...)
#endif

#ifdef AWINIC_INFO_LOG
#define aw_pr_info(format, ...); \
	do { \
		PRINTF("[Awinic]%s: " format "\r\n", __func__, ##__VA_ARGS__);  \
	} while (0)
#else
#define aw_pr_info(format, ...)
#endif

#ifdef AWINIC_DEBUG_LOG
#define aw_pr_dbg(format, ...); \
	do { \
		PRINTF("[Awinic]%s: " format "\r\n", __func__, ##__VA_ARGS__);  \
	} while (0)
#else
#define aw_pr_dbg(format, ...)
#endif


/* Platform customization realization */
#define aw_mutex_lock() \
	do { \
		\
	} while (0)

#define aw_mutex_unlock() \
	do { \
		\
	} while (0)

/********************************************
 * dev_index information
 *******************************************/
typedef enum {
	AW_DEV_0 = 0,
	AW_DEV_1,
	AW_DEV_2,
	AW_DEV_3,
	AW_DEV_4,
	AW_DEV_5,
	AW_DEV_6,
	AW_DEV_7,
	AW_DEV_8,
	AW_DEV_9,
	AW_DEV_10,
	AW_DEV_11,
	AW_DEV_12,
	AW_DEV_13,
	AW_DEV_14,
	AW_DEV_15,
	AW_DEV_MAX,
} aw_dev_index_t;

typedef enum {
	AW_PHASE_SYNC_DISABLE = 0,
	AW_PHASE_SYNC_ENABLE,
} aw_phase_sync_t;

typedef enum {
	AW_FADE_DISABLE = 0,
	AW_FADE_ENABLE,
} aw_fade_en_t;

typedef enum {
	NOT_SINGLE = 0,
	IS_SINGLE,
} aw_single_t;

typedef enum {
	AW_RECORD_SEC_DATA = 0,
	AW_RECOVERY_SEC_DATA = 1,
}backup_sec_t;

typedef enum {
	AW_GET_DEV_PARAMS = 0,
	AW_SET_DEV_PARAMS = 1,
}params_option_t;

typedef enum {
	AW_GET_DEV_STATUS = 0,
	AW_SET_DEV_STATUS = 1,
}status_option_t;

typedef enum {
	AW_DEV_NONE_MSG = 0,
	AW_DEV_DSP_PARAMS,
	AW_DEV_HMUTE_PARAMS,
	AW_DEV_INT_PARAMS,
	AW_DEV_VOLUME_PARAMS,
	AW_DEV_FADE_STEP_PARAMS,
	AW_DEV_FADE_IN_TIME_PARAMS,
	AW_DEV_FADE_OUT_TIME_PARAMS,
	AW_DEV_CRC_FLAG_PARAMS,
	AW_DEV_SYSST_PARAMS,
	AW_DEV_DSP_RE_PARAMS,
	AW_DEV_BIN_RE_PARAMS,
	AW_DEV_TE_PARAMS,
	AW_DEV_F0_PARAMS,
	AW_DEV_R0_PARAMS,
	AW_DEV_RE_RANGE_PARAMS,
	AW_DEV_CALI_RE_PARAMS,
	AW_DEV_CALI_F0_PARAMS,
	AW_DEV_CALI_Q_PARAMS,
	AW_DEV_CALI_RE_F0_PARAMS,
	AW_DEV_CALI_F0_Q_PARAMS,
	AW_DEV_CALI_TIME_PARAMS,
	AW_DEV_STORE_CALI_RE_PARAMS,
	AW_DEV_REALTIME_CRC_GET_PARAMS,
	AW_DEV_REALTIME_CRC_SET_PARAMS,
}dev_params_t;

typedef enum {
	AW_DEV_NONE_STATUS = 0,
	AW_DEV_PLL_WDT_STATUS,
	AW_DEV_CLEAR_INT_STATUS,
	AW_DEV_INTERRUPT_CLEAR_STATUS,
	AW_DEV_SOFT_RESET_STATUS,
	AW_DEV_REG_DUMP_STATUS,
}dev_status_t;

struct cali_msg_hdr {
	uint32_t opcode_id;
	uint32_t msg[AW_DEV_MAX];
};

/********************************************
 * error symbol
 *******************************************/
#define	EPERM		 1	/* Operation not permitted */
#define	ENOENT		 2	/* No such file or directory */
#define	ESRCH		 3	/* No such process */
#define	EINTR		 4	/* Interrupted system call */
#define	EIO		 5	/* I/O error */
#define	ENXIO		 6	/* No such device or address */
#define	E2BIG		 7	/* Argument list too long */
#define	ENOEXEC		 8	/* Exec format error */
#define	EBADF		 9	/* Bad file number */
#define	ECHILD		10	/* No child processes */
#define	EAGAIN		11	/* Try again */
#define	ENOMEM		12	/* Out of memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	ENOTBLK		15	/* Block device required */
#define	EBUSY		16	/* Device or resource busy */
#define	EEXIST		17	/* File exists */
#define	EXDEV		18	/* Cross-device link */
#define	ENODEV		19	/* No such device */
#define	ENOTDIR		20	/* Not a directory */
#define	EISDIR		21	/* Is a directory */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* File table overflow */
#define	EMFILE		24	/* Too many open files */
#define	ENOTTY		25	/* Not a typewriter */
//#define	ETXTBSY		26	/* Text file busy */
#define	EFBIG		27	/* File too large */
#define	ENOSPC		28	/* No space left on device */
#define	ESPIPE		29	/* Illegal seek */
#define	EROFS		30	/* Read-only file system */
#define	EMLINK		31	/* Too many links */
#define	EPIPE		32	/* Broken pipe */
#define	EDOM		33	/* Math argument out of domain of func */
#define	ERANGE		34	/* Math result not representable */


#endif

