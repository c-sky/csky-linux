obj-y				+= cacheflush.o
obj-$(CONFIG_CPU_HAS_FPU)	+= fpu.o
obj-y				+= memcmp.o
ifeq ($(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS), y)
obj-y				+= memcpy.o
obj-y				+= memmove.o
obj-y				+= memset.o
endif
obj-y				+= strcmp.o
obj-y				+= strcpy.o
obj-y				+= strlen.o
obj-y				+= strksyms.o
obj-$(CONFIG_FUNCTION_TRACER)	+= mcount.o
