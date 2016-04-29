#
# Makefile for the Linux/CSKY-specific parts of the memory manager.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

obj-y := cache.o init.o ioremap.o fault.o dma-mapping.o mmap.o mmu.o

obj-$(CONFIG_CPU_CSKYV1)	+= cachev1.o tlbv1.o
obj-$(CONFIG_CPU_CSKYV2)	+= cachev2.o tlbv2.o

obj-$(CONFIG_HIGHMEM)		+= highmem.o
