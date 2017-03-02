extra-y := head.o vmlinux.lds


obj-y := entry.o signal.o traps.o alignment.o irq.o ptrace.o
obj-y += time.o vdso.o power.o syscall.o platform.o syscall_table.o
obj-y += setup.o csky_ksyms.o process.o cpu-probe.o

obj-$(CONFIG_MODULES)		 += module.o

obj-$(CONFIG_NATIONALCHIP_TIMER) += timer-nationalchip.o

obj-$(CONFIG_NATIONALCHIP_IRQ)	 += irq-nationalchip.o
obj-$(CONFIG_CSKY_IRQ)		 += irq-csky.o

