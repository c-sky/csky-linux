// SPDX-License-Identifier: GPL-2.0+
/*
 * Mellanox boot control driver
 *
 * This driver provides a sysfs interface for systems management
 * software to manage reset-time actions.
 *
 * Copyright (C) 2019 Mellanox Technologies
 */

#include <linux/acpi.h>
#include <linux/arm-smccc.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "mlxbf-bootctl.h"

#define MLXBF_BOOTCTL_SB_SECURE_MASK		0x03
#define MLXBF_BOOTCTL_SB_TEST_MASK		0x0c

#define MLXBF_SB_KEY_NUM			4

/* UUID used to probe ATF service. */
static const char *mlxbf_bootctl_svc_uuid_str =
	"89c036b4-e7d7-11e6-8797-001aca00bfc4";

struct mlxbf_bootctl_name {
	u32 value;
	const char *name;
};

static struct mlxbf_bootctl_name boot_names[] = {
	{ MLXBF_BOOTCTL_EXTERNAL, "external" },
	{ MLXBF_BOOTCTL_EMMC, "emmc" },
	{ MLNX_BOOTCTL_SWAP_EMMC, "swap_emmc" },
	{ MLXBF_BOOTCTL_EMMC_LEGACY, "emmc_legacy" },
	{ MLXBF_BOOTCTL_NONE, "none" },
};

static const char * const mlxbf_bootctl_lifecycle_states[] = {
	[0] = "Production",
	[1] = "GA Secured",
	[2] = "GA Non-Secured",
	[3] = "RMA",
};

/* Log header format. */
#define MLXBF_RSH_LOG_TYPE_MASK		GENMASK_ULL(59, 56)
#define MLXBF_RSH_LOG_LEN_MASK		GENMASK_ULL(54, 48)
#define MLXBF_RSH_LOG_LEVEL_MASK	GENMASK_ULL(7, 0)

/* Log module ID and type (only MSG type in Linux driver for now). */
#define MLXBF_RSH_LOG_TYPE_MSG		0x04ULL

/* Log ctl/data register offset. */
#define MLXBF_RSH_SCRATCH_BUF_CTL_OFF	0
#define MLXBF_RSH_SCRATCH_BUF_DATA_OFF	0x10

/* Log message levels. */
enum {
	MLXBF_RSH_LOG_INFO,
	MLXBF_RSH_LOG_WARN,
	MLXBF_RSH_LOG_ERR,
	MLXBF_RSH_LOG_ASSERT
};

/* Mapped pointer for RSH_BOOT_FIFO_DATA and RSH_BOOT_FIFO_COUNT register. */
static void __iomem *mlxbf_rsh_boot_data;
static void __iomem *mlxbf_rsh_boot_cnt;

/* Mapped pointer for rsh log semaphore/ctrl/data register. */
static void __iomem *mlxbf_rsh_semaphore;
static void __iomem *mlxbf_rsh_scratch_buf_ctl;
static void __iomem *mlxbf_rsh_scratch_buf_data;

/* Rsh log levels. */
static const char * const mlxbf_rsh_log_level[] = {
	"INFO", "WARN", "ERR", "ASSERT"};

/* ARM SMC call which is atomic and no need for lock. */
static int mlxbf_bootctl_smc(unsigned int smc_op, int smc_arg)
{
	struct arm_smccc_res res;

	arm_smccc_smc(smc_op, smc_arg, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

/* Return the action in integer or an error code. */
static int mlxbf_bootctl_reset_action_to_val(const char *action)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(boot_names); i++)
		if (sysfs_streq(boot_names[i].name, action))
			return boot_names[i].value;

	return -EINVAL;
}

/* Return the action in string. */
static const char *mlxbf_bootctl_action_to_string(int action)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(boot_names); i++)
		if (boot_names[i].value == action)
			return boot_names[i].name;

	return "invalid action";
}

static ssize_t post_reset_wdog_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int ret;

	ret = mlxbf_bootctl_smc(MLXBF_BOOTCTL_GET_POST_RESET_WDOG, 0);
	if (ret < 0)
		return ret;

	return sprintf(buf, "%d\n", ret);
}

static ssize_t post_reset_wdog_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long value;
	int ret;

	ret = kstrtoul(buf, 10, &value);
	if (ret)
		return ret;

	ret = mlxbf_bootctl_smc(MLXBF_BOOTCTL_SET_POST_RESET_WDOG, value);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t mlxbf_bootctl_show(int smc_op, char *buf)
{
	int action;

	action = mlxbf_bootctl_smc(smc_op, 0);
	if (action < 0)
		return action;

	return sprintf(buf, "%s\n", mlxbf_bootctl_action_to_string(action));
}

