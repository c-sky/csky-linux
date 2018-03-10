#include <linux/init.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/pm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <asm/io.h>

static volatile void *pmaddr;

static void qemu_pm_power_off(void)
{
	*(unsigned int *)pmaddr = 0;
}

static int qemuexit_platform_probe(struct platform_device *dev)
{
	struct resource *res_mem;
	void * __pmaddr;
	int err;

	res_mem = platform_get_resource(dev, IORESOURCE_MEM, 0);
	__pmaddr = devm_ioremap_resource(&dev->dev, res_mem);
	if (IS_ERR(__pmaddr)) {
		err = PTR_ERR(__pmaddr);
		return err;
	}

	pmaddr = __pmaddr;
	pm_power_off = qemu_pm_power_off;

	return 0;
}

static const struct of_device_id qemuexit_ids[] = {
	{ .compatible = "csky,qemu-exit", },
	{}
};
MODULE_DEVICE_TABLE(of, qemuexit_ids);

static struct platform_driver qemuexit_platform_driver = {
	.probe		= qemuexit_platform_probe,
	.driver		= {
		.name	= "qemu-exit",
		.of_match_table = qemuexit_ids,
	}
};

static int __init qemuexit_platform_init(void)
{
	return platform_driver_register(&qemuexit_platform_driver);
}
module_init(qemuexit_platform_init);

static void __exit qemuexit_cleanup(void)
{
	platform_driver_unregister(&qemuexit_platform_driver);
}
module_exit(qemuexit_cleanup);

MODULE_DESCRIPTION("C-SKY QMEU exit");
MODULE_LICENSE("GPL");
