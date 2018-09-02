#!/bin/bash
find . -name "*.c" -print | xargs csky_upstream/rm_kernel_version.py
find . -name "*.h" -print | xargs csky_upstream/rm_kernel_version.py
find . -name "Makefile" -print | xargs csky_upstream/rm_kernel_version.py
rm arch/csky/kernel/libgcc_ksyms.c
rm arch/csky/include/asm/cputime.h
