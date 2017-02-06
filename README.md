# Linux/arch/csky

## Get codes

```sh
	$ wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.9.2.tar.xz
	$ git clone https://github.com/c-sky/csky-linux.git
	$ cd csky-linux;git checkout 4.9.2-20170111;cd - # checkout the tag you want
	$ git clone https://github.com/c-sky/addons-linux.git
	$ cd addons-linux;git checkout 4.9.2-20170111;cd - # checkout the tag you want
```

## Prepare linux kernel source-tree

```sh
	$ tar xf linux-4.9.2.tar.xz
	$ mv linux-4.9.2 linux  # just rename
	$ cp -raf csky-linux/arch/csky	linux/arch
	$ cp -raf addons-linux		linux/addons
```
## Download Cross Compiler

	https://pan.baidu.com/s/1hrVkBMO

	ck8xx(abiv2): csky-abiv2-linux-tools-x86_64-glibc-linux-4.9.2-20170111.tar.gz
	ck610(abiv1): csky-linux-tools-x86_64-glibc-linux-4.9.2-20170111.tar.gz

	Setup your $PATH with csky-linux-* and csky-abiv2-linux-* binary :)

## Make

```sh
	$ cd linux
	# (for ck810)
	$ make ARCH=csky CROSS_COMPILE=csky-abiv2-linux- O=/tmp/kernel_build sc8925_defconfig uImage
	# or (for ck610)
	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build gx66xx_defconfig uImage
```

## Download Jtag-Server
	https://pan.baidu.com/s/1o7VEPbO

	install it and run it:
```sh
	$ DebugServerConsole -ddc -rstwait 1000 -port 1025
```

## Run

```sh
	# See the .gdbinit and it connect the port 1025 :)

	# (for ck810)
	$ cp addons/gdbinit/gdbinit_sc8925 .gdbinit
	$ csky-abiv2-linux-gdb /tmp/kernel_build/vmlinux

	# or (for ck610)
	$ cp addons/gdbinit/gdbinit_gx6605s .gdbinit
	$ csky-abiv2-linux-gdb /tmp/kernel_build/vmlinux
```

