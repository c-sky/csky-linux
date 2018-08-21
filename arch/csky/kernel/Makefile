extra-y := head.o vmlinux.lds

obj-y += entry.o atomic.o signal.o traps.o irq.o time.o vdso.o \
	 power.o syscall.o platform.o syscall_table.o setup.o \
	 process.o cpu-probe.o ptrace.o dumpstack.o

obj-$(CONFIG_MODULES)			+= module.o
obj-$(CONFIG_SMP)			+= smp.o

ifeq ($(VERSION)_$(PATCHLEVEL), 4_9)
obj-y					+= cskyksyms.o
endif

ifeq ($(VERSION)_$(PATCHLEVEL), 4_14)
obj-y					+= cskyksyms.o
endif
