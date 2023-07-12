// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SPI Flash Master Controller (SPIFMC)
 *
 * Copyright (c) 2023 Sophgo.
 */
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>

/* Hardware register definitions */
#define SPIFMC_CTRL	0x00
#define SPIFMC_CTRL_CPHA	BIT(12)
#define SPIFMC_CTRL_CPOL	BIT(13)
#define SPIFMC_CTRL_HOLD_OL	BIT(14)
#define SPIFMC_CTRL_WP_OL	BIT(15)
#define SPIFMC_CTRL_LSBF	BIT(20)
#define SPIFMC_CTRL_SRST	BIT(21)
#define SPIFMC_CTRL_SCK_DIV_SHIFT	0
#define SPIFMC_CTRL_FRAME_LEN_SHIFT	16

#define SPIFMC_CE_CTRL	0x04
#define SPIFMC_CE_CTRL_CEMANUAL	BIT(0)
#define SPIFMC_CE_CTRL_CEMANUAL_EN	BIT(1)

#define SPIFMC_DLY_CTRL	0x08
#define SPIFMC_CTRL_FM_INTVL_MASK	0x000f
#define SPIFMC_CTRL_FM_INTVL	BIT(0)
#define SPIFMC_CTRL_CET_MASK	0x0f00
#define SPIFMC_CTRL_CET	BIT(8)

#define SPIFMC_DMMR	0x0c

