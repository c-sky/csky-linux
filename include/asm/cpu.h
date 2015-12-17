#ifndef __ASM_CSKY_CPU_H
#define __ASM_CSKY_CPU_H

/* 
   CPUID(cr13):
  Rev1
  +--------+-------+---------+---------+-----+---------+-----+--------+
  | Family | Model | I/Dcache| Foundry | Oth | Process | Rev | Version
  +--------+-------+---------+---------+-----+---------+-----+--------+
   31    28 27   24 23     16 15     12 11 10 9       8 7   4 3

  Rev2
  +--------+-------+---------+------------+----------+----------------+
  | Family | Model | Foundry |   Process  | Revision |  Version
  +--------+-------+---------+------------+----------+----------------+
   31    28 27   16 15     12 11         8 7        4 3

  Rev3
  +--------+-------+---------+--------+---------+----------+----------+
  | Index0 | Arch  | Family  | Class  |  Model  | Revision |  Version
  +--------+-------+---------+--------+---------+----------+----------+
   31    28 27   26 25     22 21    18 17      8 7        4 3
*/

#define CPUPID_UNKNOWN            0x0000

#define CPUID_VER_0               0x0000
#define CPUID_VER_1               0x0001
#define CPUID_VER_2               0x0002
#define CPUID_VER_3               0x0003

#define	CPU_UNKNOWN               0x0000
#define	CPU_CK500                 0x0001
#define CPU_CK600                 0x0002
#define CPU_CK800                 0x0003

#define CPUID_FPU_NONE            0x0000
#define CPUID_FPU_V1              0x0001
#define CPUID_FPU_V2              0x0002

#define CPUID_V1_FAMILY_CK500     0x00000000
#define CPUID_V1_FAMILY_DSP       0x10000000
#define CPUID_V1_FAMILY_DCORE     0x20000000
#define CPUID_V1_FAMILY_CK600     0x40000000

#define CPUID_V2_FAMILY_CK500     0x00000000
#define CPUID_V2_FAMILY_CK600     0x10000000

#define CPUID_V2_MODEL_DM         0x00010000
#define CPUID_V2_MODEL_SPM        0x00020000
#define CPUID_V2_MODEL_DS         0x00040000
#define CPUID_V2_MODEL_MMU        0x00080000
#define CPUID_V2_MODEL_FPU        0x00100000
#define CPUID_V2_MODEL_AXI        0x00200000

#define CPUID_V3_FAMILY_CK500     0x00000000
#define CPUID_V3_FAMILY_CK600     0x00400000
#define CPUID_V3_FAMILY_CK800     0x00800000

#define CPUID_V3_CLASS_CK803      0x00000000
#define CPUID_V3_CLASS_CK810      0x00040000

#define CPUID_V3_MODEL_DM         0x00000100
#define CPUID_V3_MODEL_MMU        0x00000200
#define CPUID_V3_MODEL_FPU        0x00000400
#define CPUID_V3_MODEL_SPM        0x00000800
#define CPUID_V3_MODEL_MGU        0x00001000
#define CPUID_V3_MODEL_BCTM       0x00002000
#define CPUID_V3_MODEL_VDSP       0x00004000

#define CPUID_DCACHE_NONE         0x00000000
#define CPUID_DCACHE_1K           0x00010000
#define CPUID_DCACHE_2K           0x00020000
#define CPUID_DCACHE_4K           0x00040000
#define CPUID_DCACHE_8K           0x00080000
#define CPUID_DCACHE_16K          0x00100000
#define CPUID_DCACHE_32K          0x00200000
#define CPUID_DCACHE_64K          0x00400000
#define CPUID_DCACHE_128K         0x00800000
#define CPUID_DCACHE_256K         0x01000000

#define CPUID_ICACHE_NONE         0x00000000
#define CPUID_ICACHE_1K           0x00000001
#define CPUID_ICACHE_2K           0x00000002
#define CPUID_ICACHE_4K           0x00000004
#define CPUID_ICACHE_8K           0x00000008
#define CPUID_ICACHE_16K          0x00000010
#define CPUID_ICACHE_32K          0x00000020
#define CPUID_ICACHE_64K          0x00000040
#define CPUID_ICACHE_128K         0x00000080
#define CPUID_ICACHE_256K         0x00000100

#define CPUID_DSRAM_NONE          0x00000000
#define CPUID_DSRAM_1K            0x00010000
#define CPUID_DSRAM_2K            0x00020000
#define CPUID_DSRAM_4K            0x00040000
#define CPUID_DSRAM_8K            0x00080000
#define CPUID_DSRAM_16K           0x00100000
#define CPUID_DSRAM_32K           0x00200000
#define CPUID_DSRAM_64K           0x00400000
#define CPUID_DSRAM_128K          0x00800000
#define CPUID_DSRAM_256K          0x01000000
#define CPUID_DSRAM_512K          0x02000000

#define CPUID_ISRAM_NONE          0x00000000
#define CPUID_ISRAM_1K            0x00000001
#define CPUID_ISRAM_2K            0x00000002
#define CPUID_ISRAM_4K            0x00000004
#define CPUID_ISRAM_8K            0x00000008
#define CPUID_ISRAM_16K           0x00000010
#define CPUID_ISRAM_32K           0x00000020
#define CPUID_ISRAM_64K           0x00000040
#define CPUID_ISRAM_128K          0x00000080
#define CPUID_ISRAM_256K          0x00000100
#define CPUID_ISRAM_512K          0x00000200

#endif /* __ASM_CSKY_CPU_H */

