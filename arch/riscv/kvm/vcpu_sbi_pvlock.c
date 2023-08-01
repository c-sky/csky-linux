// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c), 2023 Alibaba Cloud
 *
 * Authors:
 *     Guo Ren <guoren@linux.alibaba.com>
 */

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kvm_host.h>
#include <asm/sbi.h>
#include <asm/kvm_vcpu_sbi.h>

static int kvm_sbi_ext_pvlock_kick_cpu(struct kvm_vcpu *vcpu)
{
	struct kvm_cpu_context *cp = &vcpu->arch.guest_context;
	struct kvm *kvm = vcpu->kvm;
	struct kvm_vcpu *target;

	target = kvm_get_vcpu_by_id(kvm, cp->a0);
	if (!target)
		return SBI_ERR_INVALID_PARAM;

	kvm_vcpu_kick(target);

	if (READ_ONCE(target->ready))
		kvm_vcpu_yield_to(target);

	return SBI_SUCCESS;
}

static int kvm_sbi_ext_pvlock_handler(struct kvm_vcpu *vcpu, struct kvm_run *run,
				      struct kvm_vcpu_sbi_return *retdata)
{
	int ret = 0;
	struct kvm_cpu_context *cp = &vcpu->arch.guest_context;
	unsigned long funcid = cp->a6;

	switch (funcid) {
	case SBI_EXT_PVLOCK_KICK_CPU:
		ret = kvm_sbi_ext_pvlock_kick_cpu(vcpu);
		break;
	default:
		ret = SBI_ERR_NOT_SUPPORTED;
	}

	retdata->err_val = ret;

	return 0;
}

const struct kvm_vcpu_sbi_extension vcpu_sbi_ext_pvlock = {
	.extid_start = SBI_EXT_PVLOCK,
	.extid_end = SBI_EXT_PVLOCK,
	.handler = kvm_sbi_ext_pvlock_handler,
};
