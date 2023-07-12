/*
 * Copyright (c) 2007 Ben Dooks
 * Copyright (c) 2008 Simtec Electronics
 *     Ben Dooks <ben@simtec.co.uk>, <ben-linux@fluff.org>
 * Copyright (c) 2013 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * PWM driver for Samsung SoCs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>



#define REG_HLPERIOD		0x0
#define REG_PERIOD			0x4
#define REG_GROUP			0x8
#define REG_POLARITY		0x20


/**
 * struct sophgo_pwm_channel - private data of PWM channel
 * @period_ns:	current period in nanoseconds programmed to the hardware
 * @duty_ns:	current duty time in nanoseconds programmed to the hardware
 * @tin_ns:	time of one timer tick in nanoseconds with current timer rate
 */
struct sophgo_pwm_channel {
	u32 period;
	u32 hlperiod;
};

/**
 * struct sophgo_pwm_chip - private data of PWM chip
 * @chip:		generic PWM chip
 * @variant:		local copy of hardware variant data
 * @inverter_mask:	inverter status for all channels - one bit per channel
 * @base:		base address of mapped PWM registers
 * @base_clk:		base clock used to drive the timers
 * @tclk0:		external clock 0 (can be ERR_PTR if not present)
 * @tclk1:		external clock 1 (can be ERR_PTR if not present)
 */
struct sophgo_pwm_chip {
	struct pwm_chip chip;
	void __iomem *base;
	struct clk *base_clk;
	u8 polarity_mask;
	bool no_polarity;
};


static inline
struct sophgo_pwm_chip *to_sophgo_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct sophgo_pwm_chip, chip);
}

static int pwm_sophgo_request(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct sophgo_pwm_channel *channel;

	channel = kzalloc(sizeof(*channel), GFP_KERNEL);
	if (!channel)
		return -ENOMEM;

	return pwm_set_chip_data(pwm_dev, channel);
}

static void pwm_sophgo_free(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct sophgo_pwm_channel *channel = pwm_get_chip_data(pwm_dev);

	pwm_set_chip_data(pwm_dev, NULL);
	kfree(channel);
}

static int pwm_sophgo_config(struct pwm_chip *chip, struct pwm_device *pwm_dev,
			     int duty_ns, int period_ns)
{
	struct sophgo_pwm_chip *our_chip = to_sophgo_pwm_chip(chip);
	struct sophgo_pwm_channel *channel = pwm_get_chip_data(pwm_dev);
	u64 cycles;

	cycles = clk_get_rate(our_chip->base_clk);
	cycles *= period_ns;
	do_div(cycles, NSEC_PER_SEC);

	channel->period = cycles;
	cycles = cycles * duty_ns;
	do_div(cycles, period_ns);
	channel->hlperiod = channel->period - cycles;

	return 0;
}

static int pwm_sophgo_enable(struct pwm_chip *chip, struct pwm_device *pwm_dev)
{
	struct sophgo_pwm_chip *our_chip = to_sophgo_pwm_chip(chip);
	struct sophgo_pwm_channel *channel = pwm_get_chip_data(pwm_dev);

	writel(channel->period, our_chip->base + REG_GROUP * pwm_dev->hwpwm + REG_PERIOD);
	writel(channel->hlperiod, our_chip->base + REG_GROUP * pwm_dev->hwpwm + REG_HLPERIOD);

	return 0;
}

static void pwm_sophgo_disable(struct pwm_chip *chip,
			       struct pwm_device *pwm_dev)
{
	struct sophgo_pwm_chip *our_chip = to_sophgo_pwm_chip(chip);

	writel(0, our_chip->base + REG_GROUP * pwm_dev->hwpwm + REG_PERIOD);
	writel(0, our_chip->base + REG_GROUP * pwm_dev->hwpwm + REG_HLPERIOD);
}

