#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/netlink.h>
#include <net/sock.h>

#define DEV_NAME "sophgo-tach"
#define MHZ 1000000
#define USER_PORT 100
#define USER_MSG  30

struct fan_state {
	unsigned int freq_num;
	bool enable;
};

struct sophgo_fan_speed_device {
	struct device *dev;
	struct device *parent;
	struct class *class;
	dev_t devno;
	u32 __iomem *regs;
	struct delayed_work poll_queue;
	struct fan_state fan_state;
	struct mutex enable_lock;
	struct mutex freqnum_lock;
};

static int fan_index;
static struct class *sophgo_fan_speed_class;
static struct sock *nl_fd;

static int send_msg(char *pbuf, uint16_t len)
{
	struct sk_buff *nl_skb;
	struct nlmsghdr *nlh;
	int ret = 0;

	//alloc a new netlink message
	nl_skb = nlmsg_new(len, GFP_ATOMIC);
	if (!nl_skb) {
		pr_err("sophgo_fan_speed, netlink alloc skb error!\n");
		return -ENOMEM;
	}

	//add a new netlink message to skb
	nlh = nlmsg_put(nl_skb, 0, 0, USER_MSG, len, 0);
	if (nlh == NULL) {
		pr_err("sophgo_fan_speed, nlmsg_put error!\n");
		nlmsg_free(nl_skb);
		return -EFAULT;
	}

	memcpy(nlmsg_data(nlh), pbuf, len);
	ret = netlink_unicast(nl_fd, nl_skb, USER_PORT, MSG_DONTWAIT);

	return ret;
}

static void recv_cb(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	void *data = NULL;

	if (skb->len >= nlmsg_total_size(0)) {
		nlh = nlmsg_hdr(skb);
		data = nlmsg_data(nlh);
		if (data) {
			pr_info("sophgo_fan_speed, kernel receive data: %s\n", (int8_t *)data);
			send_msg(data, nlmsg_len(nlh));
		}
	}
}

struct netlink_kernel_cfg cfg = {
	.input = recv_cb,
};

static void fan_speed_check(struct work_struct *work)
{
	struct sophgo_fan_speed_device *sophgo_fan = container_of(work,
				struct sophgo_fan_speed_device, poll_queue.work);
	int speed, ret = 0;
	char buf[64];

	speed = readl(sophgo_fan->regs + 1);
	if (speed == 0) {
		dev_dbg(sophgo_fan->dev, "fan stop!");
		ret = snprintf(buf, 32, "%s fan stop!\n", dev_name(sophgo_fan->dev));
		if (ret <= 0 || ret > 32) {
			dev_err(sophgo_fan->dev, "%s snprintf failed\n", __func__);
			return;
		}
		ret = send_msg(buf, sizeof(buf));
		if (ret < 0)
			dev_dbg(sophgo_fan->dev, "%s send msg failed, ret=%d\n", __func__, ret);
	}
	mod_delayed_work(system_freezable_wq, &sophgo_fan->poll_queue,
				round_jiffies(msecs_to_jiffies(5000)));
}

static void fan_speed_enable(bool enalbe, struct sophgo_fan_speed_device *sophgo_fan)
{
	if (enalbe) {
		cancel_delayed_work(&sophgo_fan->poll_queue);
		writel(sophgo_fan->fan_state.freq_num, sophgo_fan->regs);
		mod_delayed_work(system_freezable_wq, &sophgo_fan->poll_queue,
					round_jiffies(msecs_to_jiffies(5000)));
		sophgo_fan->fan_state.enable = true;
	} else {
		cancel_delayed_work(&sophgo_fan->poll_queue);
		writel(0, sophgo_fan->regs);
		sophgo_fan->fan_state.enable = false;
	}
}

static ssize_t freq_num_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct sophgo_fan_speed_device *sophgo_fan;

	sophgo_fan = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", sophgo_fan->fan_state.freq_num);
}

static ssize_t freq_num_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t size)
{
	int val, ret = 0;
	struct sophgo_fan_speed_device *sophgo_fan;

	sophgo_fan = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &val);
	if (ret)
		return ret;

	if (val < (MHZ/10) || val > (1000*MHZ))
		ret = -EINVAL;

	mutex_lock(&sophgo_fan->freqnum_lock);
	sophgo_fan->fan_state.freq_num = val;
	mutex_unlock(&sophgo_fan->freqnum_lock);

	return ret ? : size;
}

static ssize_t enable_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct sophgo_fan_speed_device *sophgo_fan;

	sophgo_fan = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", sophgo_fan->fan_state.enable);
}

static ssize_t enable_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t size)
{
	int val, ret = 0;
	struct sophgo_fan_speed_device *sophgo_fan;

	sophgo_fan = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &val);
	if (ret)
		return ret;

	mutex_lock(&sophgo_fan->enable_lock);
	switch (val) {
	case 0:
		fan_speed_enable(false, sophgo_fan);
		break;
	case 1:
		fan_speed_enable(true, sophgo_fan);
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&sophgo_fan->enable_lock);

	return ret ? : size;
}

