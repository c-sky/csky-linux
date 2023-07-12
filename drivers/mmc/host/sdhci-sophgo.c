/*
 * Sophgo SDHCI Platform driver
 *
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include "sdhci-pltfm.h"
#include "sdhci-sophgo.h"

#define DRIVER_NAME "bm"

#define BM_SDHCI_VENDOR_OFFSET		0x500
#define BM_SDHCI_VENDOR_MSHC_CTRL_R (BM_SDHCI_VENDOR_OFFSET + 0x8)
#define BM_SDHCI_VENDOR_A_CTRL_R (BM_SDHCI_VENDOR_OFFSET + 0x40)
#define BM_SDHCI_VENDOR_A_STAT_R (BM_SDHCI_VENDOR_OFFSET + 0x44)

static void bm_sdhci_set_tap(struct sdhci_host *host, unsigned int tap)
{
	sdhci_writel(host, 0x0, BM_SDHCI_VENDOR_MSHC_CTRL_R);
	sdhci_writel(host, 0x18, BM_SDHCI_VENDOR_A_CTRL_R);
	sdhci_writel(host, tap, BM_SDHCI_VENDOR_A_STAT_R);
}

static int sdhci_bm_execute_software_tuning(struct sdhci_host *host, u32 opcode)
{
	unsigned int maxwidth = 0;
	unsigned int tuntap;
	struct {
		unsigned int start;
		unsigned int end;
		unsigned int width;
	} tunlist[4];
	unsigned int listcount;
	unsigned int listsel;

	unsigned int tun = 0;
	unsigned int max = 256;
	int i;

	listcount = 0;
	for (i = 0; i < ARRAY_SIZE(tunlist); i++) {
		while (tun < max) {
			bm_sdhci_set_tap(host, tun);
			if (!mmc_send_tuning(host->mmc, opcode, NULL))
				break;
			tun++;
		}
		tunlist[i].start = tun;
		tun++;
		while (tun < max) {
			bm_sdhci_set_tap(host, tun);
			if (mmc_send_tuning(host->mmc, opcode, NULL))
				break;
			tun++;
		}
		tunlist[i].end = tun-1;
		tunlist[i].width = tunlist[i].end - tunlist[i].start;
		listcount++;
		tun++;
		pr_info("%s id:%d start:%d end:%d width:%d\n", mmc_hostname(host->mmc),
			i, tunlist[i].start, tunlist[i].end, tunlist[i].width);
		if (tun >= max)
			break;
	}

	//find maxwidth
	listsel = 0;
	for (i = 0; i < listcount; i++) {
		if (tunlist[i].width > maxwidth) {
			maxwidth = tunlist[i].width;
			listsel = i;
		}
	}
	tuntap = tunlist[listsel].start + (tunlist[listsel].width/2);

	/* The TRM states the ideal tap value is at 75% in the passing range. */
	bm_sdhci_set_tap(host, tuntap);
	pr_info("%s listsel:%d tuntap:%d\n",
		mmc_hostname(host->mmc), listsel, tuntap);

	return mmc_send_tuning(host->mmc, opcode, NULL);
}

static int sdhci_bm_select_drive_strength(struct mmc_card *card,
		unsigned int max_dtr, int host_drv,
		int card_drv, int *drv_type)
{
	struct sdhci_host *host = mmc_priv(card->host);
	struct mmc_host *mmc = host->mmc;
	uint32_t reg;
	int driver_type;

	pr_info("%s max_dtr %d, host_drv %d, card_drv %d, drv_type %d\n",
		mmc_hostname(mmc),
		max_dtr, host_drv, card_drv, *drv_type);

	driver_type = MMC_SET_DRIVER_TYPE_C;
	*drv_type = MMC_SET_DRIVER_TYPE_C;

	reg = (1 << PHY_CNFG_PHY_PWRGOOD) | (0xe << PHY_CNFG_PAD_SP) |
		(0xe << PHY_CNFG_PAD_SN) | (1 << PHY_CNFG_PHY_RSTN);
	sdhci_writel(host, reg, SDHCI_P_PHY_CNFG);

	return driver_type;
}

