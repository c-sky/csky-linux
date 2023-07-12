/*
 * drivers/mmc/host/sdhci-bm.c - BitMain SDHCI Platform driver
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

#ifndef __SDHCI_BM_H
#define __SDHCI_BM_H

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>

/*register macro */
#define P_VENDOR_SPECIFIC_AREA		0xE8
#define P_VENDOR2_SPECIFIC_AREA		0xEA
#define VENDOR_EMMC_CTRL		0x2C
#define SW_RST_R			0x2F
#define SDHCI_NORMAL_INT_STATUS		0x30
#define SDHCI_ERR_INT_STATUS		0x32
#define SDHCI_ERR_INT_STATUS_EN		0x36
#define SDHCI_HOST_CTRL2_R		0x3E
#define SDHCI_MSHC_CTRL			0x508
#define SDHCI_AT_CTRL			0x540
#define SDHCI_AT_STAT			0x544

/* PHY register */
#define SDHCI_PHY_R_OFFSET			0x300

#define SDHCI_P_PHY_CNFG           (SDHCI_PHY_R_OFFSET + 0x00)
#define SDHCI_P_CMDPAD_CNFG        (SDHCI_PHY_R_OFFSET + 0x04)
#define SDHCI_P_DATPAD_CNFG        (SDHCI_PHY_R_OFFSET + 0x06)
#define SDHCI_P_CLKPAD_CNFG        (SDHCI_PHY_R_OFFSET + 0x08)
#define SDHCI_P_STBPAD_CNFG        (SDHCI_PHY_R_OFFSET + 0x0A)
#define SDHCI_P_RSTNPAD_CNFG       (SDHCI_PHY_R_OFFSET + 0x0C)
#define SDHCI_P_PADTEST_CNFG       (SDHCI_PHY_R_OFFSET + 0x0E)
#define SDHCI_P_PADTEST_OUT        (SDHCI_PHY_R_OFFSET + 0x10)
#define SDHCI_P_PADTEST_IN         (SDHCI_PHY_R_OFFSET + 0x12)
#define SDHCI_P_COMMDL_CNFG        (SDHCI_PHY_R_OFFSET + 0x1C)
#define SDHCI_P_SDCLKDL_CNFG       (SDHCI_PHY_R_OFFSET + 0x1D)
#define SDHCI_P_SDCLKDL_DC         (SDHCI_PHY_R_OFFSET + 0x1E)
#define SDHCI_P_SMPLDL_CNFG        (SDHCI_PHY_R_OFFSET + 0x20)
#define SDHCI_P_ATDL_CNFG          (SDHCI_PHY_R_OFFSET + 0x21)
#define SDHCI_P_DLL_CTRL           (SDHCI_PHY_R_OFFSET + 0x24)
#define SDHCI_P_DLL_CNFG1          (SDHCI_PHY_R_OFFSET + 0x25)
#define SDHCI_P_DLL_CNFG2          (SDHCI_PHY_R_OFFSET + 0x26)
#define SDHCI_P_DLLDL_CNFG         (SDHCI_PHY_R_OFFSET + 0x28)
#define SDHCI_P_DLL_OFFST          (SDHCI_PHY_R_OFFSET + 0x29)
#define SDHCI_P_DLLMST_TSTDC       (SDHCI_PHY_R_OFFSET + 0x2A)
#define SDHCI_P_DLLLBT_CNFG        (SDHCI_PHY_R_OFFSET + 0x2C)
#define SDHCI_P_DLL_STATUS         (SDHCI_PHY_R_OFFSET + 0x2E)
#define SDHCI_P_DLLDBG_MLKDC       (SDHCI_PHY_R_OFFSET + 0x30)
#define SDHCI_P_DLLDBG_SLKDC       (SDHCI_PHY_R_OFFSET + 0x32)

#define PHY_CNFG_PHY_RSTN               0
#define PHY_CNFG_PHY_PWRGOOD            1
#define PHY_CNFG_PAD_SP                 16
#define PHY_CNFG_PAD_SP_MSK             0xf
#define PHY_CNFG_PAD_SN                 20
#define PHY_CNFG_PAD_SN_MSK             0xf

#define PAD_CNFG_RXSEL                  0
#define PAD_CNFG_RXSEL_MSK              0x7
#define PAD_CNFG_WEAKPULL_EN            3
#define PAD_CNFG_WEAKPULL_EN_MSK        0x3
#define PAD_CNFG_TXSLEW_CTRL_P          5
#define PAD_CNFG_TXSLEW_CTRL_P_MSK      0xf
#define PAD_CNFG_TXSLEW_CTRL_N          9
#define PAD_CNFG_TXSLEW_CTRL_N_MSK      0xf

#define COMMDL_CNFG_DLSTEP_SEL          0
#define COMMDL_CNFG_DLOUT_EN            1

#define SDCLKDL_CNFG_EXTDLY_EN          0
#define SDCLKDL_CNFG_BYPASS_EN          1
#define SDCLKDL_CNFG_INPSEL_CNFG        2
#define SDCLKDL_CNFG_INPSEL_CNFG_MSK    0x3
#define SDCLKDL_CNFG_UPDATE_DC          4

#define SMPLDL_CNFG_EXTDLY_EN           0
#define SMPLDL_CNFG_BYPASS_EN           1
#define SMPLDL_CNFG_INPSEL_CNFG         2
#define SMPLDL_CNFG_INPSEL_CNFG_MSK     0x3
#define SMPLDL_CNFG_INPSEL_OVERRIDE     4

#define ATDL_CNFG_EXTDLY_EN             0
#define ATDL_CNFG_BYPASS_EN             1
#define ATDL_CNFG_INPSEL_CNFG           2
#define ATDL_CNFG_INPSEL_CNFG_MSK       0x3

#define MAX_TUNING_STEP                           128

struct sdhci_bm_host {
	struct platform_device *pdev;
	void __iomem *core_mem; /* bm SDCC mapped address */
	struct clk *clk;    /* main SD/MMC bus clock */
	struct clk *clk100k;
	struct clk *clkaxi;
	struct mmc_host *mmc;
	struct reset_control *reset;

	struct reset_control *clk_rst_axi_emmc_ctrl;
	struct reset_control *clk_rst_emmc_ctrl;
	struct reset_control *clk_rst_100k_emmc_ctrl;
};

int bm_sdhci_phy_init(struct sdhci_host *host);
bool sdhci_send_command(struct sdhci_host *host, struct mmc_command *cmd);
#endif