#define SPIFMC_TRAN_CSR	0x10
#define SPIFMC_TRAN_CSR_TRAN_MODE_MASK	0x0003
#define SPIFMC_TRAN_CSR_TRAN_MODE_RX	BIT(0)
#define SPIFMC_TRAN_CSR_TRAN_MODE_TX	BIT(1)
#define SPIFMC_TRAN_CSR_CNTNS_READ	BIT(2)
#define SPIFMC_TRAN_CSR_FAST_MODE	BIT(3)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_1_BIT	(0x00 << 4)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_2_BIT	(0x01 << 4)
#define SPIFMC_TRAN_CSR_BUS_WIDTH_4_BIT	(0x02 << 4)
#define SPIFMC_TRAN_CSR_DMA_EN	BIT(6)
#define SPIFMC_TRAN_CSR_MISO_LEVEL	BIT(7)
#define SPIFMC_TRAN_CSR_ADDR_BYTES_MASK	0x0700
#define SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT	8
#define SPIFMC_TRAN_CSR_WITH_CMD	BIT(11)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_MASK	0x3000
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE	(0x00 << 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_2_BYTE	(0x01 << 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_4_BYTE	(0x02 << 12)
#define SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE	(0x03 << 12)
#define SPIFMC_TRAN_CSR_GO_BUSY	BIT(15)

#define SPIFMC_TRAN_NUM	0x14
#define SPIFMC_FIFO_PORT	0x18
#define SPIFMC_FIFO_PT	0x20

#define SPIFMC_INT_STS	0x28
#define SPIFMC_INT_TRAN_DONE	BIT(0)
#define SPIFMC_INT_RD_FIFO	BIT(2)
#define SPIFMC_INT_WR_FIFO	BIT(3)
#define SPIFMC_INT_RX_FRAME	BIT(4)
#define SPIFMC_INT_TX_FRAME	BIT(5)

#define SPIFMC_INT_EN	0x2c
#define SPIFMC_INT_TRAN_DONE_EN	BIT(0)
#define SPIFMC_INT_RD_FIFO_EN	BIT(2)
#define SPIFMC_INT_WR_FIFO_EN	BIT(3)
#define SPIFMC_INT_RX_FRAME_EN	BIT(4)
#define SPIFMC_INT_TX_FRAME_EN	BIT(5)

#define SPIFMC_MAX_FIFO_DEPTH	8

struct sophgo_spifmc {
	struct device *dev;
	struct clk *clk;
	void __iomem *io_base;
	struct spi_nor nor;
};

static inline int sophgo_spifmc_wait_int(struct sophgo_spifmc *spifmc,
		u8 int_type)
{
	u32 stat;

	return readl_poll_timeout(spifmc->io_base + SPIFMC_INT_STS, stat,
			(stat & int_type), 0, 0);
}

static inline u32 sophgo_spifmc_init_reg(struct sophgo_spifmc *spifmc)
{
	u32 reg;

	reg = readl(spifmc->io_base + SPIFMC_TRAN_CSR);
	reg &= ~(SPIFMC_TRAN_CSR_TRAN_MODE_MASK
			| SPIFMC_TRAN_CSR_CNTNS_READ
			| SPIFMC_TRAN_CSR_FAST_MODE
			| SPIFMC_TRAN_CSR_BUS_WIDTH_2_BIT
			| SPIFMC_TRAN_CSR_BUS_WIDTH_4_BIT
			| SPIFMC_TRAN_CSR_DMA_EN
			| SPIFMC_TRAN_CSR_ADDR_BYTES_MASK
			| SPIFMC_TRAN_CSR_WITH_CMD
			| SPIFMC_TRAN_CSR_FIFO_TRG_LVL_MASK);

	return reg;
}

/*
 * sophgo_spifmc_read_reg is a workaround function:
 * AHB bus could only do 32-bit access to SPIFMC fifo,
 * so cmd without 3-byte addr will leave 3-byte data in fifo.
 * Set TX to mark that these 3-byte data would be sent out.
 */
static int sophgo_spifmc_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf,
				 size_t len)
{
	struct sophgo_spifmc *spifmc = nor->priv;
	u32 reg;
	int ret, i;

	reg = sophgo_spifmc_init_reg(spifmc);
	reg |= SPIFMC_TRAN_CSR_BUS_WIDTH_1_BIT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	reg |= SPIFMC_TRAN_CSR_TRAN_MODE_RX | SPIFMC_TRAN_CSR_TRAN_MODE_TX;
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = 0; i < len; i++)
		writeb(0, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	ret = sophgo_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	if (ret)
		return ret;

	while (len--)
		*buf++ = readb(spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return 0;
}

static int sophgo_spifmc_write_reg(struct spi_nor *nor, u8 opcode,
				  const u8 *buf, size_t len)
{
	struct sophgo_spifmc *spifmc = nor->priv;
	u32 reg;
	int i;

	reg = sophgo_spifmc_init_reg(spifmc);
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;

	/*
	 * If write values to the Status Register,
	 * configure TRAN_CSR register as the same as sophgo_spifmc_read_reg.
	 */
	if (opcode == SPINOR_OP_WRSR) {
		reg |= SPIFMC_TRAN_CSR_TRAN_MODE_RX | SPIFMC_TRAN_CSR_TRAN_MODE_TX;
		writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
	}

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = 0; i < len; i++)
		writeb(buf[i], spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);
	sophgo_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return 0;
}

static ssize_t sophgo_spifmc_read(struct spi_nor *nor, loff_t from,
		size_t len, u_char *buf)
{
	struct sophgo_spifmc *spifmc = nor->priv;
	u32 reg;
	int xfer_size, offset;
	int i;

	reg = sophgo_spifmc_init_reg(spifmc);
	reg |= (nor->addr_nbytes) << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	reg |= SPIFMC_TRAN_CSR_TRAN_MODE_RX;
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(nor->read_opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = nor->addr_nbytes - 1; i >= 0; i--)
		writeb((from >> i * 8) & 0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);
	sophgo_spifmc_wait_int(spifmc, SPIFMC_INT_RD_FIFO);

	offset = 0;
	while (offset < len) {
		xfer_size = min_t(size_t, SPIFMC_MAX_FIFO_DEPTH, len - offset);

		while ((readl(spifmc->io_base + SPIFMC_FIFO_PT) & 0xf) != xfer_size)
			;

		for (i = 0; i < xfer_size; i++)
			buf[i + offset] = readb(spifmc->io_base + SPIFMC_FIFO_PORT);

		offset += xfer_size;
	}

	sophgo_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return len;
}

static ssize_t sophgo_spifmc_write(struct spi_nor *nor, loff_t to,
		size_t len, const u_char *buf)
{
	struct sophgo_spifmc *spifmc = nor->priv;
	u32 reg;
	int i, offset;
	int xfer_size, wait;

	reg = sophgo_spifmc_init_reg(spifmc);
	reg |= nor->addr_nbytes << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_8_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	reg |= SPIFMC_TRAN_CSR_TRAN_MODE_TX;
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(nor->program_opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = nor->addr_nbytes - 1; i >= 0; i--)
		writeb((to >> i * 8) & 0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	writel(len, spifmc->io_base + SPIFMC_TRAN_NUM);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);

	while ((readl(spifmc->io_base + SPIFMC_FIFO_PT) & 0xf) != 0)
		;

	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	offset = 0;
	while (offset < len) {
		xfer_size = min_t(size_t, SPIFMC_MAX_FIFO_DEPTH, len - offset);

		wait = 0;
		while ((readl(spifmc->io_base + SPIFMC_FIFO_PT) & 0xf) != 0) {
			wait++;
			udelay(10);
			if (wait > 30000) {
				dev_warn(spifmc->dev, "Wait to write FIFO timeout.\n");
				return -1;
			}
		}

		for (i = 0; i < xfer_size; i++)
			writeb(buf[i + offset], spifmc->io_base + SPIFMC_FIFO_PORT);

		offset += xfer_size;
	}

	sophgo_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return len;
}

static int sophgo_spifmc_erase(struct spi_nor *nor, loff_t offs)
{
	struct sophgo_spifmc *spifmc = nor->priv;
	u32 reg;
	int i;

	reg = sophgo_spifmc_init_reg(spifmc);
	reg |= nor->addr_nbytes << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_1_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);
	writeb(nor->erase_opcode, spifmc->io_base + SPIFMC_FIFO_PORT);

	for (i = nor->addr_nbytes - 1; i >= 0; i--)
		writeb((offs >> i * 8) & 0xff, spifmc->io_base + SPIFMC_FIFO_PORT);

	writel(0, spifmc->io_base + SPIFMC_INT_STS);
	reg |= SPIFMC_TRAN_CSR_GO_BUSY;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);
	sophgo_spifmc_wait_int(spifmc, SPIFMC_INT_TRAN_DONE);
	writel(0, spifmc->io_base + SPIFMC_FIFO_PT);

	return 0;
}

