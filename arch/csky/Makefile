OBJCOPYFLAGS	:=-O binary
GZFLAGS		:=-9

ifdef CONFIG_CPU_HAS_DSP
DSPEXT =e
endif

ifdef CONFIG_CPU_HAS_FPU
FPUEXT =f
endif

ifdef CONFIG_CPU_CK610
CPUTYPE	= -Wa,-mcpu=ck610$(DSPEXT)$(FPUEXT)
CSKYHAL	= v1
endif

ifdef CONFIG_CPU_CK810
CPUTYPE= -mcpu=ck810$(DSPEXT)$(FPUEXT)
CSKYHAL	= v2
endif

ifdef CONFIG_CPU_CK807
CPUTYPE= -mcpu=ck807$(DSPEXT)$(FPUEXT)
CSKYHAL	= v2
endif

KBUILD_CFLAGS +=	-ffreestanding \
			-fno-tree-dse \
			-pipe \
			-Wno-uninitialized \
			$(CPUTYPE)

ifeq ($(CONFIG_CSKY_DEBUG_INFO),y)
KBUILD_CFLAGS += -g
endif

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

head-y				:= arch/csky/kernel/head.o

core-y				+= arch/csky/kernel/
core-y				+= arch/csky/mm/
libs-y				+= arch/csky/lib/
core-y				+= arch/csky/hal/$(CSKYHAL)/

ifdef CONFIG_CSKY_BUILTIN_DTB
core-y				+= arch/csky/boot/dts/
endif

drivers-$(CONFIG_OPROFILE)	+= arch/csky/oprofile/
drivers-$(CONFIG_CSKY_ADDONS)	+= addons/

all: zImage

boot	:= arch/csky/boot

dtbs: scripts
	$(Q)$(MAKE) $(build)=$(boot)/dts

%.dtb %.dtb.S %.dtb.o: scripts
	$(Q)$(MAKE) $(build)=$(boot)/dts $(boot)/dts/$@

zImage Image uImage: vmlinux dtbs
	$(Q)$(MAKE) $(build)=$(boot) $(boot)/$@

archmrproper:

archclean:
	$(Q)$(MAKE) $(clean)=$(boot)
	rm -rf arch/csky/include/generated

define archhelp
  echo  '* zImage       - Compressed kernel image (arch/$(ARCH)/boot/zImage)'
  echo  '  Image        - Uncompressed kernel image (arch/$(ARCH)/boot/Image)'
  echo  '  uImage       - U-Boot wrapped zImage'
endef

