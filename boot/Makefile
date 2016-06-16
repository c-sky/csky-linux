MKIMAGE := $(srctree)/scripts/mkuboot.sh

targets := Image uImage
extra-y += Image

quiet_cmd_uimage = UIMAGE  $@
	cmd_uimage = $(MKIMAGE) -A mips -O linux -T kernel \
		-C none -n 'Linux-$(KERNELRELEASE)' -a 0x90000000 \
		-e 0x90000000 \
		-d $< $@


$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

$(obj)/uImage: $(obj)/Image
	$(call if_changed,uimage)
	@echo 'Image: $@ is ready'

install:
	sh $(srctree)/$(src)/install.sh $(KERNELRELEASE) $(BOOTIMAGE) System.map "$(INSTALL_PATH)

subdir-     := compressed

