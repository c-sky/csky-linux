#ifndef __ASM_CSKY_CPU_H
#define __ASM_CSKY_CPU_H

/* 
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

#define CPUID_VER_2               0x0002
#define CPUID_VER_3               0x0003

/* V2 */
#define	CPU_UNKNOWN               0x0000
#define CPU_CK600                 0x0002
#define CPU_CK800                 0x0003

#define CPUID_FPU_NONE            0x0000
#define CPUID_FPU_V2              0x0002

#define CPUID_V2_FAMILY_CK500     0x00000000
#define CPUID_V2_FAMILY_CK600     0x10000000

#define CPUID_V2_MODEL_MMU        0x00080000
#define CPUID_V2_MODEL_FPU        0x00100000
#define CPUID_V2_MODEL_AXI        0x00200000

/* V3 */
#define CPUID_V3_FAMILY_CK500     0x00000000
#define CPUID_V3_FAMILY_CK600     0x00400000
#define CPUID_V3_FAMILY_CK800     0x00800000

#define CPUID_V3_CLASS_CK810      0x00040000
#define CPUID_V3_CLASS_CK807      0x000c0000

#define CPUID_V3_MODEL_DM         0x00000100
#define CPUID_V3_MODEL_MMU        0x00000200
#define CPUID_V3_MODEL_FPU        0x00000400
#define CPUID_V3_MODEL_SPM        0x00000800
#define CPUID_V3_MODEL_MGU        0x00001000
#define CPUID_V3_MODEL_BCTM       0x00002000
#define CPUID_V3_MODEL_VDSP       0x00004000

#endif /* __ASM_CSKY_CPU_H */

