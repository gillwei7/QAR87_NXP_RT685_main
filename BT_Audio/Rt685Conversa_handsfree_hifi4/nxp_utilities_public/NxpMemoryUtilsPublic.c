/*Copyright 2021 Retune DSP 
* Copyright 2022-2023 NXP 
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
#include "NxpMemoryUtilsPublic.h"

#ifdef _WIN32
#ifndef NXP_MEMORY_UTILS_USES_STDLIB
#define NXP_MEMORY_UTILS_USES_STDLIB 1
#endif
#endif

#if NXP_MEMORY_UTILS_USES_STDLIB
#include <stdlib.h>
#endif

#define NXP_MEMORY_UTILS_USES_STRING 1
#if NXP_MEMORY_UTILS_USES_STRING
#include <string.h>
#endif

#define NXP_ENABLE_MEMCHECK 0
#if NXP_ENABLE_MEMCHECK
#include "memcheck.h"
#endif

#if NXP_ENABLE_PRINTF
#include "NxpStringUtilsPublic.h"
#endif

/*
 *  ----------- Flags definition -----------
 */
// When this flag is set to 1, nxp_plugin_malloc will only increment ext_mem_bytes_used and returning 0xFFFF.
// This is used to get the required amount of heap
static uint32_t extmem_analysis_mode_flag = 0;
/*
 *  ----------- Cacheable memory indicators -----------
 */
static void* ext_mem_baseptr;
static void* ext_mem_nextptr;
static size_t ext_mem_size_bytes;
static size_t ext_mem_bytes_left;
static size_t ext_mem_bytes_used;
/*
 *  ----------- NON-Cacheable memory indicators -----------
 */
static void* ext_noncache_mem_baseptr;
static void* ext_noncache_mem_nextptr;
static size_t ext_noncache_mem_size_bytes;
static size_t ext_noncache_mem_bytes_left;
static size_t ext_noncache_mem_bytes_used;
/*
 *  ----------- Scratch memory indicators -----------
 */
static void* ext_mem_scratch_baseptr;
static size_t ext_mem_scratch_size_bytes;
static scratch_state_t scratch_state;

/*
 *  ----------- Flags functions -----------
 */
uint32_t nxp_plugin_get_heapmem_analysis_flag(void) {
	return extmem_analysis_mode_flag;
}

void nxp_plugin_set_heapmem_analysis_flag(uint32_t Aanalysis_mode_flag) {
	extmem_analysis_mode_flag = Aanalysis_mode_flag;
}

/*
 *  ----------- Cacheable memory functions -----------
 */

void nxp_plugin_malloc_init(void* Aextmem_baseptr, void* Aextmem_nextptr, size_t AextmemSize) {
	ext_mem_baseptr = Aextmem_baseptr;
	ext_mem_nextptr = Aextmem_nextptr;
	ext_mem_size_bytes = AextmemSize;
	ext_mem_bytes_left = AextmemSize;
	ext_mem_bytes_used = 0;
}

size_t nxp_plugin_malloc_GetAllocatedBytes(void) {
	return ext_mem_bytes_used;
}

void* nxp_plugin_malloc(size_t Asize, MemAlign_t Aalign) {
#if NXP_ENABLE_MEMCHECK
	void* p = memcheck_malloc_align(Asize, align, NULL, 0, NULL);
	return p;
#else
	// Pointer to the start of the aligned memory section
	uintptr_t pRet = (uintptr_t)NULL;

	// Ensure pointer is aligned in case previous malloc had a different alignment
	uintptr_t tmp = (uintptr_t)ext_mem_nextptr;
	uintptr_t mem_start = (tmp + ((size_t)Aalign - 1)) & ~(uintptr_t)((size_t)Aalign - 1);

	// Compute total memory size including added alignment
	size_t aligned_size = (Asize + (size_t)Aalign - 1) & ~((size_t)Aalign - 1);
	uintptr_t mem_end = mem_start + aligned_size;
	size_t mem_size = mem_end - (uintptr_t)ext_mem_nextptr;

	/* For heap memory analysis */
	if (extmem_analysis_mode_flag == 1) {
		ext_mem_bytes_used += mem_size;
		return (void*)0xFFFF;
	}

	int32_t failed = 0;
	// Only allocate if we have enough memory
	if (ext_mem_bytes_left >= mem_size) {

#if NXP_MEMORY_UTILS_USES_STDLIB
#if _WIN32
		pRet = _aligned_malloc(mem_size, Aalign);
#else
		pRet = aligned_malloc(Aalign, mem_size);
#endif
#else
		pRet = mem_start;
		ext_mem_nextptr = (void*)mem_end;
#endif // NXP_MEMORY_UTILS_USES_STDLIB==1

		// Check for alignment
		if ((pRet % (size_t)Aalign) != 0) {
			failed = 1;
		}
		else {
			ext_mem_bytes_left -= mem_size;
			ext_mem_bytes_used += mem_size;
		}
	}
	else {
		failed = 1;
	}

	if (failed == 1) {
		pRet = (uintptr_t)NULL;
#if NXP_ENABLE_PRINTF
		(void)nxp_printf("Not enough memory available! Requested %zu bytes but only %zu bytes available.\n", Asize, ext_mem_bytes_left);
#endif
}

	return (void*)pRet;
#endif
}

