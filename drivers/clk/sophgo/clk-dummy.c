/*
 * Copyright (c) 2022 SOPHGO
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/mfd/syscon.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/string.h>
#include <linux/log2.h>

#include "clk.h"

/*
 * @hw:		handle between common and hardware-specific interfaces
 * @reg:	register containing divider
 * @shift:	shift to the divider bit field
 * @width:	width of the divider bit field
 * @initial_val:initial value of the divider
 * @table:	the div table that the divider supports
 * @lock:	register lock
 */
struct mango_clk_divider {
	struct clk_hw	hw;
	void __iomem	*reg;
	u8		shift;
	u8		width;
	u8		flags;
	u32		initial_val;
	const struct clk_div_table *table;
	spinlock_t	*lock;
};

static unsigned long mango_clk_divider_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	int rc, index = 0;
	u32 rate;
	struct property *prop;
	const __be32 *cur;
	struct clk *clk;

	node = of_find_node_by_name(NULL, "default_rates");

	of_property_for_each_u32 (node, "clock-rates", prop, cur, rate) {
		if (rate) {
			rc = of_parse_phandle_with_args(node, "clocks",
							"#clock-cells", index, &clkspec);
			if (rc < 0) {
				/* skip empty (null) phandles */
				if (rc == -ENOENT)
					continue;
				else
					return rc;
			}

			clk = of_clk_get_from_provider(&clkspec);
			if (IS_ERR(clk))
				return PTR_ERR(clk);
			if (!strcmp(clk_hw_get_name(hw), __clk_get_name(clk)))
				return rate;
		}
		index++;
	}
	return 0;
}

static long mango_clk_divider_round_rate(struct clk_hw *hw, unsigned long rate,
				      unsigned long *prate)
{
	return rate;
}

static int mango_clk_divider_set_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long parent_rate)
{
	return 0;
}

/*
 * @hw: ccf use to hook get mango_pll_clock
 * @parent_rate: parent rate
 *
 * The is function will be called through clk_get_rate
 * and return current rate after decoding reg value
 */
static unsigned long mango_clk_pll_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct device_node *node;
	struct of_phandle_args clkspec;
	int rc, index = 0;
	u32 rate;
	struct property *prop;
	const __be32 *cur;

	node = of_find_node_by_name(NULL, "default_rates");

	of_property_for_each_u32 (node, "clock-rates", prop, cur, rate) {
		if (rate) {
			rc = of_parse_phandle_with_args(node, "clocks",
							"#clock-cells", index, &clkspec);
			if (rc < 0) {
				/* skip empty (null) phandles */
				if (rc == -ENOENT)
					continue;
				else
					return rc;
			}

			if (!strncmp(clk_hw_get_name(hw), clkspec.np->name, 4))
				return rate;
		}
		index++;
	}
	return 0;
}

static long mango_clk_pll_round_rate(struct clk_hw *hw,
				  unsigned long req_rate, unsigned long *prate)
{
	return req_rate;
}

static int mango_clk_pll_determine_rate(struct clk_hw *hw,
				     struct clk_rate_request *req)
{
	req->rate = mango_clk_pll_round_rate(hw, min(req->rate, req->max_rate),
					  &req->best_parent_rate);
	return 0;
}

static int mango_clk_pll_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	return 0;
}

const struct clk_ops dm_mango_clk_divider_ops = {
	.recalc_rate = mango_clk_divider_recalc_rate,
	.round_rate = mango_clk_divider_round_rate,
	.set_rate = mango_clk_divider_set_rate,
};

const struct clk_ops dm_mango_clk_divider_ro_ops = {
	.recalc_rate = mango_clk_divider_recalc_rate,
	.round_rate = mango_clk_divider_round_rate,
};

const struct clk_ops dm_mango_clk_pll_ops = {
	.recalc_rate = mango_clk_pll_recalc_rate,
	.round_rate = mango_clk_pll_round_rate,
	.determine_rate = mango_clk_pll_determine_rate,
	.set_rate = mango_clk_pll_set_rate,
};

const struct clk_ops dm_mango_clk_pll_ro_ops = {
	.recalc_rate = mango_clk_pll_recalc_rate,
	.round_rate = mango_clk_pll_round_rate,
};

struct mux_cb_clk_name {
	const char *name;
	struct list_head node;
};