static int mlxbf_bootctl_store(int smc_op, const char *buf, size_t count)
{
	int ret, action;

	action = mlxbf_bootctl_reset_action_to_val(buf);
	if (action < 0)
		return action;

	ret = mlxbf_bootctl_smc(smc_op, action);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t reset_action_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return mlxbf_bootctl_show(MLXBF_BOOTCTL_GET_RESET_ACTION, buf);
}

static ssize_t reset_action_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	return mlxbf_bootctl_store(MLXBF_BOOTCTL_SET_RESET_ACTION, buf, count);
}

static ssize_t second_reset_action_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return mlxbf_bootctl_show(MLXBF_BOOTCTL_GET_SECOND_RESET_ACTION, buf);
}

static ssize_t second_reset_action_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	return mlxbf_bootctl_store(MLXBF_BOOTCTL_SET_SECOND_RESET_ACTION, buf,
				   count);
}

static ssize_t lifecycle_state_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int lc_state;

	lc_state = mlxbf_bootctl_smc(MLXBF_BOOTCTL_GET_TBB_FUSE_STATUS,
				     MLXBF_BOOTCTL_FUSE_STATUS_LIFECYCLE);
	if (lc_state < 0)
		return lc_state;

	lc_state &=
		MLXBF_BOOTCTL_SB_TEST_MASK | MLXBF_BOOTCTL_SB_SECURE_MASK;

	/*
	 * If the test bits are set, we specify that the current state may be
	 * due to using the test bits.
	 */
	if (lc_state & MLXBF_BOOTCTL_SB_TEST_MASK) {
		lc_state &= MLXBF_BOOTCTL_SB_SECURE_MASK;

		return sprintf(buf, "%s(test)\n",
			       mlxbf_bootctl_lifecycle_states[lc_state]);
	}

	return sprintf(buf, "%s\n", mlxbf_bootctl_lifecycle_states[lc_state]);
}

static ssize_t secure_boot_fuse_state_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	int burnt, valid, key, key_state, buf_len = 0, upper_key_used = 0;
	const char *status;

	key_state = mlxbf_bootctl_smc(MLXBF_BOOTCTL_GET_TBB_FUSE_STATUS,
				      MLXBF_BOOTCTL_FUSE_STATUS_KEYS);
	if (key_state < 0)
		return key_state;

	/*
	 * key_state contains the bits for 4 Key versions, loaded from eFuses
	 * after a hard reset. Lower 4 bits are a thermometer code indicating
	 * key programming has started for key n (0000 = none, 0001 = version 0,
	 * 0011 = version 1, 0111 = version 2, 1111 = version 3). Upper 4 bits
	 * are a thermometer code indicating key programming has completed for
	 * key n (same encodings as the start bits). This allows for detection
	 * of an interruption in the programming process which has left the key
	 * partially programmed (and thus invalid). The process is to burn the
	 * eFuse for the new key start bit, burn the key eFuses, then burn the
	 * eFuse for the new key complete bit.
	 *
	 * For example 0000_0000: no key valid, 0001_0001: key version 0 valid,
	 * 0011_0011: key 1 version valid, 0011_0111: key version 2 started
	 * programming but did not complete, etc. The most recent key for which
	 * both start and complete bit is set is loaded. On soft reset, this
	 * register is not modified.
	 */
	for (key = MLXBF_SB_KEY_NUM - 1; key >= 0; key--) {
		burnt = key_state & BIT(key);
		valid = key_state & BIT(key + MLXBF_SB_KEY_NUM);

		if (burnt && valid)
			upper_key_used = 1;

		if (upper_key_used) {
			if (burnt)
				status = valid ? "Used" : "Wasted";
			else
				status = valid ? "Invalid" : "Skipped";
		} else {
			if (burnt)
				status = valid ? "InUse" : "Incomplete";
			else
				status = valid ? "Invalid" : "Free";
		}
		buf_len += sprintf(buf + buf_len, "%d:%s ", key, status);
	}
	buf_len += sprintf(buf + buf_len, "\n");

	return buf_len;
}

static ssize_t fw_reset_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	unsigned long key;
	int err;

	err = kstrtoul(buf, 16, &key);
	if (err)
		return err;

	if (mlxbf_bootctl_smc(MLXBF_BOOTCTL_FW_RESET, key) < 0)
		return -EINVAL;

	return count;
}

