/* hts.h - Health Thermometer Service sample */

/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 * Copyright 2022 NXP
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/
void peripheral_gls_task(void *pvParameters);
void le_adv_start(void);
#ifdef __cplusplus
}
#endif
