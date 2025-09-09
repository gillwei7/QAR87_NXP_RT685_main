/*Copyright 2023 NXP
*
* NXP Confidential. This software is owned or controlled by NXP
* and may only be used strictly in accordance with the applicable license terms.
* By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read,
* and that you agree to comply with and are bound by, such license terms.
* If you do not agree to be bound by the applicable license terms,
* then you may not retain, install, activate or otherwise use the software.
*
*/

#include "NxpUtilities.h"

#define NXP_CRC32_POLY 0xedb88320U		// CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order.

NXP_PUT_FUNCTION_IN_TEXT_FLASH
uint32_t get_crc32(uint32_t crc, const uint8_t* buf, size_t len) {
	int k;
	crc = ~crc;
	while (len--) {
		crc ^= *buf++;
		for (k = 0; k < 8; k++) {
			crc = (crc >> 1) ^ (-((int32_t)(crc & 1)) & ((uint32_t)NXP_CRC32_POLY)); // More operations, but branch-free. May be faster/slower depending on platform.
		}
	}
	return ~crc;
}

#define is_aligned(POINTER, BYTE_COUNT) \
    (((uintptr_t)(const void *)(POINTER)) % (BYTE_COUNT) == 0)

/*
 * conversion utilities between float32 and fixed-point
 */
void nxp_convert_Q31_to_float32(nxp_float *yf32, int32_t *xq31, uint32_t L) {
#ifdef HIFI4
	// Convert pairs first
	xtfloatx2* yf32_x2 = (xtfloatx2*) yf32;
	ae_int32x2* xq31_x2 = (ae_int32x2*) xq31;
	int32_t Lx2 = L>>1;	// xtensa compiler use truncation
	int32_t i=0;
	for (; i < Lx2; i++) {
		yf32_x2[i] = XT_FLOAT_SX2(xq31_x2[i], 31);
	}
	// Convert remaining value
	i <<= 1;
	if(L - i >0){
		yf32[i] = XT_FLOAT_S(xq31[i], 31);
	}
#elif defined(FUSIONDSP)
	for (uint32_t i = 0; i < L; i++) {
		yf32[i] = XT_FLOAT_S(xq31[i], 31);
	}
#else
	const float scl = 1.0f / 2147483648.0f;
	for (uint32_t i = 0; i < L; i++) {
		yf32[i] = ((nxp_float)xq31[i]) * scl;
	}
#endif
}

void nxp_convert_Q23_to_float32(nxp_float *yf32, int32_t *xq23, uint32_t L) {
#ifdef HIFI4
	// Convert pairs first
	xtfloatx2* yf32_x2 = (xtfloatx2*) yf32;
	ae_int32x2* xq23_x2 = (ae_int32x2*) xq23;
	int32_t Lx2 = L>>1;	// xtensa compiler use truncation
	int32_t i=0;
	for (; i < Lx2; i++) {
		yf32_x2[i] = XT_FLOAT_SX2(xq23_x2[i], 23);
	}
	// Convert remaining value
	i <<= 1;
	if(L - i >0){
		yf32[i] = XT_FLOAT_S(xq23[i], 23);
	}
#elif defined(FUSIONDSP)
	for (uint32_t i = 0; i < L; i++) {
		yf32[i] = XT_FLOAT_S(xq23[i], 23);
	}
#else
	const float scl = 1.0f / 8388608.0f;
	for (uint32_t i = 0; i < L; i++) {
		yf32[i] = ((nxp_float) xq23[i]) * scl;
	}
#endif
}

void nxp_convert_float32_to_Q31(int32_t *yq31, nxp_float *xf32, uint32_t L) {
#ifdef HIFI4
	// Convert pairs first
	ae_int32x2* yq31_x2 = (ae_int32x2*) yq31;
	xtfloatx2* xf32_x2 = (xtfloatx2*) xf32;
	int32_t Lx2 = L>>1;	// xtensa compiler use truncation
	int32_t i=0;
	for (; i < Lx2; i++) {
		yq31_x2[i] = XT_TRUNC_SX2(xf32_x2[i], 31);
	}
	// Convert remaining value
	i <<= 1;
	if(L - i >0){
		yq31[i] = XT_TRUNC_S(xf32[i], 31);
	}
#elif defined(FUSIONDSP)
	for (uint32_t i = 0; i < L; i++) {
		yq31[i] = XT_TRUNC_S(xf32[i], 31);
	}
#else
	for (uint32_t i = 0; i < L; i++) {
		yq31[i] = (int32_t) (xf32[i] * 2147483648.0f);
	}
#endif
}

void nxp_convert_float32_to_Q23(int32_t* yq23, nxp_float* xf32, uint32_t L) {
#ifdef HIFI4
	// Convert pairs first
	ae_int32x2* yq23_x2 = (ae_int32x2*) yq23;
	xtfloatx2* xf32_x2 = (xtfloatx2*) xf32;
	int32_t Lx2 = L>>1;	// xtensa compiler use truncation
	int32_t i=0;
	for (; i < Lx2; i++) {
		yq23_x2[i] = XT_TRUNC_SX2(xf32_x2[i], 23);
	}
	// Convert remaining value
	i <<= 1;
	if(L - i >0){
		yq23[i] = XT_TRUNC_S(xf32[i], 23);
	}
#elif defined(FUSIONDSP)
	for (uint32_t i = 0; i < L; i++) {
		yq23[i] = XT_TRUNC_S(xf32[i], 23);
	}
#else
	for (uint32_t i = 0; i < L; i++) {
		yq23[i] = (int32_t)(xf32[i] * 8388608.0f);
	}
#endif
}
