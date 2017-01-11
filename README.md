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

## Make

```sh
	$ cd linux
	$ make ARCH=csky CROSS_COMPILE=csky-linux- O=/tmp/kernel_build sc8925_defconfig uImage
```

## Run

```sh
	$ cp addons/gdbinit/gdbinit_sc8925 .gdbinit
	$ csky-linux-gdb /tmp/kernel_build/vmlinux
```

## Download csky-linux-gcc

	[Download csky linux gcc](https://pan.baidu.com/s/1hrVkBMO)

	[GerritHub](https://review.gerrithub.io/#/admin/projects/riscv/riscv-go)

	ck610: csky-linux-tools-x86_64-glibc-linux-4.9.2-20170111.tar.gz
	ck8xx: csky-abiv2-linux-tools-x86_64-glibc-linux-4.9.2-20170111.tar.gz

