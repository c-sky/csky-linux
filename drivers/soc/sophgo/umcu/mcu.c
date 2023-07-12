#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/nvmem-consumer.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/mutex.h>
#include <linux/slab.h>

/* fixed MCU registers */
#define MCU_REG_BOARD_TYPE		0x00
#define MCU_REG_VERSION			0x01
#define MCU_REG_SOC_TEMP		0x04
#define MCU_REG_BOARD_TEMP		0x05
#define MCU_REG_PWROFF_REASON1		0x06
#define MCU_REG_PWROFF_REASON2		0x07

#define MCU_REG_CRITICAL_ACTIONS	0x65
#define MCU_REG_CRITICAL_TEMP		0x66
#define MCU_REG_REPOWERON_TEMP		0x67
#define MCU_REG_KEEP_DDR_POWERON	0x68

#define MCU_CRITICAL_ACTION_POWEROFF	0x2
#define MCU_CRITICAL_ACTION_REBOOT	0X1

#define MANGO_BOARD_TYPE_MASK	1 << 7

#ifndef assert
#define assert(exp)	WARN_ON(!(exp))
#endif

struct mcu_features {
	u8 id;
	char *proj;
	char *soc;
	char *chip;

	int board_type;
	int mcu_ver;
	int pcb_ver;
	int soc_tmp;
	int board_tmp;
	int alert_status;
	int alert_mask;
	int rst_cnt;
	int uptime;
	int lock;
	int power;
	int power_tpu;
	int brd_id;
	int brd_ip;

	int critical_action;
	int critical_temp;
	int repoweron_temp;
	int keep_ddr_poweron;

	char *alert_table[16];
};

struct mcu_info {
	u8 board_type;
	u8 mcu_ver;
	u8 pcb_ver;
	u8 rst_cnt;
	int updated;
};

struct mcu_ctx {
	const struct mcu_features *features;
	struct i2c_client *i2c;
	struct mcu_info info;
	u32 channel_config[4];
	struct hwmon_channel_info temp_info;
	const struct hwmon_channel_info *channel_info[3];
	struct hwmon_chip_info chip;
	struct mutex update_lock;
	unsigned int hwmon_update_interval; /* in milliseconds */
};

const struct mcu_features mcu_list[] = {
	{
		0x80, "SG2042 EVB", "SG2042", "GD32",
		0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x0a, 0x0b, -1,
		-1, -1, -1, -1, MCU_REG_CRITICAL_ACTIONS,
		MCU_REG_CRITICAL_TEMP, MCU_REG_REPOWERON_TEMP,
		MCU_REG_KEEP_DDR_POWERON,
		{
			"SoC overheat",
			"Power supply overheat",
			"Board overheat",
			"Board overheat and shutdown",
			"SoC overheat and shutdown",
			"Power supply failure",
			"12V power supply failure",
			"SoC required reboot",
		},
	},

	{
		0x83, "SG2042 X4", "SG2042", "GD32",
		0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x0a, 0x0b, -1,
		-1, -1, -1, -1, MCU_REG_CRITICAL_ACTIONS,
		MCU_REG_CRITICAL_TEMP, MCU_REG_REPOWERON_TEMP,
		MCU_REG_KEEP_DDR_POWERON,
		{
			"SoC overheat",
			"Power supply overheat",
			"Board overheat",
			"Board overheat and shutdown",
			"SoC overheat and shutdown",
			"Power supply failure",
			"12V power supply failure",
			"SoC required reboot",
		},
	},

		{
		0x90, "MILKV PIONEER", "SG2042", "GD32",
		0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08, 0x0a, 0x0b, -1,
		-1, -1, -1, -1, MCU_REG_CRITICAL_ACTIONS,
		MCU_REG_CRITICAL_TEMP, MCU_REG_REPOWERON_TEMP,
		MCU_REG_KEEP_DDR_POWERON,
		{
			"SoC overheat",
			"Power supply overheat",
			"Board overheat",
			"Board overheat and shutdown",
			"SoC overheat and shutdown",
			"Power supply failure",
			"12V power supply failure",
			"SoC required reboot",
		},
	},
};

