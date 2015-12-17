/*
 * arch/csky/lib/backtrace.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2015, Chen Linfei (linfei_chen@c-sky.com)
 */


#include <linux/kernel.h>

#include <asm/addrspace.h>
#include <asm/readreg.h>

#include "backtrace.h"


#ifdef CONFIG_KALLSYMS
extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const unsigned long kallsyms_num_syms
__attribute__((weak, section(".rodata")));
extern unsigned long kallsyms_lookup_name(const char *name);

/*
 * addr: Get the insn of addr
 * insn: Store the insn in insn
 * Return: The insn's length
 */
#ifdef __CSKYABIV1__
inline static int csky_get_insn(unsigned long addr, unsigned int *insn)
{
	*insn = *(unsigned short *)addr;
	return 2;
}
#else
noinline static int csky_get_insn(unsigned long addr, unsigned int *insn)
{
	if ((*(unsigned short *)addr >> 14) == 0x3)
	{
		*insn = *(unsigned short *)(addr + 2) | (*(unsigned short *)addr) << 16;
		return 4;
	}
	else
	{
		*insn = *(unsigned short *)addr;
		return 2;
	}
}
#endif

/*
 * Analyze a funtion insn by insn to get the stack information.
 * start_pc: Start pc of the function
 * limit_pc: End pc of the function
 * r15_offset: Store offset of r15 storage
 * subi_len: Store stack length of the function
 */
