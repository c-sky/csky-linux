/*
 * Target-dependent code for Renesas CSKY, for GDB.
 *
 * Copyright (C) 2010 Hangzhou C-Sky Micro, Inc.
 *
 * This file is copied from GDB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2015, Chen Linfei (linfei_chen@c-sky.com)
 */

#ifndef CSKY_BACKTRACE_H
#define CSKY_BACKTRACE_H


/* Instruction macros used for analyzing the prologue */
#define V1_IS_SUBI0(x)   (((x) & 0xfe0f) == 0x2400)        /* subi r0,oimm5    */
#define V1_IS_STWx0(x)   (((x) & 0xf00f) == 0x9000)        /* stw rz,(r0,disp) */
#define V1_IS_STWxy(x)   (((x) & 0xf000) == 0x9000)        /* stw rx,(ry,disp) */
#define V1_IS_MOVx0(x)   (((x) & 0xfff0) == 0x1200)        /* mov rn,r0        */
#define V1_IS_MOV80(x)   (((x) & 0xfff0) == 0x1208)        /* mov r8,r0        */
#define V1_IS_LRW1(x)    (((x) & 0xff00) == 0x7100)        /* lrw r1,literal   */
#define V1_IS_MOVI1(x)   (((x) & 0xf80f) == 0x6001)        /* movi r1,imm7     */
#define V1_IS_BGENI1(x)  (((x) & 0xfe0f) == 0x3201)        /* bgeni r1,imm5    */
#define V1_IS_BMASKI1(x) (((x) & 0xfe0f) == 0x2C01)        /* bmaski r1,imm5   */
#define V1_IS_ADDI1(x)   (((x) & 0xfe0f) == 0x2001)        /* addi r1,oimm5    */
#define V1_IS_SUBI1(x)   (((x) & 0xfe0f) == 0x2401)        /* subi r1,oimm5    */
#define V1_IS_RSUBI1(x)  (((x) & 0xfe0f) == 0x2801)        /* rsubi r1,imm5    */
#define V1_IS_NOT1(x)    (((x) & 0xffff) == 0x01f1)        /* not r1           */
#define V1_IS_ROTLI1(x)  (((x) & 0xfe0f) == 0x3801)        /* rotli r1,imm5    */
#define V1_IS_LSLI1(x)   (((x) & 0xfe0f) == 0x3c01)        /* lsli r1, imm5    */
#define V1_IS_BSETI1(x)  (((x) & 0xfe0f) == 0x3401)        /* bseti r1,imm5    */
#define V1_IS_BCLRI1(x)  (((x) & 0xfe0f) == 0x3001)        /* bclri r1,imm5    */
#define V1_IS_STWSP(x)	 (x == 0x9000)			   /* st r0, (r0, 0)   */
#define V1_IS_IXH1(x)    (((x) & 0xffff) == 0x1d11)        /* ixh r1,r1        */
#define V1_IS_IXW1(x)    (((x) & 0xffff) == 0x1511)        /* ixw r1,r1        */
#define V1_IS_SUB01(x)   (((x) & 0xffff) == 0x0510)        /* subu r0,r1       */
#define V1_IS_RTS(x)     (((x) & 0xffff) == 0x00cf)        /* jmp r15          */

#define V1_IS_R1_ADJUSTER(x) \
    (V1_IS_ADDI1(x) || V1_IS_SUBI1(x) || V1_IS_ROTLI1(x) || V1_IS_BSETI1(x) \
     || V1_IS_BCLRI1(x) || V1_IS_RSUBI1(x) || V1_IS_NOT1(x) || V1_IS_MOVI1(x) \
     || V1_IS_IXH1(x) || V1_IS_IXW1(x)) || V1_IS_LSLI1(x) || V1_IS_STWSP(x) \
     || V1_IS_SUB01(x)

#define V1_IS_ST(insn)               ((insn & 0x9000) == 0x9000)    /* st.b/h/w */
#define V1_ST_SIZE(insn)             ((insn & 0x6000) ? ((insn & 0x6000) >> 13 == 1 ? 1 : 2) : 4)
                                             /* size: 0(w)~4   1(b)~1  2(h)~2 */