static const char help[] =
"\n"
"sys files description\n"
"======================\n"
"Bitmain unified mcu device driver\n"
"You can get/set MCU though read/write operation\n"
"eg. You can get fixed information though command\n"
"$cat /sys/bus/i2c/0-0017/information\n"
"You can set alert mask though command\n"
"$echo 0xffff /sys/bus/i2c/0-0017/alert\n"
"\n"
"information\n"
"-----------\n"
"Fixed information during SoC uptime\n"
"Read this file will return such information\n"
"Write this file to force SoC re-get such information from MCU\n"
"No matter what data you write to or just invoke write with a length of 0\n"
"File pointer will move forward after read,\n"
"write has no effect on file pointer\n"
"\n"
"temperature\n"
"-----------\n"
"Temperature value, sensors located on SoC and on board\n"
"Read this file will return temperature of both in celsius\n"
"Write is forbidden\n"
"\n"
"uptime\n"
"------\n"
"Uptime (from SoC poweron) in seconds\n"
"Read will get this value, write is forbidden\n"
"\n"
"alert\n"
"-----\n"
"Alert control and status\n"
"Read will get current alert status\n"
"Write corresponding bit to 1 will mask this alert\n"
"You can use 0x/0X for hex, 0 for octal\n"
"other leading characters will be considered as decimal\n"
"Values larger than 0xffff is forbidden\n"
"\n";

enum {
	MCU_I2C_TYPE_U = 0,
	MCU_I2C_TYPE_U8,
	MCU_I2C_TYPE_U16,
	MCU_I2C_TYPE_U32,
	MCU_I2C_TYPE_U64,
	MCU_I2C_TYPE_D,
	MCU_I2C_TYPE_S8,
	MCU_I2C_TYPE_S16,
	MCU_I2C_TYPE_S32,
	MCU_I2C_TYPE_S64,
	MCU_I2C_TYPE_MAX,
};

static const char *mcu_i2c_type_list[MCU_I2C_TYPE_MAX] = {
	[MCU_I2C_TYPE_U]	=	"u",
	[MCU_I2C_TYPE_U8]	=	"u8",
	[MCU_I2C_TYPE_U16]	=	"u16",
	[MCU_I2C_TYPE_U32]	=	"u32",
	[MCU_I2C_TYPE_U64]	=	"u64",
	[MCU_I2C_TYPE_D]	=	"d",
	[MCU_I2C_TYPE_S8]	=	"s8",
	[MCU_I2C_TYPE_S16]	=	"s16",
	[MCU_I2C_TYPE_S32]	=	"s32",
	[MCU_I2C_TYPE_S64]	=	"s64",
};

static int check_token(char *token)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mcu_i2c_type_list); ++i) {
		if (strcmp(token, mcu_i2c_type_list[i]) == 0)
			return i;
	}
	return -EINVAL;
}

static inline struct device *i2c2dev(struct i2c_client *i2c)
{
	return &i2c->dev;
}

static int mcu_i2c_write_byte(struct i2c_client *i2c, int reg, u8 data)
{
	int err;

	if (reg == -1)
		return 0;
	err = i2c_smbus_write_byte_data(i2c, reg, data);
	dev_dbg(i2c2dev(i2c), "%d : %u\n", reg, data);
	return err;
}

static int mcu_i2c_read_byte(struct i2c_client *i2c, int reg)
{
	int err;

	if (reg == -1)
		return 0;
	err = i2c_smbus_read_byte_data(i2c, reg);
	dev_dbg(i2c2dev(i2c), "%d : %d\n", reg, err);
	return err;
}

static int mcu_i2c_write_block(struct i2c_client *i2c, int reg,
			       int len, void *data)
{
	int err, i;

	if (reg == -1)
		return 0;

	err = i2c_smbus_write_i2c_block_data(i2c, reg, len, data);
	for (i = 0; i < len; ++i)
		dev_dbg(i2c2dev(i2c), "%d : %u\n", reg + i, ((u8 *)data)[i]);
	return err;
}

