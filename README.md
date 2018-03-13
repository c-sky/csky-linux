# C-SKY Linux Port

* Directory arch/csky is the C-SKY CPU Linux Port.
* Directory arch/arch-csky-drivers is the drivers of some intc&timer.

# How to use
* Copy the arch/csky to the linux/arch directory. It support linux-4.9/4.14/4.15/4.16.
```sh
    cp -raf $(CSKY_ARCH_DIR)/arch/csky $(LINUX_DIR)/arch/
    cp -raf $(CSKY_ARCH_DIR)/arch-csky-drivers $(LINUX_DIR)/
    awk '/:= drivers/{print $$0,"arch-csky-drivers/";next}{print $$0}' \
        $(LINUX_DIR)/Makefile 1<>$(LINUX_DIR)/Makefile
```
  ref:https://gitlab.com/c-sky/buildroot/blob/master/linux/linux-ext-csky-arch.mk

* You can use buildroot to quick start with simple steps:

```sh
    $ git clone https://gitlab.com/c-sky/buildroot.git
    $ cd buildroot
    $ make qemu_csky_ck810_uclibc_bt_defconfig
    $ make
```