#define V1_ST_ADDR_REGNUM(insn)      (insn & 0x000f)                /* rx */
#define V1_ST_OFFSET(insn)           ((insn & 0x00f0) >> 4)         /* disp */

#define V1_IS_STM(insn)              ((insn & 0xfff0) == 0x0070)    /* stm rf-r15,r0 */
#define V1_STM_SIZE(insn)            (15 - (insn & 0x000f) + 1)     /* count of registers */

#define V1_IS_STQ(insn)              ((insn & 0xfff0) == 0x0050)    /*  */
#define V1_STQ_ADDR_REGNUM(insn)     (insn & 0x000f)                /* rx */

#define V1_IS_LD(insn)               ((insn & 0x9000) == 0x8000)    /* ld.b/h/w */
#define V1_LD_SIZE(insn)             V1_ST_SIZE(insn)               /* size */
#define V1_LD_ADDR_REGNUM(insn)      V1_ST_ADDR_REGNUM(insn)        /* rx */
#define V1_LD_OFFSET(insn)           V1_ST_OFFSET(insn)             /* disp */

#define V1_IS_LDM(insn)              ((insn & 0xfff0) == 0x0060)    /*  */
#define V1_LDM_SIZE(insn)            V1_STM_SIZE(insn)              /* count of registers */

#define V1_IS_LDQ(insn)              ((insn & 0xfff0) == 0x0040)    /*  */
#define V1_LDQ_ADDR_REGNUM(insn)     (insn & 0x000f)                /* rx */

// for jmp/branch insn
#define V1_IS_BR(insn)               ((insn & 0xf800) == 0xf000)    /*  */
#define V1_IS_BT(insn)               ((insn & 0xf800) == 0xe000)    /*  */
#define V1_IS_BF(insn)               ((insn & 0xf800) == 0xe800)    /*  */
#define V1_IS_BSR(insn)              ((insn & 0xf800) == 0xf800)    /*  */
#define V1_IS_BTBF(insn)             ((insn & 0xf000) == 0xe000)    /*  */
#define V1_IS_JMP(insn)              ((insn & 0xfff0) == 0x00c0)    /*  */
#define V1_IS_JMPI(insn)             ((insn & 0xff00) == 0x7000)    /*  */
#define V1_IS_JSR(insn)              ((insn & 0xfff0) == 0x00d0)    /*  */
#define V1_IS_JSRI(insn)             ((insn & 0xff00) == 0x7f00)    /*  */

#define V1_BR_OFFSET(insn)           ((insn & 0x400) ? \
                                     (0 - (2 + ((0x3ff & insn) << 1))) : \
                                     (2 + ((0x3ff & insn) << 1)))
#define V1_BSR_OFFSET(insn)          V1_BR_OFFSET(insn)
#define V1_BTBF_OFFSET(insn)         V1_BR_OFFSET(insn)

#define V1_JMP_REGNUM(insn)          (insn & 0xf)

#define V1_JSR_REGNUM(insn)          (insn & 0xf)

#define V1_JSRI_OFFSET(insn)         ((((insn & 0xff) << 2) + 2) & 0xfffffffc)
#define V1_JMPI_OFFSET(insn)         V1_JSRI_OFFSET(insn)

#define V1_IS_MFCR_EPSR(insn)        ((insn & 0xfff0) == 0x1020)
#define V1_IS_MFCR_FPSR(insn)        ((insn & 0xfff0) == 0x1030)
#define V1_IS_MFCR_EPC(insn)         ((insn & 0xfff0) == 0x1040)
#define V1_IS_MFCR_FPC(insn)         ((insn & 0xfff0) == 0x1050)
#define V1_IS_RFI(insn)              (insn == 0x0003)
#define V1_IS_RTE(insn)              (insn == 0x0002)
#define V1_IS_MOV_FP_SP(insn)        (insn == 0x1208)  /* mov r8, r0 */

/*
 * Define three insn as follows for *.elf or *.so built with fPIC
 * We call it "fPIC insns"
 *            bsr label            --------- insn1
 * label :    lrw r14, imm         --------- insn2
 *            addu r14, r15        --------- insn3
 *  	      ld   r15, (r0, imm)  --------- insn4
 */
