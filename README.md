# Linux/arch/csky

## Get codes

	$ wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.8.15.tar.xz
	$ git clone https://github.com/c-sky/csky-linux.git
	$ git clone https://github.com/c-sky/addons-linux.git

## Prepare linux kernel source-tree

	$ tar xf linux-4.8.15.tar.xz
	$ mv linux-4.8.15 linux
	$ cp -raf csky-linux/arch/csky	linux/arch
	$ cp -raf addons-linux		linux/addons

## Make

	$ cd linux
	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build_ck810 ck810_defconfig uImage

## Run

	$ cp addons/gdbinit/gdbinit_sc8925 .gdbinit
	$ csky-linux-gdb /tmp/kernel_build_ck810/vmlinux