static int mcu_i2c_read_block(struct i2c_client *i2c, int reg,
			      int len, void *data)
{
	int err, i;

	if (reg == -1)
		return 0;

	err = i2c_smbus_read_i2c_block_data(i2c, reg, len, data);
	for (i = 0; i < len; ++i)
		dev_dbg(i2c2dev(i2c), "%d : %u\n", reg + i, ((u8 *)data)[i]);

	return err;
}

static int mcu_i2c_sreadf(struct i2c_client *i2c, const char *fmt, ...)
{
	va_list arg;
	const char *p = fmt;
	const char *start, *end;
	char token[16];
	int tokenlen;
	int ret = -EINVAL;
	int idx;

	if (fmt == NULL)
		return -EINVAL;

	va_start(arg, fmt);

	while (*p) {
		/* skip all % */
		while (*p == '%')
			++p;
		start = p;
		while (*p && *p != '%')
			++p;
		/* now *p is ether \0 or % */
		end = p;
		tokenlen = end - start;
		if (tokenlen > sizeof(token) - 1) {
			ret = -EINVAL;
			goto end;
		}
		if (tokenlen == 0)
			continue;
		/* get this token */
		memcpy(token, start, tokenlen);
		token[tokenlen] = 0; /* terminat this string */
		idx = check_token(token);
		if (idx < 0) {
			ret = idx;
			goto end;
		}

		ret = mcu_i2c_read_byte(i2c, va_arg(arg, int));
		if (ret < 0)
			goto end;

		switch (idx) {
		case MCU_I2C_TYPE_U:
			*va_arg(arg, unsigned int *) = ret;
			break;
		case MCU_I2C_TYPE_U8:
			*va_arg(arg, u8 *) = ret;
			break;
		case MCU_I2C_TYPE_U16:
			*va_arg(arg, u16 *) = ret;
			break;
		case MCU_I2C_TYPE_U32:
			*va_arg(arg, u32 *) = ret;
			break;
		case MCU_I2C_TYPE_U64:
			*va_arg(arg, u64 *) = ret;
			break;
		case MCU_I2C_TYPE_D:
			*va_arg(arg, int *) = ret;
			break;
		case MCU_I2C_TYPE_S8:
			*va_arg(arg, s8 *) = ret;
			break;
		case MCU_I2C_TYPE_S16:
			*va_arg(arg, s16 *) = ret;
			break;
		case MCU_I2C_TYPE_S32:
			*va_arg(arg, s32 *) = ret;
			break;
		case MCU_I2C_TYPE_S64:
			*va_arg(arg, s64 *) = ret;
			break;
		default:
			assert(false);
			break;
		}
	}

	ret = 0;
end:
	va_end(arg);
	return ret;
}

/* sysfs callbacks */

static inline struct i2c_client *dev2i2c(struct device *dev)
{
	return container_of(dev, struct i2c_client, dev);
}

static const inline struct mcu_features *dev2features(struct device *dev)
{
	struct i2c_client *i2c = dev2i2c(dev);
	struct mcu_ctx *ctx = (struct mcu_ctx *)i2c_get_clientdata(i2c);

	return ctx->features;
}

static ssize_t help_show(struct device *dev, struct device_attribute *attr,
		  char *buf)
{
	buf[0] = 0;
	strncpy(buf, help, PAGE_SIZE);
	return strlen(buf);
}

static int mcu_msg_append(char *base, unsigned long limit,
		   const char *fmt, ...)
{
	int len = strlen(base);
	va_list arg;

	va_start(arg, fmt);
	len += vsnprintf(base + len, limit - len, fmt, arg);
	va_end(arg);
	return len;
}