#define V1_IS_BSR_NEXT(insn)	     (insn == 0xf800)		 /* insn1 */
#define V1_IS_LRW_R14(insn)	     ((insn & 0xff00) == 0x7e00) /* insn2 */
#define V1_IS_ADDU_R14_R15(insn)     (insn == 0x1cfe)		 /* insn3 */
#define V1_IS_LD_R15(insn)	     ((insn & 0xff0f) == 0x8f00) /* insn4 */

// ----------------------V2P insn ------------------------
// for insn v2p
#define V2P_16_IS_ST(insn)            (((insn & 0xf000) == 0x5000) && ((insn & 0x0c00) != 0x0c00))
                                                /* st.b/h/w, the buttom half 0x0c00 is stm  */
#define V2P_16_ST_SIZE(insn)          (1 << ((insn & 0x0c00) >> 10)) /* size */
#define V2P_16_ST_ADDR_REGNUM(insn)   ((insn & 0x003c) >> 2)         /* rx */
#define V2P_16_ST_OFFSET(insn)        (insn & 0x3)                   /* disp */
#define V2P_16_ST_VAL_REGNUM(insn)    ((insn & 0x03c0) >> 6)         /* ry */
#define V2P_16_IS_STWx0(insn)         ((insn & 0xfc3c) == 0x5800)    /* stw ry, (r0, disp) */

#define V2P_16_IS_STM(insn)           ((insn & 0xfc00) == 0x5c00)
#define V2P_16_STM_ADDR_REGNUM(insn)  V2P_16_ST_ADDR_REGNUM(insn)     /* rx */
#define V2P_16_STM_SIZE(insn)         ((insn & 0x3) + 1)             /* count of registers */

#define V2P_16_IS_LD(insn)            (((insn & 0xf000) == 0x4000) && ((insn & 0x0c00) != 0x0c00))
                                                /* ld.b/h/w, the buttom half 0x0c00 is ldm  */
#define V2P_16_LD_SIZE(insn)          V2P_16_ST_SIZE(insn)            /* size */
#define V2P_16_LD_ADDR_REGNUM(insn)   V2P_16_ST_ADDR_REGNUM(insn)     /* rx */
#define V2P_16_LD_OFFSET(insn)        V2P_16_ST_OFFSET(insn)          /* disp */

#define V2P_16_IS_LDM(insn)           ((insn & 0xfc00) == 0x4c00)
#define V2P_16_LDM_ADDR_REGNUM(insn)  V2P_16_STM_ADDR_REGNUM(insn)    /* rx */
#define V2P_16_LDM_SIZE(insn)         V2P_16_STM_SIZE(insn)           /* count of registers */
#define V2P_16_STM_VAL_REGNUM(insn)   ((insn & 0x03c0) >> 6)         /* ry */
#define V2P_16_IS_STMx0(insn)         ((insn & 0xfc3c) == 0x5c00)    /* stm  ry-rz,(r0) */

#define V2P_32_IS_ST(insn)            ((insn & 0xfc00c000) == 0xdc000000)
                                                                    /* st.b/h/w */
#define V2P_32_ST_SIZE(insn)          (1 << ((insn & 0x3000) >> 12)) /* size: b/h/w */
#define V2P_32_ST_ADDR_REGNUM(insn)   ((insn & 0x001f0000) >> 16)    /* rx */
#define V2P_32_ST_OFFSET(insn)        (insn & 0x00000fff)            /* disp */
#define V2P_32_ST_VAL_REGNUM(insn)    ((insn & 0x03e00000) >> 21)    /* ry */
#define V2P_32_IS_STWx0(insn)         ((insn & 0xfc1ff000) == 0xdc002000)
                                                                    /* stw ry, (r0, disp) */

