/*
 * Sophgo SoCs Reset Controller driver
 *
 * Copyright (c) 2018 Bitmain Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>

#define BITS_PER_REG	32

struct bm_reset_data {
	spinlock_t			lock;
	void __iomem			*membase;
	struct reset_controller_dev	rcdev;
	struct regmap		*syscon_rst;
	u32			top_rst_offset;
};

static int bm_reset_assert(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct bm_reset_data *data = container_of(rcdev,
						     struct bm_reset_data,
						     rcdev);
	int bank = id / BITS_PER_REG;
	int offset = id % BITS_PER_REG;
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&data->lock, flags);

	regmap_read(data->syscon_rst, data->top_rst_offset + (bank * 4),
		&reg);
	regmap_write(data->syscon_rst, data->top_rst_offset + (bank * 4),
		reg & ~BIT(offset));

	spin_unlock_irqrestore(&data->lock, flags);

	return 0;
}

static int bm_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	struct bm_reset_data *data = container_of(rcdev,
						     struct bm_reset_data,
						     rcdev);
	int bank = id / BITS_PER_REG;
	int offset = id % BITS_PER_REG;
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&data->lock, flags);

	regmap_read(data->syscon_rst, data->top_rst_offset + (bank * 4),
		&reg);
	regmap_write(data->syscon_rst,  data->top_rst_offset + (bank * 4),
		reg | BIT(offset));

	spin_unlock_irqrestore(&data->lock, flags);

	return 0;
}

static const struct reset_control_ops bm_reset_ops = {
	.assert		= bm_reset_assert,
	.deassert	= bm_reset_deassert,
};

static const struct of_device_id bm_reset_dt_ids[] = {
	 { .compatible = "bitmain,reset", },
	 { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, bm_reset_dt_ids);

static int bm_reset_probe(struct platform_device *pdev)
{
	struct bm_reset_data *data;
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node, *np_top;
	static struct regmap *syscon;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	np_top = of_parse_phandle(np, "subctrl-syscon", 0);
	if (!np_top) {
		dev_err(dev, "%s can't get subctrl-syscon node\n", __func__);
		goto out_free_devm;
	}

	syscon = syscon_node_to_regmap(np_top);
	if (IS_ERR(syscon)) {
		dev_err(dev, "cannot get regmap\n");
		goto out_free_devm;
	}

	data->syscon_rst = syscon;
	ret = device_property_read_u32(&pdev->dev, "top_rst_offset",
		&data->top_rst_offset);
	if (ret < 0) {
		dev_err(dev, "cannot get top_rst_offset\n");
		goto out_free_devm;
	}

	ret = device_property_read_u32(&pdev->dev, "nr_resets",
		&data->rcdev.nr_resets);
	if (ret < 0) {
		dev_err(dev, "cannot get nr_resets\n");
		goto out_free_devm;
	}

	spin_lock_init(&data->lock);

	data->rcdev.owner = THIS_MODULE;
	data->rcdev.ops = &bm_reset_ops;
	data->rcdev.of_node = pdev->dev.of_node;

	ret = devm_reset_controller_register(&pdev->dev, &data->rcdev);
	if (!ret)
		return 0;

out_free_devm:
	devm_kfree(&pdev->dev, data);
	return ret;
}

static struct platform_driver bm_reset_driver = {
	.probe	= bm_reset_probe,
	.driver = {
		.name		= "bm-reset",
		.of_match_table	= bm_reset_dt_ids,
	},
};

static int __init bm_reset_init(void)
{
	return platform_driver_register(&bm_reset_driver);
}
postcore_initcall(bm_reset_init);

MODULE_AUTHOR("Wei Huang<wei.huang01@bitmain.com>");
MODULE_DESCRIPTION("Bitmain SoC Reset Controoler Driver");
MODULE_LICENSE("GPL");