ssize_t info_show(struct device *dev, struct device_attribute *attr,
		  char *buf)
{
	struct i2c_client *i2c = dev2i2c(dev);
	struct mcu_ctx *ctx = (struct mcu_ctx *)i2c_get_clientdata(i2c);
	struct mcu_info *info = &ctx->info;
	const struct mcu_features *features = ctx->features;
	int err;

	if (info->updated == 0) {
		/* get information from mcu through i2c */
		err = mcu_i2c_sreadf(i2c, "%u8%u8%u8%u8",
				     features->board_type, &info->board_type,
				     features->mcu_ver, &info->mcu_ver,
				     features->pcb_ver, &info->pcb_ver,
				     features->rst_cnt, &info->rst_cnt);
		if (err)
			return err;
		info->updated = 1;
	}

	/* convert to json text */
	mcu_msg_append(buf, PAGE_SIZE, "{\n");
	mcu_msg_append(buf, PAGE_SIZE, "\t\"model\": \"%s\",\n", features->proj);
	mcu_msg_append(buf, PAGE_SIZE, "\t\"chip\": \"%s\",\n", features->soc);
	mcu_msg_append(buf, PAGE_SIZE, "\t\"mcu\": \"%s\",\n", features->chip);
	mcu_msg_append(buf, PAGE_SIZE, "\t\"board type\": \"0x%02X\",\n", info->board_type);
	mcu_msg_append(buf, PAGE_SIZE, "\t\"mcu version\": \"0x%02X\",\n", info->mcu_ver);
	mcu_msg_append(buf, PAGE_SIZE, "\t\"pcb version\": \"0x%02X\",\n", info->pcb_ver);
	mcu_msg_append(buf, PAGE_SIZE, "\t\"reset count\": %u\n", info->rst_cnt);
	err = mcu_msg_append(buf, PAGE_SIZE, "}\n");

	return err;
}

static ssize_t info_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct i2c_client *i2c = dev2i2c(dev);
	struct mcu_ctx *ctx = (struct mcu_ctx *)i2c_get_clientdata(i2c);
	struct mcu_info *info = &ctx->info;

	info->updated = 0;
	return count;
}

ssize_t brdid_show(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	int err = 0;
	const struct mcu_features *features = dev2features(dev);

	if (features->brd_id == -1)
		return -ENODEV;

	err = mcu_i2c_read_byte(dev2i2c(dev), features->brd_id);
	if (err < 0)
		return err;

	return sprintf(buf, "brdid:%u\n", err);
}

ssize_t brdip_show(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	u8 ip[4];
	int err;
	const struct mcu_features *features = dev2features(dev);

	if (features->brd_ip == -1)
		return -ENODEV;

	memset(ip, 0, sizeof(ip));
	err = mcu_i2c_read_block(dev2i2c(dev), features->brd_ip,
					    sizeof(ip), ip);
	if (err < 0)
		return err;

	return mcu_msg_append(buf, PAGE_SIZE, "brdip:%u.%u.%u.%u\n",
		ip[0], ip[1], ip[2], ip[3]);
}

static ssize_t brdip_store(struct device *dev, struct device_attribute *attr,
	const char *ubuf, size_t len)
{
	u8 ip[4];
	char buf[32];
	char *s, *p, *n;
	unsigned long res;
	int i, err;
	const struct mcu_features *features = dev2features(dev);

	if (features->brd_ip == -1)
		return -ENODEV;

	memset(buf, 0, sizeof(buf));
	len = min(sizeof(buf), len);
	memcpy(buf, ubuf, len);
	s = buf;
	for (i = 0; i < 4; i++) {
		if (i != 3) {
			p = strchr(s, '.');
			n = p+1;
			*p = '\0';
		}
		err = kstrtoul(s, 10, &res);
		if (err)
			return err;
		ip[i] = (u8)res;
		dev_dbg(dev, "ip[%d] = %d\n", i, ip[i]);
		s = n;
	}
	err = mcu_i2c_write_block(dev2i2c(dev), features->brd_ip,
					     sizeof(ip), ip);
	if (err < 0)
		return err;

	return len;
}
ssize_t uptime_show(struct device *dev, struct device_attribute *attr,
		    char *buf)
{
	u8 t[2];
	int err;
	const struct mcu_features *features = dev2features(dev);

	err = mcu_i2c_read_block(dev2i2c(dev), features->uptime,
					    sizeof(t), t);
	if (err < 0)
		return err;

	return mcu_msg_append(buf, PAGE_SIZE,
			      "%u Seconds\n", t[0] | (t[1] << 8));
}