#define V2P_32_IS_STM(insn)           ((insn & 0xfc00ffe0) == 0xd4001c20)
#define V2P_32_STM_ADDR_REGNUM(insn)  V2P_32_ST_ADDR_REGNUM(insn)     /* rx */
#define V2P_32_STM_SIZE(insn)         ((insn & 0x1f) + 1)            /* count of registers */
#define V2P_32_STM_VAL_REGNUM(insn)   ((insn & 0x03e00000) >> 21)    /* ry */
#define V2P_32_IS_STMx0(insn)         ((insn & 0xfc1fffe0) == 0xd4001c20)
                                                                    /* stm ry-rz,(r0) */

#define V2P_32_IS_STR(insn)           (((insn & 0xfc000000) == 0xd4000000) && !(V2P_32_IS_STM(insn)))
#define V2P_32_STR_X_REGNUM(insn)     V2P_32_ST_ADDR_REGNUM(insn)     /* rx */
#define V2P_32_STR_Y_REGNUM(insn)     ((insn >> 21) & 0x1f)          /* ry */
#define V2P_32_STR_SIZE(insn)         (1 << ((insn & 0x0c00) >> 10)) /* size: b/h/w */
#define V2P_32_STR_OFFSET(insn)       ((insn & 0x000003e0) >> 5)     /* imm (for rx + ry*imm)*/

#define V2P_32_IS_STEX(insn)          ((insn & 0xfc00f000) == 0xdc007000)
#define V2P_32_STEX_ADDR_REGNUM(insn) ((insn & 0xf0000) >> 16)       /* rx */
#define V2P_32_STEX_OFFSET(insn)      (insn & 0x0fff)                /* disp */

#define V2P_32_IS_LD(insn)            ((insn & 0xfc008000) == 0xd8000000)
                                                                    /* ld.b/h/w */
#define V2P_32_LD_SIZE(insn)          V2P_32_ST_SIZE(insn)            /* size */
#define V2P_32_LD_ADDR_REGNUM(insn)   V2P_32_ST_ADDR_REGNUM(insn)     /* rx */
#define V2P_32_LD_OFFSET(insn)        V2P_32_ST_OFFSET(insn)          /* disp */

#define V2P_32_IS_LDM(insn)           ((insn & 0xfc00ffe0) == 0xd0001c20)
#define V2P_32_LDM_ADDR_REGNUM(insn)  V2P_32_STM_ADDR_REGNUM(insn)    /* rx */
#define V2P_32_LDM_SIZE(insn)         V2P_32_STM_SIZE(insn)           /* count of registers */

#define V2P_32_IS_LDR(insn)           (((insn & 0xfc00fe00) == 0xd0000000) && !(V2P_32_IS_LDM(insn))) 
#define V2P_32_LDR_X_REGNUM(insn)     V2P_32_STR_X_REGNUM(insn)       /* rx */
#define V2P_32_LDR_Y_REGNUM(insn)     V2P_32_STR_Y_REGNUM(insn)       /* ry */
#define V2P_32_LDR_SIZE(insn)         V2P_32_STR_SIZE(insn)           /* size: b/h/w */
#define V2P_32_LDR_OFFSET(insn)       V2P_32_STR_OFFSET(insn)         /* imm (for rx + ry*imm) */

#define V2P_32_IS_LDEX(insn)          ((insn & 0xfc00f000) == 0xd8007000)
#define V2P_32_LDEX_ADDR_REGNUM(insn) V2P_32_STEX_ADDR_REGNUM(insn)   /* rx */
#define V2P_32_LDEX_OFFSET(insn)      V2P_32_STEX_OFFSET(insn)        /* disp */


#define V2P_16_IS_SUBI0(insn)         ((insn & 0xffc1) == 0x2001)    /* subi r0, imm */
#define V2P_16_SUBI_IMM(insn)         (((insn & 0x003e) >> 1) + 1)


#define V2P_32_IS_SUBI0(insn)         ((insn & 0xffff0000) == 0xa4000000)
                                                                    /* subi r0, imm */
#define V2P_32_SUBI_IMM(insn)         ((insn & 0xffff) + 1)

#define V2P_16_IS_MOV_FP_SP(insn)     (insn == 0x1e00)     /* mov r8, r0 */
#define V2P_32_IS_MOV_FP_SP(insn)     (insn == 0xc4004828) /* mov r8, r0 */
              

