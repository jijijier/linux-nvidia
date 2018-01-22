/*
 * Copyright (c) 2017-2018, NVIDIA CORPORATION.  All rights reserved.
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

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_pci.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/resource.h>
#include <soc/tegra/chip-id.h>
#include <soc/tegra/bpmp_abi.h>
#include <soc/tegra/tegra_bpmp.h>
#include <linux/pci.h>

#define CTRL_0	(0)
#define CTRL_1	(1)
#define CTRL_2	(2)
#define CTRL_3	(3)
#define CTRL_4	(4)
#define CTRL_5	(5)

#define APPL_PINMUX				(0X0)
#define APPL_PINMUX_PEX_RST_IN_OVERRIDE_EN	BIT(11)

#define APPL_CTRL				(0X4)
#define APPL_SYS_PRE_DET_STATE			BIT(6)
#define APPL_CTRL_LTSSM_EN			BIT(7)
#define APPL_CTRL_READY_ENTR_L23			BIT(12)
#define APPL_CTRL_HW_HOT_RST_EN			BIT(20)

#define APPL_INTR_EN_L0_0			0x8
#define APPL_INTR_EN_L0_0_SYS_INTR_EN		BIT(30)
#define APPL_INTR_EN_L0_0_PEX_RST_INT_EN		BIT(16)
#define APPL_INTR_EN_L0_0_PCI_CMD_EN_INT_EN	BIT(15)
#define APPL_INTR_EN_L0_0_ERROR_INT_EN		BIT(1)
#define APPL_INTR_EN_L0_0_LINK_STATE_INT_EN	BIT(0)

#define APPL_INTR_STATUS_L0			0xC
#define APPL_INTR_STATUS_L0_PEX_RST_INT_SHIFT	16
#define APPL_INTR_STATUS_L0_PEX_RST_INT		BIT(16)
#define APPL_INTR_STATUS_L0_PCI_CMD_EN_INT	BIT(15)
#define APPL_INTR_STATUS_L0_LINK_STATE_INT	BIT(0)

#define APPL_INTR_EN_L1_0			0x1C
#define APPL_INTR_EN_L1_0_LINK_REQ_RST_INT_EN	BIT(1)
#define APPL_INTR_EN_L1_0_HOT_RESET_DONE_INT_EN	BIT(30)

#define APPL_INTR_STATUS_L1			0x20
#define APPL_INTR_STATUS_L1_LINK_REQ_RST_CHGED	BIT(1)
#define APPL_INTR_STATUS_L1_HOT_RESET_DONE	BIT(30)

#define APPL_INTR_STATUS_L1_1			0x2C
#define APPL_INTR_STATUS_L1_2			0x30
#define APPL_INTR_STATUS_L1_3			0x34
#define APPL_INTR_STATUS_L1_6			0x3C
#define APPL_INTR_STATUS_L1_7			0x40
#define APPL_INTR_STATUS_L1_8			0x4C
#define APPL_INTR_STATUS_L1_9			0x54
#define APPL_INTR_STATUS_L1_10			0x58
#define APPL_INTR_STATUS_L1_11			0x64
#define APPL_INTR_STATUS_L1_13			0x74
#define APPL_INTR_STATUS_L1_14			0x78
#define APPL_INTR_STATUS_L1_15			0x7C
#define APPL_INTR_STATUS_L1_15_CFG_BME_CHGED	BIT(1)
#define APPL_INTR_STATUS_L1_17			0x88

#define APPL_MSI_CTRL_2				0xB0

#define APPL_LTR_MSG_1				0xC4
#define APPL_LTR_MSG_2				0xC8
#define APPL_LTR_MSG_2_LTR_MSG_REQ_STATE	BIT(3)

#define LTR_MSG_REQ				BIT(15)
#define LTR_MST_NO_SNOOP_SHIFT			16

#define APPL_PM_STATUS				0xFC

#define APPL_DM_TYPE				0x100
#define APPL_DM_TYPE_MASK			0xF
#define APPL_DM_TYPE_EP				0x0

#define APPL_CFG_BASE_ADDR			0x104
#define APPL_CFG_BASE_ADDR_MASK			0xFFFFF000

#define APPL_CFG_IATU_DMA_BASE_ADDR		0x108
#define APPL_CFG_IATU_DMA_BASE_ADDR_MASK	0xFFFC0000

#define APPL_CFG_SLCG_OVERRIDE			0x114
#define APPL_CFG_SLCG_OVERRIDE_SLCG_EN_MASTER	BIT(0)

#define APPL_GTH_PHY				0x138
#define APPL_GTH_PHY_RST			0x1

#define EP_CS_STATUS_COMMAND			0x4
#define EP_CS_STATUS_COMMAND_BME		BIT(2)

#define EP_CFG_LINK_CAP				0x7C

#define CAP_SPCIE_CAP_OFF	0x154
#define CAP_SPCIE_CAP_OFF_DSP_TX_PRESET0_MASK	GENMASK(3, 0)

#define PL16G_CAP_OFF		0x188
#define PL16G_CAP_OFF_DSP_16G_TX_PRESET_MASK	GENMASK(3, 0)

#define CFG_TIMER_CTRL_MAX_FUNC_NUM_OFF		0x718
#define CFG_TIMER_CTRL_ACK_NAK_SHIFT		(19)

#define CFG_LINK_CAP_L1SUB			0x1C4

#define GEN3_RELATED_OFF	0x890
#define GEN3_RELATED_OFF_GEN3_EQ_DISABLE	BIT(16)
#define GEN3_RELATED_OFF_RATE_SHADOW_SEL_SHIFT	24
#define GEN3_RELATED_OFF_RATE_SHADOW_SEL_MASK	GENMASK(25, 24)

#define GEN3_EQ_CONTROL_OFF	0x8a8
#define GEN3_EQ_CONTROL_OFF_PSET_REQ_VEC_SHIFT	8
#define GEN3_EQ_CONTROL_OFF_PSET_REQ_VEC_MASK	GENMASK(23, 8)
#define GEN3_EQ_CONTROL_OFF_FB_MODE_MASK	GENMASK(3, 0)

#define MISC_CONTROL_1				0X8BC
#define MISC_CONTROL_1_DBI_RO_WR_EN		BIT(0)

#define AUX_CLK_FREQ				0xB40

#define PCIE_ATU_REGION_INDEX0	0 /* used for BAR-0 translations */
#define PCIE_ATU_REGION_INDEX1	1
#define PCIE_ATU_REGION_INDEX2	2
#define PCIE_ATU_REGION_INDEX3	3