static struct list_head mux_cb_clk_name_list =
	LIST_HEAD_INIT(mux_cb_clk_name_list);
static int mux_notifier_cb(struct notifier_block *nb,
				unsigned long event, void *data)
{
	int ret = 0;
	static unsigned char mux_id = 1;
	struct clk_notifier_data *ndata = data;
	struct clk_hw *hw = __clk_get_hw(ndata->clk);
	const struct clk_ops *ops = &clk_mux_ops;
	struct mux_cb_clk_name *cb_lsit;

	if (event == PRE_RATE_CHANGE) {
		struct clk_hw *hw_p = clk_hw_get_parent(hw);

		cb_lsit = kmalloc(sizeof(*cb_lsit), GFP_KERNEL);
		if (cb_lsit) {
			INIT_LIST_HEAD(&cb_lsit->node);
			list_add_tail(&cb_lsit->node, &mux_cb_clk_name_list);
		} else {
			pr_err("mux cb kmalloc mem fail\n");
			goto out;
		}

		cb_lsit->name = clk_hw_get_name(hw_p);
		mux_id = ops->get_parent(hw);
		if (mux_id > 1) {
			ret = 1;
			goto out;
		}
		ops->set_parent(hw, !mux_id);
	} else if (event == POST_RATE_CHANGE) {
		struct clk_hw *hw_p = clk_hw_get_parent(hw);

		cb_lsit = list_first_entry_or_null(&mux_cb_clk_name_list,
						typeof(*cb_lsit), node);
		if (cb_lsit) {
			const char *pre_name = cb_lsit->name;

			list_del_init(&cb_lsit->node);
			kfree(cb_lsit);
			if (strcmp(clk_hw_get_name(hw_p), pre_name))
				goto out;
		}

		ops->set_parent(hw, mux_id);
	}

out:
	return notifier_from_errno(ret);
}

int dm_set_default_clk_rates(struct device_node *node)
{
	struct of_phandle_args clkspec;
	struct property *prop;
	const __be32 *cur;
	int rc, index = 0;
	struct clk *clk;
	u32 rate;

	of_property_for_each_u32 (node, "clock-rates", prop, cur, rate) {
		if (rate) {
			rc = of_parse_phandle_with_args(node, "clocks",
							"#clock-cells", index, &clkspec);
			if (rc < 0) {
				/* skip empty (null) phandles */
				if (rc == -ENOENT)
					continue;
				else
					return rc;
			}

			clk = of_clk_get_from_provider(&clkspec);
			if (IS_ERR(clk)) {
				pr_warn("clk: couldn't get clock %d for %s\n",
					index, node->full_name);
				return PTR_ERR(clk);
			}

			rc = clk_set_rate(clk, rate);
			if (rc < 0)
				pr_err("clk: couldn't set %s clk rate to %d (%d), current rate: %ld\n",
				       __clk_get_name(clk), rate, rc,
				       clk_get_rate(clk));
			clk_put(clk);
		}
		index++;
	}

	return 0;
}

static struct clk *__register_divider_clks(struct device *dev, const char *name,
					   const char *parent_name,
					   unsigned long flags,
					   void __iomem *reg, u8 shift,
					   u8 width, u32 initial_val,
					   u8 clk_divider_flags,
					   const struct clk_div_table *table,
					   spinlock_t *lock)
{
	struct mango_clk_divider *div;
	struct clk_hw *hw;
	struct clk_init_data init;
	int ret;

	if (clk_divider_flags & CLK_DIVIDER_HIWORD_MASK) {
		if (width + shift > 16) {
			pr_warn("divider value exceeds LOWORD field\n");
			return ERR_PTR(-EINVAL);
		}
	}

	/* allocate the divider */
	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	if (clk_divider_flags & CLK_DIVIDER_READ_ONLY)
		init.ops = &dm_mango_clk_divider_ro_ops;
	else
		init.ops = &dm_mango_clk_divider_ops;
	init.flags = flags;
	init.parent_names = (parent_name ? &parent_name : NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct mango_clk_divider assignments */
	div->reg = reg;
	div->shift = shift;
	div->width = width;
	div->flags = clk_divider_flags;
	div->lock = lock;
	div->hw.init = &init;
	div->table = table;
	div->initial_val = initial_val;

	/* register the clock */
	hw = &div->hw;
	ret = clk_hw_register(dev, hw);
	if (ret) {
		kfree(div);
		hw = ERR_PTR(ret);
		return ERR_PTR(-EBUSY);
	}

	return hw->clk;
}

static inline int register_provider_clks
(struct device_node *node, struct mango_clk_data *clk_data, int clk_num)
{
	return of_clk_add_provider(node, of_clk_src_onecell_get,
				   &clk_data->clk_data);
}

static int register_gate_clks(struct device *dev, struct mango_clk_data *clk_data)
{
	struct clk *clk;
	const struct mango_clk_table *table = clk_data->table;
	const struct mango_gate_clock *gate_clks = table->gate_clks;
	void __iomem *base = clk_data->base;
	int clk_num = table->gate_clks_num;
	int i;

	for (i = 0; i < clk_num; i++) {
		clk = clk_register_gate(
			dev, gate_clks[i].name, gate_clks[i].parent_name,
			gate_clks[i].flags | CLK_IS_CRITICAL, base + gate_clks[i].offset,
			gate_clks[i].bit_idx, gate_clks[i].gate_flags,
			&clk_data->lock);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				gate_clks[i].name);
			goto err;
		}

		if (gate_clks[i].alias)
			clk_register_clkdev(clk, gate_clks[i].alias, NULL);

		clk_data->clk_data.clks[gate_clks[i].id] = clk;
	}

	return 0;

