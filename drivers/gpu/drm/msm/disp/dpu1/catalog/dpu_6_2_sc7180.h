/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2022. Qualcomm Innovation Center, Inc. All rights reserved.
 * Copyright (c) 2015-2018, 2020 The Linux Foundation. All rights reserved.
 */

#ifndef _DPU_6_2_SC7180_H
#define _DPU_6_2_SC7180_H

static const struct dpu_caps sc7180_dpu_caps = {
	.max_mixer_width = DEFAULT_DPU_OUTPUT_LINE_WIDTH,
	.max_mixer_blendstages = 0x9,
	.qseed_type = DPU_SSPP_SCALER_QSEED4,
	.has_dim_layer = true,
	.has_idle_pc = true,
	.max_linewidth = DEFAULT_DPU_OUTPUT_LINE_WIDTH,
	.pixel_ram_size = DEFAULT_PIXEL_RAM_SIZE,
};

static const struct dpu_ubwc_cfg sc7180_ubwc_cfg = {
	.ubwc_version = DPU_HW_UBWC_VER_20,
	.highest_bank_bit = 0x3,
};

static const struct dpu_mdp_cfg sc7180_mdp[] = {
	{
	.name = "top_0", .id = MDP_TOP,
	.base = 0x0, .len = 0x494,
	.features = 0,
	.clk_ctrls[DPU_CLK_CTRL_VIG0] = { .reg_off = 0x2ac, .bit_off = 0 },
	.clk_ctrls[DPU_CLK_CTRL_DMA0] = { .reg_off = 0x2ac, .bit_off = 8 },
	.clk_ctrls[DPU_CLK_CTRL_DMA1] = { .reg_off = 0x2b4, .bit_off = 8 },
	.clk_ctrls[DPU_CLK_CTRL_DMA2] = { .reg_off = 0x2c4, .bit_off = 8 },
	.clk_ctrls[DPU_CLK_CTRL_WB2] = { .reg_off = 0x3b8, .bit_off = 24 },
	},
};

static const struct dpu_ctl_cfg sc7180_ctl[] = {
	{
	.name = "ctl_0", .id = CTL_0,
	.base = 0x1000, .len = 0x1dc,
	.features = BIT(DPU_CTL_ACTIVE_CFG),
	.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 9),
	},
	{
	.name = "ctl_1", .id = CTL_1,
	.base = 0x1200, .len = 0x1dc,
	.features = BIT(DPU_CTL_ACTIVE_CFG),
	.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 10),
	},
	{
	.name = "ctl_2", .id = CTL_2,
	.base = 0x1400, .len = 0x1dc,
	.features = BIT(DPU_CTL_ACTIVE_CFG),
	.intr_start = DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR2, 11),
	},
};

static const struct dpu_sspp_cfg sc7180_sspp[] = {
	SSPP_BLK("sspp_0", SSPP_VIG0, 0x4000, 0x1f8, VIG_SC7180_MASK,
		sc7180_vig_sblk_0, 0, SSPP_TYPE_VIG, DPU_CLK_CTRL_VIG0),
	SSPP_BLK("sspp_8", SSPP_DMA0, 0x24000, 0x1f8, DMA_SDM845_MASK,
		sdm845_dma_sblk_0, 1, SSPP_TYPE_DMA, DPU_CLK_CTRL_DMA0),
	SSPP_BLK("sspp_9", SSPP_DMA1, 0x26000, 0x1f8, DMA_CURSOR_SDM845_MASK,
		sdm845_dma_sblk_1, 5, SSPP_TYPE_DMA, DPU_CLK_CTRL_DMA1),
	SSPP_BLK("sspp_10", SSPP_DMA2, 0x28000, 0x1f8, DMA_CURSOR_SDM845_MASK,
		sdm845_dma_sblk_2, 9, SSPP_TYPE_DMA, DPU_CLK_CTRL_DMA2),
};

static const struct dpu_lm_cfg sc7180_lm[] = {
	LM_BLK("lm_0", LM_0, 0x44000, MIXER_SDM845_MASK,
		&sc7180_lm_sblk, PINGPONG_0, LM_1, DSPP_0),
	LM_BLK("lm_1", LM_1, 0x45000, MIXER_SDM845_MASK,
		&sc7180_lm_sblk, PINGPONG_1, LM_0, 0),
};