/* Size(8-byte words) of the log buffer. */
#define RSH_SCRATCH_BUF_CTL_IDX_MASK	0x7f

/* 100ms timeout */
#define RSH_SCRATCH_BUF_POLL_TIMEOUT	100000

static int mlxbf_rsh_log_sem_lock(void)
{
	unsigned long reg;

	return readq_poll_timeout(mlxbf_rsh_semaphore, reg, !reg, 0,
				  RSH_SCRATCH_BUF_POLL_TIMEOUT);
}

static void mlxbf_rsh_log_sem_unlock(void)
{
	writeq(0, mlxbf_rsh_semaphore);
}

static ssize_t rsh_log_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int rc, idx, num, len, level = MLXBF_RSH_LOG_INFO;
	size_t size = count;
	u64 data;

	if (!size)
		return -EINVAL;

	if (!mlxbf_rsh_semaphore || !mlxbf_rsh_scratch_buf_ctl)
		return -EOPNOTSUPP;

	/* Ignore line break at the end. */
	if (buf[size - 1] == '\n')
		size--;

	/* Check the message prefix. */
	for (idx = 0; idx < ARRAY_SIZE(mlxbf_rsh_log_level); idx++) {
		len = strlen(mlxbf_rsh_log_level[idx]);
		if (len + 1 < size &&
		    !strncmp(buf, mlxbf_rsh_log_level[idx], len)) {
			buf += len;
			size -= len;
			level = idx;
			break;
		}
	}

	/* Ignore leading spaces. */
	while (size > 0 && buf[0] == ' ') {
		size--;
		buf++;
	}

	/* Take the semaphore. */
	rc = mlxbf_rsh_log_sem_lock();
	if (rc)
		return rc;

	/* Calculate how many words are available. */
	idx = readq(mlxbf_rsh_scratch_buf_ctl);
	num = min((int)DIV_ROUND_UP(size, sizeof(u64)),
		  RSH_SCRATCH_BUF_CTL_IDX_MASK - idx - 1);
	if (num <= 0)
		goto done;

	/* Write Header. */
	data = FIELD_PREP(MLXBF_RSH_LOG_TYPE_MASK, MLXBF_RSH_LOG_TYPE_MSG);
	data |= FIELD_PREP(MLXBF_RSH_LOG_LEN_MASK, num);
	data |= FIELD_PREP(MLXBF_RSH_LOG_LEVEL_MASK, level);
	writeq(data, mlxbf_rsh_scratch_buf_data);

	/* Write message. */
	for (idx = 0; idx < num && size > 0; idx++) {
		if (size < sizeof(u64)) {
			data = 0;
			memcpy(&data, buf, size);
			size = 0;
		} else {
			memcpy(&data, buf, sizeof(u64));
			size -= sizeof(u64);
			buf += sizeof(u64);
		}
		writeq(data, mlxbf_rsh_scratch_buf_data);
	}

done:
	/* Release the semaphore. */
	mlxbf_rsh_log_sem_unlock();

	/* Ignore the rest if no more space. */
	return count;
}

static DEVICE_ATTR_RW(post_reset_wdog);
static DEVICE_ATTR_RW(reset_action);
static DEVICE_ATTR_RW(second_reset_action);
static DEVICE_ATTR_RO(lifecycle_state);
static DEVICE_ATTR_RO(secure_boot_fuse_state);
static DEVICE_ATTR_WO(fw_reset);
static DEVICE_ATTR_WO(rsh_log);

static struct attribute *mlxbf_bootctl_attrs[] = {
	&dev_attr_post_reset_wdog.attr,
	&dev_attr_reset_action.attr,
	&dev_attr_second_reset_action.attr,
	&dev_attr_lifecycle_state.attr,
	&dev_attr_secure_boot_fuse_state.attr,
	&dev_attr_fw_reset.attr,
	&dev_attr_rsh_log.attr,
	NULL
};

ATTRIBUTE_GROUPS(mlxbf_bootctl);

static const struct acpi_device_id mlxbf_bootctl_acpi_ids[] = {
	{"MLNXBF04", 0},
	{}
};

MODULE_DEVICE_TABLE(acpi, mlxbf_bootctl_acpi_ids);