#define V2_16_IS_ST(insn)            (((insn & 0xe000)==0xa000) && ((insn & 0x1800) != 0x1800)) /* check st16 but except st16 sp  */
#define V2_16_ST_SIZE(insn)          (1 << ((insn & 0x1800) >> 11))              /* size */
#define V2_16_ST_ADDR_REGNUM(insn)   ((insn & 0x700) >> 8)                       /* rx */
#define V2_16_ST_OFFSET(insn)        ((insn & 0x1f) << ((insn & 0x1800) >> 11))  /* disp */
#define V2_16_ST_VAL_REGNUM(insn)    ((insn &0xe0) >> 5)                         /* ry */
#define V2_16_IS_STWx0(insn)         ((insn & 0xf800) == 0xb800)          /* st16.w rz, (sp, disp) */
#define V2_16_STWx0_VAL_REGNUM(insn) V2_16_ST_ADDR_REGNUM(insn)
#define V2_16_STWx0_OFFSET(insn)     ((((insn & 0x700) >> 3) + (insn &0x1f)) <<2) /*disp*/


/* "stm" only exists in 32_bit insn now */
//#define V2_16_IS_STM(insn)           ((insn & 0xfc00) == 0x5c00)
//#define V2_16_STM_ADDR_REGNUM(insn)  V2_16_ST_ADDR_REGNUM(insn)     /* rx */
//#define V2_16_STM_SIZE(insn)         ((insn & 0x3) + 1)             /* count of registers */

#define V2_16_IS_LD(insn)            (((insn & 0xe000)==0x8000) && (insn & 0x1800) != 0x1800)/*check ld16 but except ld16 sp */
#define V2_16_LD_SIZE(insn)          V2_16_ST_SIZE(insn)            /* size */
#define V2_16_LD_ADDR_REGNUM(insn)   V2_16_ST_ADDR_REGNUM(insn)     /* rx */
#define V2_16_LD_OFFSET(insn)        V2_16_ST_OFFSET(insn)          /* disp */
#define V2_16_IS_LDWx0(insn)         ((insn & 0xf800) == 0x9800)    /*ld16.w rz,(sp,disp)*/
#define V2_16_LDWx0_OFFSET(insn)     V2_16_STWx0_OFFSET(insn)       /*disp*/
/* "ldm" only exists in 32_bit insn now */
//#define V2_16_IS_LDM(insn)           ((insn & 0xfc00) == 0x4c00)
//#define V2_16_LDM_ADDR_REGNUM(insn)  V2_16_STM_ADDR_REGNUM(insn)    /* rx */
//#define V2_16_LDM_SIZE(insn)         V2_16_STM_SIZE(insn)           /* count of registers */
//#define V2_16_STM_VAL_REGNUM(insn)   ((insn & 0x03c0) >> 6)         /* ry */
//#define V2_16_IS_STMx0(insn)         ((insn & 0xfc3c) == 0x5c00)    /* stm  ry-rz,(r0) */

#define V2_32_IS_ST(insn)            ((insn & 0xfc00c000) == 0xdc000000)
                                                                          /* st32.b/h/w/d */
#define V2_32_ST_SIZE(insn)          (1 << ((insn & 0x3000) >> 12))       /* size: b/h/w/d */
#define V2_32_ST_ADDR_REGNUM(insn)   ((insn & 0x001f0000) >> 16)          /* rx */
#define V2_32_ST_OFFSET(insn)        ((insn & 0xfff) << ((insn & 0x3000) >> 12))  /* disp */
#define V2_32_ST_VAL_REGNUM(insn)    ((insn & 0x03e00000) >> 21)          /* ry */
#define V2_32_IS_STWx0(insn)         ((insn & 0xfc1ff000) == 0xdc0e2000)  /* stw ry, (sp, disp) */


#define V2_32_IS_STM(insn)           ((insn & 0xfc00ffe0) == 0xd4001c20)  /*stm32 ry-rz, (rx)*/
#define V2_32_STM_ADDR_REGNUM(insn)  V2_32_ST_ADDR_REGNUM(insn)           /* rx */
#define V2_32_STM_SIZE(insn)         ((insn & 0x1f) + 1)                  /* count of registers */
#define V2_32_STM_VAL_REGNUM(insn)   ((insn & 0x03e00000) >> 21)          /* ry */
#define V2_32_IS_STMx0(insn)         ((insn & 0xfc1fffe0) == 0xd40e1c20)  /* stm32 ry-rz,(sp) */

