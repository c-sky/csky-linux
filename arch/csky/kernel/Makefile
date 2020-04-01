extra-y := head.o vmlinux.lds

obj-y += entry.o atomic.o signal.o traps.o irq.o time.o vdso.o \
	 power.o syscall.o platform.o syscall_table.o setup.o \
	 process.o cpu-probe.o ptrace.o dumpstack.o
obj-y += probes/

obj-$(CONFIG_MODULES)			+= module.o
obj-$(CONFIG_SMP)			+= smp.o
obj-$(CONFIG_CSKY_PMU_V1)		+= perf_event.o
obj-y					+= libgcc_ksyms.o
obj-$(CONFIG_FUNCTION_TRACER)		+= ftrace.o
obj-$(CONFIG_STACKTRACE)		+= stacktrace.o
obj-$(CONFIG_PERF_EVENTS)		+= perf_callchain.o
obj-$(CONFIG_HAVE_PERF_REGS)            += perf_regs.o
obj-$(CONFIG_PCI)			+= pci.o

ifdef CONFIG_FUNCTION_TRACER
CFLAGS_REMOVE_ftrace.o = $(CC_FLAGS_FTRACE)
endif
