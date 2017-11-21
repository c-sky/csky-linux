#ifndef __CSKY_MATH_64BITS_H__
#define __CSKY_MATH_64BITS_H__

#include <asm/byteorder.h>

#define BITS_PER_UNIT 8

typedef          int SItype     __attribute__ ((mode (SI)));
typedef unsigned int USItype    __attribute__ ((mode (SI)));
typedef          int DItype     __attribute__ ((mode (DI)));
typedef int word_type __attribute__ ((mode (__word__)));

#ifdef __BIG_ENDIAN
struct DIstruct {
    SItype high, low;
};
#elif defined(__LITTLE_ENDIAN)
struct DIstruct {
    SItype low, high;
};
#else
#error I feel sick.
#endif

typedef union
{
    struct DIstruct s;
    DItype ll;
} DIunion;

#endif /* __CSKY_MATH_64BITS_H__ */