#define V2_32_IS_STR(insn)           (((insn & 0xfc000000) == 0xd4000000) && !(V2_32_IS_STM(insn)))
                                                                    /*str32.b/h/w rz,(rx,ry<<offset)*/
#define V2_32_STR_X_REGNUM(insn)     V2_32_ST_ADDR_REGNUM(insn)     /* rx */
#define V2_32_STR_Y_REGNUM(insn)     ((insn >> 21) & 0x1f)          /* ry */
#define V2_32_STR_SIZE(insn)         (1 << ((insn & 0x0c00) >> 10)) /* size: b/h/w */
#define V2_32_STR_OFFSET(insn)       ((insn & 0x000003e0) >> 5)     /* imm (for rx + ry*imm)*/

#define V2_32_IS_STEX(insn)          ((insn & 0xfc00f000) == 0xdc007000) /*stex32.w rz,(rx,disp)*/
#define V2_32_STEX_ADDR_REGNUM(insn) ((insn & 0x1f0000) >> 16)           /* rx */
#define V2_32_STEX_OFFSET(insn)      ((insn & 0x0fff) << 2)              /* disp */

#define V2_32_IS_LD(insn)            ((insn & 0xfc00c000) == 0xd8000000) /* ld.b/h/w */
#define V2_32_LD_SIZE(insn)          V2_32_ST_SIZE(insn)                 /* size */
#define V2_32_LD_ADDR_REGNUM(insn)   V2_32_ST_ADDR_REGNUM(insn)          /* rx */
#define V2_32_LD_OFFSET(insn)        V2_32_ST_OFFSET(insn)               /* disp */

#define V2_32_IS_LDM(insn)           ((insn & 0xfc00ffe0) == 0xd0001c20)
#define V2_32_LDM_ADDR_REGNUM(insn)  V2_32_STM_ADDR_REGNUM(insn)    /* rx */
#define V2_32_LDM_SIZE(insn)         V2_32_STM_SIZE(insn)           /* count of registers */

#define V2_32_IS_LDR(insn)           (((insn & 0xfc00fe00) == 0xd0000000) && !(V2_32_IS_LDM(insn)))
                                                                    /*ldr32.b/h/w rz,(rx,ry<<offset)*/
#define V2_32_LDR_X_REGNUM(insn)     V2_32_STR_X_REGNUM(insn)       /* rx */
#define V2_32_LDR_Y_REGNUM(insn)     V2_32_STR_Y_REGNUM(insn)       /* ry */
#define V2_32_LDR_SIZE(insn)         V2_32_STR_SIZE(insn)           /* size: b/h/w */
#define V2_32_LDR_OFFSET(insn)       V2_32_STR_OFFSET(insn)         /* imm (for rx + ry*imm) */

#define V2_32_IS_LDEX(insn)          ((insn & 0xfc00f000) == 0xd8007000)
#define V2_32_LDEX_ADDR_REGNUM(insn) V2_32_STEX_ADDR_REGNUM(insn)   /* rx */
#define V2_32_LDEX_OFFSET(insn)      V2_32_STEX_OFFSET(insn)        /* disp */


#define V2_16_IS_SUBI0(insn)         ((insn & 0xfce0) == 0x1420)    /* subi.sp sp, disp*/
#define V2_16_SUBI_IMM(insn)         ((((insn & 0x300) >> 3) + (insn & 0x1f)) << 2) /*disp*/

#define V2_32_IS_SUBI0(insn)         ((insn & 0xfffff000) == 0xe5ce1000)    /*subi32 sp,sp,oimm12*/
#define V2_32_SUBI_IMM(insn)         ((insn & 0xfff) + 1)                   /*oimm12*/

/* for new instrctions in V2: push */
//push16
#define V2_16_IS_PUSH(insn)          ((insn & 0xffe0) == 0x14c0)
#define V2_16_IS_PUSH_R15(insn)      ((insn & 0x10) == 0x10)
#define V2_16_PUSH_LIST1(insn)       ( insn & 0xf)                          // r4 - r11