void nxp_plugin_free(void* Aptr) {
#if NXP_MEMORY_UTILS_USES_STDLIB
#if _WIN32
	_aligned_free(Aptr);
#else
	free(Aptr);
#endif
#else
	// extmem should be free'd outside
#endif
}
/*
 *  ----------- NON-Cacheable memory functions -----------
 */

void nxp_plugin_noncache_malloc_init(void* Aextmem_baseptr, void* Aextmem_nextptr, size_t AextmemSize) {
	ext_noncache_mem_baseptr = Aextmem_baseptr;
	ext_noncache_mem_nextptr = Aextmem_nextptr;
	ext_noncache_mem_size_bytes = AextmemSize;
	ext_noncache_mem_bytes_left = AextmemSize;
	ext_noncache_mem_bytes_used = 0;
}

size_t nxp_plugin_noncache_malloc_GetAllocatedBytes(void) {
	return ext_noncache_mem_bytes_used;
}

void* nxp_plugin_noncache_malloc(size_t Asize, MemAlign_t Aalign) {
#if NXP_ENABLE_MEMCHECK
	void* p = memcheck_malloc_align(Asize, align, NULL, 0, NULL);
	return p;
#else
	// Pointer to the start of the aligned memory section
	uintptr_t pRet = (uintptr_t)NULL;

	// Ensure pointer is aligned in case previous malloc had a different alignment
	uintptr_t tmp = (uintptr_t)ext_noncache_mem_nextptr;
	uintptr_t mem_start = (tmp + ((size_t)Aalign - 1)) & ~(uintptr_t)((size_t)Aalign - 1);

	// Compute total memory size including added alignment
	size_t aligned_size = (Asize + (size_t)Aalign - 1) & ~((size_t)Aalign - 1);
	uintptr_t mem_end = mem_start + aligned_size;
	size_t mem_size = mem_end - (uintptr_t)ext_noncache_mem_nextptr;

	/* For heap memory analysis */
	if (extmem_analysis_mode_flag == 1) {
		ext_noncache_mem_bytes_used += mem_size;
		return (void*)0xFFFF;
	}

	int32_t failed = 0;
	// Only allocate if we have enough memory
	if (ext_noncache_mem_bytes_left >= mem_size) {

#if NXP_MEMORY_UTILS_USES_STDLIB
#if _WIN32
		pRet = _aligned_malloc(mem_size, Aalign);
#else
		pRet = aligned_malloc(Aalign, mem_size);
#endif
#else
		pRet = mem_start;
		ext_noncache_mem_nextptr = (void*)mem_end;
#endif // NXP_MEMORY_UTILS_USES_STDLIB==1

		// Check for alignment
		if ((pRet % (size_t)Aalign) != 0) {
			failed = 1;
		}
		else {
			ext_noncache_mem_bytes_left -= mem_size;
			ext_noncache_mem_bytes_used += mem_size;
		}
	}
	else {
		failed = 1;
	}

	if (failed == 1) {
		pRet = (uintptr_t)NULL;
#if NXP_ENABLE_PRINTF
		(void)nxp_printf("Not enough memory available! Requested %zu bytes but only %zu bytes available.\n", Asize, ext_mem_noncache_bytes_left);
#endif
}

	return (void*)pRet;
#endif
}

void nxp_plugin_noncache_free(void* Aptr) {
#if NXP_MEMORY_UTILS_USES_STDLIB
#if _WIN32
	_aligned_free(Aptr);
#else
	free(Aptr);
#endif
#else
	// extmem should be free'd outside
#endif
}


/*
 *  ----------- Scratch memory functions -----------
 */