#define PCIE_ATU_CR1		0x0
#define PCIE_ATU_TYPE_MEM	(0x0 << 0)
#define PCIE_ATU_TYPE_IO	(0x2 << 0)
#define PCIE_ATU_TYPE_CFG0	(0x4 << 0)
#define PCIE_ATU_TYPE_CFG1	(0x5 << 0)
#define PCIE_ATU_CR2		0x4
#define PCIE_ATU_ENABLE		(0x1 << 31)
#define PCIE_ATU_CR2_BAR_SHIFT	8
#define PCIE_ATU_CR2_MATCH_MODE_SHIFT	30
#define PCIE_ATU_CR2_MATCH_MODE_ADDR	0
#define PCIE_ATU_CR2_MATCH_MODE_BAR	1
#define PCIE_ATU_LOWER_BASE	0x8
#define PCIE_ATU_UPPER_BASE	0xC
#define PCIE_ATU_LIMIT		0x10
#define PCIE_ATU_LOWER_TARGET	0x14
#define PCIE_ATU_UPPER_TARGET	0x18

#define LTR_MSG_TIMEOUT (100*1000)

enum ep_event {
	EP_EVENT_NONE = 0,
	EP_PEX_RST_DE_ASSERT,
	EP_PEX_HOT_RST_DONE,
	EP_PEX_BME_CHANGE,
	EP_EVENT_INVALID,
};

struct tegra_pcie_dw_ep {
	struct device *dev;
	struct resource *appl_res;
	struct resource	*dbi_res;
	struct resource	*atu_dma_res;
	void __iomem		*appl_base;
	void __iomem		*dbi_base;
	void __iomem		*atu_dma_base;
	struct clk		*core_clk;
	struct reset_control	*core_apb_rst;
	struct reset_control	*core_rst;
	int			irq;
	int			phy_count;
	struct phy		**phy;
	struct work_struct pcie_ep_work;
	u32 bar0_size;
	u32 cid;
	u16 device_id;
	u8 disabled_aspm_states;
	dma_addr_t dma_handle;
	void *cpu_virt;
	bool update_fc_fixup;
	enum ep_event event;
	struct regulator *pex_ctl_reg;

	u32 num_lanes;
	u32 cfg_link_cap_l1sub;
};

static int tegra_pcie_power_on_phy(struct tegra_pcie_dw_ep *pcie);

static inline void prog_atu(struct tegra_pcie_dw_ep *pcie, int i, u32 val,
			    u32 reg)
{
	writel(val, pcie->atu_dma_base + (i * 0x200) + 0x100 + reg);
}

static void inbound_atu(struct tegra_pcie_dw_ep *pcie, int i, int type,
			u64 wire_addr, u64 int_addr, u32 size,
			bool match_mode, u8 bar)
{
	prog_atu(pcie, i, lower_32_bits(wire_addr), PCIE_ATU_LOWER_BASE);
	prog_atu(pcie, i, upper_32_bits(wire_addr), PCIE_ATU_UPPER_BASE);
	prog_atu(pcie, i, lower_32_bits(wire_addr + size - 1), PCIE_ATU_LIMIT);
	prog_atu(pcie, i, lower_32_bits(int_addr), PCIE_ATU_LOWER_TARGET);
	prog_atu(pcie, i, upper_32_bits(int_addr), PCIE_ATU_UPPER_TARGET);
	prog_atu(pcie, i, type, PCIE_ATU_CR1);
	prog_atu(pcie, i, PCIE_ATU_ENABLE | (bar << PCIE_ATU_CR2_BAR_SHIFT) |
		 (match_mode <<  PCIE_ATU_CR2_MATCH_MODE_SHIFT), PCIE_ATU_CR2);
}

