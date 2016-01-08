#ifndef __ASM_CSKY_SEGMENT_H
#define __ASM_CSKY_SEGMENT_H

typedef struct {
	unsigned long seg;
} mm_segment_t;
#define KERNEL_DS		((mm_segment_t) { 0xFFFFFFFF })
#define get_ds()		KERNEL_DS

#ifdef CONFIG_MMU

#define USER_DS			((mm_segment_t) { 0x80000000UL })
#define get_fs()		(current_thread_info()->addr_limit)
#define set_fs(x)		(current_thread_info()->addr_limit = (x))
#define segment_eq(a,b)		((a).seg == (b).seg)

#else /* CONFIG_MMU */

#define USER_DS			KERNEL_DS
#define get_fs()		KERNEL_DS
#define set_fs(x)		do {} while(0)
#define segment_eq(a,b)		(1)

#endif /* CONFIG_MMU */
#endif /* __ASM_CSKY_SEGMENT_H */
