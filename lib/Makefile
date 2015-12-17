#
# Makefile for csky-specific library files..
#


lib-y  := ashldi3.o ashrdi3.o lshrdi3.o muldi3.o \
	 checksum.o memset.o usercopy.o delay.o

obj-$(CONFIG_CPU_CSKYV1) = misc_abiv1.o memcpy_abiv1.o
obj-$(CONFIG_CPU_CSKYV2) = misc_abiv2.o memcpy.o

# more
obj-y  += ucmpdi2.o
