/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2019-2022, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#define APE_SN_AXI2APB_1_BASE	0x00000
#define APE_SN_AGIC_BASE	0x15000
#define APE_SN_AMC_BASE		0x16000
#define APE_SN_AST0_T_BASE	0x17000
#define APE_SN_AST1_T_BASE	0x18000
#define APE_SN_AST2_T_BASE	0x19000
#define APE_SN_CBB_BASE		0x1A000

#define CBB_SN_AON_SLAVE_BASE	0x40000
#define CBB_SN_APE_SLAVE_BASE	0x50000
#define CBB_SN_BPMP_SLAVE_BASE	0x41000
#define CBB_SN_HOST1X_BASE	0x43000
#define CBB_SN_STM_BASE		0x44000
#define CBB_CENTRAL_BASE	0x00000
#define CBB_SN_PCIE_C0_BASE	0x51000
#define CBB_SN_PCIE_C1_BASE	0x47000
#define CBB_SN_PCIE_C2_BASE	0x48000
#define CBB_SN_PCIE_C3_BASE	0x49000
#define CBB_SN_GPU_BASE		0x4C000
#define CBB_SN_SMMU0_BASE	0x4D000
#define CBB_SN_SMMU1_BASE	0x4E000
#define CBB_SN_SMMU2_BASE	0x4F000
#define CBB_SN_PSC_SLAVE_BASE	0x52000
#define CBB_SN_AXI2APB_1_BASE	0x70000
#define CBB_SN_AXI2APB_12_BASE	0x73000
#define CBB_SN_AXI2APB_13_BASE	0x74000
#define CBB_SN_AXI2APB_15_BASE	0x76000
#define CBB_SN_AXI2APB_16_BASE	0x77000
#define CBB_SN_AXI2APB_18_BASE	0x79000
#define CBB_SN_AXI2APB_19_BASE	0x7A000
#define CBB_SN_AXI2APB_2_BASE	0x7B000
#define CBB_SN_AXI2APB_23_BASE	0x7F000
#define CBB_SN_AXI2APB_25_BASE	0x80000
#define CBB_SN_AXI2APB_26_BASE	0x81000
#define CBB_SN_AXI2APB_27_BASE	0x82000
#define CBB_SN_AXI2APB_28_BASE	0x83000
#define CBB_SN_AXI2APB_32_BASE	0x87000
#define CBB_SN_AXI2APB_33_BASE	0x88000
#define CBB_SN_AXI2APB_4_BASE	0x8B000
#define CBB_SN_AXI2APB_5_BASE	0x8C000
#define CBB_SN_AXI2APB_6_BASE	0x93000
#define CBB_SN_AXI2APB_9_BASE	0x90000
#define CBB_SN_AXI2APB_3_BASE	0x91000

static struct tegra_sn_addr_map tegra239_cbb_sn_lookup[] = {
	{ SLAVE_LOOKUP(CBB_SN_AON_SLAVE_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_APE_SLAVE_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_BPMP_SLAVE_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_HOST1X_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_STM_BASE) },
	{ SLAVE_LOOKUP(CBB_CENTRAL_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_PCIE_C0_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_PCIE_C1_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_PCIE_C2_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_PCIE_C3_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_GPU_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_SMMU0_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_SMMU1_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_SMMU2_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_PSC_SLAVE_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_1_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_12_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_13_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_15_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_16_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_18_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_19_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_2_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_23_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_25_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_26_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_27_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_28_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_32_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_33_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_4_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_5_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_6_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_9_BASE) },
	{ SLAVE_LOOKUP(CBB_SN_AXI2APB_3_BASE) },
};

static struct tegra_sn_addr_map tegra239_ape_sn_lookup[] = {
	{ SLAVE_LOOKUP(APE_SN_AXI2APB_1_BASE) },
	{ SLAVE_LOOKUP(APE_SN_AGIC_BASE) },
	{ SLAVE_LOOKUP(APE_SN_AMC_BASE) },
	{ SLAVE_LOOKUP(APE_SN_AST0_T_BASE) },
	{ SLAVE_LOOKUP(APE_SN_AST1_T_BASE) },
	{ SLAVE_LOOKUP(APE_SN_AST2_T_BASE) },
	{ SLAVE_LOOKUP(APE_SN_CBB_BASE) },
};
