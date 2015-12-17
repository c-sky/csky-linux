#ifndef __ASM_CSKY_SEGMENT_H
#define __ASM_CSKY_SEGMENT_H

#ifndef __ASSEMBLY__

typedef struct {
	unsigned long seg;
} mm_segment_t;

#define KERNEL_DS		((mm_segment_t) { 0xFFFFFFFF })

#ifdef CONFIG_MMU
#define USER_DS			((mm_segment_t) { 0x80000000UL })
#define get_fs()		(current_thread_info()->addr_limit)
#define set_fs(x)		(current_thread_info()->addr_limit = (x))
#define segment_eq(a,b)	((a).seg == (b).seg)

#else /*CONFIG_MMU*/
#define USER_DS   KERNEL_DS
#define get_fs()		(KERNEL_DS)
static inline void set_fs(mm_segment_t fs)
{
}
#define segment_eq(a,b)	(1)
#endif /*CONFIG_MMU*/

#define get_ds()		(KERNEL_DS)

#endif /* __ASSEMBLY__ */

#endif /* __ASM_CSKY_SEGMENT_H */
