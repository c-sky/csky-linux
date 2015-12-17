/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef _ASM_CSKY_FUTEX_H
#define _ASM_CSKY_FUTEX_H

#ifdef __KERNEL__

#include <linux/futex.h>
#include <linux/uaccess.h>
#include <asm/errno.h>

#define __futex_atomic_ex_table(err_reg)            \
    "3: br      5f;"                   \
    "4: mov %0, " err_reg ";"          \
    "   br      5f;"                   \
    "   .section __ex_table,\"a\";"        \
    "   .align  2 ;"                    \
    "   .long   1b, 4b, 2b, 4b;"           \
    "   .previous;"                  \
    "5:\n"                          

#define __futex_atomic_op(insn, ret, oldval, tmp, uaddr, oparg)	\
	__asm__ __volatile__(					\
	"1:	  "                      \
	"   ld.w	%1, (%3);"			\
	"	" insn ";"					\
	"2:	;"               \
	"   st.w	%0, (%3);"			\
	"   movi	%0, 0;"			\
	__futex_atomic_ex_table("%5")				\
	: "=&r" (ret), "=&r" (oldval), "=&r" (tmp)		\
	: "r" (uaddr), "r" (oparg), "r" (-EFAULT)		\
	: "c", "memory")

static inline int
futex_atomic_cmpxchg_inatomic(u32 *uval, u32 __user *uaddr,
			      u32 oldval, u32 newval)
{
	int ret = 0;
	u32 val;

	if (!access_ok(VERIFY_WRITE, uaddr, sizeof(u32)))
		return -EFAULT;

	__asm__ __volatile__(" /*futex_atomic_cmpxchg_inatomic */ ;"
	"1:    "
	"   ld.w	%1, (%4);"
	"	cmpne	%1, %2;"
	"   bt      3f;"
	"2:              "
	"   st.w	%3, (%4);"
	__futex_atomic_ex_table("%5")
	: "+r" (ret), "=&r" (val)
	: "r" (oldval), "r" (newval), "r" (uaddr), "r" (-EFAULT)
	: "c", "memory");

	*uval = val;
	return ret;
}

static inline int
futex_atomic_op_inuser (int encoded_op, u32 __user *uaddr)
{
	int op = (encoded_op >> 28) & 7;
	int cmp = (encoded_op >> 24) & 15;
	int oparg = (encoded_op << 8) >> 20;
	int cmparg = (encoded_op << 20) >> 20;
	int oldval = 0, ret, tmp;

	if (encoded_op & (FUTEX_OP_OPARG_SHIFT << 28))
		oparg = 1 << oparg;

	if (!access_ok(VERIFY_WRITE, uaddr, sizeof(u32)))
		return -EFAULT;

	pagefault_disable();	/* implies preempt_disable() */

	switch (op) {
	case FUTEX_OP_SET:
		__futex_atomic_op("mov	%0, %4", ret, oldval, tmp, uaddr, oparg);
		break;
	case FUTEX_OP_ADD:
		__futex_atomic_op("mov	%0, %1; add	%0, %4", ret, oldval, tmp, uaddr, oparg);
		break;
	case FUTEX_OP_OR:
		__futex_atomic_op("mov	%0, %1; or	%0, %4", ret, oldval, tmp, uaddr, oparg);
		break;
	case FUTEX_OP_ANDN:
		__futex_atomic_op("mov	%0, %1; andn  %0, %4", ret, oldval, tmp, uaddr, oparg);
		break;
	case FUTEX_OP_XOR:
		__futex_atomic_op("mov	%0, %1; xor	%0, %4", ret, oldval, tmp, uaddr, oparg);
		break;
	default:
		ret = -ENOSYS;
	}

	pagefault_enable();	/* subsumes preempt_enable() */

	if (!ret) {
		switch (cmp) {
		case FUTEX_OP_CMP_EQ: ret = (oldval == cmparg); break;
		case FUTEX_OP_CMP_NE: ret = (oldval != cmparg); break;
		case FUTEX_OP_CMP_LT: ret = (oldval < cmparg); break;
		case FUTEX_OP_CMP_GE: ret = (oldval >= cmparg); break;
		case FUTEX_OP_CMP_LE: ret = (oldval <= cmparg); break;
		case FUTEX_OP_CMP_GT: ret = (oldval > cmparg); break;
		default: ret = -ENOSYS;
		}
	}
	return ret;
}

#endif /* __KERNEL__ */
#endif /* _ASM_CSKY_FUTEX_H */