static irqreturn_t tegra_pcie_irq_handler(int irq, void *arg)
{
	struct tegra_pcie_dw_ep *pcie = (struct tegra_pcie_dw_ep *)arg;
	u32 val = 0;

	val = readl(pcie->appl_base + APPL_INTR_STATUS_L0);
	dev_dbg(pcie->dev, "APPL_INTR_STATUS_L0 = 0x%08X\n", val);
	if (val & APPL_INTR_STATUS_L0_PEX_RST_INT) {
		/* clear any stale PEX_RST interrupt */
		writel(APPL_INTR_STATUS_L0_PEX_RST_INT,
		       pcie->appl_base + APPL_INTR_STATUS_L0);
		pcie->event = EP_PEX_RST_DE_ASSERT;
		schedule_work(&pcie->pcie_ep_work);
	} else if (val & APPL_INTR_STATUS_L0_LINK_STATE_INT) {
		val = readl(pcie->appl_base + APPL_INTR_STATUS_L1);
		writel(val, pcie->appl_base + APPL_INTR_STATUS_L1);
		dev_dbg(pcie->dev, "APPL_INTR_STATUS_L1 = 0x%08X\n", val);
		if (val & APPL_INTR_STATUS_L1_HOT_RESET_DONE) {
			/* clear any stale PEX_RST interrupt */
			pcie->event = EP_PEX_HOT_RST_DONE;
			schedule_work(&pcie->pcie_ep_work);
		}
	} else if (val & APPL_INTR_STATUS_L0_PCI_CMD_EN_INT) {
		val = readl(pcie->appl_base + APPL_INTR_STATUS_L1_15);
		writel(val, pcie->appl_base + APPL_INTR_STATUS_L1_15);
		dev_dbg(pcie->dev, "APPL_INTR_STATUS_L1_15 = 0x%08X\n", val);
		if (val & APPL_INTR_STATUS_L1_15_CFG_BME_CHGED) {
			pcie->event = EP_PEX_BME_CHANGE;
			schedule_work(&pcie->pcie_ep_work);
		}
	} else {
		dev_info(pcie->dev, "Random interrupt (STATUS = 0x%08X)\n",
			 val);
		writel(val, pcie->appl_base + APPL_INTR_STATUS_L0);
	}

	return IRQ_HANDLED;
}

static int bpmp_send_uphy_message_atomic(struct mrq_uphy_request *req, int size,
					 struct mrq_uphy_response *reply,
					 int reply_size)
{
	unsigned long flags;
	int err;

	local_irq_save(flags);
	err = tegra_bpmp_send_receive_atomic(MRQ_UPHY, req, size, reply,
					     reply_size);
	local_irq_restore(flags);

	return err;
}

static int bpmp_send_uphy_message(struct mrq_uphy_request *req, int size,
				  struct mrq_uphy_response *reply,
				  int reply_size)
{
	int err;

	err = tegra_bpmp_send_receive(MRQ_UPHY, req, size, reply, reply_size);
	if (err != -EAGAIN)
		return err;

	/*
	 * in case the mail systems worker threads haven't been started yet,
	 * use the atomic send/receive interface. This happens because the
	 * clocks are initialized before the IPC mechanism.
	 */
	return bpmp_send_uphy_message_atomic(req, size, reply, reply_size);
}

static int uphy_bpmp_pcie_ep_controller_pll_init(u32 id)
{
	struct mrq_uphy_request req;
	struct mrq_uphy_response resp;

	req.cmd = CMD_UPHY_PCIE_EP_CONTROLLER_PLL_INIT;
	req.ep_ctrlr_pll_init.ep_controller = id;

	return bpmp_send_uphy_message(&req, sizeof(req), &resp, sizeof(resp));
}

static int uphy_bpmp_pcie_controller_state_set(int controller, int enable)
{
	struct mrq_uphy_request req;
	struct mrq_uphy_response resp;

	req.cmd = CMD_UPHY_PCIE_CONTROLLER_STATE;
	req.controller_state.pcie_controller = controller;
	req.controller_state.enable = enable;

	return bpmp_send_uphy_message(&req, sizeof(req), &resp, sizeof(resp));
}

static void disable_aspm_l0s(struct tegra_pcie_dw_ep *pcie)
{
	u32 val = 0;

	val = readl(pcie->dbi_base + EP_CFG_LINK_CAP);
	val &= ~(PCI_EXP_LNKCTL_ASPM_L0S << 10);
	writel(val, pcie->dbi_base + EP_CFG_LINK_CAP);
}

static void disable_aspm_l10(struct tegra_pcie_dw_ep *pcie)
{
	u32 val = 0;

	val = readl(pcie->dbi_base + EP_CFG_LINK_CAP);
	val &= ~(PCI_EXP_LNKCTL_ASPM_L1 << 10);
	writel(val, pcie->dbi_base + EP_CFG_LINK_CAP);
}

