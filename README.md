# Linux/arch/csky

## Get codes

	$ curl -L https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.7.tar.xz | tar -xJ
	$ git clone https://github.com/c-sky/csky-linux.git
		(git://git.coding.net/c-sky/csky-linux.git)
	$ git clone https://github.com/c-sky/addons-linux.git
		(git://git.coding.net/c-sky/addons-linux.git)

## Prepare linux kernel source-tree
	$ mkdir linux-4.7/arch/csky
	$ mkdir linux-4.7/addons
	$ unionfs-fuse csky-linux/arch/csky=RW linux-4.7/arch/csky
	$ unionfs-fuse addons-linux=RW linux-4.7/addons

## Make
	$ cd linux-4.7
	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build gx6605s_defconfig uImage

## Run
	$ cp addons/gdbinit .gdbinit
	$ csky-linux-gdb /tmp/kernel_build/vmlinux