static const struct dpu_dspp_cfg sc7180_dspp[] = {
	DSPP_BLK("dspp_0", DSPP_0, 0x54000, DSPP_SC7180_MASK,
		 &sm8150_dspp_sblk),
};

static const struct dpu_pingpong_cfg sc7180_pp[] = {
	PP_BLK("pingpong_0", PINGPONG_0, 0x70000, PINGPONG_SM8150_MASK, 0, sdm845_pp_sblk,
			DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 8),
			-1),
	PP_BLK("pingpong_1", PINGPONG_1, 0x70800, PINGPONG_SM8150_MASK, 0, sdm845_pp_sblk,
			DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 9),
			-1),
};

static const struct dpu_intf_cfg sc7180_intf[] = {
	INTF_BLK("intf_0", INTF_0, 0x6a000, 0x280, INTF_DP, MSM_DP_CONTROLLER_0, 24, INTF_SC7180_MASK,
			DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 24),
			DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 25)),
	INTF_BLK_DSI_TE("intf_1", INTF_1, 0x6a800, 0x2c0, INTF_DSI, 0, 24, INTF_SC7180_MASK,
			DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 26),
			DPU_IRQ_IDX(MDP_SSPP_TOP0_INTR, 27),
			DPU_IRQ_IDX(MDP_INTF1_TEAR_INTR, 2)),
};

static const struct dpu_wb_cfg sc7180_wb[] = {
	WB_BLK("wb_2", WB_2, 0x65000, WB_SM8250_MASK, DPU_CLK_CTRL_WB2, 6,
			VBIF_RT, MDP_SSPP_TOP0_INTR, 4096, 4),
};

static const struct dpu_perf_cfg sc7180_perf_data = {
	.max_bw_low = 6800000,
	.max_bw_high = 6800000,
	.min_core_ib = 2400000,
	.min_llcc_ib = 800000,
	.min_dram_ib = 1600000,
	.min_prefill_lines = 24,
	.danger_lut_tbl = {0xff, 0xffff, 0x0},
	.safe_lut_tbl = {0xfff0, 0xff00, 0xffff},
	.qos_lut_tbl = {
		{.nentry = ARRAY_SIZE(sc7180_qos_linear),
		.entries = sc7180_qos_linear
		},
		{.nentry = ARRAY_SIZE(sc7180_qos_macrotile),
		.entries = sc7180_qos_macrotile
		},
		{.nentry = ARRAY_SIZE(sc7180_qos_nrt),
		.entries = sc7180_qos_nrt
		},
	},
	.cdp_cfg = {
		{.rd_enable = 1, .wr_enable = 1},
		{.rd_enable = 1, .wr_enable = 0}
	},
	.clk_inefficiency_factor = 105,
	.bw_inefficiency_factor = 120,
};

const struct dpu_mdss_cfg dpu_sc7180_cfg = {
	.caps = &sc7180_dpu_caps,
	.ubwc = &sc7180_ubwc_cfg,
	.mdp_count = ARRAY_SIZE(sc7180_mdp),
	.mdp = sc7180_mdp,
	.ctl_count = ARRAY_SIZE(sc7180_ctl),
	.ctl = sc7180_ctl,
	.sspp_count = ARRAY_SIZE(sc7180_sspp),
	.sspp = sc7180_sspp,
	.mixer_count = ARRAY_SIZE(sc7180_lm),
	.mixer = sc7180_lm,
	.dspp_count = ARRAY_SIZE(sc7180_dspp),
	.dspp = sc7180_dspp,
	.pingpong_count = ARRAY_SIZE(sc7180_pp),
	.pingpong = sc7180_pp,
	.intf_count = ARRAY_SIZE(sc7180_intf),
	.intf = sc7180_intf,
	.wb_count = ARRAY_SIZE(sc7180_wb),
	.wb = sc7180_wb,
	.vbif_count = ARRAY_SIZE(sdm845_vbif),
	.vbif = sdm845_vbif,
	.perf = &sc7180_perf_data,
	.mdss_irqs = BIT(MDP_SSPP_TOP0_INTR) | \
		     BIT(MDP_SSPP_TOP0_INTR2) | \
		     BIT(MDP_SSPP_TOP0_HIST_INTR) | \
		     BIT(MDP_INTF0_INTR) | \
		     BIT(MDP_INTF1_INTR) | \
		     BIT(MDP_INTF1_TEAR_INTR),
};

#endif