err:
	while (i--)
		clk_unregister_gate(clk_data->clk_data.clks[gate_clks[i].id]);

	return PTR_ERR(clk);
}

static int register_divider_clks(struct device *dev,
				 struct mango_clk_data *clk_data)
{
	struct clk *clk;
	const struct mango_clk_table *table = clk_data->table;
	const struct mango_divider_clock *div_clks = table->div_clks;
	void __iomem *base = clk_data->base;
	int clk_num = table->div_clks_num;
	int i, val;

	for (i = 0; i < clk_num; i++) {
		clk = __register_divider_clks(
			NULL, div_clks[i].name, div_clks[i].parent_name,
			div_clks[i].flags, base + div_clks[i].offset,
			div_clks[i].shift, div_clks[i].width,
			div_clks[i].initial_val,
			(div_clks[i].initial_sel & MANGO_CLK_USE_INIT_VAL) ?
				div_clks[i].div_flags | CLK_DIVIDER_READ_ONLY :
				div_clks[i].div_flags,
			div_clks[i].table, &clk_data->lock);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				div_clks[i].name);
			goto err;
		}

		clk_data->clk_data.clks[div_clks[i].id] = clk;

		if (div_clks[i].initial_sel == MANGO_CLK_USE_REG_VAL) {
			regmap_read(clk_data->syscon_top, div_clks[i].offset,
				    &val);

			/*
			 * set a default divider factor,
			 * clk driver should not select divider clock as the
			 * clock source, before set the divider by right process
			 * (assert div, set div factor, de assert div).
			 */
			if (div_clks[i].initial_val > 0)
				val |= (div_clks[i].initial_val << 16 | 1 << 3);
			else {
				/*
				 * the div register is config to use divider factor, don't change divider
				 */
				if (!(val >> 3 & 0x1))
					val |= 1 << 16;
			}

			regmap_write(clk_data->syscon_top, div_clks[i].offset,
				     val);
		}
	}

	return 0;

err:
	while (i--)
		clk_unregister_divider(clk_data->clk_data.clks[div_clks[i].id]);

	return PTR_ERR(clk);
}