ssize_t temp_show(struct device *dev, struct device_attribute *attr,
		  char *buf)
{
	s8 t[2];
	int err;
	const struct mcu_features *features = dev2features(dev);

	/* get information from mcu through i2c */
	err = mcu_i2c_sreadf(dev2i2c(dev), "%s8%s8",
			     features->soc_tmp, t,
			     features->board_tmp, t + 1);
	if (err)
		return err;

	mcu_msg_append(buf, PAGE_SIZE,
		       "SoC temperature: %d Cel\n", t[0]);
	return mcu_msg_append(buf, PAGE_SIZE,
			      "Board temperature: %d Cel\n", t[1]);
}


static const char *alert_id2name(const struct mcu_features *features, int id)
{
	if (features->alert_table[id])
		return features->alert_table[id];
	return "Unknown alert";
}

ssize_t alert_show(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	int cnt = 0;
	int i, err;
	u8 t[4];
	u16 mask, status;
	const struct mcu_features *features = dev2features(dev);
	const char *alt_msg;

	if (features->alert_status == -1)
		return 0;

	err = mcu_i2c_read_block(dev2i2c(dev), features->alert_mask,
					    sizeof(t), t);
	/* get information from mcu through i2c */
	if (err < 0)
		return err;

	status = t[0] | (t[1] << 8);
	mask = t[2] | (t[3] << 8);
	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
			"Mask 0x%02x, Status 0x%02x\n",
			mask, status);

	if (status == 0) {
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"Working fine\n");
	} else {
		for (i = 0; i < sizeof(status) * 8; ++i) {
			if ((status >> i) & 1) {
				alt_msg = alert_id2name(features, i);
				cnt += snprintf(buf + cnt,
						PAGE_SIZE - cnt,
						"%d: %s\n",
						i, alt_msg);
			}
		}
	}
	cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
			"**************************************************\n");

	for (i = 0; i < 16; ++i) {
		if (features->alert_table[i] == NULL)
			continue;
		cnt += snprintf(buf + cnt, PAGE_SIZE - cnt,
				"%d: %s\n", i, alert_id2name(features, i));
	}

	return cnt;
}

static ssize_t alert_store(struct device *dev, struct device_attribute *attr,
			  const char *ubuf, size_t len)
{
	char buf[32];
	u8 t[2];
	unsigned long res;
	int err;
	const struct mcu_features *features = dev2features(dev);

	if (features->alert_mask == -1)
		return -ENODEV;

	len = min(sizeof(buf) - 1, len);
	memcpy(buf, ubuf, len);

	buf[len] = 0; // zero terminated
	err = kstrtoul(buf, 0, &res);
	if (err)
		return err;
	if (res > 0xffff)
		return -EINVAL;

	t[0] = res & 0xff;
	t[1] = (res >> 8) & 0xff;
	err = mcu_i2c_write_block(dev2i2c(dev), features->alert_mask,
					     sizeof(t), t);
	if (err < 0)
		return err;

	return len;
}

ssize_t lock_show(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	int err;
	const struct mcu_features *features = dev2features(dev);

	if (features->lock == -1)
		return -ENODEV;

	err = mcu_i2c_read_byte(dev2i2c(dev), features->lock);

	if (err < 0)
		return err;

	return mcu_msg_append(buf, PAGE_SIZE,
			      "%d", err ? 1 : 0);
}