static void disable_aspm_l11(struct tegra_pcie_dw_ep *pcie)
{
	u32 val = 0;

	val = readl(pcie->dbi_base + pcie->cfg_link_cap_l1sub);
	val &= ~PCI_L1SS_CAP_ASPM_L11S;
	writel(val, pcie->dbi_base + pcie->cfg_link_cap_l1sub);
}

static void disable_aspm_l12(struct tegra_pcie_dw_ep *pcie)
{
	u32 val = 0;

	val = readl(pcie->dbi_base + pcie->cfg_link_cap_l1sub);
	val &= ~PCI_L1SS_CAP_ASPM_L12S;
	writel(val, pcie->dbi_base + pcie->cfg_link_cap_l1sub);
}

static void program_gen3_gen4_eq_presets(struct tegra_pcie_dw_ep *pcie)
{
	int i, init_preset = 5;
	u32 val = 0;
	u16 val_16 = 0;
	u8 val_8 = 0;

	/* program init preset */
	if (init_preset < 11) {
		for (i = 0; i < pcie->num_lanes; i++) {
			val_16 = readw(pcie->dbi_base + CAP_SPCIE_CAP_OFF +
				       (i * 2));
			val_16 &= ~CAP_SPCIE_CAP_OFF_DSP_TX_PRESET0_MASK;
			val_16 |= init_preset;
			writew(val_16, pcie->dbi_base + CAP_SPCIE_CAP_OFF +
			       (i * 2));

			val_8 = readb(pcie->dbi_base + PL16G_CAP_OFF + i);
			val_8 &= ~PL16G_CAP_OFF_DSP_16G_TX_PRESET_MASK;
			val_8 |= init_preset;
			writeb(val_8, pcie->dbi_base + PL16G_CAP_OFF + i);
		}
	}

	val = readl(pcie->dbi_base + GEN3_RELATED_OFF);
	val &= ~GEN3_RELATED_OFF_RATE_SHADOW_SEL_MASK;
	writel(val, pcie->dbi_base + GEN3_RELATED_OFF);

	val = readl(pcie->dbi_base + GEN3_EQ_CONTROL_OFF);
	val &= ~GEN3_EQ_CONTROL_OFF_PSET_REQ_VEC_MASK;
	val |= (0x3ff << GEN3_EQ_CONTROL_OFF_PSET_REQ_VEC_SHIFT);
	val &= ~GEN3_EQ_CONTROL_OFF_FB_MODE_MASK;
	writel(val, pcie->dbi_base + GEN3_EQ_CONTROL_OFF);

	val = readl(pcie->dbi_base + GEN3_RELATED_OFF);
	val &= ~GEN3_RELATED_OFF_RATE_SHADOW_SEL_MASK;
	val |= (0x1 << GEN3_RELATED_OFF_RATE_SHADOW_SEL_SHIFT);
	writel(val, pcie->dbi_base + GEN3_RELATED_OFF);

	val = readl(pcie->dbi_base + GEN3_EQ_CONTROL_OFF);
	val &= ~GEN3_EQ_CONTROL_OFF_PSET_REQ_VEC_MASK;
	val |= (0x270 << GEN3_EQ_CONTROL_OFF_PSET_REQ_VEC_SHIFT);
	val &= ~GEN3_EQ_CONTROL_OFF_FB_MODE_MASK;
	val |= 0x1;
	writel(val, pcie->dbi_base + GEN3_EQ_CONTROL_OFF);

	val = readl(pcie->dbi_base + GEN3_RELATED_OFF);
	val &= ~GEN3_RELATED_OFF_RATE_SHADOW_SEL_MASK;
	writel(val, pcie->dbi_base + GEN3_RELATED_OFF);
}