static int register_mux_clks(struct device *dev, struct mango_clk_data *clk_data)
{
	struct clk *clk;
	const struct mango_clk_table *table = clk_data->table;
	const struct mango_mux_clock *mux_clks = table->mux_clks;
	void __iomem *base = clk_data->base;
	int clk_num = table->mux_clks_num;
	int i;

	for (i = 0; i < clk_num; i++) {
		u32 mask = BIT(mux_clks[i].width) - 1;

		clk = clk_register_mux_table(
			dev, mux_clks[i].name, mux_clks[i].parent_names,
			mux_clks[i].num_parents, mux_clks[i].flags,
			base + mux_clks[i].offset, mux_clks[i].shift, mask,
			mux_clks[i].mux_flags, mux_clks[i].table,
			&clk_data->lock);
		if (IS_ERR(clk)) {
			pr_err("%s: failed to register clock %s\n", __func__,
				mux_clks[i].name);
			goto err;
		}

		clk_data->clk_data.clks[mux_clks[i].id] = clk;

		if (!(mux_clks[i].flags & CLK_MUX_READ_ONLY)) {
			struct clk *parent;
			struct notifier_block *clk_nb;

			/* set mux clock default parent here, it's parent index
			 * value is read from the mux clock reg. dts can override
			 * setting the mux clock parent later.
			 */
			parent = clk_get_parent(clk);
			clk_set_parent(clk, parent);

			/* add a notify callback function */
			clk_nb = kzalloc(sizeof(*clk_nb), GFP_KERNEL);
			if (!clk_nb)
				goto err;
			clk_nb->notifier_call = mux_notifier_cb;
			if (clk_notifier_register(clk, clk_nb))
				pr_err("%s: failed to register clock notifier for %s\n",
					__func__, mux_clks[i].name);
		}
	}

	return 0;

err:
	while (i--)
		clk_unregister_mux(clk_data->clk_data.clks[mux_clks[i].id]);

	return PTR_ERR(clk);
}

/* pll clock init */
int dm_mango_register_pll_clks(struct device_node *node,
			 struct mango_clk_data *clk_data, const char *clk_name)
{
	struct clk *clk = NULL;
	struct mango_pll_clock *pll_clks;
	int i, ret = 0;
	const struct clk_ops *local_ops;

	pll_clks = (struct mango_pll_clock *)clk_data->table->pll_clks;
	for (i = 0; i < clk_data->table->pll_clks_num; i++) {
		if (!strcmp(clk_name,  pll_clks[i].name)) {
			/* have to assigne pll_clks.syscon_top first
			 * since clk_register_composite will need it
			 * to calculate current rate.
			 */
			pll_clks[i].syscon_top = clk_data->syscon_top;
			pll_clks[i].lock = &clk_data->lock;
			if (pll_clks[i].ini_flags & MANGO_CLK_RO)
				local_ops = &dm_mango_clk_pll_ro_ops;
			else
				local_ops = &dm_mango_clk_pll_ops;
			clk = clk_register_composite(
				NULL, pll_clks[i].name, &pll_clks[i].parent_name,
				1, NULL, NULL, &pll_clks[i].hw, local_ops,
				NULL, NULL, pll_clks[i].flags);

			if (IS_ERR(clk)) {
				pr_err("%s: failed to register clock %s\n", __func__,
					pll_clks[i].name);
				ret = -EINVAL;
				goto out;
			}
			ret = of_clk_add_provider(node, of_clk_src_simple_get, clk);
			if (ret)
				clk_unregister(clk);
		} else {
			continue;
		}
	}

out:
	return ret;
}

/* mux clk init */
int dm_mango_register_mux_clks(struct device_node *node, struct mango_clk_data *clk_data)
{
	int ret;
	int count;
	struct clk **clk_table;

	count = clk_data->table->mux_clks_num + clk_data->table->gate_clks_num;
	clk_table = kcalloc(count, sizeof(*clk_table), GFP_KERNEL);
	if (!clk_table)
		return -ENOMEM;

	clk_data->clk_data.clks = clk_table;
	clk_data->clk_data.clk_num = count;

	ret = register_mux_clks(NULL, clk_data);
	if (ret)
		goto err;

	ret = register_gate_clks(NULL, clk_data);
	if (ret)
		goto err;

	ret = register_provider_clks(node, clk_data, count);
	if (ret)
		goto err;

	return 0;
err:
	kfree(clk_table);
	return ret;
}

/* pll divider init */
int dm_mango_register_div_clks(struct device_node *node, struct mango_clk_data *clk_data)
{
	int ret;
	int count;

	struct clk **clk_table;

	count = clk_data->table->div_clks_num + clk_data->table->gate_clks_num;
	clk_table = kcalloc(count, sizeof(*clk_table), GFP_KERNEL);
	if (!clk_table)
		return -ENOMEM;

	clk_data->clk_data.clks = clk_table;
	clk_data->clk_data.clk_num = count;

	ret = register_divider_clks(NULL, clk_data);
	if (ret)
		goto err;

	ret = register_gate_clks(NULL, clk_data);
	if (ret)
		goto err;

	ret = register_provider_clks(node, clk_data, count);
	if (ret)
		goto err;


	return 0;
err:
	kfree(clk_table);
	pr_err("%s error %d\n", __func__, ret);
	return ret;
}
