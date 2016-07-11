#ifndef __ASM_CSKY_TRAPS_H
#define __ASM_CSKY_TRAPS_H

#ifndef __ASSEMBLY__

#include <linux/linkage.h>
#include <asm/ptrace.h>

typedef void (*e_vector)(void);

#endif

#define VEC_RESET       0
#define VEC_ALIGN       1
#define VEC_ACCESS      2
#define VEC_ZERODIV     3
#define VEC_ILLEGAL     4
#define VEC_PRIV        5
#define VEC_TRACE       6
#define VEC_BREAKPOINT  7
#define VEC_UNRECOVER   8
#define VEC_SOFTRESET   9
#define VEC_AUTOVEC     10
#define VEC_FAUTOVEC    11
#define VEC_HWACCEL     12

#define	VEC_TLBMISS	    14
#define	VEC_TLBMODIFIED	15

#define VEC_TRAP0       16
#define VEC_TRAP1       17
#define VEC_TRAP2       18
#define VEC_TRAP3       19

#define	VEC_TLBINVALIDL	20
#define	VEC_TLBINVALIDS	21

#define VEC_PRFL        29
#define VEC_FPE         30

#define VEC_USER        32

#define VEC_INT1        33
#define VEC_INT2        34
#define VEC_INT3        35
#define VEC_INT4        36
#define VEC_INT5        37
#define VEC_INT6        38
#define VEC_INT7        39
#define VEC_INT8        40

#define VECOFF(vec)     ((vec)<<2)

#ifndef __ASSEMBLY__

/* Status register bits */
#define PS_S            0x80000000              /* Supervisor Mode */
#define PS_TM           0x0000c000              /* Trace mode */
#define PS_TP           0x00002000              /* Trace pending */
#define PS_TC           0x00001000              /* Translation control */
#define PS_SC           0x00000400              /* Spare control */
#define PS_MM           0x00000200              /* Extern memory manager */
#define PS_EE           0x00000100              /* Exception enable */
#define PS_IC           0x00000080              /* Interrupt Control */
#define PS_IE           0x00000040              /* Interrupt enable */
#define PS_FE           0x00000010              /* Fast interrupt enable */
#define PS_AF           0x00000002              /* Alternate register file */
#define PS_C            0x00000001              /* Carrier */

#define PS_VECMASK      0x007f0000              /* VEC mask */



/* structure for stack frames */

struct frame {
    struct pt_regs ptregs;
    union {
	    struct {
		    unsigned long  iaddr;    /* instruction address */
	    } fmt2;
	    struct {
		    unsigned long  effaddr;  /* effective address */
	    } fmt3;
	    struct {
		    unsigned long  effaddr;  /* effective address */
		    unsigned long  pc;	     /* pc of faulted instr */
	    } fmt4;
	    struct {
		    unsigned long  effaddr;  /* effective address */
		    unsigned short ssw;      /* special status word */
		    unsigned short wb3s;     /* write back 3 status */
		    unsigned short wb2s;     /* write back 2 status */
		    unsigned short wb1s;     /* write back 1 status */
		    unsigned long  faddr;    /* fault address */
		    unsigned long  wb3a;     /* write back 3 address */
		    unsigned long  wb3d;     /* write back 3 data */
		    unsigned long  wb2a;     /* write back 2 address */
		    unsigned long  wb2d;     /* write back 2 data */
		    unsigned long  wb1a;     /* write back 1 address */
		    unsigned long  wb1dpd0;  /* write back 1 data/push data 0*/
		    unsigned long  pd1;      /* push data 1*/
		    unsigned long  pd2;      /* push data 2*/
		    unsigned long  pd3;      /* push data 3*/
	    } fmt7;
	    struct {
		    unsigned long  iaddr;    /* instruction address */
		    unsigned short int1[4];  /* internal registers */
	    } fmt9;
	    struct {
		    unsigned short int1;
		    unsigned short ssw;      /* special status word */
		    unsigned short isc;      /* instruction stage c */
		    unsigned short isb;      /* instruction stage b */
		    unsigned long  daddr;    /* data cycle fault address */
		    unsigned short int2[2];
		    unsigned long  dobuf;    /* data cycle output buffer */
		    unsigned short int3[2];
	    } fmta;
	    struct {
		    unsigned short int1;
		    unsigned short ssw;     /* special status word */
		    unsigned short isc;     /* instruction stage c */
		    unsigned short isb;     /* instruction stage b */
		    unsigned long  daddr;   /* data cycle fault address */
		    unsigned short int2[2];
		    unsigned long  dobuf;   /* data cycle output buffer */
		    unsigned short int3[4];
		    unsigned long  baddr;   /* stage B address */
		    unsigned short int4[2];
		    unsigned long  dibuf;   /* data cycle input buffer */
		    unsigned short int5[3];
		    unsigned	   ver : 4; /* stack frame version # */
		    unsigned	   int6:12;
		    unsigned short int7[18];
	    } fmtb;
    } un;
};

#endif /* __ASSEMBLY__ */

#endif /* __ASM_CSKY_TRAPS_H */