void nxp_plugin_scratch_init(void* Aext_scratch_mem_baseptr, void* Aext_scratch_mem_nextptr, size_t Aext_scratch_memSize) {
	ext_mem_scratch_baseptr = Aext_scratch_mem_baseptr;
	scratch_state.ext_mem_scratch_nextptr = Aext_scratch_mem_nextptr;

	ext_mem_scratch_size_bytes = Aext_scratch_memSize;
	scratch_state.ext_mem_scratch_bytes_left = Aext_scratch_memSize;
	scratch_state.ext_mem_scratch_bytes_used = 0;
	scratch_state.ext_mem_scratch_max_bytes_used = 0;
}

size_t nxp_plugin_scratch_GetAllocatedBytes(void) {
	return scratch_state.ext_mem_scratch_max_bytes_used;
}

void nxp_plugin_scratch_reset(void) {
	scratch_state.ext_mem_scratch_nextptr = ext_mem_scratch_baseptr;
	scratch_state.ext_mem_scratch_bytes_left = ext_mem_scratch_size_bytes;
	scratch_state.ext_mem_scratch_bytes_used = 0;
}

scratch_state_t nxp_plugin_scratch_get_state(void) {
	return scratch_state;
}

void nxp_plugin_scratch_set_state(scratch_state_t state) {
	scratch_state = state;
}

void* nxp_plugin_scratch_malloc(size_t Asize, MemAlign_t Aalign) {
	// Pointer to the start of the aligned memory section
	uintptr_t pRet = (uintptr_t)NULL;

	// Ensure pointer is aligned in case previous malloc had a different alignment
	uintptr_t tmp = (uintptr_t)scratch_state.ext_mem_scratch_nextptr;
	uintptr_t mem_start = (tmp + ((size_t)Aalign - 1)) & ~(uintptr_t)((size_t)Aalign - 1);

	// Compute total memory size including added alignment
	size_t aligned_size = (Asize + (size_t)Aalign - 1) & ~((size_t)Aalign - 1);
	uintptr_t mem_end = mem_start + aligned_size;
	size_t mem_size = mem_end - (uintptr_t)scratch_state.ext_mem_scratch_nextptr;

	/* For heap memory analysis */
	if (extmem_analysis_mode_flag == 1) {
		scratch_state.ext_mem_scratch_bytes_used += mem_size;
		if (scratch_state.ext_mem_scratch_bytes_used > scratch_state.ext_mem_scratch_max_bytes_used) {
			scratch_state.ext_mem_scratch_max_bytes_used = scratch_state.ext_mem_scratch_bytes_used;
		}
		return (void*) 0xFFFF;
	}

#if NXP_MEMORY_UTILS_USES_STDLIB
#if _WIN32
	pRet = _aligned_malloc(mem_size, Aalign);
#else
	pRet = aligned_malloc(Aalign, mem_size);
#endif
#else
	// Only allocate if we have enough memory
	if (scratch_state.ext_mem_scratch_bytes_left >= mem_size) {
		pRet = mem_start;
		scratch_state.ext_mem_scratch_nextptr = (void*)mem_end;
		scratch_state.ext_mem_scratch_bytes_left -= mem_size;
		scratch_state.ext_mem_scratch_bytes_used += mem_size;
		if (scratch_state.ext_mem_scratch_bytes_used > scratch_state.ext_mem_scratch_max_bytes_used) {
			scratch_state.ext_mem_scratch_max_bytes_used = scratch_state.ext_mem_scratch_bytes_used;
		}
	}
	else {
#if NXP_ENABLE_PRINTF
		(void)nxp_printf("Not enough memory available! Requested %zu scratch bytes but only %zu scratch bytes available.\n", Asize, scratch_state.ext_mem_scratch_bytes_left);
#endif
	}
#endif // NXP_MEMORY_UTILS_USES_STDLIB
	return (void*)pRet;
}

/*
 *  -----------  Memory utilities functions -----------
 */

void* nxp_plugin_memset(void* Aptr, uint8_t Aval, size_t Asize) {
#if NXP_MEMORY_UTILS_USES_STRING
	return memset(Aptr, Aval, Asize);
#else
    // Replicate val from 8b to 32b
    uint32_t val32;
    uint8_t* val8 = (uint8_t*)&val32;
	size_t i;
    for (i = 0; i < 4; i++) {
        val8[i] = Aval;
    }

    // Set 32b chunks
    i = 0;
    size_t* ptr32 = (uint32_t*)Aptr;
    for (; i < Asize >> 2; i++) {
        *ptr32++ = val32;
    }

    // Do the rest
    uint8_t* ptr8 = (uint8_t*)ptr32;
	size_t rem = Asize - (i << 2);
    while (rem-- > 0) {
        *ptr8++ = Aval;
    }

    return Aptr;
#endif
}