static void sdhci_bm_set_uhs_signaling(struct sdhci_host *host, unsigned int uhs)
{
	struct mmc_host *mmc = host->mmc;
	u16 ctrl_2;

	ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
	switch (uhs) {
	case MMC_TIMING_UHS_SDR12:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
		break;
	case MMC_TIMING_UHS_SDR25:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
		break;
	case MMC_TIMING_UHS_SDR50:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
		break;
	case MMC_TIMING_MMC_HS200:
	case MMC_TIMING_UHS_SDR104:
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
		break;
	case MMC_TIMING_UHS_DDR50:
	case MMC_TIMING_MMC_DDR52:
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
		break;
	}

	/*
	 * When clock frequency is less than 100MHz, the feedback clock must be
	 * provided and DLL must not be used so that tuning can be skipped. To
	 * provide feedback clock, the mode selection can be any value less
	 * than 3'b011 in bits [2:0] of HOST CONTROL2 register.
	 */
	if (host->clock <= 100000000 &&
	    (uhs == MMC_TIMING_MMC_HS400 ||
	     uhs == MMC_TIMING_MMC_HS200 ||
	     uhs == MMC_TIMING_UHS_SDR104))
		ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;

	dev_dbg(mmc_dev(mmc), "%s: clock=%u uhs=%u ctrl_2=0x%x\n",
		mmc_hostname(host->mmc), host->clock, uhs, ctrl_2);
	sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);
}

static unsigned int bm_sdhci_get_min_clock(struct sdhci_host *host)
{
	return 200 * 1000;
}

static unsigned int bm_sdhci_get_max_clock(struct sdhci_host *host)
{
	return 50 * 1000 * 1000;
}

#if 0 // FIXME, SD card not working after this.
static void bm_sdhci_hw_reset(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_bm_host *bm_host;
	struct mmc_host *mmc = host->mmc;

	pltfm_host = sdhci_priv(host);
	bm_host = sdhci_pltfm_priv(pltfm_host);

	pr_info("%s hardware reset\n", mmc_hostname(mmc));
	reset_control_assert(bm_host->reset);
	udelay(10);
	reset_control_deassert(bm_host->reset);
}
#endif

void bm_sdhci_reset(struct sdhci_host *host, u8 mask)
{
#if 0 // FIXME, SD card not working after this.
	bm_sdhci_hw_reset(host);
#endif
	sdhci_reset(host, mask);

	if (mask & SDHCI_RESET_ALL)
		bm_sdhci_phy_init(host);
}