static ssize_t mlxbf_bootctl_bootfifo_read(struct file *filp,
					   struct kobject *kobj,
					   struct bin_attribute *bin_attr,
					   char *buf, loff_t pos,
					   size_t count)
{
	unsigned long timeout = msecs_to_jiffies(500);
	unsigned long expire = jiffies + timeout;
	u64 data, cnt = 0;
	char *p = buf;

	while (count >= sizeof(data)) {
		/* Give up reading if no more data within 500ms. */
		if (!cnt) {
			cnt = readq(mlxbf_rsh_boot_cnt);
			if (!cnt) {
				if (time_after(jiffies, expire))
					break;
				usleep_range(10, 50);
				continue;
			}
		}

		data = readq(mlxbf_rsh_boot_data);
		memcpy(p, &data, sizeof(data));
		count -= sizeof(data);
		p += sizeof(data);
		cnt--;
		expire = jiffies + timeout;
	}

	return p - buf;
}

static struct bin_attribute mlxbf_bootctl_bootfifo_sysfs_attr = {
	.attr = { .name = "bootfifo", .mode = 0400 },
	.read = mlxbf_bootctl_bootfifo_read,
};

static bool mlxbf_bootctl_guid_match(const guid_t *guid,
				     const struct arm_smccc_res *res)
{
	guid_t id = GUID_INIT(res->a0, res->a1, res->a1 >> 16,
			      res->a2, res->a2 >> 8, res->a2 >> 16,
			      res->a2 >> 24, res->a3, res->a3 >> 8,
			      res->a3 >> 16, res->a3 >> 24);

	return guid_equal(guid, &id);
}

static int mlxbf_bootctl_probe(struct platform_device *pdev)
{
	struct arm_smccc_res res = { 0 };
	void __iomem *reg;
	guid_t guid;
	int ret;

	/* Map the resource of the bootfifo data register. */
	mlxbf_rsh_boot_data = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mlxbf_rsh_boot_data))
		return PTR_ERR(mlxbf_rsh_boot_data);

	/* Map the resource of the bootfifo counter register. */
	mlxbf_rsh_boot_cnt = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(mlxbf_rsh_boot_cnt))
		return PTR_ERR(mlxbf_rsh_boot_cnt);

	/* Map the resource of the rshim semaphore register. */
	mlxbf_rsh_semaphore = devm_platform_ioremap_resource(pdev, 2);
	if (IS_ERR(mlxbf_rsh_semaphore))
		return PTR_ERR(mlxbf_rsh_semaphore);

	/* Map the resource of the scratch buffer (log) registers. */
	reg = devm_platform_ioremap_resource(pdev, 3);
	if (IS_ERR(reg))
		return PTR_ERR(reg);
	mlxbf_rsh_scratch_buf_ctl = reg + MLXBF_RSH_SCRATCH_BUF_CTL_OFF;
	mlxbf_rsh_scratch_buf_data = reg + MLXBF_RSH_SCRATCH_BUF_DATA_OFF;

	/* Ensure we have the UUID we expect for this service. */
	arm_smccc_smc(MLXBF_BOOTCTL_SIP_SVC_UID, 0, 0, 0, 0, 0, 0, 0, &res);
	guid_parse(mlxbf_bootctl_svc_uuid_str, &guid);
	if (!mlxbf_bootctl_guid_match(&guid, &res))
		return -ENODEV;

	/*
	 * When watchdog is used, it sets boot mode to MLXBF_BOOTCTL_SWAP_EMMC
	 * in case of boot failures. However it doesn't clear the state if there
	 * is no failure. Restore the default boot mode here to avoid any
	 * unnecessary boot partition swapping.
	 */
	ret = mlxbf_bootctl_smc(MLXBF_BOOTCTL_SET_RESET_ACTION,
				MLXBF_BOOTCTL_EMMC);
	if (ret < 0)
		dev_warn(&pdev->dev, "Unable to reset the EMMC boot mode\n");

	ret = sysfs_create_bin_file(&pdev->dev.kobj,
				    &mlxbf_bootctl_bootfifo_sysfs_attr);
	if (ret)
		pr_err("Unable to create bootfifo sysfs file, error %d\n", ret);

	return ret;
}

static int mlxbf_bootctl_remove(struct platform_device *pdev)
{
	sysfs_remove_bin_file(&pdev->dev.kobj,
			      &mlxbf_bootctl_bootfifo_sysfs_attr);

	return 0;
}

static struct platform_driver mlxbf_bootctl_driver = {
	.probe = mlxbf_bootctl_probe,
	.remove = mlxbf_bootctl_remove,
	.driver = {
		.name = "mlxbf-bootctl",
		.dev_groups = mlxbf_bootctl_groups,
		.acpi_match_table = mlxbf_bootctl_acpi_ids,
	}
};

module_platform_driver(mlxbf_bootctl_driver);

MODULE_DESCRIPTION("Mellanox boot control driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Mellanox Technologies");
