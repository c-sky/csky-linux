# SPDX-License-Identifier: GPL-2.0-only

# Include the generic Makefile to check the built vdso.
include $(srctree)/lib/vdso/Makefile

VDSO_CC := $(CC)
VDSO_LD := $(LD)

# Disable profiling and instrumentation for VDSO code
GCOV_PROFILE := n
KCOV_INSTRUMENT := n
KASAN_SANITIZE := n
UBSAN_SANITIZE := n

ccflags-y := -fno-stack-protector
ccflags-y += -DDISABLE_BRANCH_PROFILING

CPPFLAGS_vdso.lds += -P -C -U$(ARCH)
ifneq ($(filter vgettimeofday, $(vdso-cc-syms)),)
	CPPFLAGS_vdso.lds += -DHAS_VGETTIMEOFDAY
endif

# strip rule for the .so file
$(obj)/%.so: OBJCOPYFLAGS := -S
$(obj)/%.so: $(obj)/%.so.dbg FORCE
	$(call if_changed,objcopy)

# install commands for the unstripped file
quiet_cmd_vdso_install = INSTALL $@
      cmd_vdso_install = cp $(obj)/$@.dbg $(MODLIB)/vdso/$@

# Symbols present in the vdso
ifdef CONFIG_VDSO64
vdso64-as-syms  = rt_sigreturn
vdso64-as-syms += getcpu
vdso64-as-syms += flush_icache
vdso64-as-syms += sys_hwprobe

vdso64-cc-syms  = vgettimeofday
vdso64-cc-syms += hwprobe

obj-as-vdso64  = $(patsubst %, %-64.o, $(vdso64-as-syms)) note-64.o
obj-as-vdso64 := $(addprefix $(obj)/, $(obj-as-vdso64))

obj-cc-vdso64  = $(patsubst %, %-64.o, $(vdso64-cc-syms))
obj-cc-vdso64 := $(addprefix $(obj)/, $(obj-cc-vdso64))

targets += $(obj-as-vdso64) $(obj-cc-vdso64) vdso64.so vdso64.so.dbg vdso64.lds

$(obj)/vdso64.so.dbg: $(obj)/vdso.lds $(obj-as-vdso64) $(obj-cc-vdso64) FORCE
	$(call if_changed,vdso64ld)
LDFLAGS_vdso64.so.dbg = -shared -S -soname=linux-vdso64.so.1 \
	--build-id=sha1 --hash-style=both --eh-frame-hdr
# The DSO images are built using a special linker script
# Make sure only to export the intended __vdso_xxx symbol offsets.
quiet_cmd_vdso64ld = VDSO64LD  $@
      cmd_vdso64ld = $(VDSO_LD) $(ld_flags) $(VDSO64_LD_FLAGS) -T $(filter-out FORCE,$^) -o $@.tmp && \
                   $(OBJCOPY) $(patsubst %, -G __vdso_%, $(vdso64-as-syms) $(vdso64-cc-syms)) $@.tmp $@ && \
                   rm $@.tmp

# actual build commands
quiet_cmd_vdso64as = VDSO64AS $@
      cmd_vdso64as = $(VDSO_CC) $(a_flags) $(VDSO64_CC_FLAGS) -c -o $@ $<
quiet_cmd_vdso64cc = VDSO64CC $@
      cmd_vdso64cc = $(VDSO_CC) $(c_flags) $(VDSO64_CC_FLAGS) -c -o $@ $<

# Force dependency
$(obj)/vdso64.o: $(obj)/vdso64.so

$(obj-as-vdso64): %-64.o: %.S FORCE
	$(call if_changed_dep,vdso64as)
$(obj-cc-vdso64): %-64.o: %.c FORCE
	$(call if_changed_dep,vdso64cc)

CFLAGS_vgettimeofday-64.o += -fPIC -include $(c-gettimeofday-y)
# Disable -pg to prevent insert call site
CFLAGS_REMOVE_vgettimeofday-64.o = $(CC_FLAGS_FTRACE)

CFLAGS_hwprobe-64.o += -fPIC