static int pwm_sophgo_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			      const struct pwm_state *state)
{
	int ret;

	bool enabled = pwm->state.enabled;

	if (state->polarity != pwm->state.polarity && pwm->state.enabled) {
		pwm_sophgo_disable(chip, pwm);
		enabled = false;
	}

	if (!state->enabled) {
		if (enabled)
			pwm_sophgo_disable(chip, pwm);
		return 0;
	}

	ret = pwm_sophgo_config(chip, pwm, state->duty_cycle, state->period);
	if (ret) {
		dev_err(chip->dev, "pwm apply err\n");
		return ret;
	}
	dev_dbg(chip->dev, "%s tate->enabled =%d\n", __func__, state->enabled);
	if (state->enabled)
		ret = pwm_sophgo_enable(chip, pwm);
	else
		pwm_sophgo_disable(chip, pwm);

	if (ret) {
		dev_err(chip->dev, "pwm apply failed\n");
		return ret;
	}
	return ret;
}

static const struct pwm_ops pwm_sophgo_ops = {
	.request	= pwm_sophgo_request,
	.free		= pwm_sophgo_free,
	.apply		= pwm_sophgo_apply,
	.owner		= THIS_MODULE,
};

static const struct of_device_id sophgo_pwm_match[] = {
	{ .compatible = "sophgo,sophgo-pwm" },
	{ },
};
MODULE_DEVICE_TABLE(of, sophgo_pwm_match);

static int pwm_sophgo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sophgo_pwm_chip *chip;
	struct resource *res;
	int ret;

	pr_info("%s\n", __func__);

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->chip.dev = &pdev->dev;
	chip->chip.ops = &pwm_sophgo_ops;
	chip->chip.base = -1;
	chip->polarity_mask = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(chip->base))
		return PTR_ERR(chip->base);

	chip->base_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(chip->base_clk)) {
		dev_err(dev, "failed to get pwm source clk\n");
		return PTR_ERR(chip->base_clk);
	}

	ret = clk_prepare_enable(chip->base_clk);
	if (ret < 0) {
		dev_err(dev, "failed to enable base clock\n");
		return ret;
	}

	//pwm-num default is 4, compatible with sg2042
	if (of_property_read_bool(pdev->dev.of_node, "pwm-num"))
		device_property_read_u32(&pdev->dev, "pwm-num", &chip->chip.npwm);
	else
		chip->chip.npwm = 4;

	//no_polarity default is false(have polarity) , compatible with sg2042
	if (of_property_read_bool(pdev->dev.of_node, "no-polarity"))
		chip->no_polarity = true;
	else
		chip->no_polarity = false;
	pr_debug("chip->chip.npwm =%d  chip->no_polarity=%d\n", chip->chip.npwm, chip->no_polarity);

	platform_set_drvdata(pdev, chip);

	ret = pwmchip_add(&chip->chip);
	if (ret < 0) {
		dev_err(dev, "failed to register PWM chip\n");
		clk_disable_unprepare(chip->base_clk);
		return ret;
	}

	return 0;
}

static int pwm_sophgo_remove(struct platform_device *pdev)
{
	struct sophgo_pwm_chip *chip = platform_get_drvdata(pdev);

	pwmchip_remove(&chip->chip);

	clk_disable_unprepare(chip->base_clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pwm_sophgo_suspend(struct device *dev)
{
	return 0;
}

static int pwm_sophgo_resume(struct device *dev)
{
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(pwm_sophgo_pm_ops, pwm_sophgo_suspend,
			 pwm_sophgo_resume);

static struct platform_driver pwm_sophgo_driver = {
	.driver		= {
		.name	= "sophgo-pwm",
		.pm	= &pwm_sophgo_pm_ops,
		.of_match_table = of_match_ptr(sophgo_pwm_match),
	},
	.probe		= pwm_sophgo_probe,
	.remove		= pwm_sophgo_remove,
};
module_platform_driver(pwm_sophgo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chunzhi.lin");
MODULE_DESCRIPTION("Sophgo PWM driver");
