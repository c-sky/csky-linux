#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/errno.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/signal.h>

#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/asm-offsets.h>

/*
 * does not yet catch signals sent when the child dies.
 * in exit.c or in signal.c.
 */

/* sets the trace bits. */
#define TRACE_MODE_SI      1 << 14
#define TRACE_MODE_RUN     0
#define TRACE_MODE_JMP     0x3 << 14
#define TRACE_MODE_MASK    ~(0x3 << 14)

/*
 * PT_xxx is the stack offset at which the register is  saved.
 * Notice that usp has no stack-slot and needs to be treated
 * specially (see get_reg/put_reg below).
 * No usrsp of CSKY ABIV2 in this array !
 */
static int regoff[] = {
#if defined(__CSKYABIV2__)
        PT_A0,    PT_A1,     PT_A2,    PT_A3,
        PT_REGS0, PT_REGS1,  PT_REGS2, PT_REGS3,
        PT_REGS4, PT_REGS5,  PT_REGS6, PT_REGS7,
        PT_REGS8, PT_REGS9,  -1,       PT_R15,
        PT_R16,   PT_R17,    PT_R18,   PT_R19,
        PT_R20,   PT_R21,    PT_R22,   PT_R23,
        PT_R24,   PT_R25,    PT_R26,   PT_R27,
        PT_R28,   PT_R29,    PT_R30,   PT_R31,
        PT_SR,    PT_PC,     PT_RHI,   PT_RLO,
#else
        -1,       PT_REGS9,  PT_A0,    PT_A1,
        PT_A2,    PT_A3,     PT_REGS0, PT_REGS1,
        PT_REGS2, PT_REGS3,  PT_REGS4, PT_REGS5,
        PT_REGS6, PT_REGS7,  PT_REGS8, PT_R15,
        -1,       -1,        -1,       -1,
        -1,       -1,        -1,       -1,
        -1,       -1,        -1,       -1,
        -1,       -1,        -1,       -1,
        PT_SR,    PT_PC,     -1,       -1,
#endif
};

/*
 * Get contents of register REGNO in task TASK.
 */
static long get_reg(struct task_struct *task, int regno)
{
	unsigned long *addr;

	if (regno == REGNO_USP)
		addr = &task->thread.usp;
#if !defined(__CSKYABIV2__)  // ABIV2 task->thread have no hi & lo;
	else if (regno == CSKY_HI_NUM)
		addr = &task->thread.hi;
	else if (regno == CSKY_LO_NUM)
		addr = &task->thread.lo;
#endif
	else if ((regno < sizeof(regoff)/sizeof(regoff[0])) && (regoff[regno] != -1))
		addr = (unsigned long *)(task->thread.esp0 + regoff[regno]);
	else
		return 0;
	return *addr;
}

/*
 * Write contents of register REGNO in task TASK.
 */
static int put_reg(struct task_struct *task, int regno,
			  unsigned long data)
{
	unsigned long *addr;

	if (regno == REGNO_USP)
		addr = &task->thread.usp;
#if !defined(__CSKYABIV2__)  // ABIV2 task->thread have no hi & lo;
        else if (regno == CSKY_HI_NUM)
                addr = &task->thread.hi;
        else if (regno == CSKY_LO_NUM)
                addr = &task->thread.lo;
#endif
	else if ((regno < sizeof(regoff) / sizeof(regoff[0])) && (regoff[regno] != -1))
		addr = (unsigned long *) (task->thread.esp0 + regoff[regno]);
	else
		return -1;
	*addr = data;
	return 0;
}
/*
 * Make sure the single step bit is not set.
 */
static void singlestep_disable(struct task_struct *child)
{
	unsigned long tmp;
	tmp = (get_reg(child, REGNO_SR) & TRACE_MODE_MASK) | TRACE_MODE_RUN;
	put_reg(child, REGNO_SR, tmp);
	/* FIXME maybe wrong here: if clear flag of TIF_DELAYED_TRACE? */
}


static void singlestep_enable(struct task_struct *child)
{
	unsigned long tmp;
    tmp = (get_reg(child, REGNO_SR) & TRACE_MODE_MASK) | TRACE_MODE_SI;
    put_reg(child, REGNO_SR, tmp);
	/* FIXME maybe wrong here: if set flag of TIF_DELAYED_TRACE? */

}

/*
 * Make sure the single step bit is set.
 */
void user_enable_single_step(struct task_struct *child)
{
	if (child->thread.esp0 == 0) return;
	singlestep_enable(child);
}

void user_disable_single_step(struct task_struct *child)
{
	if (child->thread.esp0 == 0) return;
	singlestep_disable(child);
}

int ptrace_getfpregs(struct task_struct *child, void __user *data)
{

	if (!access_ok(VERIFY_WRITE, data, sizeof(struct user_cskyfp_struct)))
		return -EIO;

	if(copy_to_user(data, &child->thread.fcr,
				sizeof(struct user_cskyfp_struct)))
		return -EFAULT;

	return 0;
}