// pop16
#define V2_16_IS_POP(insn)           ((insn & 0xffe0) == 0x1480)
#define V2_16_IS_POP_R15(insn)       V2_16_IS_PUSH_R15(insn)
#define V2_16_POP_LIST1(insn)        V2_16_PUSH_LIST1(insn)                 // r4 - r11


//push32
#define V2_32_IS_PUSH(insn)          ((insn & 0xfffffe00) == 0xebe00000)
#define V2_32_IS_PUSH_R29(insn)      ((insn & 0x100) == 0x100)
#define V2_32_IS_PUSH_R15(insn)      ((insn & 0x10) == 0x10)
#define V2_32_PUSH_LIST1(insn)       ( insn & 0xf)         // r4 - r11
#define V2_32_PUSH_LIST2(insn)       ((insn & 0xe0) >> 5)  // r16 - r17

//pop32
#define V2_32_IS_POP(insn)           ((insn & 0xfffffe00) == 0xebc00000)
#define V2_32_IS_POP_R29(insn)      V2_32_IS_PUSH_R29(insn)
#define V2_32_IS_POP_R15(insn)      V2_32_IS_PUSH_R15(insn)
#define V2_32_POP_LIST1(insn)       V2_32_PUSH_LIST1(insn)                // r4 - r11
#define V2_32_POP_LIST2(insn)       V2_32_PUSH_LIST2(insn)                // r16 - r17

//adjust sp by r4(l0)
#define V2_16_IS_LRW4(x)    (((x) & 0xfce0) == 0x1080)        /* lrw r4,literal   */
#define V2_16_IS_MOVI4(x)   (((x) & 0xff00) == 0x3400)        /* movi r4,imm8     */
// insn:bgeni,bmaski only exist in 32_insn now
//#define V2_16_IS_BGENI4(x)  (((x) & 0xfe0f) == 0x3201)      /* bgeni r4,imm5    */
//#define V2_16_IS_BMASKI4(x) (((x) & 0xfe0f) == 0x2C01)      /* bmaski r4,imm5   */
#define V2_16_IS_ADDI4(x)   (((x) & 0xff00) == 0x2400)        /* addi r4,oimm8    */
#define V2_16_IS_SUBI4(x)   (((x) & 0xff00) == 0x2c00)        /* subi r4,oimm8    */
// insn:rsubi not exist in v2_insn
#define V2_16_IS_NOR4(x)    ((x)            == 0x6d12)        /* nor16 r4,r4      */
//insn:rotli not exist in v2_16_insn
#define V2_16_IS_LSLI4(x)   (((x) & 0xffe0) == 0x4480)        /* lsli r4,r4,imm5  */
#define V2_16_IS_BSETI4(x)  (((x) & 0xffe0) == 0x3ca0)        /* bseti r4,imm5    */
#define V2_16_IS_BCLRI4(x)  (((x) & 0xffe0) == 0x3c80)        /* bclri r4,imm5    */
// insn:ixh,ixw only exist in 32_insn now
#define V2_16_IS_SUBU4(x)   ((x)            == 0x6392)        /* subu sp,r4       */

#define V2_16_IS_R4_ADJUSTER(x)   (V2_16_IS_ADDI4(x) || V2_16_IS_SUBI4(x) \
            || V2_16_IS_BSETI4(x) || V2_16_IS_BCLRI4(x) || V2_16_IS_NOR4(x) || V2_16_IS_LSLI4(x))

