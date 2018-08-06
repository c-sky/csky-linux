obj-y += nationalchip/
obj-y += qemu-exit.o
obj-y += irq-csky-apb-intc.o
ifeq ($(CONFIG_CPU_HAS_CACHEV2),y)
obj-y += irq-csky-mpintc.o
obj-y += timer-csky-v1.o
endif
ifneq ($(CONFIG_CPU_CK610),y)
obj-y += prfl-csky.o
endif
