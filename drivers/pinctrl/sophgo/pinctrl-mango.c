#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/device.h>
#include <linux/of.h>

#include "../pinctrl-utils.h"
#include "pinctrl-sophgo.h"

#define DRV_PINCTRL_NAME "mango_pinctrl"
#define DRV_PINMUX_NAME "mango_pinmux"

#define FUNCTION(fname, gname, fmode) \
	{ \
		.name = #fname, \
		.groups = gname##_group, \
		.num_groups = ARRAY_SIZE(gname##_group), \
		.mode = fmode, \
	}

#define PIN_GROUP(gname) \
	{ \
		.name = #gname "_grp", \
		.pins = gname##_pins, \
		.num_pins = ARRAY_SIZE(gname##_pins), \
	}

static const struct pinctrl_pin_desc mango_pins[] = {
	PINCTRL_PIN(0,   "MIO0"),
	PINCTRL_PIN(1,   "MIO1"),
	PINCTRL_PIN(2,   "MIO2"),
	PINCTRL_PIN(3,   "MIO3"),
	PINCTRL_PIN(4,   "MIO4"),
	PINCTRL_PIN(5,   "MIO5"),
	PINCTRL_PIN(6,   "MIO6"),
	PINCTRL_PIN(7,   "MIO7"),
	PINCTRL_PIN(8,   "MIO8"),
	PINCTRL_PIN(9,   "MIO9"),
	PINCTRL_PIN(10,   "MIO10"),
	PINCTRL_PIN(11,   "MIO11"),
	PINCTRL_PIN(12,   "MIO12"),
	PINCTRL_PIN(13,   "MIO13"),
	PINCTRL_PIN(14,   "MIO14"),
	PINCTRL_PIN(15,   "MIO15"),
	PINCTRL_PIN(16,   "MIO16"),
	PINCTRL_PIN(17,   "MIO17"),
	PINCTRL_PIN(18,   "MIO18"),
	PINCTRL_PIN(19,   "MIO19"),
	PINCTRL_PIN(20,   "MIO20"),
	PINCTRL_PIN(21,   "MIO21"),
	PINCTRL_PIN(22,   "MIO22"),
	PINCTRL_PIN(23,   "MIO23"),
	PINCTRL_PIN(24,   "MIO24"),
	PINCTRL_PIN(25,   "MIO25"),
	PINCTRL_PIN(26,   "MIO26"),
	PINCTRL_PIN(27,   "MIO27"),
	PINCTRL_PIN(28,   "MIO28"),
	PINCTRL_PIN(29,   "MIO29"),
	PINCTRL_PIN(30,   "MIO30"),
	PINCTRL_PIN(31,   "MIO31"),
	PINCTRL_PIN(32,   "MIO32"),
	PINCTRL_PIN(33,   "MIO33"),
	PINCTRL_PIN(34,   "MIO34"),
	PINCTRL_PIN(35,   "MIO35"),
	PINCTRL_PIN(36,   "MIO36"),
	PINCTRL_PIN(37,   "MIO37"),
	PINCTRL_PIN(38,   "MIO38"),
	PINCTRL_PIN(39,   "MIO39"),
	PINCTRL_PIN(40,   "MIO40"),
	PINCTRL_PIN(41,   "MIO41"),
	PINCTRL_PIN(42,   "MIO42"),
	PINCTRL_PIN(43,   "MIO43"),
	PINCTRL_PIN(44,   "MIO44"),
	PINCTRL_PIN(45,   "MIO45"),
	PINCTRL_PIN(46,   "MIO46"),
	PINCTRL_PIN(47,   "MIO47"),
	PINCTRL_PIN(48,   "MIO48"),
	PINCTRL_PIN(49,   "MIO49"),
	PINCTRL_PIN(50,   "MIO50"),
	PINCTRL_PIN(51,   "MIO51"),
	PINCTRL_PIN(52,   "MIO52"),
	PINCTRL_PIN(53,   "MIO53"),
	PINCTRL_PIN(54,   "MIO54"),
	PINCTRL_PIN(55,   "MIO55"),
	PINCTRL_PIN(56,   "MIO56"),
	PINCTRL_PIN(57,   "MIO57"),
	PINCTRL_PIN(58,   "MIO58"),
	PINCTRL_PIN(59,   "MIO59"),
	PINCTRL_PIN(60,   "MIO60"),
	PINCTRL_PIN(61,   "MIO61"),
	PINCTRL_PIN(62,   "MIO62"),
	PINCTRL_PIN(63,   "MIO63"),
	PINCTRL_PIN(64,   "MIO64"),
	PINCTRL_PIN(65,   "MIO65"),
	PINCTRL_PIN(66,   "MIO66"),
	PINCTRL_PIN(67,   "MIO67"),
	PINCTRL_PIN(68,   "MIO68"),
	PINCTRL_PIN(69,   "MIO69"),
	PINCTRL_PIN(70,   "MIO70"),
	PINCTRL_PIN(71,   "MIO71"),
	PINCTRL_PIN(72,   "MIO72"),
	PINCTRL_PIN(73,   "MIO73"),
	PINCTRL_PIN(74,   "MIO74"),
	PINCTRL_PIN(75,   "MIO75"),
	PINCTRL_PIN(76,   "MIO76"),
	PINCTRL_PIN(77,   "MIO77"),
	PINCTRL_PIN(78,   "MIO78"),
	PINCTRL_PIN(79,   "MIO79"),
	PINCTRL_PIN(80,   "MIO80"),
	PINCTRL_PIN(81,   "MIO81"),
	PINCTRL_PIN(82,   "MIO82"),
	PINCTRL_PIN(83,   "MIO83"),
	PINCTRL_PIN(84,   "MIO84"),
	PINCTRL_PIN(85,   "MIO85"),
	PINCTRL_PIN(86,   "MIO86"),
	PINCTRL_PIN(87,   "MIO87"),
	PINCTRL_PIN(88,   "MIO88"),
	PINCTRL_PIN(89,   "MIO89"),
	PINCTRL_PIN(90,   "MIO90"),
	PINCTRL_PIN(91,   "MIO91"),
	PINCTRL_PIN(92,   "MIO92"),
	PINCTRL_PIN(93,   "MIO93"),
	PINCTRL_PIN(94,   "MIO94"),
	PINCTRL_PIN(95,   "MIO95"),
	PINCTRL_PIN(96,   "MIO96"),
	PINCTRL_PIN(97,   "MIO97"),
	PINCTRL_PIN(98,   "MIO98"),
	PINCTRL_PIN(99,   "MIO99"),
	PINCTRL_PIN(100,  "MIO100"),
	PINCTRL_PIN(101,  "MIO101"),
	PINCTRL_PIN(102,  "MIO102"),
	PINCTRL_PIN(103,  "MIO103"),
	PINCTRL_PIN(104,  "MIO104"),
	PINCTRL_PIN(105,  "MIO105"),
	PINCTRL_PIN(106,  "MIO106"),
	PINCTRL_PIN(107,  "MIO107"),
	PINCTRL_PIN(108,  "MIO108"),
	PINCTRL_PIN(109,  "MIO109"),
	PINCTRL_PIN(110,  "MIO110"),
	PINCTRL_PIN(111,  "MIO111"),
	PINCTRL_PIN(112,  "MIO112"),
	PINCTRL_PIN(113,  "MIO113"),
	PINCTRL_PIN(114,  "MIO114"),
	PINCTRL_PIN(115,  "MIO115"),
	PINCTRL_PIN(116,  "MIO116"),
	PINCTRL_PIN(117,  "MIO117"),
	PINCTRL_PIN(118,  "MIO118"),
	PINCTRL_PIN(119,  "MIO119"),
	PINCTRL_PIN(120,  "MIO120"),
	PINCTRL_PIN(121,  "MIO121"),
	PINCTRL_PIN(122,  "MIO122"),
	PINCTRL_PIN(123,  "MIO123"),
	PINCTRL_PIN(124,  "MIO124"),
	PINCTRL_PIN(125,  "MIO125"),
	PINCTRL_PIN(126,  "MIO126"),
	PINCTRL_PIN(127,  "MIO127"),
	PINCTRL_PIN(128,  "MIO128"),
	PINCTRL_PIN(129,  "MIO129"),
	PINCTRL_PIN(130,  "MIO130"),
	PINCTRL_PIN(131,  "MIO131"),
	PINCTRL_PIN(132,  "MIO132"),
	PINCTRL_PIN(133,  "MIO133"),
	PINCTRL_PIN(134,  "MIO134"),
	PINCTRL_PIN(135,  "MIO135"),
	PINCTRL_PIN(136,  "MIO136"),
	PINCTRL_PIN(137,  "MIO137"),
	PINCTRL_PIN(138,  "MIO138"),
	PINCTRL_PIN(139,  "MIO139"),
	PINCTRL_PIN(140,  "MIO140"),
	PINCTRL_PIN(141,  "MIO141"),
	PINCTRL_PIN(142,  "MIO142"),
	PINCTRL_PIN(143,  "MIO143"),
	PINCTRL_PIN(144,  "MIO144"),
	PINCTRL_PIN(145,  "MIO145"),
	PINCTRL_PIN(146,  "MIO146"),
	PINCTRL_PIN(147,  "MIO147"),
	PINCTRL_PIN(148,  "MIO148"),
	PINCTRL_PIN(149,  "MIO149"),
	PINCTRL_PIN(150,  "MIO150"),
	PINCTRL_PIN(151,  "MIO151"),
	PINCTRL_PIN(152,  "MIO152"),
	PINCTRL_PIN(153,  "MIO153"),
	PINCTRL_PIN(154,  "MIO154"),
	PINCTRL_PIN(155,  "MIO155"),
	PINCTRL_PIN(156,  "MIO156"),
};

static const unsigned int lpc_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static const unsigned int pcie_pins[] = {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
static const unsigned int spif_pins[] = {25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40};
static const unsigned int emmc_pins[] = {41, 42, 43, 44};
static const unsigned int sdio_pins[] = {45, 46, 47, 48};
static const unsigned int eth0_pins[] = {49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};
static const unsigned int pwm0_pins[] = {65};
static const unsigned int pwm1_pins[] = {66};
static const unsigned int pwm2_pins[] = {67};
static const unsigned int pwm3_pins[] = {68};
static const unsigned int fan0_pins[] = {69};
static const unsigned int fan1_pins[] = {70};
static const unsigned int fan2_pins[] = {71};
static const unsigned int fan3_pins[] = {72};
static const unsigned int i2c0_pins[] = {73, 74};
static const unsigned int i2c1_pins[] = {75, 76};
static const unsigned int i2c2_pins[] = {77, 78};
static const unsigned int i2c3_pins[] = {79, 80};
static const unsigned int uart0_pins[] = {81, 82, 83, 84};
static const unsigned int uart1_pins[] = {85, 86, 87, 88};
static const unsigned int uart2_pins[] = {89, 90, 91, 92};
static const unsigned int uart3_pins[] = {93, 94, 95, 96};
static const unsigned int spi0_pins[] = {97, 98, 99, 100, 101};
static const unsigned int spi1_pins[] = {102, 103, 104, 105, 106};
static const unsigned int jtag0_pins[] = {107, 108, 109, 110, 111, 112};
static const unsigned int jtag1_pins[] = {113, 114, 115, 116, 117, 118};
static const unsigned int jtag2_pins[] = {119, 120, 121, 122, 123, 124};
static const unsigned int gpio0_pins[] = {125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,\
					 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,\
					 151, 152, 153};
static const unsigned int dbgi2c_pins[] = {154, 155, 156};

static const char * const lpc_group[] = {"lpc_grp"};
static const char * const pcie_group[] = {"pcie_grp"};
static const char * const spif_group[] = {"spif_grp"};
static const char * const emmc_group[] = {"emmc_grp"};
static const char * const sdio_group[] = {"sdio_grp"};
static const char * const eth0_group[] = {"eth0_grp"};
static const char * const pwm0_group[] = {"pwm0_grp"};
static const char * const pwm1_group[] = {"pwm1_grp"};
static const char * const pwm2_group[] = {"pwm2_grp"};
static const char * const pwm3_group[] = {"pwm3_grp"};
static const char * const fan0_group[] = {"fan0_grp"};
static const char * const fan1_group[] = {"fan1_grp"};
static const char * const fan2_group[] = {"fan2_grp"};
static const char * const fan3_group[] = {"fan3_grp"};
static const char * const i2c0_group[] = {"i2c0_grp"};
static const char * const i2c1_group[] = {"i2c1_grp"};
static const char * const i2c2_group[] = {"i2c2_grp"};
static const char * const i2c3_group[] = {"i2c3_grp"};
static const char * const uart0_group[] = {"uart0_grp"};
static const char * const uart1_group[] = {"uart1_grp"};
static const char * const uart2_group[] = {"uart2_grp"};
static const char * const uart3_group[] = {"uart3_grp"};
static const char * const spi0_group[] = {"spi0_grp"};
static const char * const spi1_group[] = {"spi1_grp"};
static const char * const jtag0_group[] = {"jtag0_grp"};
static const char * const jtag1_group[] = {"jtag1_grp"};
static const char * const jtag2_group[] = {"jtag2_grp"};
static const char * const gpio0_group[] = {"gpio0_grp"};
static const char * const dbgi2c_group[] = {"dbgi2c_grp"};

static struct mango_group mango_groups[] = {
	PIN_GROUP(lpc),
	PIN_GROUP(pcie),
	PIN_GROUP(spif),
	PIN_GROUP(emmc),
	PIN_GROUP(sdio),
	PIN_GROUP(eth0),
	PIN_GROUP(pwm0),
	PIN_GROUP(pwm1),
	PIN_GROUP(pwm2),
	PIN_GROUP(pwm3),
	PIN_GROUP(fan0),
	PIN_GROUP(fan1),
	PIN_GROUP(fan2),
	PIN_GROUP(fan3),
	PIN_GROUP(i2c0),
	PIN_GROUP(i2c1),
	PIN_GROUP(i2c2),
	PIN_GROUP(i2c3),
	PIN_GROUP(uart0),
	PIN_GROUP(uart1),
	PIN_GROUP(uart2),
	PIN_GROUP(uart3),
	PIN_GROUP(spi0),
	PIN_GROUP(spi1),
	PIN_GROUP(jtag0),
	PIN_GROUP(jtag1),
	PIN_GROUP(jtag2),
	PIN_GROUP(gpio0),
	PIN_GROUP(dbgi2c),
};

static const struct mango_pmx_func mango_funcs[] = {
	FUNCTION(lpc_a, lpc, FUNC_MODE0),
	FUNCTION(lpc_r, lpc, FUNC_MODE1),
	FUNCTION(pcie_a, pcie, FUNC_MODE0),
	FUNCTION(pcie_r, pcie, FUNC_MODE1),
	FUNCTION(spif_a, spif, FUNC_MODE0),
	FUNCTION(spif_r, spif, FUNC_MODE1),
	FUNCTION(emmc_a, emmc, FUNC_MODE0),
	FUNCTION(emmc_r, emmc, FUNC_MODE1),
	FUNCTION(sdio_a, sdio, FUNC_MODE0),
	FUNCTION(sdio_r, sdio, FUNC_MODE1),
	FUNCTION(eth0_a, eth0, FUNC_MODE1),
	FUNCTION(eth0_r, eth0, FUNC_MODE0),
	FUNCTION(pwm0_a, pwm0, FUNC_MODE0),
	FUNCTION(pwm0_r, pwm0, FUNC_MODE1),
	FUNCTION(pwm1_a, pwm1, FUNC_MODE0),
	FUNCTION(pwm1_r, pwm1, FUNC_MODE1),
	FUNCTION(pwm2_a, pwm2, FUNC_MODE0),
	FUNCTION(pwm2_r, pwm2, FUNC_MODE1),
	FUNCTION(pwm3_a, pwm3, FUNC_MODE0),
	FUNCTION(pwm3_r, pwm3, FUNC_MODE1),
	FUNCTION(fan0_a, fan0, FUNC_MODE1),
	FUNCTION(fan0_r, fan0, FUNC_MODE0),
	FUNCTION(fan1_a, fan1, FUNC_MODE1),
	FUNCTION(fan1_r, fan1, FUNC_MODE0),
	FUNCTION(fan2_a, fan2, FUNC_MODE1),
	FUNCTION(fan2_r, fan2, FUNC_MODE0),
	FUNCTION(fan3_a, fan3, FUNC_MODE1),
	FUNCTION(fan3_r, fan3, FUNC_MODE0),
	FUNCTION(i2c0_a, i2c0, FUNC_MODE0),
	FUNCTION(i2c0_r, i2c0, FUNC_MODE1),
	FUNCTION(i2c1_a, i2c1, FUNC_MODE0),
	FUNCTION(i2c1_r, i2c1, FUNC_MODE1),
	FUNCTION(i2c2_a, i2c2, FUNC_MODE1),
	FUNCTION(i2c2_r, i2c2, FUNC_MODE0),
	FUNCTION(i2c3_a, i2c3, FUNC_MODE1),
	FUNCTION(i2c3_r, i2c3, FUNC_MODE0),
	FUNCTION(uart0_a, uart0, FUNC_MODE0),
	FUNCTION(uart0_r, uart0, FUNC_MODE1),
	FUNCTION(uart1_a, uart1, FUNC_MODE0),
	FUNCTION(uart1_r, uart1, FUNC_MODE1),
	FUNCTION(uart2_a, uart2, FUNC_MODE1),
	FUNCTION(uart2_r, uart2, FUNC_MODE0),
	FUNCTION(uart3_a, uart3, FUNC_MODE1),
	FUNCTION(uart3_r, uart3, FUNC_MODE0),
	FUNCTION(spi0_a, spi0, FUNC_MODE1),
	FUNCTION(spi0_r, spi0, FUNC_MODE0),
	FUNCTION(spi1_a, spi1, FUNC_MODE0),
	FUNCTION(spi1_r, spi1, FUNC_MODE1),
	FUNCTION(jtag0_a, jtag0, FUNC_MODE0),
	FUNCTION(jtag0_r, jtag0, FUNC_MODE1),
	FUNCTION(jtag1_a, jtag1, FUNC_MODE1),
	FUNCTION(jtag1_r, jtag1, FUNC_MODE0),
	FUNCTION(jtag2_a, jtag2, FUNC_MODE1),
	FUNCTION(jtag2_r, jtag2, FUNC_MODE0),
	FUNCTION(gpio0_a, gpio0, FUNC_MODE1),
	FUNCTION(gpio0_r, gpio0, FUNC_MODE0),
	FUNCTION(dbgi2c_a, dbgi2c, FUNC_MODE0),
	FUNCTION(dbgi2c_r, dbgi2c, FUNC_MODE1),
};

static struct device_attribute lpc_attr =	__ATTR(lpc, 0664, pinmux_show, pinmux_store);
static struct device_attribute pcie_attr =	__ATTR(pcie, 0664, pinmux_show, pinmux_store);
static struct device_attribute spif_attr =	__ATTR(spif, 0664, pinmux_show, pinmux_store);
static struct device_attribute emmc_attr =	__ATTR(emmc, 0664, pinmux_show, pinmux_store);
static struct device_attribute sdio_attr =	__ATTR(sdio, 0664, pinmux_show, pinmux_store);
static struct device_attribute eth0_attr =	__ATTR(eth0, 0664, pinmux_show, pinmux_store);
static struct device_attribute pwm0_attr =	__ATTR(pwm0, 0664, pinmux_show, pinmux_store);
static struct device_attribute pwm1_attr =	__ATTR(pwm1, 0664, pinmux_show, pinmux_store);
static struct device_attribute pwm2_attr =	__ATTR(pwm2, 0664, pinmux_show, pinmux_store);
static struct device_attribute pwm3_attr =	__ATTR(pwm3, 0664, pinmux_show, pinmux_store);
static struct device_attribute fan0_attr =	__ATTR(fan0, 0664, pinmux_show, pinmux_store);
static struct device_attribute fan1_attr =	__ATTR(fan1, 0664, pinmux_show, pinmux_store);
static struct device_attribute fan2_attr =	__ATTR(fan2, 0664, pinmux_show, pinmux_store);
static struct device_attribute fan3_attr =	__ATTR(fan3, 0664, pinmux_show, pinmux_store);
static struct device_attribute i2c0_attr =	__ATTR(i2c0, 0664, pinmux_show, pinmux_store);
static struct device_attribute i2c1_attr =	__ATTR(i2c1, 0664, pinmux_show, pinmux_store);
static struct device_attribute i2c2_attr =	__ATTR(i2c2, 0664, pinmux_show, pinmux_store);
static struct device_attribute i2c3_attr =	__ATTR(i2c3, 0664, pinmux_show, pinmux_store);
static struct device_attribute uart0_attr =	__ATTR(uart0, 0664, pinmux_show, pinmux_store);
static struct device_attribute uart1_attr =	__ATTR(uart1, 0664, pinmux_show, pinmux_store);
static struct device_attribute uart2_attr =	__ATTR(uart2, 0664, pinmux_show, pinmux_store);
static struct device_attribute uart3_attr =	__ATTR(uart3, 0664, pinmux_show, pinmux_store);
static struct device_attribute spi0_attr =	__ATTR(spi0, 0664, pinmux_show, pinmux_store);
static struct device_attribute spi1_attr =	__ATTR(spi1, 0664, pinmux_show, pinmux_store);
static struct device_attribute jtag0_attr =	__ATTR(jtag0, 0664, pinmux_show, pinmux_store);
static struct device_attribute jtag1_attr =	__ATTR(jtag1, 0664, pinmux_show, pinmux_store);
static struct device_attribute jtag2_attr =	__ATTR(jtag2, 0664, pinmux_show, pinmux_store);
static struct device_attribute gpio0_attr =	__ATTR(gpio0, 0664, pinmux_show, pinmux_store);
static struct device_attribute dbgi2c_attr =	__ATTR(dbgi2c, 0664, pinmux_show, pinmux_store);


static struct attribute *pinmux_attrs[] = {
	&lpc_attr.attr,
	&pcie_attr.attr,
	&spif_attr.attr,
	&emmc_attr.attr,
	&sdio_attr.attr,
	&eth0_attr.attr,
	&pwm0_attr.attr,
	&pwm1_attr.attr,
	&pwm2_attr.attr,
	&pwm3_attr.attr,
	&fan0_attr.attr,
	&fan1_attr.attr,
	&fan2_attr.attr,
	&fan3_attr.attr,
	&i2c0_attr.attr,
	&i2c1_attr.attr,
	&i2c2_attr.attr,
	&i2c3_attr.attr,
	&uart0_attr.attr,
	&uart1_attr.attr,
	&uart2_attr.attr,
	&uart3_attr.attr,
	&spi0_attr.attr,
	&spi1_attr.attr,
	&jtag0_attr.attr,
	&jtag1_attr.attr,
	&jtag2_attr.attr,
	&gpio0_attr.attr,
	&dbgi2c_attr.attr,
	NULL,
};
ATTRIBUTE_GROUPS(pinmux);

static struct class  pinmux_class = {
	.name = "pinmux",
	.dev_groups = pinmux_groups,
};

static struct mango_soc_pinctrl_data mango_pinctrl_data = {
	.pins = mango_pins,
	.npins = ARRAY_SIZE(mango_pins),
	.groups = mango_groups,
	.groups_count = ARRAY_SIZE(mango_groups),
	.functions = mango_funcs,
	.functions_count = ARRAY_SIZE(mango_funcs),
	.p_class = &pinmux_class,
};

static const struct of_device_id mango_pinctrl_of_table[] = {
	{
		.compatible = "sophgo, pinctrl-mango",
		.data = &mango_pinctrl_data,
	},
	{}
};

static int mango_pinctrl_probe(struct platform_device *pdev)
{
	return sophgo_pinctrl_probe(pdev);
}

static struct platform_driver mango_pinctrl_driver = {
	.probe = mango_pinctrl_probe,
	.driver = {
		.name = DRV_PINCTRL_NAME,
		.of_match_table = of_match_ptr(mango_pinctrl_of_table),
	},
};

static int __init mango_pinctrl_init(void)
{
	return platform_driver_register(&mango_pinctrl_driver);
}
postcore_initcall(mango_pinctrl_init);