static ssize_t fan_speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = -1;
	struct sophgo_fan_speed_device *sophgo_fan;

	sophgo_fan = dev_get_drvdata(dev);
	ret = snprintf(buf, 32, "fan_speed:%d\n", readl(sophgo_fan->regs + 1));
	if (ret <= 0 || ret > 32) {
		dev_err(sophgo_fan->dev, "%s snprintf failed %d\n", __func__, ret);
		return -EFAULT;
	}
	dev_dbg(sophgo_fan->dev, "%s\n", buf);
	return ret;
}

static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR_RW(freq_num);
static DEVICE_ATTR_RO(fan_speed);

static struct attribute *fan_speed_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_freq_num.attr,
	&dev_attr_fan_speed.attr,
	NULL,
};
ATTRIBUTE_GROUPS(fan_speed);

static int sophgo_fan_speed_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct sophgo_fan_speed_device *sophgo_fan;
	char dev_name[32];
	int ret;

	sophgo_fan = devm_kzalloc(&pdev->dev, sizeof(*sophgo_fan), GFP_KERNEL);
	if (sophgo_fan == NULL)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	sophgo_fan->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(sophgo_fan->regs))
		return PTR_ERR(sophgo_fan->regs);

	ret = snprintf(dev_name, 15, "%s-%d", DEV_NAME, fan_index++);
	if (ret <= 0 || ret > 15) {
		dev_err(&pdev->dev, "%s snprintf failed\n", __func__);
		return -EINVAL;
	}
	ret = alloc_chrdev_region(&sophgo_fan->devno, 0, 1, dev_name);
	if (ret < 0) {
		dev_err(&pdev->dev, "register chrdev error\n");
		return ret;
	}
	sophgo_fan->class = sophgo_fan_speed_class;
	sophgo_fan->dev = device_create(sophgo_fan->class, sophgo_fan->parent,
					sophgo_fan->devno, sophgo_fan, dev_name);
	if (IS_ERR(sophgo_fan->dev)) {
		ret = PTR_ERR(sophgo_fan->dev);
		dev_err(&pdev->dev, "create device failed\n");
		unregister_chrdev_region(sophgo_fan->devno, 1);
		return ret;
	}

	//as fan source clk 100M, we advise to set freq num  100M
	sophgo_fan->fan_state.freq_num = 100*MHZ;
	mutex_init(&sophgo_fan->freqnum_lock);
	mutex_init(&sophgo_fan->enable_lock);

	platform_set_drvdata(pdev, sophgo_fan);
	INIT_DELAYED_WORK(&sophgo_fan->poll_queue, fan_speed_check);

	return 0;
}

static int sophgo_fan_speed_remove(struct platform_device *pdev)
{
	struct sophgo_fan_speed_device *sophgo_fan = platform_get_drvdata(pdev);

	cancel_delayed_work(&sophgo_fan->poll_queue);
	device_destroy(sophgo_fan->class, sophgo_fan->devno);
	unregister_chrdev_region(sophgo_fan->devno, 1);
	kfree(sophgo_fan);
	sophgo_fan = NULL;
	return 0;
}

static const struct of_device_id sophgo_fan_speed_of_match[] = {
	{
		.compatible = "sophgo,sophgo-tach",
	},
	{}
};

static struct platform_driver sophgo_fan_speed_driver = {
	.probe = sophgo_fan_speed_probe,
	.remove =  sophgo_fan_speed_remove,
	.driver = {
		.name = "sophgo,sophgo-tach",
		.of_match_table = sophgo_fan_speed_of_match,
	},
};

static int __init sophgo_fan_speed_init(void)
{
	sophgo_fan_speed_class = class_create(DEV_NAME);
	if (IS_ERR(sophgo_fan_speed_class)) {
		pr_err("class create failed\n");
		return PTR_ERR(sophgo_fan_speed_class);
	}
	sophgo_fan_speed_class->dev_groups = fan_speed_groups;

	nl_fd = netlink_kernel_create(&init_net, USER_MSG, &cfg);
	if (!nl_fd) {
		pr_err("sophgo_fan_speed, cannot create netlink socket!\n");
		return -1;
	}
	fan_index = 0;
	return platform_driver_register(&sophgo_fan_speed_driver);
}

static void __exit sophgo_fan_speed_exit(void)
{
	class_destroy(sophgo_fan_speed_class);
	if (nl_fd) {
		netlink_kernel_release(nl_fd);
		nl_fd = NULL;
	}
}

module_init(sophgo_fan_speed_init);
module_exit(sophgo_fan_speed_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xiao Wang");
MODULE_DESCRIPTION("minimal module");
MODULE_VERSION("ALPHA");
