#ifndef __AW_TYPE_H__
#define __AW_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AW_CONST
#define AW_CONST	const
#endif

#ifndef AW_NULL
#define AW_NULL    ((void *)0)
#endif

#ifndef AW_TRUE
#define AW_TRUE    (1)
#endif

#ifndef AW_FALSE
#define AW_FALSE   (0)
#endif

enum aw_bit {
	AW_BIT0,
	AW_BIT1,
	AW_BIT2,
	AW_BIT3,
	AW_BIT7 = 7,
	AW_BIT8 = 8,
	AW_BIT12 = 12,
	AW_BIT16 = 16,
	AW_BIT24 = 24,
	AW_BIT28 = 28,
	AW_BIT32 = 32,
};

#ifndef AW_ONE_BYTE_1
#define AW_ONE_BYTE_1			(0xff)
#endif

#ifndef OFFSET_BIT_0
#define OFFSET_BIT_0				(0)
#endif

#ifndef OFFSET_BIT_8
#define OFFSET_BIT_8				(8)
#endif

#ifndef OFFSET_BIT_16
#define OFFSET_BIT_16				(16)
#endif

#ifndef OFFSET_BIT_24
#define OFFSET_BIT_24				(24)
#endif

#ifndef WORD_LEN
#define WORD_LEN				(4)
#endif

#ifndef GET_BITS_7_0
#define GET_BITS_7_0				(0x00FF)
#endif

#ifndef GET_BITS_15_8
#define GET_BITS_15_8				(0xFF00)
#endif

#ifndef GET_BITS_24_16
#define GET_BITS_24_16				(0x00FF0000)
#endif

#ifndef GET_BITS_31_25
#define GET_BITS_31_25				(0xFF000000)
#endif

#ifndef AW_BOOL
typedef unsigned char AW_BOOL;
#endif

#ifndef AW_S8
typedef signed char AW_S8;
#endif

#ifndef AW_S16
typedef signed short AW_S16;
#endif

#ifndef AW_S32
typedef signed int AW_S32;
#endif

#ifndef AW_S64
typedef signed long long AW_S64;
#endif

#ifndef AW_U8
typedef unsigned char AW_U8;
#endif

#ifndef AW_U16
typedef unsigned short AW_U16;
#endif

#ifndef AW_U32
typedef unsigned int AW_U32;
#endif

#ifndef AW_U64
typedef unsigned long long AW_U64;
#endif

#ifdef __cplusplus
}
#endif

#endif  /* __AW_TYPE_H__ */
