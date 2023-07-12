#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#include "pinctrl-sophgo.h"


static int mango_get_groups(struct pinctrl_dev *pctldev, unsigned int selector,
		const char * const **groups,
		unsigned int * const num_groups);

static struct mango_soc_pinctrl_data *get_pinmux_data(struct pinctrl_dev *pctldev)
{
	struct mango_pinctrl *mangopctrl = pinctrl_dev_get_drvdata(pctldev);

	return mangopctrl->data;
}

static int mango_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	return data->functions_count;
}

static const char *mango_get_fname(struct pinctrl_dev *pctldev,
		unsigned int selector)
{
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	return data->functions[selector].name;
}

static int mango_set_mux(struct pinctrl_dev *pctldev, unsigned int selector,
		unsigned int group)
{
	int p;
	unsigned int pidx;
	u32 offset, regval, mux_offset;
	struct mango_pinctrl *ctrl = pinctrl_dev_get_drvdata(pctldev);
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	data->groups[group].cur_func_idx = data->functions[selector].mode;
	for (p = 0; p < data->groups[group].num_pins; p++) {
		pidx = data->groups[group].pins[p];
		offset = (pidx >> 1) << 2;
		regmap_read(ctrl->syscon_pinctl,
			ctrl->top_pinctl_offset + offset, &regval);
		mux_offset = ((!((pidx + 1) & 1) << 4) + 4);

		regval = regval & ~(3 << mux_offset);
		regval |= data->functions[selector].mode << mux_offset;
		regmap_write(ctrl->syscon_pinctl,
			ctrl->top_pinctl_offset + offset, regval);
		regmap_read(ctrl->syscon_pinctl,
			ctrl->top_pinctl_offset + offset, &regval);
		dev_dbg(ctrl->dev, "%s : check new reg=0x%x val=0x%x\n",
			data->groups[group].name,
			ctrl->top_pinctl_offset + offset, regval);
	}

	return 0;
}

static const struct pinmux_ops mango_pinmux_ops = {
	.get_functions_count = mango_get_functions_count,
	.get_function_name = mango_get_fname,
	.get_function_groups = mango_get_groups,
	.set_mux = mango_set_mux,
	.strict = true,
};

static int mango_pinconf_cfg_get(struct pinctrl_dev *pctldev, unsigned int pin,
		unsigned long *config)
{
	return 0;
}

static int mango_pinconf_cfg_set(struct pinctrl_dev *pctldev, unsigned int pin,
		unsigned long *configs, unsigned int num_configs)
{
	return 0;
}

static int mango_pinconf_group_set(struct pinctrl_dev *pctldev,
		unsigned int selector, unsigned long *configs, unsigned int num_configs)
{
	return 0;
}

static const struct pinconf_ops mango_pinconf_ops = {
	.is_generic = true,
	.pin_config_get = mango_pinconf_cfg_get,
	.pin_config_set = mango_pinconf_cfg_set,
	.pin_config_group_set = mango_pinconf_group_set,
};

static int mango_get_groups(struct pinctrl_dev *pctldev, unsigned int selector,
		const char * const **groups,
		unsigned int * const num_groups)
{
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	*groups = data->functions[selector].groups;
	*num_groups = data->functions[selector].num_groups;

	return 0;
}

static int mango_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	return data->groups_count;
}

static const char *mango_get_group_name(struct pinctrl_dev *pctldev,
					   unsigned int selector)
{
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	return data->groups[selector].name;
}

static int mango_get_group_pins(struct pinctrl_dev *pctldev, unsigned int selector,
		const unsigned int **pins,
		unsigned int *num_pins)
{
	struct mango_soc_pinctrl_data *data = get_pinmux_data(pctldev);

	*pins = data->groups[selector].pins;
	*num_pins = data->groups[selector].num_pins;

	return 0;
}

static void mango_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s,
	unsigned int offset)
{
}

static const struct pinctrl_ops mango_pctrl_ops = {
	.get_groups_count = mango_get_groups_count,
	.get_group_name = mango_get_group_name,
	.get_group_pins = mango_get_group_pins,
	.pin_dbg_show = mango_pin_dbg_show,
	.dt_node_to_map = pinconf_generic_dt_node_to_map_all,
	.dt_free_map = pinctrl_utils_free_map,
};

