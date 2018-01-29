#ifndef __ASM_CSKY_BYTEORDER_H
#define __ASM_CSKY_BYTEORDER_H

#if defined(__cskyBE__)
#include <linux/byteorder/big_endian.h>
#elif defined(__cskyLE__)
#include <linux/byteorder/little_endian.h>
#else
# error "csky, but neither __cskyBE__, nor __cskyLE__???"
#endif

#endif /* __ASM_CSKY_BYTEORDER_H */
