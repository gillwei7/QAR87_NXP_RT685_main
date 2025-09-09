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
#ifndef NXP_MEMORY_UTILS_PUBLIC_
#define NXP_MEMORY_UTILS_PUBLIC_

#include "NxpTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MEM_ALIGN_1 = 1,	/*!< Align 1 byte */
    MEM_ALIGN_2 = 2,	/*!< Align 2 byte */
    MEM_ALIGN_4 = 4,	/*!< Align 4 byte */
    MEM_ALIGN_8 = 8,	/*!< Align 8 byte */
    MEM_ALIGN_16 = 16,	/*!< Align 16 byte */
} MemAlign_t;

typedef struct {
    void* ext_mem_scratch_nextptr;
    size_t ext_mem_scratch_bytes_left;
    size_t ext_mem_scratch_bytes_used;
    size_t ext_mem_scratch_max_bytes_used;
} scratch_state_t;

#if defined(__ARM_NEON) || defined(_WIN64)
#define NXP_ALIGN_SIZE MEM_ALIGN_16
#else
#define NXP_ALIGN_SIZE MEM_ALIGN_8
#endif

#ifdef _WIN32
#define NXP_MEM_ALIGN
#elif defined(HIFI4) || defined(FUSIONDSP) || defined(HIFI4)
#define NXP_MEM_ALIGN __attribute__((aligned(8)))
#else
#define NXP_MEM_ALIGN __attribute__((aligned(NXP_ALIGN_SIZE)))
#endif

#define NXP_PLUGIN_MALLOC_ALIGN(type, n) (type*)nxp_plugin_malloc(sizeof(type) * n, NXP_ALIGN_SIZE)
#define NXP_PLUGIN_NONCACHE_MALLOC_ALIGN(type, n) (type*)nxp_plugin_noncache_malloc(sizeof(type) * n, NXP_ALIGN_SIZE)
#define NXP_PLUGIN_SCRATCH_MALLOC_ALIGN(type, n) (type*)nxp_plugin_scratch_malloc(sizeof(type) * n, NXP_ALIGN_SIZE)

extern uint32_t nxp_plugin_get_heapmem_analysis_flag(void);
extern void nxp_plugin_set_heapmem_analysis_flag(uint32_t Aanalysis_mode_flag);

extern void nxp_plugin_malloc_init(void* Aextmem_baseptr, void* Aextmem_nextptr, size_t AextmemSize);
extern size_t nxp_plugin_malloc_GetAllocatedBytes(void);
extern void* nxp_plugin_malloc(size_t Asize, MemAlign_t Aalign);

extern void nxp_plugin_noncache_malloc_init(void* Aextmem_baseptr, void* Aextmem_nextptr, size_t AextmemSize);
extern size_t nxp_plugin_noncache_malloc_GetAllocatedBytes(void);
extern void* nxp_plugin_noncache_malloc(size_t Asize, MemAlign_t Aalign);

extern void nxp_plugin_scratch_init(void* Aext_scratch_mem_baseptr, void* Aext_scratch_mem_nextptr, size_t Aext_scratch_memSize);
extern size_t nxp_plugin_scratch_GetAllocatedBytes(void);
extern void nxp_plugin_scratch_reset(void);
extern scratch_state_t nxp_plugin_scratch_get_state(void);
extern void nxp_plugin_scratch_set_state(scratch_state_t state);
extern void* nxp_plugin_scratch_malloc(size_t Asize, MemAlign_t Aalign);

extern void* nxp_plugin_memset(void* Aptr, uint8_t Aval, size_t Asize);
extern void* nxp_plugin_memset_32b(void* Aptr, uint32_t Aval, size_t Alength);
extern void* nxp_plugin_memset_64b(void* Aptr, uint64_t Aval, size_t Alength);
extern void* nxp_plugin_memset_float(nxp_float* Aptr, nxp_float Aval, size_t Alength);
extern void* nxp_plugin_memset_complex(nxp_complex* Aptr, nxp_complex Aval, size_t Alength);

extern void* nxp_plugin_memcpy(void* Adest, const void* Asrc, size_t Asize);
extern void* nxp_plugin_memcpy_32b(void* Adest, const void* Asrc, size_t Asize);
extern void* nxp_plugin_memcpy_64b(void* Adest, const void* Asrc, size_t Asize);
extern void* nxp_plugin_memcpy_float(nxp_float* Adest, const nxp_float* Asrc, size_t Alength);
extern void* nxp_plugin_memcpy_complex(nxp_complex* Adest, const nxp_complex* Asrc, size_t Alength);

extern void* nxp_plugin_memmove(void* Adest, const void* Asrc, size_t Asize);
extern int32_t nxp_plugin_memcompare(void* Ax, void* Ay, size_t Asize);
extern void nxp_plugin_free(void* Aptr);

#ifdef __cplusplus
}
#endif

#endif /* NXP_MEMORY_UTILS_PUBLIC_ */
