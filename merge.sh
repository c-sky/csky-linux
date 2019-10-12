#!/bin/sh
echo "Merge $1"

cp -raf ./arch/csky $1/arch/

cp -raf ./drivers $1/arch-csky-drivers
awk '/:= drivers/{print $$0,"arch-csky-drivers/";next}{print $$0}' $1/Makefile 1<>$1/Makefile

mkdir -p $1/tools/arch/csky/include/uapi/asm/
cp $1/tools/arch/arm/include/uapi/asm/mman.h $1/tools/arch/csky/include/uapi/asm/mman.h

cat ./patch/*.patch | patch -g0 -p1 -E -d "$1" -t -N -s