static ssize_t lock_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t len)
{
	int err;
	unsigned long res;
	const u8 *code[] = { "CK", "LO", };
	const u8 *p;
	const struct mcu_features *features = dev2features(dev);

	if (features->lock == -1)
		return -ENODEV;

	err = kstrtoul(buf, 0, &res);
	if (err)
		return err;

	res = res ? 1 : 0;

	for (p = code[res]; *p; ++p) {
		err = mcu_i2c_write_byte(dev2i2c(dev), features->lock, *p);
		if (err < 0)
			return err;
	}

	return len;
}

ssize_t power_show(struct device *dev, struct device_attribute *attr,
		   char *buf)
{
	u8 t[2];
	int err;
	const struct mcu_features *features = dev2features(dev);

	if (features->power < 0)
		return -EOPNOTSUPP;

	err = mcu_i2c_read_block(dev2i2c(dev), features->power, sizeof(t), t);
	if (err < 0)
		return err;

	err = sprintf(buf, "%umW\n", ((u16)t[0]) | (t[1] << 8));

	return err;
}

static ssize_t power_tpu_store(struct device *dev, struct device_attribute *attr,
			       const char *buf, size_t len)
{
	int err;
	unsigned long res;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	if (features->power_tpu < 0)
		return -EOPNOTSUPP;

	err = kstrtoul(buf, 0, &res);
	if (err)
		return err;

	data = res ? 1 : 0;

	err = mcu_i2c_write_block(dev2i2c(dev), features->power_tpu,
					     sizeof(data), &data);
	return len;
}

static ssize_t power_tpu_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	if (features->power_tpu < 0)
		return -EOPNOTSUPP;

	err = mcu_i2c_read_block(dev2i2c(dev), features->power_tpu, sizeof(data), &data);
	if (err < 0)
		return err;

	err = sprintf(buf, "%u\n", data);

	return err;
}

static ssize_t mcu_critical_action_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t len)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);


	if (!strcmp(buf, "reboot\n"))
		data = MCU_CRITICAL_ACTION_REBOOT;
	else if (!strcmp(buf, "poweroff\n"))
		data = MCU_CRITICAL_ACTION_POWEROFF;
	else
		data = 0;

	if (data) {
		err = mcu_i2c_write_block(dev2i2c(dev),
			features->critical_action, sizeof(data), &data);
	}
	return len;
}

static ssize_t mcu_critical_action_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	err = mcu_i2c_read_block(dev2i2c(dev), features->critical_action,
				 sizeof(data), &data);
	if (err < 0)
		return err;

	if (data == MCU_CRITICAL_ACTION_REBOOT)
		err = sprintf(buf, "reboot\n");
	else if (data == MCU_CRITICAL_ACTION_POWEROFF)
		err = sprintf(buf, "poweroff\n");
	else
		err = sprintf(buf, "unknown critical action\n");

	return err;
}


static ssize_t mcu_critical_temp_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	int err;
	unsigned long res;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	err = kstrtoul(buf, 0, &res);
	if (err)
		return err;

	data = res;

	err = mcu_i2c_write_block(dev2i2c(dev), features->critical_temp,
					     sizeof(data), &data);
	return len;
}

static ssize_t mcu_critical_temp_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	err = mcu_i2c_read_block(dev2i2c(dev), features->critical_temp,
				 sizeof(data), &data);
	if (err < 0)
		return err;

	err = sprintf(buf, "%u Cel\n", data);

	return err;
}


static ssize_t mcu_repoweron_temp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	int err;
	unsigned long res;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	err = kstrtoul(buf, 0, &res);
	if (err)
		return err;

	data = res;

	err = mcu_i2c_write_block(dev2i2c(dev), features->repoweron_temp,
					     sizeof(data), &data);
	return len;
}

static ssize_t mcu_repoweron_temp_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	err = mcu_i2c_read_block(dev2i2c(dev), features->repoweron_temp,
				 sizeof(data), &data);
	if (err < 0)
		return err;

	err = sprintf(buf, "%u Cel\n", data);

	return err;
}


