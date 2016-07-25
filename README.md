# Linux/arch/csky

## Get codes

	$ git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
	$ git clone https://github.com/c-sky/csky-linux.git
		(git://git.coding.net/c-sky/csky-linux.git)
	$ git clone https://github.com/c-sky/addons-linux.git
		(git://git.coding.net/c-sky/addons-linux.git)

## Prepare linux kernel source-tree
	$ mkdir linux/arch/csky
	$ mkdir linux/addons
	$ unionfs-fuse csky-linux/arch/csky=RW linux/arch/csky
	$ unionfs-fuse addons-linux=RW linux/addons

## Make
	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build gx6605s_defconfig uImage

## Run
	$ cp addons/gdbinit .gdbinit
	$ csky-linux-gdb /tmp/kernel_build/vmlinux

