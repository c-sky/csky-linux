# Linux/arch/csky

## Get codes

	$ wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.8.4.tar.xz
	$ git clone -b https://github.com/c-sky/csky-linux.git
	$ git clone -b https://github.com/c-sky/addons-linux.git

## Prepare linux kernel source-tree

	$ tar xf linux-4.8.4.tar.xz
	$ mv linux-4.8.4 linux
	$ cp -raf csky-linux/arch/csky	linux/arch
	$ cp -raf addons-linux		linux/addons

## Make

	$ cd linux
	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build gx6605s_defconfig uImage

## Run

	$ cp addons/gdbinit .gdbinit
	$ csky-linux-gdb /tmp/kernel_build/vmlinux