# Generate VDSO offsets using helper script
gen-vdso64sym := $(srctree)/$(src)/gen_vdso64_offsets.sh
quiet_cmd_vdso64sym = VDSO64SYM $@
	cmd_vdso64sym = $(NM) $< | $(gen-vdso64sym) | LC_ALL=C sort > $@

include/generated/vdso64-offsets.h: $(obj)/vdso64.so.dbg $(obj)/vdso64.so FORCE
	$(call if_changed,vdso64sym)

vdso64.so: $(obj)/vdso64.so.dbg
	@mkdir -p $(MODLIB)/vdso
	$(call cmd,vdso_install)

vdso64_install: vdso64.so
endif

ifdef CONFIG_VDSO32
vdso32-as-syms  = rt_sigreturn
vdso32-as-syms += getcpu
vdso32-as-syms += flush_icache
vdso32-as-syms += sys_hwprobe

vdso32-cc-syms += hwprobe

VDSO32_CC_FLAGS := -march=rv32g -mabi=ilp32
VDSO32_LD_FLAGS := -melf32lriscv

obj-as-vdso32  = $(patsubst %, %-32.o, $(vdso32-as-syms)) note-32.o
obj-as-vdso32 := $(addprefix $(obj)/, $(obj-as-vdso32))

obj-cc-vdso32  = $(patsubst %, %-32.o, $(vdso32-cc-syms))
obj-cc-vdso32 := $(addprefix $(obj)/, $(obj-cc-vdso32))

targets += $(obj-as-vdso32) $(obj-cc-vdso32) vdso32.so vdso32.so.dbg vdso32.lds

$(obj)/vdso32.so.dbg: $(obj)/vdso.lds $(obj-as-vdso32) $(obj-cc-vdso32) FORCE
	$(call if_changed,vdso32ld)
LDFLAGS_vdso32.so.dbg = -shared -S -soname=linux-vdso32.so.1 \
	--build-id=sha1 --hash-style=both --eh-frame-hdr

# The DSO images are built using a special linker script
# Make sure only to export the intended __vdso_xxx symbol offsets.
quiet_cmd_vdso32ld = VDSO32LD  $@
      cmd_vdso32ld = $(VDSO_LD) $(ld_flags) $(VDSO32_LD_FLAGS) -T $(filter-out FORCE,$^) -o $@.tmp && \
                   $(OBJCOPY) $(patsubst %, -G __vdso_%, $(vdso32-as-syms) $(vdso32-cc-syms)) $@.tmp $@ && \
                   rm $@.tmp

# actual build commands
quiet_cmd_vdso32as = VDSO32AS $@
      cmd_vdso32as = $(VDSO_CC) $(a_flags) $(VDSO32_CC_FLAGS) -c -o $@ $<
quiet_cmd_vdso32cc = VDSO32CC $@
      cmd_vdso32cc = $(VDSO_CC) $(c_flags) $(VDSO32_CC_FLAGS) -c -o $@ $<

# Force dependency
$(obj)/vdso32.o: $(obj)/vdso32.so

$(obj-as-vdso32): %-32.o: %.S FORCE
	$(call if_changed_dep,vdso32as)
$(obj-cc-vdso32): %-32.o: %.c FORCE
	$(call if_changed_dep,vdso32cc)

CFLAGS_hwprobe-32.o += -fPIC

# Generate VDSO offsets using helper script
gen-vdso32sym := $(srctree)/$(src)/gen_vdso32_offsets.sh
quiet_cmd_vdso32sym = VDSO32SYM $@
	cmd_vdso32sym = $(NM) $< | $(gen-vdso32sym) | LC_ALL=C sort > $@

include/generated/vdso32-offsets.h: $(obj)/vdso32.so.dbg $(obj)/vdso32.so FORCE
	$(call if_changed,vdso32sym)

vdso32.so: $(obj)/vdso32.so.dbg
	@mkdir -p $(MODLIB)/vdso
	$(call cmd,vdso_install)

vdso32_install: vdso32.so
endif