static ssize_t mcu_keep_ddr_poweron_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	if (!strcmp(buf, "disable\n"))
		data = 1;
	else if (!strcmp(buf, "enable\n"))
		data = 0;
	else
		return 0;

	err = mcu_i2c_write_block(dev2i2c(dev), features->keep_ddr_poweron,
					     sizeof(data), &data);
	return len;
}

static ssize_t mcu_keep_ddr_poweron_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	int err;
	unsigned char data;
	const struct mcu_features *features = dev2features(dev);

	err = mcu_i2c_read_block(dev2i2c(dev), features->keep_ddr_poweron,
				 sizeof(data), &data);
	if (err < 0)
		return err;

	if (data == 1)
		err = sprintf(buf, "disable\n");
	else if (data == 0)
		err = sprintf(buf, "enable\n");
	else
		err = sprintf(buf, "unknown states\n");

	return err;
}

/* end of sysfs callbacks */

const struct device_attribute mcu_attrs[] =  {
	{{"help", 0444}, help_show, NULL},
	{{"information", 0644}, info_show, info_store},
	{{"temperature", 0444}, temp_show, NULL},
	{{"uptime", 0444}, uptime_show, NULL},
	{{"alert", 0644}, alert_show, alert_store},
	{{"lock", 0644}, lock_show, lock_store},
	{{"power-now", 0444}, power_show, NULL},
	{{"power-tpu", 0644}, power_tpu_show, power_tpu_store},
	{{"board-id", 0444}, brdid_show, NULL},
	{{"board-ip", 0644}, brdip_show, brdip_store},
	{{"critical-action", 0644}, mcu_critical_action_show,
	  mcu_critical_action_store},
	{{"critical-temp", 0644}, mcu_critical_temp_show,
	  mcu_critical_temp_store},
	{{"repoweron-temp", 0664}, mcu_repoweron_temp_show,
	  mcu_repoweron_temp_store},
	{{"keep-ddr-poweron", 0664}, mcu_keep_ddr_poweron_show,
	  mcu_keep_ddr_poweron_store},
};

static umode_t mcu_chip_is_visible(const void *data, enum hwmon_sensor_types type,
								 u32 attr, int channel)
{
	switch (type) {
	case hwmon_chip:
		return 0444;
	case hwmon_temp:
		return 0444;
	default:
		return 0;
	}
}