void pcie_ep_work_fn(struct work_struct *work)
{
	struct tegra_pcie_dw_ep *pcie =
	    container_of(work, struct tegra_pcie_dw_ep, pcie_ep_work);
	u32 val = 0;
	int ret = 0;

	if (pcie->event == EP_PEX_RST_DE_ASSERT) {
		ret = uphy_bpmp_pcie_ep_controller_pll_init(pcie->cid);
		if (ret)
			dev_err(pcie->dev, "UPHY init failed for PCIe EP:%d\n",
				ret);

		ret = clk_prepare_enable(pcie->core_clk);
		if (ret) {
			dev_err(pcie->dev, "Failed to enable core clock\n");
			return;
		}

		reset_control_assert(pcie->core_apb_rst);
		reset_control_deassert(pcie->core_apb_rst);

		ret = tegra_pcie_power_on_phy(pcie);
		if (ret) {
			dev_err(pcie->dev, "failed to power_on phy\n");
			return;
		}

		/* clear any stale interrupt statuses */
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L0);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_1);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_2);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_3);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_6);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_7);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_8);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_9);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_10);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_11);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_13);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_14);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_15);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_17);

		/* configure this core for EP mode operation */
		val = readl(pcie->appl_base + APPL_DM_TYPE);
		val &= ~APPL_DM_TYPE_MASK;
		val |= APPL_DM_TYPE_EP;
		writel(val, pcie->appl_base + APPL_DM_TYPE);

		val = readl(pcie->appl_base + APPL_CTRL);
		val |= APPL_SYS_PRE_DET_STATE;
		val |= APPL_CTRL_HW_HOT_RST_EN;
		writel(val, pcie->appl_base + APPL_CTRL);

		if (tegra_platform_is_fpga()) {
			val = readl(pcie->appl_base + APPL_PINMUX);
			val &= ~APPL_PINMUX_PEX_RST_IN_OVERRIDE_EN;
			writel(val, pcie->appl_base + APPL_PINMUX);
		}

		/* update CFG base address */
		writel(pcie->dbi_res->start & APPL_CFG_BASE_ADDR_MASK,
		       pcie->appl_base + APPL_CFG_BASE_ADDR);

		/* update iATU_DMA base address */
		writel(pcie->atu_dma_res->start &
		       APPL_CFG_IATU_DMA_BASE_ADDR_MASK,
		       pcie->appl_base + APPL_CFG_IATU_DMA_BASE_ADDR);

		/* enable PEX_RST interrupt generation */
		val = readl(pcie->appl_base + APPL_INTR_EN_L0_0);
		val |= APPL_INTR_EN_L0_0_SYS_INTR_EN;
		if (tegra_platform_is_fpga())
			val |= APPL_INTR_EN_L0_0_PEX_RST_INT_EN;
		val |= APPL_INTR_EN_L0_0_LINK_STATE_INT_EN;
		val |= APPL_INTR_EN_L0_0_PCI_CMD_EN_INT_EN;
		writel(val, pcie->appl_base + APPL_INTR_EN_L0_0);

		val = readl(pcie->appl_base + APPL_INTR_EN_L1_0);
		val |= APPL_INTR_EN_L1_0_HOT_RESET_DONE_INT_EN;
		writel(val, pcie->appl_base + APPL_INTR_EN_L1_0);

		reset_control_assert(pcie->core_rst);
		reset_control_deassert(pcie->core_rst);

		/* FPGA specific PHY initialization */
		if (tegra_platform_is_fpga()) {
			val = readl(pcie->appl_base + APPL_GTH_PHY);
			val &= ~APPL_GTH_PHY_RST;
			writel(val, pcie->appl_base + APPL_GTH_PHY);
			usleep_range(900, 1100);

			val = readl(pcie->appl_base + APPL_GTH_PHY);
			val &= 0xFFFF0000;
			val |= 0x780; /* required for multiple L1.2 entries */
			val |= APPL_GTH_PHY_RST;
			writel(val, pcie->appl_base + APPL_GTH_PHY);
			usleep_range(900, 1100);
		}

		/* Enable only 1MB of BAR */
		writel(pcie->bar0_size - 1, pcie->dbi_base + 0x1010);
		writel(0x00000000, pcie->dbi_base + 0x1014);

		val = readl(pcie->dbi_base + AUX_CLK_FREQ);
		val &= ~(0x3FF);
		if (tegra_platform_is_fpga())
			val |= 0x6;
		else
			val |= 19;	/* CHECK: for Silicon */
		writel(val, pcie->dbi_base + AUX_CLK_FREQ);

		inbound_atu(pcie, PCIE_ATU_REGION_INDEX0, PCIE_ATU_TYPE_MEM,
			    0x0, pcie->dma_handle, pcie->bar0_size,
			    PCIE_ATU_CR2_MATCH_MODE_BAR, 0);

		if (pcie->update_fc_fixup) {
			val = readl(pcie->dbi_base +
				    CFG_TIMER_CTRL_MAX_FUNC_NUM_OFF);
			val |= 0x1 << CFG_TIMER_CTRL_ACK_NAK_SHIFT;
			writel(val, pcie->dbi_base +
			       CFG_TIMER_CTRL_MAX_FUNC_NUM_OFF);
		}

		program_gen3_gen4_eq_presets(pcie);

		val = readl(pcie->dbi_base + MISC_CONTROL_1);
		val |= MISC_CONTROL_1_DBI_RO_WR_EN;
		writel(val, pcie->dbi_base + MISC_CONTROL_1);

		if (pcie->disabled_aspm_states) {
			if (val & 0x1)
				disable_aspm_l0s(pcie); /* Disable L0s */
			if (val & 0x2) {
				disable_aspm_l10(pcie); /* Disable L1 */
				disable_aspm_l11(pcie); /* Disable L1.1 */
				disable_aspm_l12(pcie); /* Disable L1.2 */
			}
			if (val & 0x4)
				disable_aspm_l11(pcie); /* Disable L1.1 */
			if (val & 0x8)
				disable_aspm_l12(pcie); /* Disable L1.2 */
		}

		writew(pcie->device_id, pcie->dbi_base + PCI_DEVICE_ID);

		writew(PCI_CLASS_MEMORY_OTHER,
		       pcie->dbi_base + PCI_CLASS_DEVICE);

		val = readl(pcie->dbi_base + MISC_CONTROL_1);
		val &= ~MISC_CONTROL_1_DBI_RO_WR_EN;
		writel(val, pcie->dbi_base + MISC_CONTROL_1);

		/* enable LTSSM */
		val = readl(pcie->appl_base + APPL_CTRL);
		val |= APPL_CTRL_LTSSM_EN;
		writel(val, pcie->appl_base + APPL_CTRL);
		pcie->event = EP_EVENT_INVALID;
	}
	if (pcie->event == EP_PEX_HOT_RST_DONE) {
		/* SW FixUp required during hot reset */
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L0);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_1);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_2);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_3);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_6);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_7);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_8);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_9);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_10);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_11);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_13);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_14);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_15);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_INTR_STATUS_L1_17);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_MSI_CTRL_2);
		writel(0xFFFFFFFF, pcie->appl_base + APPL_PM_STATUS);

		val = readl(pcie->appl_base + APPL_CTRL);
		val |= APPL_CTRL_LTSSM_EN;
		writel(val, pcie->appl_base + APPL_CTRL);
		pcie->event = EP_EVENT_INVALID;
	}
	if (pcie->event == EP_PEX_BME_CHANGE) {
		/* Check if BME is set to '1' */
		val = readl(pcie->dbi_base + EP_CS_STATUS_COMMAND);
		if (val & EP_CS_STATUS_COMMAND_BME) {
			ktime_t timeout;

			/* 100us for both snoop and no-snoop */
			/* TODO : Value should be updated for Silicon*/
			val = 100 | (2 << PCI_LTR_SCALE_SHIFT) | LTR_MSG_REQ;
			val |= (val << LTR_MST_NO_SNOOP_SHIFT);
			writel(val, pcie->appl_base + APPL_LTR_MSG_1);
			/* Send LTR upstream */
			val = readl(pcie->appl_base + APPL_LTR_MSG_2);
			val |= APPL_LTR_MSG_2_LTR_MSG_REQ_STATE;
			writel(val, pcie->appl_base + APPL_LTR_MSG_2);

			timeout = ktime_add_us(ktime_get(), LTR_MSG_TIMEOUT);
			for (;;) {
				val = readl(pcie->appl_base + APPL_LTR_MSG_2);
				if (!(val & APPL_LTR_MSG_2_LTR_MSG_REQ_STATE))
					break;
				if (ktime_after(ktime_get(), timeout))
					break;
				usleep_range(1000, 1100);
			}
			if (val & APPL_LTR_MSG_2_LTR_MSG_REQ_STATE)
				dev_err(pcie->dev, "LTR_MSG sending failed\n");
		}
		pcie->event = EP_EVENT_INVALID;
	}
}

