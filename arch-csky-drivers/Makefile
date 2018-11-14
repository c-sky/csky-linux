obj-y += irq-csky-apb.o
obj-y += irq-csky-apb-intc.o
obj-y += timer-gx6605s.o
ifeq ($(CONFIG_CPU_CK860),y)
obj-y += irq-csky-mpintc.o
obj-y += csky_mptimer.o
endif

ifndef CONFIG_TIMER_OF
obj-y += timer-of.o
endif

ifneq ($(CONFIG_CPU_CK610),y)
obj-y += prfl-csky.o
endif

obj-y += qemu-exit.o
