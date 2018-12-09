extra-y := head.o vmlinux.lds

obj-y += entry.o atomic.o signal.o traps.o irq.o time.o vdso.o \
	 power.o syscall.o platform.o syscall_table.o setup.o \
	 process.o cpu-probe.o ptrace.o dumpstack.o

obj-$(CONFIG_MODULES)			+= module.o
obj-$(CONFIG_SMP)			+= smp.o
ifndef CONFIG_CPU_CK610
obj-$(CONFIG_PERF_EVENTS)		+= perf_event.o
endif
obj-y					+= libgcc_ksyms.o
obj-$(CONFIG_STACKTRACE)		+= stacktrace.o