#ifdef __CSKYABIV1__
void csky_analyze_prologue(unsigned long start_pc, unsigned long limit_pc,
                              unsigned long *r15_offset, unsigned long *subi_len)
{
	unsigned long addr;
	unsigned int insn, rn;
	int framesize;

	int mfcr_regnum;
	int stw_regnum;

	/*
         * When build programe with -fno-omit-frame-pointer, there must be
         * mov r8, r0
         * in prologue, here, we check such insn, once hit, we set IS_FP_SAVED.
         */
	int is_fp_saved = 0;

#define CSKY_NUM_GREGS_v1  16
#define CSKY_NUM_GREGS_SAVED_v1 (16+4)
	int register_offsets[CSKY_NUM_GREGS_SAVED_v1];   // 16 general registers + EPSR EPC, FPSR, FPC

	for (rn = 0; rn < CSKY_NUM_GREGS_SAVED_v1; rn++)
		register_offsets[rn] = -1;

	/*
         * Analyze the prologue. Things we determine from analyzing the
         * prologue include:
         *   the size of the frame
         *   where saved registers are located (and which are saved)
         *   FP used?
         */

	framesize = 0;
	for (addr = start_pc; addr < limit_pc; addr += 2)
	{
		/* Get next insn */
		csky_get_insn (addr, &insn);

		if (V1_IS_SUBI0 (insn))
		{
			int offset = 1 + ((insn >> 4) & 0x1f);
			if (!is_fp_saved)
				framesize += offset;

			continue;
		}
		else if (V1_IS_STM (insn))
		{
			/* Spill register(s) */
			int offset;
			int start_register;

			/* BIG WARNING! The CKCore ABI does not restrict functions
                         * to taking only one stack allocation. Therefore, when
                         * we save a register, we record the offset of where it was
                         * saved relative to the current framesize. This will
                         * then give an offset from the SP upon entry to our
                         * function. Remember, framesize is NOT constant until
                         * we're done scanning the prologue.
                         */
			start_register = (insn & 0xf);

			for (rn = start_register, offset = 0; rn <= 15; rn++, offset += 4)
			{
				register_offsets[rn] = framesize - offset;
			}
			continue;
		}
		else if (V1_IS_STWx0 (insn))
		{
			/* Spill register: see note for IS_STM above. */
			int imm;

			rn = (insn >> 8) & 0xf;
			imm = (insn >> 4) & 0xf;
			register_offsets[rn] = framesize - (imm << 2);
			continue;
			}
		else if (V1_IS_MOV_FP_SP (insn))
		{
			// fp have saved, so we skip this insn, and go on prologue.
			is_fp_saved = 1;
			continue;
		}
		else if (V1_IS_BSR_NEXT (insn))
		{
			/*
			 * We skip the following three insn in elf or so built with fPIC
			 * We call it "fPIC insns"
			 *		bsr label     --------- insn1
			 * label : 	lrw r14, imm  --------- insn2
			 * 		addu r14, r15 --------- insn3
			 * 	 	ld   r15, (r0, imm)   ---------insn4 (-O0)
			 */
			csky_get_insn (addr + 2, &insn);
			if (!(V1_IS_LRW_R14(insn)))
			{
				// Not fPIC insns
				break;
			}
			csky_get_insn (addr + 4, &insn);
			if (!(V1_IS_ADDU_R14_R15(insn)))
			{
				// Not fPIC insn
				break;
			}
			// Yes! It is fPIC insn, We continue to prologue.
			addr += 4; // pc -> addr r14, r15
			csky_get_insn (addr + 2, &insn);
			// when compile without optimization,
			if (V1_IS_LD_R15 (insn))
			{
				addr += 2;
			}
			continue;
		}
		else if (V1_IS_MFCR_EPSR (insn))
		{
			unsigned int insn2;
			addr += 2;
			mfcr_regnum = insn & 0xf;
			csky_get_insn (addr, &insn2);
			stw_regnum = insn2 & 0xf00;
			if (V1_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
			{
				int imm;

				rn = CSKY_NUM_GREGS_v1 ; //CSKY_EPSR_REGNUM
				imm = (insn2 >> 4) & 0xf;
				register_offsets[rn] = framesize - (imm << 2);
				continue;
			}
			break;
		}
		else if (V1_IS_MFCR_FPSR (insn))
		{
			unsigned int insn2;
			addr += 2;
			mfcr_regnum = insn & 0xf;
			csky_get_insn (addr, &insn2);
			stw_regnum = insn2 & 0xf00;
			if (V1_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
			{
				int imm;

				rn = CSKY_NUM_GREGS_v1 + 1; //CSKY_FPSR_REGNUM
				imm = (insn2 >> 4) & 0xf;
				register_offsets[rn] = framesize - (imm << 2);
				continue;
			}
			break;
		}
		else if (V1_IS_MFCR_EPC (insn) )
		{
			unsigned int insn2;
			addr += 2;
			mfcr_regnum = insn & 0xf;
			csky_get_insn (addr, &insn2);
			stw_regnum = insn2 & 0xf00;
			if (V1_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
			{
				int imm;

				rn = CSKY_NUM_GREGS_v1 + 2; //CSKY_EPC_REGNUM
				imm = (insn2 >> 4) & 0xf;
				register_offsets[rn] = framesize - (imm << 2);
				continue;
			}
			break;
		}
		else if (V1_IS_MFCR_FPC (insn) )
		{
			unsigned int insn2;
			addr += 2;
			mfcr_regnum = insn & 0xf;
			csky_get_insn (addr, &insn2);
			stw_regnum = insn2 & 0xf00;
			if (V1_IS_STWx0 (insn2) && (mfcr_regnum == stw_regnum))
			{
				int imm;

				rn = CSKY_NUM_GREGS_v1 + 3; //CSKY_FPC_REGNUM
				imm = (insn2 >> 4) & 0xf;
				register_offsets[rn] = framesize - (imm << 2);
				continue;
			}
			break;
		}
		// begin analyze adjust frame by r1
		else if (V1_IS_LRW1 (insn) || V1_IS_MOVI1 (insn)
                         || V1_IS_BGENI1 (insn) || V1_IS_BMASKI1 (insn))
		{
			int adjust = 0;
			int offset = 0;
			unsigned int insn2;

			if (V1_IS_LRW1 (insn))
			{
				int literal_addr = (addr + 2 + ((insn & 0xff) << 2)) & 0xfffffffc;
				adjust = *(unsigned long*)literal_addr;

			}
			else if (V1_IS_MOVI1 (insn))
				adjust = (insn >> 4) & 0x7f;
			else if (V1_IS_BGENI1 (insn))
				adjust = 1 << ((insn >> 4) & 0x1f);
			else  /* IS_BMASKI (insn) */
				adjust = (1 << (adjust >> 4) & 0x1f) - 1;


			/* May have zero or more insns which modify r1 */
			offset = 2;

			// if out of prologue range, we should exit right now.
			if ((addr + offset) < limit_pc)
				csky_get_insn (addr + offset, &insn2);
			else
				break;

			while ((V1_IS_R1_ADJUSTER (insn2)) && (addr + offset) < limit_pc)
			{
				int imm;

				imm = (insn2 >> 4) & 0x1f;
				if (V1_IS_ADDI1 (insn2))
				{
					adjust += (imm + 1);
				}
				else if (V1_IS_SUBI1 (insn2))
				{
					adjust -= (imm + 1);
				}
				else if (V1_IS_RSUBI1 (insn2))
				{
					adjust = imm - adjust;
				}
				else if (V1_IS_NOT1 (insn2))
				{
					adjust = ~adjust;
				}
				else if (V1_IS_ROTLI1 (insn2))
				{
					int temp = adjust >> (16 - imm);
					adjust <<= imm;
					adjust |= temp;
				}
				else if ( V1_IS_LSLI1(insn2))
				{
					adjust <<= imm;
				}
				else if (V1_IS_BSETI1 (insn2))
				{
					adjust |= (1 << imm);
				}
				else if (V1_IS_BCLRI1 (insn2))
				{
					adjust &= ~(1 << imm);
				}
				else if (V1_IS_IXH1 (insn2))
				{
					adjust *= 3;
				}
				else if (V1_IS_IXW1 (insn2))
				{
					adjust *= 5;
				}
				else if (V1_IS_STWSP(insn2))
				{
					// junc insn, ignore it.
					offset += 2;
					csky_get_insn (addr + offset, &insn2);
					continue;
				}
				else if (V1_IS_SUB01(insn2))
				{
					if (!is_fp_saved)
						framesize += adjust;
				}
				else if (V1_IS_MOVI1 (insn2))
				{
					adjust = (insn2 >> 4) & 0x7f;
				}

				offset += 2;
				csky_get_insn (addr + offset, &insn2);
			};

			/*
                         * If the next insn adjusts the stack pointer, we keep everything;
                         * if not, we scrap it and we've found the end of the prologue.
                         */
			if (V1_IS_MOV_FP_SP(insn2) && (addr+offset) < limit_pc)
			{
				// Do not forget to skip this insn.
				is_fp_saved = 1;
			}

			/* None of these instructions are prologue, so don't touch anything. */
			addr += (offset - 2);
			continue;
		}

		/* This is not a prologue insn, so stop here. */
		break;

	}

	*subi_len = framesize;
	*r15_offset = register_offsets[15];
}

#else /* abiv2 */

void csky_analyze_prologue(unsigned long start_pc, unsigned long limit_pc,
                              unsigned long *r15_offset, unsigned long *subi_len)

{
	unsigned long addr;
	unsigned int insn, rn;
	int framesize;
	int stacksize;
#define CSKY_NUM_GREGS_v2  32
#define CSKY_NUM_GREGS_v2_SAVED_GREGS   (CSKY_NUM_GREGS_v2+4)
	int register_offsets[CSKY_NUM_GREGS_v2_SAVED_GREGS]; // 32 general regs + 4
	int insn_len;

	int mfcr_regnum;
	int stw_regnum;

	/*
         * When build programe with -fno-omit-frame-pointer, there must be
         * mov r8, r0
         * in prologue, here, we check such insn, once hit, we set IS_FP_SAVED.
         */

	/* REGISTER_OFFSETS will contain offsets, from the top of the frame
         * (NOT the frame pointer), for the various saved registers or -1
         * if the register is not saved. */
	for (rn = 0; rn < CSKY_NUM_GREGS_v2_SAVED_GREGS; rn++)
		register_offsets[rn] = -1;

	/* Analyze the prologue. Things we determine from analyzing the
         * prologue include:
         * the size of the frame
         * where saved registers are located (and which are saved)
         * FP used?
         */

	stacksize = 0;
	insn_len = 2; //instruction is 16bit
	for (addr = start_pc; addr < limit_pc; addr += insn_len)
	{
		/* Get next insn */
		insn_len = csky_get_insn (addr, &insn);

		if(insn_len == 4) //if 32bit
		{
			if (V2_32_IS_SUBI0 (insn)) //subi32 sp,sp oimm12
			{
				int offset = V2_32_SUBI_IMM(insn); //got oimm12
				stacksize += offset;
				continue;
			}
			else if (V2_32_IS_STMx0 (insn))   //stm32 ry-rz,(sp)
			{
				/* Spill register(s) */
				int start_register;
				int reg_count;
				int offset;

				/* BIG WARNING! The CKCore ABI does not restrict functions
	                         * to taking only one stack allocation. Therefore, when
	                         * we save a register, we record the offset of where it was
	                         * saved relative to the current stacksize. This will
	                         * then give an offset from the SP upon entry to our
	                         * function. Remember, stacksize is NOT constant until
	                         * we're done scanning the prologue.
	                         */
				start_register = V2_32_STM_VAL_REGNUM(insn);  //ry
				reg_count = V2_32_STM_SIZE(insn);

				for (rn = start_register, offset = 0;
	                             rn <= start_register + reg_count;
	                             rn++, offset += 4)
				{
					register_offsets[rn] = stacksize - offset;
				}
				continue;
			}
			else if (V2_32_IS_STWx0 (insn))   //stw ry,(sp,disp)
			{
				/* Spill register: see note for IS_STM above. */
				int disp;

				rn = V2_32_ST_VAL_REGNUM(insn);
				disp = V2_32_ST_OFFSET(insn);
				register_offsets[rn] = stacksize - disp;
				continue;
			}
			else if (V2_32_IS_MOV_FP_SP (insn))
			{
				// Do not forget to skip this insn.
				continue;
			}
			else if (V2_32_IS_MFCR_EPSR(insn))
			{
				unsigned int insn2;
				addr += 4;
				mfcr_regnum = insn & 0x1f;
				insn_len = csky_get_insn (addr, &insn2);
				if (insn_len == 2)
				{
					stw_regnum = (insn2 >> 5) & 0x7;
					if ( V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2; //CSKY_EPSR_REGNUM
						offset = V2_16_STWx0_OFFSET(insn2);
						register_offsets[rn] = stacksize - offset;
						continue;
					}
					break;
				}
				else  // insn_len == 4
				{
					stw_regnum = (insn2 >> 21) & 0x1f;
					if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2; //CSKY_EPSR_REGNUM
						offset = V2_32_ST_OFFSET(insn2);
						register_offsets[rn] = framesize - offset;
						continue;
					}
					break;
				}
			}
			else if (V2_32_IS_MFCR_FPSR(insn))
			{
				unsigned int insn2;
				addr += 4;
				mfcr_regnum = insn & 0x1f;
				insn_len = csky_get_insn (addr, &insn2);
				if (insn_len == 2)
				{
					stw_regnum = (insn2 >> 5) & 0x7;
					if ( V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2 + 1; //CSKY_FPSR_REGNUM
						offset = V2_16_STWx0_OFFSET(insn2);
						register_offsets[rn] = stacksize - offset;
						continue;
					}
					break;
				}
				else  // insn_len == 4
				{
					stw_regnum = (insn2 >> 21) & 0x1f;
					if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2 + 1; //CSKY_FPSR_REGNUM
						offset = V2_32_ST_OFFSET(insn2);
						register_offsets[rn] = framesize - offset;
						continue;
					}
					break;
				}
			}
			else if (V2_32_IS_MFCR_EPC(insn))
			{
				unsigned int insn2;
				addr += 4;
				mfcr_regnum = insn & 0x1f;
				insn_len = csky_get_insn (addr, &insn2);
				if (insn_len == 2)
				{
					stw_regnum = (insn2 >> 5) & 0x7;
					if ( V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2 + 2; //CSKY_EPC_REGNUM
						offset = V2_16_STWx0_OFFSET(insn2);
						register_offsets[rn] = stacksize - offset;
						continue;
					}
					break;
				}
				else  // insn_len == 4
				{
					stw_regnum = (insn2 >> 21) & 0x1f;
					if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2 + 2; //CSKY_EPC_REGNUM
						offset = V2_32_ST_OFFSET(insn2);
						register_offsets[rn] = framesize - offset;
						continue;
					}
					break;
				}
			}
			else if (V2_32_IS_MFCR_FPC(insn))
			{
				unsigned int insn2;
				addr += 4;
				mfcr_regnum = insn & 0x1f;
				insn_len = csky_get_insn (addr, &insn2);
				if (insn_len == 2)
				{
					stw_regnum = (insn2 >> 5) & 0x7;
					if ( V2_16_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2 + 3; // CSKY_FPC_REGNUM
						offset = V2_16_STWx0_OFFSET(insn2);
						register_offsets[rn] = stacksize - offset;
						continue;
					}
					break;
				}
				else  // insn_len == 4
				{
					stw_regnum = (insn2 >> 21) & 0x1f;
					if (V2_32_IS_STWx0(insn2) && (mfcr_regnum == stw_regnum))
					{
						int offset;

						rn  = CSKY_NUM_GREGS_v2 + 3; //CSKY_FPC_REGNUM
						offset = V2_32_ST_OFFSET(insn2);
						register_offsets[rn] = framesize - offset;
						continue;
					}
					break;
				}
			}
			else if (V2_32_IS_PUSH(insn))    //push for 32_bit
			{
				int offset = 0;
				if (V2_32_IS_PUSH_R29(insn))
				{
					stacksize += 4;
					register_offsets[29] = stacksize;
					offset += 4;
				}
				if (V2_32_PUSH_LIST2(insn))
				{
					int num = V2_32_PUSH_LIST2(insn);
					int tmp = 0;
					stacksize += num * 4;
					offset += num * 4;
					for (rn = 16; rn <= 16 + num - 1; rn++)
					{
						register_offsets[rn] = stacksize - tmp;
						tmp += 4;
					}
				}
				if (V2_32_IS_PUSH_R15(insn))
				{
					stacksize += 4;
					register_offsets[15] = stacksize;
					offset += 4;
				}
				if (V2_32_PUSH_LIST1(insn))
				{
					int num = V2_32_PUSH_LIST1(insn);
					int tmp = 0;
					stacksize += num * 4;
					offset += num * 4;
					for (rn = 4; rn <= 4 + num - 1; rn++)
					{
						register_offsets[rn] = stacksize - tmp;
						tmp += 4;
					}
				}

				framesize = stacksize;
				continue;
			}  //end of push for 32_bit
			else if ( V2_32_IS_LRW4(insn) || V2_32_IS_MOVI4(insn)
		                        || V2_32_IS_MOVIH4(insn) || V2_32_IS_BMASKI4(insn))
			{
				int adjust = 0;
				int offset = 0;
				unsigned int insn2;

				if (V2_32_IS_LRW4(insn))
				{
					int literal_addr = (addr + ((insn & 0xffff) << 2)) & 0xfffffffc;
					csky_get_insn(literal_addr, &adjust);
				}
				else if (V2_32_IS_MOVI4 (insn))
					adjust = (insn  & 0xffff);
				else if (V2_32_IS_MOVIH4 (insn))
					adjust = (insn & 0xffff) << 16;
				else                  /* V2_32_IS_BMASKI4 (insn) */
					adjust = (1 << (((insn & 0x3e00000) >> 21) + 1)) - 1;


				/* May have zero or more insns which modify r4 */
				offset = 4;
				insn_len = csky_get_insn (addr + offset, &insn2);
				while (V2_IS_R4_ADJUSTER(insn2))
				{
					if (V2_32_IS_ADDI4(insn2))
					{
						int imm = (insn2 & 0xfff) + 1;
						adjust += imm;
					}
					else if (V2_32_IS_SUBI4(insn2))
					{
						int imm = (insn2 & 0xfff) + 1;
						adjust -= imm;
					}
					else if (V2_32_IS_NOR4(insn2))
					{
						adjust = ~adjust;
					}
					else if (V2_32_IS_ROTLI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						int temp = adjust >> (32 - imm);
						adjust <<= imm;
						adjust |= temp;
					}
					else if (V2_32_IS_LISI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						adjust <<= imm;
					}
					else if (V2_32_IS_BSETI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						adjust |= (1 << imm);
					}
					else if (V2_32_IS_BCLRI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						adjust &= ~(1 << imm);
					}
					else if (V2_32_IS_IXH4(insn2))
					{
						adjust *= 3;
					}
					else if (V2_32_IS_IXW4(insn2))
					{
						adjust *= 5;
					}
					else if (V2_16_IS_ADDI4(insn2))
					{
						int imm = (insn2 & 0xff) + 1;
						adjust += imm;
					}
					else if (V2_16_IS_SUBI4(insn2))
					{
						int imm = (insn2 & 0xff) + 1;
						adjust -= imm;
					}
					else if (V2_16_IS_NOR4(insn2))
					{
						adjust = ~adjust;
					}
					else if (V2_16_IS_BSETI4(insn2))
					{
						int imm = (insn2 & 0x1f);
						adjust |= (1 << imm);
					}
					else if (V2_16_IS_BCLRI4(insn2))
					{
						int imm = (insn2 & 0x1f);
						adjust &= ~(1 << imm);
					}
					else if (V2_16_IS_LSLI4(insn2))
					{
						int imm = (insn2 & 0x1f);
						adjust <<= imm;
					}

					offset += insn_len;
					insn_len =  csky_get_insn (addr + offset, &insn2);
				};

				/*
		                 * If the next insn adjusts the stack pointer, we keep everything;
		                 * if not, we scrap it and we've found the end of the prologue.
		                 */
				if (V2_IS_SUBU4(insn2))
				{
					addr += offset;
					stacksize += adjust;
					continue;
				}

				/* None of these instructions are prologue, so don't touch anything. */
				break;
			}
		}   // end of ' if(insn_len == 4)'
		else
		{
			if (V2_16_IS_SUBI0 (insn))  //subi.sp sp,disp
			{
				int offset = V2_16_SUBI_IMM(insn);
				stacksize += offset; //capacity of creating space in stack
				continue;
			}
			else if (V2_16_IS_STWx0 (insn))   //stw.16 rz,(sp,disp)
			{
				/* Spill register: see note for IS_STM above. */
				int disp;

				rn = V2_16_ST_VAL_REGNUM(insn);
				disp = V2_16_ST_OFFSET(insn);
				register_offsets[rn] = stacksize - disp;
				continue;
			}
			else if (V2_16_IS_MOV_FP_SP (insn))
			{
				// We do nothing here except omit this instruction
				continue;
			}
			else if (V2_16_IS_PUSH(insn)) //push for 16_bit
			{
				int offset = 0;
				if (V2_16_IS_PUSH_R15(insn))
				{
					stacksize += 4;
					register_offsets[15] = stacksize;
					offset += 4;
				}
				if (V2_16_PUSH_LIST1(insn))
				{
					int num = V2_16_PUSH_LIST1(insn);
					int tmp = 0;
					stacksize += num * 4;
					offset += num * 4;
					for (rn = 4; rn <= 4 + num - 1; rn++)
					{
						register_offsets[rn] = stacksize - tmp;
						tmp += 4;
					}
				}

				framesize = stacksize;
				continue;
			}  // end of push for 16_bit
			else if (V2_16_IS_LRW4(insn) || V2_16_IS_MOVI4(insn))
			{
				int adjust = 0;
				int offset = 0;
				unsigned int insn2;

				if (V2_16_IS_LRW4(insn))
				{
					int offset = ((insn & 0x300) >> 3) | (insn & 0x1f);
					int literal_addr = (addr + ( offset << 2)) & 0xfffffffc;
					adjust = *(unsigned long*)literal_addr;
				}
				else    // V2_16_IS_MOVI4(insn)
					adjust = (insn  & 0xff);

				/* May have zero or more insns which modify r4 */
				offset = 2;
				insn_len = csky_get_insn (addr + offset, &insn2);
				while (V2_IS_R4_ADJUSTER(insn2))
				{
					if (V2_32_IS_ADDI4(insn2))
					{
						int imm = (insn2 & 0xfff) + 1;
						adjust += imm;
					}
					else if (V2_32_IS_SUBI4(insn2))
					{
						int imm = (insn2 & 0xfff) + 1;
						adjust -= imm;
					}
					else if (V2_32_IS_NOR4(insn2))
					{
						adjust = ~adjust;
					}
					else if (V2_32_IS_ROTLI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						int temp = adjust >> (32 - imm);
						adjust <<= imm;
						adjust |= temp;
					}
					else if (V2_32_IS_LISI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						adjust <<= imm;
					}
					else if (V2_32_IS_BSETI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						adjust |= (1 << imm);
					}
					else if (V2_32_IS_BCLRI4(insn2))
					{
						int imm = ((insn2 >> 21) & 0x1f);
						adjust &= ~(1 << imm);
					}
					else if (V2_32_IS_IXH4(insn2))
					{
						adjust *= 3;
					}
					else if (V2_32_IS_IXW4(insn2))
					{
						adjust *= 5;
					}
					else if (V2_16_IS_ADDI4(insn2))
					{
						int imm = (insn2 & 0xff) + 1;
						adjust += imm;
					}
					else if (V2_16_IS_SUBI4(insn2))
					{
						int imm = (insn2 & 0xff) + 1;
						adjust -= imm;
					}
					else if (V2_16_IS_NOR4(insn2))
					{
						adjust = ~adjust;
					}
					else if (V2_16_IS_BSETI4(insn2))
					{
						int imm = (insn2 & 0x1f);
						adjust |= (1 << imm);
					}
					else if (V2_16_IS_BCLRI4(insn2))
					{
						int imm = (insn2 & 0x1f);
						adjust &= ~(1 << imm);
					}
					else if (V2_16_IS_LSLI4(insn2))
					{
						int imm = (insn2 & 0x1f);
						adjust <<= imm;
					}

					offset += insn_len;
					insn_len = csky_get_insn (addr + offset, &insn2);
				};


				/* If the next insn adjusts the stack pointer, we keep everything;
		                   if not, we scrap it and we've found the end of the prologue. */
				if (V2_IS_SUBU4(insn2))
				{
					addr += offset;
					stacksize += adjust;
					continue;
				}

				/* None of these instructions are prologue, so don't touch anything. */
				break;
			}
	        }

		/* This is not a prologue insn, so stop here. */
		break;

	}
	*subi_len = stacksize;
	*r15_offset = register_offsets[15];
}
#endif