#define V2_32_IS_LRW4(x)    (((x) & 0xffff0000) == 0xea840000)        /* lrw r4,literal   */
#define V2_32_IS_MOVI4(x)   (((x) & 0xffff0000) == 0xea040000)        /* movi r4,imm16    */
#define V2_32_IS_MOVIH4(x)  (((x) & 0xffff0000) == 0xea240000)        /* movih r4,imm16   */
#define V2_32_IS_BMASKI4(x) (((x) & 0xfc1fffff) == 0xc4005024)        /* bmaski r4,oimm5  */
#define V2_32_IS_ADDI4(x)   (((x) & 0xfffff000) == 0xe4840000)        /* addi r4,r4,oimm12*/
#define V2_32_IS_SUBI4(x)   (((x) & 0xfffff000) == 0xe4810000)        /* subi r4,r4,oimm12*/
// insn:rsubi not exist in v2_insn
#define V2_32_IS_NOR4(x)    ((x)                == 0xc4842484)        /* nor32 r4,r4,r4   */
#define V2_32_IS_ROTLI4(x)  (((x) & 0xfc1fffff) == 0xc4044904)        /* rotli r4,r4,imm5 */
#define V2_32_IS_LISI4(x)   (((x) & 0xfc1fffff) == 0xc4044824)        /* lsli r4,r4,imm5  */
#define V2_32_IS_BSETI4(x)  (((x) & 0xfc1fffff) == 0xc4042844)        /*bseti32 r4,r4,imm5*/
#define V2_32_IS_BCLRI4(x)  (((x) & 0xfc1fffff) == 0xc4042824)        /*bclri32 r4,r4,imm5*/
#define V2_32_IS_IXH4(x)    ((x)                == 0xc4840824)        /* ixh r4,r4,r4     */
#define V2_32_IS_IXW4(x)    ((x)                == 0xc4840844)        /* ixw r4,r4,r4     */
#define V2_32_IS_SUBU4(x)   ((x)                == 0xc48e008e)        /* subu32 sp,sp,r4  */

#define V2_32_IS_R4_ADJUSTER(x)   (V2_32_IS_ADDI4(x) || V2_32_IS_SUBI4(x) \
           || V2_32_IS_ROTLI4(x)  || V2_32_IS_IXH4(x) || V2_32_IS_IXW4(x) \
        || V2_32_IS_NOR4(x)  || V2_32_IS_BSETI4(x) || V2_32_IS_BCLRI4(x) || V2_32_IS_LISI4(x))

#define V2_IS_R4_ADJUSTER(x)  ( V2_32_IS_R4_ADJUSTER(x) || V2_16_IS_R4_ADJUSTER(x) )
#define V2_IS_SUBU4(x)  ( V2_32_IS_SUBU4(x) || V2_16_IS_SUBU4(x) )

// insn:mfcr only exist in v2_32
#define V2_32_IS_MFCR_EPSR(insn)    ((insn & 0xffffffe0) == 0xc0026020)   /* mfcr rz, epsr*/
#define V2_32_IS_MFCR_FPSR(insn)    ((insn & 0xffffffe0) == 0xc0036020)   /* mfcr rz, fpsr*/
#define V2_32_IS_MFCR_EPC(insn)     ((insn & 0xffffffe0) == 0xc0046020)   /* mfcr rz, epc*/
#define V2_32_IS_MFCR_FPC(insn)     ((insn & 0xffffffe0) == 0xc0056020)   /* mfcr rz, fpc*/

// insn:rte,rfi only exit in v2_32
#define V2_32_IS_RTE(insn)    (insn == 0xc0004020)
#define V2_32_IS_RFI(insn)    (insn == 0xc0004420)
#define V2_32_IS_JMP(insn)    ((insn & 0xffe0ffff) == 0xe8c00000)
#define V2_16_IS_JMP(insn)    ((insn & 0xffc3) == 0x7800)
#define V2_32_IS_JMPI(insn)   ((insn & 0xffff0000) == 0xeac00000)
#define V2_32_IS_JMPIX(insn)  ((insn & 0xffe0fffc) == 0xe9e00000)
#define V2_16_IS_JMPIX(insn)  ((insn & 0xf8fc) == 0x38e0)

#define V2_16_IS_BR(insn)     ((insn & 0xfc00) == 0x0400)
#define V2_32_IS_BR(insn)     ((insn & 0xffff0000) == 0xe8000000)
#define V2_16_IS_MOV_FP_SP(insn)     (insn == 0x6e3b)     /* mov r8, r14 */
#define V2_32_IS_MOV_FP_SP(insn)     (insn == 0xc40e4828) /* mov r8, r14 */

#endif /* backtrace.h */