static void tegra_pcie_disable_phy(struct tegra_pcie_dw_ep *pcie)
{
	int phy_count = pcie->phy_count;

	while (phy_count--) {
		phy_power_off(pcie->phy[phy_count]);
		phy_exit(pcie->phy[phy_count]);
	}
}

static int tegra_pcie_init_phy(struct tegra_pcie_dw_ep *pcie)
{
	int phy_count = pcie->phy_count;
	int ret;
	int i;

	for (i = 0; i < phy_count; i++) {
		ret = phy_init(pcie->phy[i]);
		if (ret < 0)
			goto err_phy_init;
	}

	return 0;

	while (i >= 0) {
		phy_exit(pcie->phy[i]);
err_phy_init:
		i--;
	}

	return ret;
}

static int tegra_pcie_power_on_phy(struct tegra_pcie_dw_ep *pcie)
{
	int phy_count = pcie->phy_count;
	int ret;
	int i;

	for (i = 0; i < phy_count; i++) {
		ret = phy_power_on(pcie->phy[i]);
		if (ret < 0)
			goto err_phy_power_on;
	}

	return 0;

	while (i >= 0) {
		phy_power_off(pcie->phy[i]);
err_phy_power_on:
		i--;
	}

	return ret;
}

static irqreturn_t pex_rst_isr(int irq, void *arg)
{
	struct tegra_pcie_dw_ep *pcie = arg;

	pcie->event = EP_PEX_RST_DE_ASSERT;
	schedule_work(&pcie->pcie_ep_work);
	return IRQ_HANDLED;
}

