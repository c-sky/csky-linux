#
# linux/arch/csky/boot/compressed/Makefile
#
# create a compressed vmlinuz image from the original vmlinux
#
OBJS=misc.o decompress.o
HEAD=head.o

targets       := vmlinux vmlinux.lds piggy.gz piggy.o \
		 head.o misc.o decompress.o $(OBJS)
SEDFLAGS        = s/TEXT_START/$(ZTEXTADDR)/;s/BSS_START/$(ZBSSADDR)/

suffix_$(CONFIG_KERNEL_GZIP) = gzip
suffix_$(CONFIG_KERNEL_LZO)  = lzo
suffix_$(CONFIG_KERNEL_LZMA) = lzma

LDFLAGS_vmlinux := --defsym zreladdr=$(IZTEXTADDR) --defsym operaddr=$(OPERADDR)

LDFLAGS_vmlinux +=-Ttext $(OPERADDR) --no-undefined -X \
	$(shell $(CC) $(KBUILD_CFLAGS) --print-libgcc-file-name) -T

$(obj)/vmlinux: $(obj)/vmlinux.lds $(obj)/$(HEAD) $(obj)/piggy.o \
		$(addprefix $(obj)/, $(OBJS)) FORCE
	$(call if_changed,ld)
	@:


$(obj)/piggy.gz: $(obj)/../Image FORCE
	$(call if_changed,$(suffix_y))

$(obj)/piggy.o:  $(obj)/piggy.gz FORCE


$(obj)/vmlinux.lds: $(obj)/vmlinux.lds.in arch/csky/boot/Makefile .config
	@sed "$(SEDFLAGS)" < $< > $@