void* nxp_plugin_memset_32b(void* Aptr, uint32_t Aval, size_t Asize) {
	uint32_t* ptr32 = (uint32_t*)Aptr;
	for (size_t i = 0; i < Asize; i++) {
		ptr32[i] = Aval;
	}
	return Aptr;
}

void* nxp_plugin_memset_64b(void* Aptr, uint64_t Aval, size_t Asize) {
	uint64_t* ptr64 = (uint64_t*)Aptr;
	for (size_t i = 0; i < Asize; i++) {
		ptr64[i] = Aval;
	}
	return Aptr;
}

void* nxp_plugin_memset_float(nxp_float* Aptr, nxp_float Aval, size_t Alength) {
	for (size_t i = 0; i < Alength; i++) {
		Aptr[i] = Aval;
	}
	return Aptr;
}

void* nxp_plugin_memset_complex(nxp_complex* Aptr, nxp_complex Aval, size_t Alength) {
	for (size_t i = 0; i < Alength; i++) {
#if defined(HIFI4) || defined(FUSIONDSP) || defined(HIFI4) || defined(HIFI1)
		Aptr[i] = Aval;
#else
		Aptr[i][0] = Aval[0];
		Aptr[i][1] = Aval[1];
#endif
	}
	return Aptr;
}

void* nxp_plugin_memcpy(void* Adest, const void* Asrc, size_t Asize) {
#if NXP_MEMORY_UTILS_USES_STRING
	return memcpy(Adest, Asrc, Asize);
#else
	uint32_t* src32 = (uint32_t*) Asrc;
	uint32_t* dst32 = (uint32_t*) Adest;
	size_t i = 0;
	for(; i < Asize >> 2; i++) {
		*dst32++ = *src32++;
	}

	// Do the rest
	uint8_t *src8 = (uint8_t *) src32;
	uint8_t *dst8 = (uint8_t *) dst32;
	size_t rem = Asize - (i << 2);
	while (rem-- > 0) {
		*dst8++ = *src8++;
	}

    return Adest;
#endif
}

void* nxp_plugin_memcpy_32b(void* Adest, const void* Asrc, size_t Alength) {
#if NXP_MEMORY_UTILS_USES_STRING
	return memcpy(Adest, Asrc, sizeof(uint32_t) * Alength);
#else
	uint32_t* pdest32 = (uint32_t*)Adest;
	uint32_t* psrc32 = (const uint32_t*)Asrc;
	for (size_t i = 0; i < Alength; i++) {
		pdest32[i] = psrc32[i];
	}
	return Adest;
#endif
}

void* nxp_plugin_memcpy_64b(void* Adest, const void* Asrc, size_t Alength) {
#if NXP_MEMORY_UTILS_USES_STRING
	return memcpy(Adest, Asrc, sizeof(uint64_t) * Alength);
#else
	uint64_t* pdest64 = (const uint64_t*)Adest;
	uint64_t* psrc64 = (uint64_t*)Asrc;
	for (size_t i = 0; i < Alength; i++) {
		pdest64[i] = psrc64[i];
	}
	return Adest;
#endif
}

void* nxp_plugin_memcpy_float(nxp_float* Adest, const nxp_float* Asrc, size_t Alength) {
	return memcpy(Adest, Asrc, Alength * sizeof(nxp_float));
}

void* nxp_plugin_memcpy_complex(nxp_complex* Adest, const nxp_complex* Asrc, size_t Alength) {
	return memcpy(Adest, Asrc, Alength * sizeof(nxp_complex));
}

void* nxp_plugin_memmove(void* Adest, const void* Asrc, size_t Asize) {
#if NXP_MEMORY_UTILS_USES_STRING
	return memmove(Adest, Asrc, Asize);
#else
	// FIXME: nxp_plugin_memmove requires NXP_MEMORY_UTILS_USES_STRING==1
	return memmove(Adest, Asrc, Asize);
#endif
}

int32_t nxp_plugin_memcompare(void* Ax, void* Ay, size_t Asize) {
#if NXP_MEMORY_UTILS_USES_STRING
	return memcmp(Ax, Ay, Asize);
#else
	const uint32_t *x32 = (const uint32_t *) Ax;
	const uint32_t *y32 = (const uint32_t *) Ay;
	size_t i = 0;
	for (; i < Asize >> 2; i++) {
		if (*x32++ != *y32++) {
			return 1;
		}
	}

	// Do the rest
	size_t rem = Asize - (i << 2);
	const uint8_t *x8 = (const uint8_t *) x32;
	const uint8_t *y8 = (const uint8_t *) y32;
	while (rem-- > 0) {
		if (*x8++ != *y8++) {
			return 1;
		}
	}
	return 0;
#endif
}
