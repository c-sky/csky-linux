/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2022 Intel Corporation
 */

#ifndef _INTEL_GSC_UC_H_
#define _INTEL_GSC_UC_H_

#include "intel_uc_fw.h"

struct i915_vma;
struct intel_context;
struct i915_gsc_proxy_component;

struct intel_gsc_uc {
	/* Generic uC firmware management */
	struct intel_uc_fw fw;

	/* GSC-specific additions */
	struct i915_vma *local; /* private memory for GSC usage */
	struct intel_context *ce; /* for submission to GSC FW via GSC engine */

	/* for delayed load and proxy handling */
	struct workqueue_struct *wq;
	struct work_struct work;
	u32 gsc_work_actions; /* protected by gt->irq_lock */
#define GSC_ACTION_FW_LOAD BIT(0)
#define GSC_ACTION_SW_PROXY BIT(1)

	struct {
		struct i915_gsc_proxy_component *component;
		bool component_added;
		struct i915_vma *vma;
		void *to_gsc;
		void *to_csme;
		struct mutex mutex; /* protects the tee channel binding */
	} proxy;
};

void intel_gsc_uc_init_early(struct intel_gsc_uc *gsc);
int intel_gsc_uc_init(struct intel_gsc_uc *gsc);
void intel_gsc_uc_fini(struct intel_gsc_uc *gsc);
void intel_gsc_uc_suspend(struct intel_gsc_uc *gsc);
void intel_gsc_uc_resume(struct intel_gsc_uc *gsc);
void intel_gsc_uc_flush_work(struct intel_gsc_uc *gsc);
void intel_gsc_uc_load_start(struct intel_gsc_uc *gsc);

static inline bool intel_gsc_uc_is_supported(struct intel_gsc_uc *gsc)
{
	return intel_uc_fw_is_supported(&gsc->fw);
}

static inline bool intel_gsc_uc_is_wanted(struct intel_gsc_uc *gsc)
{
	return intel_uc_fw_is_enabled(&gsc->fw);
}

static inline bool intel_gsc_uc_is_used(struct intel_gsc_uc *gsc)
{
	GEM_BUG_ON(__intel_uc_fw_status(&gsc->fw) == INTEL_UC_FIRMWARE_SELECTED);
	return intel_uc_fw_is_available(&gsc->fw);
}

#endif
