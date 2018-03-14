OBJCOPYFLAGS	:=-O binary
GZFLAGS		:=-9

ifdef CONFIG_CPU_HAS_FPU
FPUEXT =f
endif

ifdef CONFIG_CPU_CK610
CPUTYPE	= ck610
CSKYABI	= abiv1
endif

ifdef CONFIG_CPU_CK810
CPUTYPE = ck810$(FPUEXT)
CSKYABI	= abiv2
endif

ifdef CONFIG_CPU_CK807
CPUTYPE = ck807$(FPUEXT)
CSKYABI	= abiv2
endif

ifdef CONFIG_CPU_CK860
CPUTYPE = ck860$(FPUEXT)
CSKYABI	= abiv2
endif

ifeq ($(VERSION)_$(PATCHLEVEL), 4_9)
COMPAT_KERNEL_4_9 = -DCOMPAT_KERNEL_4_9
endif

KBUILD_CFLAGS +=	-ffreestanding \
			-fno-tree-dse \
			-pipe \
			-Wno-uninitialized \
			$(COMPAT_KERNEL_4_9) \
			-mcpu=$(CPUTYPE) \
			-DCSKYCPU_DEF_NAME=\"$(CPUTYPE)\"

ifeq ($(CONFIG_CSKY_DEBUG_INFO),y)
KBUILD_CFLAGS += -g
endif

abidirs := $(patsubst %,arch/csky/%/,$(CSKYABI))

KBUILD_CFLAGS += $(patsubst %,-I$(srctree)/%inc,$(abidirs))

KBUILD_CPPFLAGS += -mlittle-endian
LDFLAGS += -EL

KBUILD_AFLAGS += $(KBUILD_CFLAGS)

head-y := arch/csky/kernel/head.o

core-y += arch/csky/kernel/
core-y += arch/csky/mm/
core-y += arch/csky/$(CSKYABI)/

libs-y += arch/csky/lib/ \
	$(shell $(CC) $(KBUILD_CFLAGS) $(KCFLAGS) -print-libgcc-file-name)

ifdef CONFIG_CSKY_BUILTIN_DTB
core-y				+= arch/csky/boot/dts/
endif

drivers-$(CONFIG_OPROFILE)	+= arch/csky/oprofile/

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