/*
 * Get the index of a function in kallsyms.
 * TODO: Optimize.
 */
int get_start_addr_index(unsigned long pc)
{
	int i;
	for (i = 0; i < kallsyms_num_syms; i ++)
	{
		if ((kallsyms_addresses[i] <= pc) && (kallsyms_addresses[i + 1] > pc))
			return i;
	}

	return -1;
}

void show_trace(void)
{
	int index;
	unsigned long addr;
	unsigned long sp;
	unsigned long start_pc, limit_pc;
	unsigned long r15_offset, subi_len;

	addr = kallsyms_lookup_name(__FUNCTION__);
	sp = csky_get_sp();

	while(1)
	{
		index = get_start_addr_index(addr);
		if (index == -1)
			break;
		start_pc = kallsyms_addresses[index];
		limit_pc = kallsyms_addresses[index + 1];
		csky_analyze_prologue(start_pc, limit_pc, &r15_offset, &subi_len);

		if ((r15_offset != -1) && (subi_len != 0))
		{
			addr = *(unsigned long *)(sp + subi_len - r15_offset);
			sp += subi_len;
			printk(" [<%08lx>] %pS\n", addr, (void *)addr);
		}
		else
			break;
	}

}

#else

void show_trace(void)
{
	printk("Backtrace not supported!\n");
}
#endif
