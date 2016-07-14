# Linux/arch/csky

## Get codes

	$ curl -L https://cdn.kernel.org/pub/linux/kernel/v4.x/testing/linux-4.7-rc7.tar.xz | tar -xJ
	$ cd linux-4.7-rc7
	$ git init
	$ git remote add -t master origin https://git.coding.net/guoren83/csky.git
	$ git fetch
	$ git checkout -f -t origin/master
	$ git clone https://git.coding.net/guoren83/gxaddons.git addons

## Make

	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build gx6605s_defconfig uImage

## Run
	$ cp addons/gdbinit .gdbinit
	$ csky-linux-gdb /tmp/kernel_build/vmlinux