int ptrace_setfpregs(struct task_struct *child, void __user *data)
{
	if (!access_ok(VERIFY_READ, data, sizeof(struct user_cskyfp_struct)))
		return -EIO;

	if(copy_from_user(&child->thread.fcr, data,
                        sizeof(struct user_cskyfp_struct)))
		return -EFAULT;
	return 0;
}

/*
 * Called by kernel/ptrace.c when detaching..
 *
 * Make sure the single step bit is not set.
 */
void ptrace_disable(struct task_struct *child)
{
	singlestep_disable(child);
}

/*
 * Handle the requests of ptrace system call.
 *
 * INPUT:
 * child   - process being traced.
 * request - request type.
 * addr    - address of data that this request to read from or write to.
 * data    - address of data that this request to read to or write from.
 *
 * RETURN:
 * 0       - success
 * others  - fail
 */
long arch_ptrace(struct task_struct *child, long request, unsigned long addr,
	unsigned long data)
{
	unsigned long tmp = 0, ret = 0;
	int i;

	switch (request) {
	/* read the word at location addr in the USER area. */
	case PTRACE_PEEKUSR:
		if (addr & 3)
			goto out_eio;
		addr >>= 2;     /* temporary hack. */

		if (addr >= 0 && addr <= CSKY_GREG_NUM) {
			tmp = get_reg(child, addr);
		}else if(addr >= CSKY_FREG_NUM_LO && addr < CSKY_FREG_NUM_HI) {
			tmp = child->thread.fp[addr - CSKY_FREG_NUM_LO];
		}else if(addr >= CSKY_FREG_NUM_HI && addr < CSKY_FCR_NUM) {
			tmp = (&(child->thread.fcr))[addr - CSKY_FREG_NUM_HI];
		} else
			break;
		ret = put_user(tmp,(long unsigned int *) data);
		break;

	case PTRACE_POKEUSR:  /* write the word at location addr in the USER area */
		if (addr & 3)
			goto out_eio;
		addr >>= 2;     /* temporary hack. */

		if (addr >= 0 && addr <= CSKY_GREG_NUM) {
			if (put_reg(child, addr, data)) /*** should protect 'psr'? ***/
				goto out_eio;
		}else if(addr >= CSKY_FREG_NUM_LO && addr < CSKY_FREG_NUM_HI) {
			child->thread.fp[addr - CSKY_FREG_NUM_LO] = data;
		}else if(addr >= CSKY_FREG_NUM_HI && addr <= CSKY_FCR_NUM) {
			(&(child->thread.fcr))[addr - CSKY_FREG_NUM_HI] = data;
		}else
			goto out_eio;
		break;
	case PTRACE_GETREGS:    /* Get all gp regs from the child. */
		for (i = 0; i <= CSKY_GREG_NUM; i++) {
			tmp = get_reg(child, i);
			ret = put_user(tmp, (unsigned long *)data);
			if (ret)
				break;
			data += sizeof(long);
		}
		break;
	case PTRACE_SETREGS:    /* Set all gp regs in the child. */
		for (i = 0; i <= CSKY_GREG_NUM; i++) {
			ret = get_user(tmp, (unsigned long *)data);
			if (ret)
				break;
			put_reg(child, i, tmp);
			data += sizeof(long);
		}
		break;

	case PTRACE_GETFPREGS:
		ret = ptrace_getfpregs(child, (void  __user *) data);
		break;

	case PTRACE_SETFPREGS:
		ret = ptrace_setfpregs(child, (void __user *) data);
		break;

        case PTRACE_GET_THREAD_AREA:
                ret = put_user(task_thread_info(child)->tp_value,
                               (long unsigned int *) data);
                break;
	default:
		ret = ptrace_request(child, request, addr, data);
		break;
	}

	return ret;
out_eio:
	return -EIO;
}

/*
 * If process's system calls is traces, do some corresponding handles in this
 * fuction before entering system call function and after exiting system call
 * fuction.
 */
asmlinkage void syscall_trace(int why, struct pt_regs * regs)
{
	long saved_why;
#ifdef __CSKYABIV2__
	unsigned int SAVEDNUM =  5;  // r9 in ABIV2
#else
	unsigned int SAVEDNUM =  3;  // r9 in ABIV1
#endif
	/*
	 * Save saved_why, why is used to denote syscall entry/exit;
	 * why = 0:entry, why = 1: exit
	 */
	saved_why = regs->regs[SAVEDNUM];
	regs->regs[SAVEDNUM] = why;

        /* the 0x80 provides a way for the tracing parent to distinguish
           between a syscall stop and SIGTRAP delivery */
	ptrace_notify(SIGTRAP | ((current->ptrace & PT_TRACESYSGOOD)
                                 ? 0x80 : 0));
	/*
	 * this isn't the same as continuing with a signal, but it will do
	 * for normal use.  strace only continues with a signal if the
	 * stopping signal is not SIGTRAP.  -brl
	 */
	if (current->exit_code) {
		send_sig(current->exit_code, current, 1);
		current->exit_code = 0;
	}

	regs->regs[SAVEDNUM] = saved_why;
	return;
}
