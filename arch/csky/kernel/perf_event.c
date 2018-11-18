// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>

struct csky_pmu_t {
	struct pmu	pmu;
	unsigned int	config;
} csky_pmu;

#define cprgr(reg)				\
({						\
	unsigned int tmp;			\
	asm volatile("cprgr %0, "reg"\n"	\
		     :"=r"(tmp)			\
		     :				\
		     :"memory");		\
	tmp;					\
})

#define cpwcr(reg, val)		\
({				\
	asm volatile(		\
	"cpwcr %0, "reg"\n"	\
	:			\
	: "r"(val)		\
	: "memory");		\
})

/* read cycle counter */
static uint64_t csky_pmu_read_cc(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0x3>");
		lo  = cprgr("<0, 0x2>");
		hi  = cprgr("<0, 0x3>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read instruction counter */
static uint64_t csky_pmu_read_ic(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0x5>");
		lo  = cprgr("<0, 0x4>");
		hi  = cprgr("<0, 0x5>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read l1 icache access counter */
static uint64_t csky_pmu_read_icac(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0x7>");
		lo  = cprgr("<0, 0x6>");
		hi  = cprgr("<0, 0x7>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read l1 icache miss counter */
static uint64_t csky_pmu_read_icmc(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0x9>");
		lo  = cprgr("<0, 0x8>");
		hi  = cprgr("<0, 0x9>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read l1 dcache access counter */
static uint64_t csky_pmu_read_dcac(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0xb>");
		lo  = cprgr("<0, 0xa>");
		hi  = cprgr("<0, 0xb>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read l1 dcache miss counter */
static uint64_t csky_pmu_read_dcmc(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0xd>");
		lo  = cprgr("<0, 0xc>");
		hi  = cprgr("<0, 0xd>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read l2 cache acess counter */
static uint64_t csky_pmu_read_l2ac(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0xf>");
		lo  = cprgr("<0, 0xe>");
		hi  = cprgr("<0, 0xf>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

/* read l2 cache miss counter */
static uint64_t csky_pmu_read_l2mc(void)
{
	uint32_t lo, hi, tmp;
	uint64_t result;

	do {
		tmp = cprgr("<0, 0x11>");
		lo  = cprgr("<0, 0x10>");
		hi  = cprgr("<0, 0x11>");
	} while (hi != tmp);

	result = (uint64_t) (hi) << 32;
	result |= lo;

	return result;
}

#define HW_OP_UNSUPPORTED	0xffff
static const unsigned csky_pmu_hw_map[PERF_COUNT_HW_MAX] = {
	[PERF_COUNT_HW_CPU_CYCLES]		= (unsigned) csky_pmu_read_cc,
	[PERF_COUNT_HW_INSTRUCTIONS]		= (unsigned) csky_pmu_read_ic,
	[PERF_COUNT_HW_CACHE_REFERENCES]	= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_CACHE_MISSES]		= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_BRANCH_INSTRUCTIONS]	= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_BRANCH_MISSES]		= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_BUS_CYCLES]		= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_STALLED_CYCLES_FRONTEND]	= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_STALLED_CYCLES_BACKEND]	= HW_OP_UNSUPPORTED,
	[PERF_COUNT_HW_REF_CPU_CYCLES]		= HW_OP_UNSUPPORTED,
};

#define C(_x)			PERF_COUNT_HW_CACHE_##_x
#define CACHE_OP_UNSUPPORTED	0xffff
static const unsigned csky_pmu_cache_map[C(MAX)][C(OP_MAX)][C(RESULT_MAX)] = {
	[C(L1D)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= (unsigned) csky_pmu_read_dcac,
			[C(RESULT_MISS)]	= (unsigned) csky_pmu_read_dcmc,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
	[C(L1I)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= (unsigned) csky_pmu_read_icac,
			[C(RESULT_MISS)]	= (unsigned) csky_pmu_read_icmc,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
	[C(LL)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= (unsigned) csky_pmu_read_l2ac,
			[C(RESULT_MISS)]	= (unsigned) csky_pmu_read_l2mc,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
	[C(DTLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
	[C(ITLB)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
	[C(BPU)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
	[C(NODE)] = {
		[C(OP_READ)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_WRITE)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
		[C(OP_PREFETCH)] = {
			[C(RESULT_ACCESS)]	= CACHE_OP_UNSUPPORTED,
			[C(RESULT_MISS)]	= CACHE_OP_UNSUPPORTED,
		},
	},
};

static void csky_perf_event_update(struct perf_event *event,
				  struct hw_perf_event *hwc)
{
	uint64_t prev_raw_count = local64_read(&hwc->prev_count);
	uint64_t (*fn)(void) = (void *)hwc->idx;
	uint64_t new_raw_count = fn();
	int64_t delta = new_raw_count - prev_raw_count;

	/*
	 * We aren't afraid of hwc->prev_count changing beneath our feet
	 * because there's no way for us to re-enter this function anytime.
	 */
	local64_set(&hwc->prev_count, new_raw_count);
	local64_add(delta, &event->count);
	local64_sub(delta, &hwc->period_left);
}

static void csky_pmu_read(struct perf_event *event)
{
	csky_perf_event_update(event, &event->hw);
}

static int csky_pmu_cache_event(u64 config)
{
	unsigned int cache_type, cache_op, cache_result;

	cache_type	= (config >>  0) & 0xff;
	cache_op	= (config >>  8) & 0xff;
	cache_result	= (config >> 16) & 0xff;

	if (cache_type >= PERF_COUNT_HW_CACHE_MAX)
		return -EINVAL;
	if (cache_op >= PERF_COUNT_HW_CACHE_OP_MAX)
		return -EINVAL;
	if (cache_result >= PERF_COUNT_HW_CACHE_RESULT_MAX)
		return -EINVAL;

	return csky_pmu_cache_map[cache_type][cache_op][cache_result];
}

static int csky_pmu_event_init(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	int ret;

	if (event->attr.exclude_user)
		csky_pmu.config = 0x4;
	else if (event->attr.exclude_kernel)
		csky_pmu.config = 0x8;
	else
		csky_pmu.config = 0xc;

	switch (event->attr.type) {
	case PERF_TYPE_HARDWARE:
		if (event->attr.config >= PERF_COUNT_HW_MAX)
			return -ENOENT;
		ret = csky_pmu_hw_map[event->attr.config];
		if (ret == HW_OP_UNSUPPORTED)
			return -ENOENT;
		hwc->idx = ret;
		return 0;
	case PERF_TYPE_HW_CACHE:
		ret = csky_pmu_cache_event(event->attr.config);
		if (ret == CACHE_OP_UNSUPPORTED)
			return -ENOENT;
		hwc->idx = ret;
		return 0;
	default:
		return -ENOENT;
	}
}

/* starts all counters */
static void csky_pmu_enable(struct pmu *pmu)
{
}

/* stops all counters */
static void csky_pmu_disable(struct pmu *pmu)
{
	cpwcr("<0, 0x0>", 0);
}

static void csky_pmu_start(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	if (WARN_ON_ONCE(idx == -1))
		return;

	if (flags & PERF_EF_RELOAD)
		WARN_ON_ONCE(!(hwc->state & PERF_HES_UPTODATE));

	hwc->state = 0;

	cpwcr("<0, 0x0>", csky_pmu.config | 0x3);
}

static void csky_pmu_stop(struct perf_event *event, int flags)
{
	if (!(event->hw.state & PERF_HES_STOPPED)) {
		cpwcr("<0, 0x0>", 0);
		event->hw.state |= PERF_HES_STOPPED;
	}

	if ((flags & PERF_EF_UPDATE) &&
	    !(event->hw.state & PERF_HES_UPTODATE)) {
		csky_perf_event_update(event, &event->hw);
		event->hw.state |= PERF_HES_UPTODATE;
	}
}

static void csky_pmu_del(struct perf_event *event, int flags)
{
	csky_pmu_stop(event, PERF_EF_UPDATE);

	perf_event_update_userpage(event);
}

/* allocate hardware counter and optionally start counting */
static int csky_pmu_add(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;

	cpwcr("<0, 0x0>", 0xc0000000);
	local64_set(&hwc->prev_count, 0);

	hwc->state = PERF_HES_UPTODATE | PERF_HES_STOPPED;
	if (flags & PERF_EF_START)
		csky_pmu_start(event, PERF_EF_RELOAD);

	perf_event_update_userpage(event);

	return 0;
}

static int csky_pmu_device_probe(struct platform_device *pdev)
{
	csky_pmu.pmu = (struct pmu) {
		.pmu_enable	= csky_pmu_enable,
		.pmu_disable	= csky_pmu_disable,
		.event_init	= csky_pmu_event_init,
		.add		= csky_pmu_add,
		.del		= csky_pmu_del,
		.start		= csky_pmu_start,
		.stop		= csky_pmu_stop,
		.read		= csky_pmu_read,
	};

	csky_pmu.pmu.capabilities |= PERF_PMU_CAP_NO_INTERRUPT;

	return perf_pmu_register(&csky_pmu.pmu, pdev->name, PERF_TYPE_RAW);
}

#ifdef CONFIG_OF
static const struct of_device_id csky_pmu_match[] = {
	{ .compatible = "csky,pmu-v1" },
	{},
};
MODULE_DEVICE_TABLE(of, csky_pmu_match);
#endif

static struct platform_driver csky_pmu_driver = {
	.driver	= {
		.name		= "csky-pmu",
		.of_match_table = of_match_ptr(csky_pmu_match),
	},
	.probe		= csky_pmu_device_probe,
};
module_platform_driver(csky_pmu_driver);
