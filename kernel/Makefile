#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

extra-y := head$(MMUEXT).o
extra-y += vmlinux.lds


obj-y := entry.o signal.o traps.o alignment.o ints.o ptrace.o \
	  sys_csky.o time.o vdso.o 
obj-y += setup.o csky_ksyms.o process.o cpu-probe.o

obj-$(CONFIG_MODULES)       += module.o
obj-$(CONFIG_CPU_USE_FIQ)   += fiq.o


