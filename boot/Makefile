#
# arch/csky/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2020 Hujunshan(junshan_hu@c-sky.com)
#

MKIMAGE := $(srctree)/scripts/mkuboot.sh

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/Makefile.boot
endif

export IZTEXTADDR OPERADDR

targets := Image zImage uImage
extra-y += Image zImage

quiet_cmd_uimage = UIMAGE  $@
	cmd_uimage = $(MKIMAGE) -A mips -O linux -T kernel \
		-C none -n 'Linux-$(KERNELRELEASE)' -a 0x90000000 \
		-e 0x90000000 \
		-d $< $@


$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

$(obj)/compressed/vmlinux: $(obj)/Image FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

$(obj)/zImage:  $(obj)/compressed/vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

$(obj)/uImage: $(obj)/zImage
	$(call if_changed,uimage)
	@echo 'Image: $@ is ready'

install:
	sh $(srctree)/$(src)/install.sh $(KERNELRELEASE) $(BOOTIMAGE) System.map "$(INSTALL_PATH)

subdir-     := compressed