static struct pinctrl_desc mango_desc = {
	.name = "mango_pinctrl",
	.pctlops = &mango_pctrl_ops,
	.pmxops = &mango_pinmux_ops,
	.confops = &mango_pinconf_ops,
	.owner = THIS_MODULE,
};

ssize_t pinmux_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mango_pinctrl *mangopctrl;
	int p, ret, group, selector = -1;
	struct mango_soc_pinctrl_data *data;

	mangopctrl = dev_get_drvdata(dev);
	data = (struct mango_soc_pinctrl_data *)mangopctrl->data;

	for (p = 0; p < data->functions_count; p++) {
		if (!strncmp(attr->attr.name, data->functions[p].name,
			 strlen(attr->attr.name))) {
			selector = p;
			break;
		}
	}
	if (selector < 0)
		return -ENXIO;

	group = selector/2;
	ret = snprintf(buf, 128, "%d\n", data->groups[group].cur_func_idx);
	if (ret <= 0 || ret > 128) {
		dev_err(dev, "snprintf failed %d\n", ret);
		return -EFAULT;
	}
	return ret;
}

ssize_t pinmux_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct mango_pinctrl *mangopctrl;
	int p, ret, group, selector = -1;
	unsigned long user_data;
	struct mango_soc_pinctrl_data *data;

	ret = kstrtoul(buf, 0, &user_data);
	if (ret)
		return -EINVAL;

	if (user_data != 0 && user_data != 1)
		return -EINVAL;

	mangopctrl = dev_get_drvdata(dev);
	data = (struct mango_soc_pinctrl_data *)mangopctrl->data;

	for (p = 0; p < data->functions_count; p++) {
		if (!strncmp(attr->attr.name, data->functions[p].name,
				strlen(attr->attr.name)) &&
				(user_data == data->functions[p].mode)) {
			selector = p;
			break;
		}
	}
	if (selector < 0)
		return -ENXIO;

	group = selector/2;
	mango_set_mux(mangopctrl->pctl, selector, group);

	dev_info(dev, "pinmux store set %s to func %d\n",
			attr->attr.name, data->functions[selector].mode);
	return size;
}


int sophgo_pinctrl_probe(struct platform_device *pdev)
{
	struct mango_pinctrl *mangopctrl;
	struct pinctrl_desc *desc;
	struct mango_soc_pinctrl_data *data;
	struct device *dev = &pdev->dev;
	struct device *pin_dev = NULL;
	struct device_node *np = dev->of_node, *np_top;
	static struct regmap *syscon;
	int ret;

	data = (struct mango_soc_pinctrl_data *)of_device_get_match_data(&pdev->dev);
	if (!data)
		return -EINVAL;
	mangopctrl = devm_kzalloc(&pdev->dev, sizeof(*mangopctrl), GFP_KERNEL);
	if (!mangopctrl)
		return -ENOMEM;

	mangopctrl->dev = &pdev->dev;

	np_top = of_parse_phandle(np, "subctrl-syscon", 0);
	if (!np_top) {
		dev_err(dev, "%s can't get subctrl-syscon node\n", __func__);
		return -EINVAL;
	}
	syscon = syscon_node_to_regmap(np_top);
	if (IS_ERR(syscon)) {
		dev_err(dev, "cannot get regmap\n");
		return PTR_ERR(syscon);
	}
	mangopctrl->syscon_pinctl = syscon;

	ret = device_property_read_u32(&pdev->dev,
		"top_pinctl_offset", &mangopctrl->top_pinctl_offset);
	if (ret < 0) {
		dev_err(dev, "cannot get top_pinctl_offset\n");
		return ret;
	}

	desc = &mango_desc;
	desc->pins = data->pins;
	desc->npins = data->npins;

	mangopctrl->data = (void *)data;
	mangopctrl->pctl = devm_pinctrl_register(&pdev->dev, desc, mangopctrl);
	if (IS_ERR(mangopctrl->pctl)) {
		dev_err(&pdev->dev, "could not register Sophgo pin ctrl driver\n");
		return PTR_ERR(mangopctrl->pctl);
	}

	platform_set_drvdata(pdev, mangopctrl);

	ret = class_register(data->p_class);
	if (ret < 0) {
		dev_err(dev, "cannot register pinmux class\n");
		return ret;
	}
	pin_dev = device_create(data->p_class, &pdev->dev, MKDEV(0, 0), mangopctrl, "mango_pinmux");
	if (IS_ERR(pin_dev))
		return PTR_ERR(pin_dev);

	return 0;
}