int bm_sdhci_phy_init(struct sdhci_host *host)
{
	// Asset reset of phy
	sdhci_writel(host, sdhci_readl(host, SDHCI_P_PHY_CNFG) & ~(1 << PHY_CNFG_PHY_RSTN), SDHCI_P_PHY_CNFG);

	// Set PAD_SN PAD_SP
	sdhci_writel(host,
		     (1 << PHY_CNFG_PHY_PWRGOOD) | (0x9 << PHY_CNFG_PAD_SP) | (0x8 << PHY_CNFG_PAD_SN),
		     SDHCI_P_PHY_CNFG);

	// Set CMDPAD
	sdhci_writew(host, (0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N), SDHCI_P_CMDPAD_CNFG);

	// Set DATAPAD
	sdhci_writew(host, (0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N), SDHCI_P_DATPAD_CNFG);

	// Set CLKPAD
	sdhci_writew(host,
		     (0x2 << PAD_CNFG_RXSEL) | (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N),
		     SDHCI_P_CLKPAD_CNFG);

	// Set STB_PAD
	sdhci_writew(host, (0x2 << PAD_CNFG_RXSEL) | (0x2 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N), SDHCI_P_STBPAD_CNFG);

	// Set RSTPAD
	sdhci_writew(host, (0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
			(0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N), SDHCI_P_RSTNPAD_CNFG);

	// Set SDCLKDL_CNFG, EXTDLY_EN = 1, fix delay
	sdhci_writeb(host, (1 << SDCLKDL_CNFG_EXTDLY_EN), SDHCI_P_SDCLKDL_CNFG);

	// Add 10 * 70ps = 0.7ns for output delay
	sdhci_writeb(host, 10, SDHCI_P_SDCLKDL_DC);

	//if (host->index == 1) {
	//	 Set SMPLDL_CNFG, Bypass
	sdhci_writeb(host, (1 << SMPLDL_CNFG_BYPASS_EN), SDHCI_P_SMPLDL_CNFG);
	//}
	//else {
	//	Set SMPLDL_CNFG, INPSEL_CNFG = 0x2
	//sdhci_writeb(host, (0x2 << SMPLDL_CNFG_INPSEL_CNFG), SDHCI_P_SMPLDL_CNFG);
	//}

	// Set ATDL_CNFG, tuning clk not use for init
	sdhci_writeb(host, (2 << ATDL_CNFG_INPSEL_CNFG), SDHCI_P_ATDL_CNFG);

	// deasset reset of phy
	sdhci_writel(host, sdhci_readl(host, SDHCI_P_PHY_CNFG) | (1 << PHY_CNFG_PHY_RSTN), SDHCI_P_PHY_CNFG);

	return 0;
}

void bm_sdhci_set_clock(struct sdhci_host *host, unsigned int clock)
{
	sdhci_set_clock(host, clock);

	if (clock == 0)
		// forward tx
		sdhci_writeb(host, 0x0, SDHCI_P_SDCLKDL_DC);
	else
		// revert tx
		sdhci_writeb(host, 0x10, SDHCI_P_SDCLKDL_DC);
}

/*
 * If DMA addr spans 128MB boundary, we split the DMA transfer into two
 * so that each DMA transfer doesn't exceed the boundary.
 */
#define BOUNDARY_OK(addr, len) \
	((addr | (SZ_128M - 1)) == ((addr + len - 1) | (SZ_128M - 1)))

static void dwcmshc_adma_write_desc(struct sdhci_host *host, void **desc,
				    dma_addr_t addr, int len, unsigned int cmd)
{
	int tmplen, offset;

	if (likely(!len || BOUNDARY_OK(addr, len))) {
		sdhci_adma_write_desc(host, desc, addr, len, cmd);
		return;
	}

	offset = addr & (SZ_128M - 1);
	tmplen = SZ_128M - offset;
	sdhci_adma_write_desc(host, desc, addr, tmplen, cmd);

	addr += tmplen;
	len -= tmplen;
	sdhci_adma_write_desc(host, desc, addr, len, cmd);
}


/* ------------- bm palludium sdcard --------------- */
static const struct sdhci_ops sdhci_bm_pldm_sd_ops = {
	.reset = bm_sdhci_reset,
	.set_clock = bm_sdhci_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.set_uhs_signaling = sdhci_bm_set_uhs_signaling,
	.get_max_clock = bm_sdhci_get_max_clock,
	.get_min_clock = bm_sdhci_get_min_clock,
	.adma_write_desc = dwcmshc_adma_write_desc,
};

static const struct sdhci_pltfm_data sdhci_bm_pldm_sd_pdata = {
	.ops = &sdhci_bm_pldm_sd_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN | SDHCI_QUIRK_INVERTED_WRITE_PROTECT,
	.quirks2 = SDHCI_QUIRK2_NO_1_8_V,
};

static inline bool sdhci_data_line_cmd(struct mmc_command *cmd)
{
	return cmd->data || cmd->flags & MMC_RSP_BUSY;
}

static void sdhci_del_timer(struct sdhci_host *host, struct mmc_request *mrq)
{
	if (sdhci_data_line_cmd(mrq->cmd))
		del_timer(&host->data_timer);
	else
		del_timer(&host->timer);
}

int bm_platform_execute_tuning(struct sdhci_host *host, u32 opcode)
{
	u16 ctrl;
	int tuning_loop_counter = 0;
	int err = 0;
	unsigned long flags;
	unsigned int tuning_count = 0;
	bool hs400_tuning;
	int hit = 0;

	spin_lock_irqsave(&host->lock, flags);

	hs400_tuning = host->flags & SDHCI_HS400_TUNING;
	host->flags &= ~SDHCI_HS400_TUNING;

	if (host->tuning_mode == SDHCI_TUNING_MODE_1)
		tuning_count = host->tuning_count;

	switch (host->timing) {
	/* HS400 tuning is done in HS200 mode */
	case MMC_TIMING_MMC_HS400:
		err = -EINVAL;
		goto out_unlock;

	case MMC_TIMING_MMC_HS200:
		/*
		 * Periodic re-tuning for HS400 is not expected to be needed, so
		 * disable it here.
		 */
		if (hs400_tuning)
			tuning_count = 0;
		break;

	case MMC_TIMING_UHS_SDR104:
	case MMC_TIMING_UHS_DDR50:
		break;

	case MMC_TIMING_UHS_SDR50:
		if (host->flags & SDHCI_SDR50_NEEDS_TUNING)
			break;

	default:
		goto out_unlock;
	}

	ctrl = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	ctrl |= SDHCI_CTRL_EXEC_TUNING;

	sdhci_writel(host, SDHCI_INT_DATA_AVAIL, SDHCI_INT_ENABLE);
	sdhci_writel(host, SDHCI_INT_DATA_AVAIL, SDHCI_SIGNAL_ENABLE);

	sdhci_writew(host, 0x704b | (0x3<<4) | (0x1<<3), SDHCI_HOST_CONTROL2);/*drv_strength | 1.8v*/

	sdhci_writel(host, 0, SDHCI_DMA_ADDRESS);/*sdmasa*/
	sdhci_writel(host, 0, SDHCI_MSHC_CTRL);

	sdhci_writel(host, 0x18, SDHCI_AT_CTRL);

	sdhci_writew(host, 0x0, SDHCI_BLOCK_COUNT);
	sdhci_writew(host, 0x1040, SDHCI_BLOCK_SIZE);
	sdhci_writew(host, SDHCI_TRNS_READ, SDHCI_TRANSFER_MODE);

	do {
		struct mmc_command cmd = {0};
		struct mmc_request mrq = {NULL};

		cmd.opcode = opcode;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
		cmd.retries = 0;
		cmd.data = NULL;
		cmd.mrq = &mrq;
		cmd.error = 0;

		sdhci_writel(host, tuning_loop_counter, SDHCI_AT_STAT);
		mrq.cmd = &cmd;
		sdhci_send_command(host, &cmd);

		host->cmd = NULL;
		sdhci_del_timer(host, &mrq);
		spin_unlock_irqrestore(&host->lock, flags);

		/* Wait for Buffer Read Ready interrupt */
		wait_event_timeout(host->buf_ready_int,
					(host->tuning_done == 1),
					msecs_to_jiffies(10));

		spin_lock_irqsave(&host->lock, flags);
		if (host->tuning_done == 1) {
			u16 stat;

			stat = sdhci_readw(host, SDHCI_ERR_INT_STATUS) & 0x3F;
			if (stat == 0)
				hit = tuning_loop_counter;
		}

		host->tuning_done = 0;
		tuning_loop_counter++;
		sdhci_writeb(host, 0xFF, SDHCI_INT_STATUS);
		sdhci_writeb(host, 0xFF, SDHCI_ERR_INT_STATUS);
		sdhci_writeb(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA, SDHCI_SOFTWARE_RESET);
	} while (tuning_loop_counter < MAX_TUNING_STEP);

	if (tuning_loop_counter >= MAX_TUNING_STEP) {
		ctrl &= ~(SDHCI_CTRL_TUNED_CLK | SDHCI_CTRL_EXEC_TUNING);
		sdhci_writew(host, ctrl, SDHCI_HOST_CONTROL2);
	}

	sdhci_writel(host, 0, SDHCI_AT_CTRL);
	sdhci_writeb(host, 0xFF, SDHCI_INT_STATUS);/*clear normal int*/
	sdhci_writeb(host, 0xFF, SDHCI_ERR_INT_STATUS);/*clear error int*/
	sdhci_writel(host, sdhci_readl(host, SDHCI_AT_CTRL) | (0x1<<4), SDHCI_AT_CTRL);/*en sw_tuning_en bit*/
	sdhci_writel(host, (sdhci_readl(host, SDHCI_AT_STAT) & (~0xFF)) | hit, SDHCI_AT_STAT);/*center_ph_code*/
	sdhci_writel(host, sdhci_readl(host, SDHCI_AT_CTRL) & (~(0x1<<4)), SDHCI_AT_CTRL);/*dis sw_tuning_en bit*/
	sdhci_writeb(host, SDHCI_RESET_CMD | SDHCI_RESET_DATA, SDHCI_SOFTWARE_RESET);

	if (tuning_count)
		err = 0;

	host->mmc->retune_period = err ? 0 : tuning_count;

	sdhci_writel(host, host->ier, SDHCI_INT_ENABLE);
	sdhci_writel(host, host->ier, SDHCI_SIGNAL_ENABLE);
out_unlock:
	spin_unlock_irqrestore(&host->lock, flags);
	return err;
}

/* ------------- bm palludium emmc --------------- */
static const struct sdhci_ops sdhci_bm_pldm_emmc_ops = {
	.reset = sdhci_reset,
	.set_clock = sdhci_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.set_uhs_signaling = sdhci_bm_set_uhs_signaling,
	.get_max_clock = bm_sdhci_get_max_clock,
	.get_min_clock = bm_sdhci_get_min_clock,
	.platform_execute_tuning = bm_platform_execute_tuning,
	.adma_write_desc = dwcmshc_adma_write_desc,
};

static const struct sdhci_pltfm_data sdhci_bm_pldm_emmc_pdata = {
	.ops = &sdhci_bm_pldm_emmc_ops,
	.quirks = SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
};

/* ------------ bm asic ------------ */
static const struct sdhci_ops sdhci_bm_ops = {
	.reset = bm_sdhci_reset,
	.set_clock = sdhci_set_clock,
	.set_bus_width = sdhci_set_bus_width,
	.set_uhs_signaling = sdhci_bm_set_uhs_signaling,
	.platform_execute_tuning = sdhci_bm_execute_software_tuning,
	.adma_write_desc = dwcmshc_adma_write_desc,
};

static const struct sdhci_pltfm_data sdhci_bm_emmc_pdata = {
	.ops = &sdhci_bm_ops,
	.quirks = SDHCI_QUIRK_INVERTED_WRITE_PROTECT,
	.quirks2 = 0,
};

static const struct sdhci_pltfm_data sdhci_bm_sd_pdata = {
	.ops = &sdhci_bm_ops,
	.quirks = SDHCI_QUIRK_INVERTED_WRITE_PROTECT,
	.quirks2 = 0,
};

static const struct of_device_id sdhci_bm_dt_match[] = {
	{.compatible = "bitmain,bm-pldm-sdcard", .data = &sdhci_bm_pldm_sd_pdata},
	{.compatible = "bitmain,bm-pldm-emmc", .data = &sdhci_bm_pldm_emmc_pdata},
	{.compatible = "bitmain,bm-emmc", .data = &sdhci_bm_emmc_pdata},
	{.compatible = "bitmain,bm-sd", .data = &sdhci_bm_sd_pdata},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, sdhci_bm_dt_match);

static int sdhci_bm_probe(struct platform_device *pdev)
{
	struct sdhci_host *host;
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_bm_host *bm_host;
	const struct of_device_id *match;
	const struct sdhci_pltfm_data *pdata;
	int ret;

	match = of_match_device(sdhci_bm_dt_match, &pdev->dev);
	if (!match)
		return -EINVAL;

	pdata = match->data;

	host = sdhci_pltfm_init(pdev, pdata, sizeof(*bm_host));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);
	bm_host = sdhci_pltfm_priv(pltfm_host);
	bm_host->mmc = host->mmc;
	bm_host->pdev = pdev;
	bm_host->core_mem = host->ioaddr;

	ret = mmc_of_parse(host->mmc);
	if (ret)
		goto pltfm_free;

	sdhci_get_of_property(pdev);

	if (host->mmc->caps2 & MMC_CAP2_NO_SD) {
		bm_host->reset = devm_reset_control_get(&pdev->dev, "emmc");
		if (IS_ERR(bm_host->reset)) {
			ret = PTR_ERR(bm_host->reset);
			goto pltfm_free;
		}

		bm_host->clkaxi = devm_clk_get(&pdev->dev, "clk_gate_axi_emmc");
		if (IS_ERR(bm_host->clkaxi))
			dev_err(&pdev->dev, "get emmc clk axi failed\n");
		else
			clk_prepare_enable(bm_host->clkaxi);
		bm_host->clk = devm_clk_get(&pdev->dev, "clk_gate_emmc");
		if (IS_ERR(bm_host->clk))
			dev_err(&pdev->dev, "get emmc clk failed\n");
		else
			clk_prepare_enable(bm_host->clk);
		bm_host->clk100k = devm_clk_get(&pdev->dev, "clk_gate_100k_emmc");
		if (IS_ERR(bm_host->clk100k))
			dev_err(&pdev->dev, "get emmc clk 100k failed\n");
		else
			clk_prepare_enable(bm_host->clk100k);
	} else if (host->mmc->caps2 & MMC_CAP2_NO_MMC) {
		bm_host->reset = devm_reset_control_get(&pdev->dev, "sdio");
		if (IS_ERR(bm_host->reset)) {
			ret = PTR_ERR(bm_host->reset);
			goto pltfm_free;
		}

		bm_host->clkaxi = devm_clk_get(&pdev->dev, "clk_gate_axi_sd");
		if (IS_ERR(bm_host->clkaxi))
			dev_err(&pdev->dev, "get sd clk axi failed\n");
		else
			clk_prepare_enable(bm_host->clkaxi);
		bm_host->clk = devm_clk_get(&pdev->dev, "clk_gate_sd");
		if (IS_ERR(bm_host->clk))
			dev_err(&pdev->dev, "get sd clk failed\n");
		else
			clk_prepare_enable(bm_host->clk);
		bm_host->clk100k = devm_clk_get(&pdev->dev, "clk_gate_100k_sd");
		if (IS_ERR(bm_host->clk100k))
			dev_err(&pdev->dev, "get sd clk 100k failed\n");
		else
			clk_prepare_enable(bm_host->clk100k);
	}

	host->mmc_host_ops.select_drive_strength = sdhci_bm_select_drive_strength;

	ret = sdhci_add_host(host);
	if (ret)
		goto err_add_host;

	return 0;

err_add_host:
pltfm_free:
	sdhci_pltfm_free(pdev);
	return ret;
}

static int sdhci_bm_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	int dead = (readl_relaxed(host->ioaddr + SDHCI_INT_STATUS) == 0xffffffff);

	sdhci_remove_host(host, dead);
	sdhci_pltfm_free(pdev);
	return 0;
}

static struct platform_driver sdhci_bm_driver = {
	.probe = sdhci_bm_probe,
	.remove = sdhci_bm_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = sdhci_bm_dt_match,
	},
};

module_platform_driver(sdhci_bm_driver);
MODULE_DESCRIPTION("BitMain Secure Digital Host Controller Interface driver");
MODULE_LICENSE("GPL");
