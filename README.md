# Linux/arch/csky

## Introduction

C-SKY linux consisted of standard linux kernel source with arch/csky repo:

https://github.com/c-sky/csky-linux.git

## Setup linux kernel source with copy

```sh
    # Download the kernel source
	$ wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.9.22.tar.gz
	$ tar zxvf linux-4.9.22.tar.gz

    # Get code from arch/csky repo, and copy if to kernel source
	$ git clone https://github.com/c-sky/csky-linux.git
	$ cp csky-linux/arch/csky linux-4.9.22/arch/ -raf

    # Use an empty addons
	$ cd linux-4.9.22
	$ mkdir addons
	$ touch addons/Kconfig
	$ touch addons/Makefile
	$ touch addons/none.c
	$ echo "obj-y += none.o" > addons/Makefile

```
Now linux-4.9.22 is finished with arch of C-SKY implement.

## Setup linux kernel source with git merge

```sh
    # Download kernel source, and setup init repo
	$ wget https://cdn.kernel.org/pub/linux/kernel/v4.x/linux-4.9.22.tar.gz
	$ tar zxvf linux-4.9.22.tar.gz

    # Use an empty addons
	$ cd linux-4.9.22
	$ mkdir addons
	$ touch addons/Kconfig
	$ touch addons/Makefile
	$ touch addons/none.c
	$ echo "obj-y += none.o" > addons/Makefile

    # Setup local .git
	$ git init .
	$ git add .
	$ git commit -m "init"
 
    # Setup arch csky
	$ git remote add csky https://github.com/c-sky/csky-linux.git
	$ git pull csky master

    # Push to your own repo
	$ git remote add my_repo git://myrepo.git
	$ git push my_repo master
```
