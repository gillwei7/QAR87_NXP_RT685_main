/*
 * Copyright 2018-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */



#ifndef __IIR_H_INCLUDED
#define __IIR_H_INCLUDED

#include "GlobalDef.h"


#include "NatureDSP_Signal.h"
#include "NatureDSP_types.h"

#ifdef __cplusplus
extern "C" {
#endif


extern float Coef_HPF_20Hz_IIR_2Order[5];
extern float HpFilter1_2Order_Remove20HzBelow[4];
extern float HpFilter2_2Order_Remove20HzBelow[4];

extern bqriirf_df1_handle_t LowPassFilter1;
extern bqriirf_df1_handle_t LowPassFilter2;


extern void InitIirFilters(void);
extern void IIRBiquad_Process_Direct1(float *InputPtr, float *OtputPtr, float *CoefPtr, float *HistoryPtr, unsigned int SampleNum);

#ifdef __cplusplus
}
#endif
#endif  // SUBFUNC01_H_INCLUDED