static const struct spi_nor_controller_ops sophgo_spifmc_controller_ops = {
	.read_reg  = sophgo_spifmc_read_reg,
	.write_reg = sophgo_spifmc_write_reg,
	.read  = sophgo_spifmc_read,
	.write = sophgo_spifmc_write,
	.erase = sophgo_spifmc_erase,
};

static void sophgo_spifmc_init(struct sophgo_spifmc *spifmc)
{
	u32 reg;

	/* disable DMMR (Direct Memory Mapping Read) */
	writel(0, spifmc->io_base + SPIFMC_DMMR);
	/* soft reset */
	writel(readl(spifmc->io_base + SPIFMC_CTRL) | SPIFMC_CTRL_SRST | 0x3,
			spifmc->io_base + SPIFMC_CTRL);
	/* hardware CE contrl, soft reset cannot change the register */
	writel(0, spifmc->io_base + SPIFMC_CE_CTRL);
	reg = spifmc->nor.addr_nbytes << SPIFMC_TRAN_CSR_ADDR_BYTES_SHIFT;
	reg |= SPIFMC_TRAN_CSR_FIFO_TRG_LVL_4_BYTE;
	reg |= SPIFMC_TRAN_CSR_WITH_CMD;
	writel(reg, spifmc->io_base + SPIFMC_TRAN_CSR);
}

static int sophgo_spifmc_register(struct device_node *np,
				struct sophgo_spifmc *spifmc)
{
	/* TODO: support DUAL and QUAD operations */
	const struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_PP,
	};
	int ret;

	spifmc->nor.dev = spifmc->dev;
	spi_nor_set_flash_node(&spifmc->nor, np);
	spifmc->nor.priv = spifmc;
	spifmc->nor.controller_ops = &sophgo_spifmc_controller_ops;

	ret = spi_nor_scan(&spifmc->nor, NULL, &hwcaps);
	if (ret) {
		dev_err(spifmc->dev, "Device scan failed.\n");
		return ret;
	}

	ret = mtd_device_register(&spifmc->nor.mtd, NULL, 0);
	if (ret) {
		dev_err(spifmc->dev, "mtd device parse failed.\n");
		return ret;
	}

	return 0;
}

static int sophgo_spifmc_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct sophgo_spifmc *spifmc;
	int ret;

	spifmc = devm_kzalloc(&pdev->dev, sizeof(*spifmc), GFP_KERNEL);
	if (!spifmc)
		return -ENOMEM;

	spifmc->io_base = devm_platform_ioremap_resource_byname(pdev, "memory");
	if (IS_ERR(spifmc->io_base))
		return PTR_ERR(spifmc->io_base);

	spifmc->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(spifmc->clk)) {
		dev_err(&pdev->dev, "AHB clock not found.\n");
		return PTR_ERR(spifmc->clk);
	}

	ret = clk_prepare_enable(spifmc->clk);
	if (ret) {
		dev_err(&pdev->dev, "Unable to enable AHB clock.\n");
		return ret;
	}

	spifmc->dev = &pdev->dev;
	platform_set_drvdata(pdev, spifmc);
	sophgo_spifmc_init(spifmc);

	np = of_get_next_available_child(pdev->dev.of_node, NULL);
	if (!np) {
		dev_err(&pdev->dev, "No SPI flash device to configure.\n");
		ret = -ENODEV;
		goto fail;
	}

	ret = sophgo_spifmc_register(np, spifmc);
	of_node_put(np);
	if (ret) {
		dev_err(&pdev->dev, "Unable to register spifmc.\n");
		goto fail;
	}

	return ret;
fail:
	clk_disable_unprepare(spifmc->clk);
	return ret;
}

static int sophgo_spifmc_remove(struct platform_device *pdev)
{
	struct sophgo_spifmc *spifmc = platform_get_drvdata(pdev);

	mtd_device_unregister(&spifmc->nor.mtd);
	clk_disable_unprepare(spifmc->clk);

	return 0;
}

static const struct of_device_id sophgo_spifmc_match[] = {
	{.compatible = "sophgo,spifmc"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sophgo_spifmc_match);

static struct platform_driver sophgo_spifmc_driver = {
	.probe	= sophgo_spifmc_probe,
	.remove	= sophgo_spifmc_remove,
	.driver	= {
		.name = "sophgo-spifmc",
		.of_match_table = sophgo_spifmc_match,
	},
};
module_platform_driver(sophgo_spifmc_driver);

MODULE_DESCRIPTION("Sophgo SPI Flash Master Controller Driver");
MODULE_LICENSE("GPL v2");