static int tegra_pcie_dw_ep_probe(struct platform_device *pdev)
{
	struct tegra_pcie_dw_ep *pcie;
	struct device_node *np = pdev->dev.of_node;
	struct phy **phy;
	struct pinctrl *pin = NULL;
	struct pinctrl_state *pin_state = NULL;
	char *name;
	int phy_count;
	u32 i = 0;
	int pex_rst_gpio;
	int irq;
	int ret = 0;

	pcie = devm_kzalloc(&pdev->dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	pcie->dev = &pdev->dev;

	ret = of_property_read_u32(np, "nvidia,num-lanes", &pcie->num_lanes);
	if (ret < 0) {
		dev_err(pcie->dev, "fail to read num-lanes: %d\n", ret);
		return ret;
	}
	ret = of_property_read_u32(np, "nvidia,cfg-link-cap-l1sub",
				   &pcie->cfg_link_cap_l1sub);
	if (ret < 0) {
		dev_err(pcie->dev, "fail to read cfg-link-cap-l1sub: %d\n",
			ret);
		pcie->cfg_link_cap_l1sub = CFG_LINK_CAP_L1SUB;
	}

	ret = of_property_read_u32(np, "nvidia,controller-id", &pcie->cid);
	if (ret) {
		dev_err(pcie->dev, "Controller-ID is missing in DT: %d\n", ret);
		return ret;
	}

	if (pcie->cid != CTRL_5) {
		ret = uphy_bpmp_pcie_controller_state_set(pcie->cid, true);
		if (ret) {
			dev_err(pcie->dev, "Enabling controller-%d failed:%d\n",
				pcie->cid, ret);
			return ret;
		}
	}

	pcie->pex_ctl_reg = devm_regulator_get(&pdev->dev, "vddio-pex-ctl");
	if (IS_ERR(pcie->pex_ctl_reg)) {
		dev_err(&pdev->dev, "fail to get regulator: %ld\n",
			PTR_ERR(pcie->pex_ctl_reg));
		return PTR_ERR(pcie->pex_ctl_reg);
	}
	ret = regulator_enable(pcie->pex_ctl_reg);
	if (ret < 0) {
		dev_err(&pdev->dev, "regulator enable failed: %d\n", ret);
		return ret;
	}

	pin = devm_pinctrl_get(pcie->dev);
	if (IS_ERR(pin)) {
		ret = PTR_ERR(pin);
		dev_err(pcie->dev, "pinctrl_get failed: %d\n", ret);
		return ret;
	}
	pin_state = pinctrl_lookup_state(pin, "pex_rst");
	if (!IS_ERR(pin_state)) {
		ret = pinctrl_select_state(pin, pin_state);
		if (ret < 0) {
			dev_err(pcie->dev, "setting pex_rst state fail: %d\n",
				ret);
			return ret;
		}
	}
	pin_state = pinctrl_lookup_state(pin, "clkreq");
	if (!IS_ERR(pin_state)) {
		ret = pinctrl_select_state(pin, pin_state);
		if (ret < 0) {
			dev_err(pcie->dev, "setting clkreq state fail: %d\n",
				ret);
			return ret;
		}
	}

	pcie->core_clk = devm_clk_get(&pdev->dev, "core_clk");
	if (IS_ERR(pcie->core_clk)) {
		dev_err(&pdev->dev, "Failed to get core clock\n");
		ret = PTR_ERR(pcie->core_clk);
		goto fail_core_clk;
	}

	pcie->appl_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						      "appl");
	if (!pcie->appl_res) {
		dev_err(&pdev->dev, "missing appl space\n");
		ret = PTR_ERR(pcie->appl_res);
		goto fail_core_clk;
	}
	pcie->appl_base = devm_ioremap_resource(&pdev->dev, pcie->appl_res);
	if (IS_ERR(pcie->appl_base)) {
		dev_err(&pdev->dev, "mapping appl space failed\n");
		ret = PTR_ERR(pcie->appl_base);
		goto fail_core_clk;
	}

	pcie->core_apb_rst = devm_reset_control_get(pcie->dev, "core_apb_rst");
	if (IS_ERR(pcie->core_apb_rst)) {
		dev_err(pcie->dev, "PCIE : core_apb_rst reset is missing\n");
		ret = PTR_ERR(pcie->core_apb_rst);
		goto fail_core_clk;
	}

	phy_count = of_property_count_strings(np, "phy-names");
	if (phy_count < 0) {
		dev_err(pcie->dev, "unable to find phy entries\n");
		ret = phy_count;
		goto fail_core_clk;
	}

	phy = devm_kcalloc(pcie->dev, phy_count, sizeof(*phy), GFP_KERNEL);
	if (!phy) {
		ret = PTR_ERR(phy);
		goto fail_core_clk;
	}

	for (i = 0; i < phy_count; i++) {
		name = kasprintf(GFP_KERNEL, "pcie-p2u-%u", i);
		phy[i] = devm_phy_get(pcie->dev, name);
		kfree(name);
		if (IS_ERR(phy[i])) {
			ret = PTR_ERR(phy[i]);
			goto fail_core_clk;
		}
	}

	pcie->phy_count = phy_count;
	pcie->phy = phy;

	ret = tegra_pcie_init_phy(pcie);
	if (ret) {
		dev_err(pcie->dev, "failed to init phy\n");
		goto fail_core_clk;
	}

	pcie->dbi_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						     "config");
	if (!pcie->dbi_res) {
		dev_err(&pdev->dev, "missing config space\n");
		ret = PTR_ERR(pcie->dbi_res);
		goto fail_dbi_res;
	}
	pcie->dbi_base = devm_ioremap_resource(&pdev->dev, pcie->dbi_res);
	if (IS_ERR(pcie->dbi_base)) {
		dev_err(&pdev->dev, "mapping dbi space failed\n");
		ret = PTR_ERR(pcie->dbi_base);
		goto fail_dbi_res;
	}

	pcie->atu_dma_res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   "atu_dma");
	if (!pcie->atu_dma_res) {
		dev_err(&pdev->dev, "missing atu_dma space\n");
		ret = PTR_ERR(pcie->atu_dma_res);
		goto fail_dbi_res;
	}
	pcie->atu_dma_base = devm_ioremap_resource(&pdev->dev,
						   pcie->atu_dma_res);
	if (IS_ERR(pcie->atu_dma_base)) {
		dev_err(&pdev->dev, "mapping atu_dma space failed\n");
		ret = PTR_ERR(pcie->atu_dma_base);
		goto fail_dbi_res;
	}

	ret = of_property_read_u16(np, "nvidia,device-id", &pcie->device_id);
	if (ret) {
		dev_err(pcie->dev, "Device-ID is missing in DT: %d\n", ret);
		goto fail_dbi_res;
	}

	ret = of_property_read_u32(np, "nvidia,bar0-size", &pcie->bar0_size);
	if (ret) {
		dev_info(pcie->dev, "Setting default BAR0 size to 1MB\n");
		pcie->bar0_size = SZ_1M;
	}

	ret = of_property_read_u32(np, "nvidia,controller-id", &pcie->cid);
	if (ret) {
		dev_err(pcie->dev, "Controller-ID is missing in DT: %d\n", ret);
		goto fail_dbi_res;
	}

	pcie->cpu_virt = dma_alloc_coherent(pcie->dev, pcie->bar0_size,
					    &pcie->dma_handle, GFP_KERNEL);
	if (!pcie->cpu_virt) {
		dev_err(pcie->dev, "BAR memory alloc failed\n");
		ret = -ENOMEM;
		goto fail_dbi_res;
	}
	dev_info(pcie->dev, "EP BAR DMA addr = 0x%llX\n", pcie->dma_handle);

	if (of_property_read_bool(pdev->dev.of_node, "nvidia,update_fc_fixup"))
		pcie->update_fc_fixup = true;

	/* Program what ASPM states sould get advertised */
	of_property_read_u8(np, "nvidia,disable-aspm-states",
			    &pcie->disabled_aspm_states);

	INIT_WORK(&pcie->pcie_ep_work, pcie_ep_work_fn);

	pcie->core_rst = devm_reset_control_get(pcie->dev, "core_rst");
	if (IS_ERR(pcie->core_rst)) {
		dev_err(pcie->dev, "PCIE : core_rst reset is missing\n");
		ret = PTR_ERR(pcie->core_rst);
		goto fail_dbi_res;
	}

	pcie->irq = platform_get_irq_byname(pdev, "intr");
	if (!pcie->irq) {
		dev_err(pcie->dev, "failed to get intr interrupt\n");
		ret = -ENODEV;
		goto fail_dbi_res;
	}

	ret = devm_request_irq(&pdev->dev, pcie->irq, tegra_pcie_irq_handler,
			       IRQF_SHARED, "tegra-pcie-intr", pcie);
	if (ret) {
		dev_err(pcie->dev, "failed to request \"intr\" irq\n");
		goto fail_dbi_res;
	}

	pex_rst_gpio = of_get_named_gpio(np, "nvidia,pex-rst-gpio", 0);
	if (!gpio_is_valid(pex_rst_gpio)) {
		dev_err(pcie->dev, "pex-rst-gpio is missing\n");
		ret = pex_rst_gpio;
		goto fail_dbi_res;
	}
	ret = devm_gpio_request(pcie->dev, pex_rst_gpio, "pex_rst_gpio");
	if (ret < 0) {
		dev_err(pcie->dev, "pex_rst_gpio request failed\n");
		goto fail_dbi_res;
	}
	ret = gpio_direction_input(pex_rst_gpio);
	if (ret < 0) {
		dev_err(pcie->dev, "pex_rst_gpio direction input failed\n");
		goto fail_dbi_res;
	}
	irq = gpio_to_irq(pex_rst_gpio);
	if (irq < 0) {
		dev_err(pcie->dev, "Unable to get irq for pex_rst_gpio\n");
		goto fail_dbi_res;
	}
	ret = devm_request_irq(pcie->dev, (unsigned int)irq, pex_rst_isr,
			       IRQF_TRIGGER_RISING, "pex_rst", (void *)pcie);
	if (ret < 0) {
		dev_err(pcie->dev, "Unable to request irq for pex_rst\n");
		goto fail_dbi_res;
	}

	return ret;

fail_dbi_res:
	tegra_pcie_disable_phy(pcie);
fail_core_clk:
	regulator_disable(pcie->pex_ctl_reg);
	return ret;
}

static const struct of_device_id tegra_pcie_dw_ep_of_match[] = {
	{ .compatible = "nvidia,tegra194-pcie-ep", },
	{},
};
MODULE_DEVICE_TABLE(of, tegra_pcie_dw_ep_of_match);

static struct platform_driver tegra_pcie_dw_ep_driver = {
	.remove		= __exit_p(tegra_pcie_dw_ep_remove),
	.driver = {
		.name	= "tegra-pcie-dw-ep",
		.of_match_table = tegra_pcie_dw_ep_of_match,
	},
};

module_platform_driver_probe(tegra_pcie_dw_ep_driver, tegra_pcie_dw_ep_probe);

MODULE_AUTHOR("Vidya Sagar <vidyas@nvidia.com>");
MODULE_DESCRIPTION("Nvidia PCIe End-Point controller driver");
MODULE_LICENSE("GPL v2");
