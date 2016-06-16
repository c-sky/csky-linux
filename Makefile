OBJCOPYFLAGS	:=-O binary
GZFLAGS		:=-9

ifdef CONFIG_CPU_HAS_DSP
DSPEXT =e
endif

ifdef CONFIG_CPU_HAS_FPU
FPUEXT =f
endif

ifdef CONFIG_CPU_CSKYV1
CPUTYPE	= -Wa,-mcpu=ck610$(DSPEXT)$(FPUEXT)
CSKYHAL	= v1
else
CPUTYPE= -mcpu=ck810$(DSPEXT)$(FPUEXT)
CSKYHAL	= v2
endif

KBUILD_CFLAGS +=	-fsigned-char -fno-builtin-memcpy \
			-I$(INCGCC) -fno-tree-dse \
			-pipe -DNO_FPU -D__ELF__ -DMAGIC_ROM_PTR \
			-D__linux__ -DNO_TEXT_SECTIONS $(CPUTYPE) 

machine-y	:= gx3xxx

haldirs := $(patsubst %,arch/csky/hal/%/,$(CSKYHAL)) 

KBUILD_CFLAGS += $(patsubst %,-I$(srctree)/%inc,$(haldirs))

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
KBUILD_CPPFLAGS += -mbig-endian
LDFLAGS += -EB
else
KBUILD_CPPFLAGS += -mlittle-endian
LDFLAGS += -EL
endif

KBUILD_AFLAGS += $(KBUILD_CFLAGS)

head-y		:= arch/csky/kernel/head.o

ifdef CONFIG_CPU_CSKYV1
core-y		+= arch/csky/hal/v1/
endif
ifdef CONFIG_CPU_CSKYV2
core-y		+= arch/csky/hal/v2/
endif

core-y		+= arch/csky/kernel/
core-y		+= arch/csky/mm/

libs-y		+= arch/csky/lib/ 

drivers-$(CONFIG_OPROFILE)	+= arch/csky/oprofile/

ifdef CONFIG_CSKY_EXT
core-y		+= addons/
endif

CLEAN_FILES += \
    arch/$(ARCH)/kernel/asm-offsets.s

all: zImage

boot := arch/csky/boot

zImage Image bootpImage uImage: vmlinux
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

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

