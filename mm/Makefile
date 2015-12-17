#
# Makefile for the Linux/CSKY-specific parts of the memory manager.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

obj-y			:= cache.o init.o ioremap.o fault.o dma-mapping.o mmap.o 

obj-$(CONFIG_CPU_CSKYV1)  += cachev1.o
obj-$(CONFIG_CPU_CSKYV2)  += cachev2.o

obj-$(CONFIG_CPU_MMU_V2)  += l2cache.o

obj-$(CONFIG_CPU_MMU_V1)  += tlbv1.o
obj-$(CONFIG_CPU_MMU_V2)  += tlbv2.o

obj-$(CONFIG_MMU)	+= mmu.o

ifneq ($(CONFIG_MMU),y)
obj-y               += nommu.o
endif

obj-$(CONFIG_HIGHMEM) += highmem.o