static int mcu_hwmon_chip_read(struct device *dev, u32 attr, int channel, long *val)
{
	struct mcu_ctx *ctx = dev_get_drvdata(dev);

	switch (attr) {
	case hwmon_chip_update_interval:
		*val = ctx->hwmon_update_interval;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int mcu_hwmon_temp_read(struct device *dev, u32 attr, int channel, long *val)
{
	int soc_temp, board_temp;

	struct mcu_ctx *ctx = dev_get_drvdata(dev);
	struct i2c_client *i2c = ctx->i2c;
	mutex_lock(&ctx->update_lock);
	soc_temp = mcu_i2c_read_byte(i2c, MCU_REG_SOC_TEMP);
	mutex_unlock(&ctx->update_lock);

	mutex_lock(&ctx->update_lock);
	board_temp = mcu_i2c_read_byte(i2c, MCU_REG_BOARD_TEMP);
	mutex_unlock(&ctx->update_lock);

	switch (attr) {
	case hwmon_temp_input:
		if (channel == 0)
			*val = soc_temp * 1000;
		else if (channel == 1)
			*val = board_temp * 1000;
		else
			*val = 0;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}


static int mcu_hwmon_read(struct device *dev, enum hwmon_sensor_types type,
			u32 attr, int channel, long *val)
{

	switch (type) {
	case hwmon_chip:
		return mcu_hwmon_chip_read(dev, attr, channel, val);
	case hwmon_temp:
		return mcu_hwmon_temp_read(dev, attr, channel, val);
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static const struct hwmon_ops mcu_ops = {
	.is_visible = mcu_chip_is_visible,
	.read = mcu_hwmon_read,
	.read_string = NULL,
	.write = NULL,
};

static int register_hwmon_temp_sensor(struct i2c_client *i2c,
			 struct mcu_ctx *ctx)
{
	struct device *dev = &i2c->dev;
	struct hwmon_channel_info *info;
	struct device *hwmon_dev;

	mutex_init(&ctx->update_lock);

	ctx->i2c = i2c;
	ctx->hwmon_update_interval = 1000;

	ctx->chip.ops = &mcu_ops;
	ctx->chip.info = ctx->channel_info;
	ctx->channel_info[0] = HWMON_CHANNEL_INFO(chip,
		 HWMON_C_REGISTER_TZ | HWMON_C_UPDATE_INTERVAL);
	ctx->channel_info[1] = &ctx->temp_info;

	info = &ctx->temp_info;
	info->type = hwmon_temp;
	info->config = ctx->channel_config;

	ctx->channel_config[0] = HWMON_T_INPUT;
	ctx->channel_config[1] = HWMON_T_INPUT;
	hwmon_dev = devm_hwmon_device_register_with_info(
						 dev,
						 dev->driver->name,
						 ctx,
						 &ctx->chip,
						 NULL);

	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	return 0;
}

static int sub_probe(struct i2c_client *i2c,
		     const struct mcu_features *features)
{
	struct mcu_ctx *ctx;
	int i, err;

	ctx = devm_kzalloc(i2c2dev(i2c), sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(mcu_attrs); ++i) {
		err = device_create_file(i2c2dev(i2c), mcu_attrs + i);
		if (err)
			return err;
	}

	ctx->features = features;

	assert(features->alert_status + 2 == features->alert_mask);

	if ((features->id & MANGO_BOARD_TYPE_MASK) == 0x80 ) {
		err = register_hwmon_temp_sensor(i2c, ctx);
		if (err)
			dev_warn(i2c2dev(i2c), "mcu board id %u register hwmon failed\n", features->id);
	}

	i2c_set_clientdata(i2c, ctx);
	return 0;
}

static int mcu_i2c_probe(struct i2c_client *i2c)
{
	int id;
	int err;
	int i;
	uint8_t regs[3];

	/* get information from mcu through i2c */
	err = mcu_i2c_sreadf(i2c, "%u8%u8%u8",
			     MCU_REG_VERSION, regs,
			     MCU_REG_PWROFF_REASON1, regs + 1,
			     MCU_REG_PWROFF_REASON2, regs + 2);
	if (err)
		return err;

	dev_info(i2c2dev(i2c), "MCU: version 0x%x, reason 0x%x/0x%x\n",
		regs[0], regs[1], regs[2]);

	id = mcu_i2c_read_byte(i2c, MCU_REG_BOARD_TYPE);
	if (id < 0)
		return id;

	for (i = 0; i < ARRAY_SIZE(mcu_list); ++i) {
		if (mcu_list[i].id == id)
			return sub_probe(i2c, mcu_list + i);
	}

	dev_warn(i2c2dev(i2c), "not registered mcu id %u\n", id);
	return -ENODEV;
}

static void mcu_i2c_remove(struct i2c_client *i2c)
{
	return;
}

static const struct of_device_id mcu_i2c_dt_table[] = {
	{ .compatible = "sophgo,sg20xx-mcu" },
	{},
};

static const struct i2c_device_id mcu_i2c_id_table[] = {
	{ "sg20xx-mcu", 0 },
	{},
};

static struct i2c_driver mcu_i2c_drv = {
	.driver = {
		.name = "sg20xx-mcu",
		.of_match_table = mcu_i2c_dt_table,
	},
	.probe = mcu_i2c_probe,
	.remove = mcu_i2c_remove,
	.id_table = mcu_i2c_id_table,
};

module_i2c_driver(mcu_i2c_drv);

MODULE_DESCRIPTION("MCU I2C driver for bm16xx soc platform");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chao.Wei@bitmain.com>");
