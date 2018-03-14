extra-y := head.o vmlinux.lds

obj-y += entry.o atomic.o signal.o traps.o alignment.o irq.o \
	 time.o vdso.o power.o syscall.o platform.o syscall_table.o \
	 setup.o csky_ksyms.o process.o cpu-probe.o ptrace.o \
	 dumpstack.o

obj-$(CONFIG_MODULES) += module.o

