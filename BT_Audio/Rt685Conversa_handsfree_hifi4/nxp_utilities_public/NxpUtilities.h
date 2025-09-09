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

#ifndef NXP_UTILITIES_H
#define NXP_UTILITIES_H

#include "NxpTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * get_crc32
 *	CRC32 without lookup table, see http://create.stephan-brumme.com/crc32/
 *	
 *	Input
 *	crc: Value of CRC for existing block of data (0 if no existing data)
 *	buf: Byte array pointer to data
 *	len: Number of bytes in data array
 *	Output
 *	returns updated crc value
 *
 *	This code is licensed under the zlib License:
 *	This software is provided 'as-is', without any express or implied
 *	warranty. In no event will the authors be held liable for any damages
 *	arising from the use of this software.
 *
 *	Permission is granted to anyone to use this software for any purpose,
 *	including commercial applications, and to alter it and redistribute it
 *	freely, subject to the following restrictions:
 *
 *	1. The origin of this software must not be misrepresented; you must not
 *		claim that you wrote the original software. If you use this software
 *		in a product, an acknowledgment in the product documentation would be
 *		appreciated but is not required.
 *	2. Altered source versions must be plainly marked as such, and must not be
 *		misrepresented as being the original software.
 *	3. This notice may not be removed or altered from any source distribution.zlib License
 *
 *	Modified by:
 *	2023 NXP
 */
extern uint32_t get_crc32(uint32_t crc, const uint8_t* buf, size_t len);

/*
 * Conversion utility functions between float and Q31 & Q23
 * Note: for HIFI4 input and output are assumed to reside on an 8 byte boundary (FIX for boundary!=8bytes later)
 */
extern void nxp_convert_Q31_to_float32(nxp_float *yf32, int32_t *xq31, uint32_t L);
extern void nxp_convert_Q23_to_float32(nxp_float *yf32, int32_t *xq23, uint32_t L);
extern void nxp_convert_float32_to_Q31(int32_t *yq31, nxp_float *xf32, uint32_t L);
extern void nxp_convert_float32_to_Q23(int32_t* yq23, nxp_float* xf32, uint32_t L);

#ifdef __cplusplus
}
#endif

#endif // NXP_UTILITIES_H
