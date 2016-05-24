extra-y := head.o vmlinux.lds


obj-y := entry.o signal.o traps.o alignment.o ints.o ptrace.o
obj-y += sys_csky.o time.o vdso.o power.o syscall.o 
obj-y += setup.o csky_ksyms.o process.o cpu-probe.o

obj-$(CONFIG_MODULES) += module.o

