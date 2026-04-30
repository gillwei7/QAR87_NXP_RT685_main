/*
 * aw88166.h
 *
 *  Created on: Nov 4, 2024
 *      Author: 11301026
 */

#ifndef AW88166_H_
#define AW88166_H_

#include "aw883xx_init.h"
#include "aw883xx.h"

#define AW88166_DEV0_I2C_ADDR          0x34
#define AW88166_DEV1_I2C_ADDR          0x35

void init_aw88166(void);
void start_aw88166_pa(aw_dev_index_t dev, char *prof_name); //EX: (AW_DEV_0, "Music")
void close_aw88166_pa(aw_dev_index_t dev);


#endif /* AW88166_H_ */
