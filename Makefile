#
# csky/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" for cleaning up for this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# (C) Copyright 2004, Li Chunqiang (chunqiang_li@c-sky.com)
# (C) Copyright 2009, Hu Junshan (junshan_hu@c-sky.com)
# (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
#

#
# To select default config enrty. If you want to use default config, you can 
#   enter "make defconfig".
#
KBUILD_DEFCONFIG := ck6408evb_defconfig

OBJCOPYFLAGS    :=-O binary
GZFLAGS     :=-9

ifdef CONFIG_CPU_CSKYV1
CPUISA=ck610
else
 ifdef CONFIG_MMU
  CPUISA=ck810
 else
  CPUISA=ck803
 endif
endif

ifdef CONFIG_CPU_HAS_DSP
DSPEXT =e
endif

ifdef CONFIG_CPU_HAS_FPU
FPUEXT =f
endif

ifdef CONFIG_CPU_CSKYV1
CPUTYPE= -Wa,-mcpu=$(CPUISA)$(DSPEXT)$(FPUEXT)
else
CPUTYPE= -mcpu=$(CPUISA)$(DSPEXT)$(FPUEXT)
endif

ifeq ($(CONFIG_MMU),)
MMUEXT          := -nommu
endif

KBUILD_CFLAGS += -fsigned-char -g -fno-builtin-memcpy \
                 $(KBUILD_CFLAGS) -I$(INCGCC) \
                -pipe -DNO_FPU -D__ELF__ -DMAGIC_ROM_PTR \
                -D__linux__ -DNO_TEXT_SECTIONS $(CPUTYPE) 

machine-$(CONFIG_GX3201)	:= gx3xxx
machine-$(CONFIG_GX3211)	:= gx3xxx
machine-$(CONFIG_CK6408EVB) := ck6408evb
machine-$(CONFIG_CKM5208)   := ckm5208
machine-$(CONFIG_DIOSCURI)  := dioscuri

# The first directory contains additional information for the boot setup code
ifneq ($(machine-y),)
MACHINE  := arch/csky/$(word 1,$(machine-y))/
else
MACHINE  :=
endif

machdirs := $(patsubst %,arch/csky/%/,$(machine-y))

ifeq ($(KBUILD_SRC),)
KBUILD_CFLAGS += $(patsubst %,-I%include,$(machdirs))
else
KBUILD_CFLAGS += $(patsubst %,-I$(srctree)/%include,$(machdirs))
endif

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
KBUILD_CPPFLAGS += -mbig-endian
LDFLAGS += -EB
else
KBUILD_CPPFLAGS += -mlittle-endian
LDFLAGS += -EL
endif

KBUILD_AFLAGS += $(KBUILD_CFLAGS)

head-y := arch/csky/kernel/head$(MMUEXT).o
export  MMUEXT

core-y                      += arch/csky/kernel/    arch/csky/mm/
core-y                      += $(machdirs)
libs-y                      += arch/csky/lib/ 
core-$(CONFIG_CSKY_FPE)     += arch/csky/math-emu/

CLEAN_FILES += \
    arch/$(ARCH)/kernel/asm-offsets.s \

all: zImage

boot := arch/csky/boot

zImage Image bootpImage uImage: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) MACHINE=$(MACHINE) $(boot)/$@

bootstrap:
	$(Q)$(MAKEBOOT) bootstrap

install:
	$(Q)$(MAKE) $(build)=$(boot) BOOTIMAGE=$(KBUILD_IMAGE) install

archmrproper:

archclean:
	$(Q)$(MAKE) $(clean)=$(boot)
	rm -rf arch/csky/include/generated

define archhelp
  echo  '* zImage       - Compressed kernel image (arch/$(ARCH)/boot/zImage)'
  echo  '  Image        - Uncompressed kernel image (arch/$(ARCH)/boot/Image)'
  echo  '  uImage       - U-Boot wrapped zImage'
endef

