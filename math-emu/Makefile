#
# Makefile for csky-specific math emulate.
#

obj-$(CONFIG_CSKY_FPE)		+= math-emu.o

ifneq ($(CONFIG_CPU_CSKYV1),y)
math-emu-y			+= softfloat.o fp810_inst_op.o fp810_op.o fp_op.o
else
math-emu-y			+= softfloat.o fp610_inst_op.o fp610_op.o fp_op.o
endif
