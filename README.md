# Linux/arch/csky

## Get codes

	$ curl -L https://cdn.kernel.org/pub/linux/kernel/v4.x/testing/linux-4.7-rc7.tar.xz | tar -xJ
	$ cd linux-4.7-rc7
	$ git init
	$ git remote add -t master origin https://github.com/c-sky/csky-linux.git
	$ git fetch
	$ git checkout -f -t origin/master
	$ git clone https://github.com/c-sky/addons-linux.git addons

	Ps: (These git urls is the mirrors of above.)
		git://git.coding.net/c-sky/csky-linux.git
		git://git.coding.net/c-sky/addons-linux.git

## Make

	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build gx6605s_defconfig uImage

## Run
	$ cp addons/gdbinit .gdbinit
	$ csky-linux-gdb /tmp/kernel_build/vmlinux

