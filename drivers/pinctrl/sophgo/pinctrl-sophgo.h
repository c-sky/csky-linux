#ifndef __mango_PINCTRL_CORE_H__
#define __mango_PINCTRL_CORE_H__

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include "../pinctrl-utils.h"
#include "../core.h"

enum FUNC_MODE {
	FUNC_MODE0,
	FUNC_MODE1,
	FUNC_MODE2,
	FUNC_MODE3,
	FUNC_MASK,
};

struct mango_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctl;
	u32 top_pinctl_offset;
	struct regmap *syscon_pinctl;
	void *data;
};

struct mango_group {
	const char *name;
	const unsigned int *pins;
	const unsigned int num_pins;
	int cur_func_idx;
	struct mango_pmx_func *funcs;
};

struct mango_pmx_func {
	const char *name;
	const char * const *groups;
	unsigned int num_groups;
	enum FUNC_MODE mode;
};

struct mango_soc_pinmux_info {
	const char name[16];
	const char name_a[16];
	const char name_r[16];
	struct pinctrl_state *pinctrl_a;
	struct pinctrl_state *pinctrl_r;
	const unsigned int def_state; /* default state */
	int (*set)(struct device *dev, unsigned int data);
};

struct mango_soc_pinctrl_data {
	const struct pinctrl_pin_desc *pins;
	unsigned int npins;
	struct mango_group *groups;
	int groups_count;
	const struct mango_pmx_func *functions;
	int functions_count;
	struct class *p_class;
};

int sophgo_pinctrl_probe(struct platform_device *pdev);
int mango_pmux_probe(struct platform_device *pdev);

ssize_t pinmux_show(struct device *dev,
			struct device_attribute *attr, char *buf);

ssize_t pinmux_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t size);
#endif
