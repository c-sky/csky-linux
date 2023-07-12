// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2017 Cadence
// Cadence PCIe host controller driver.
// Author: Cyrille Pitchen <cyrille.pitchen@free-electrons.com>

#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/msi.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>

#include "pcie-cadence.h"
#include "pcie-cadence-sophgo.h"

#define MAX_MSI_IRQS			512
#define MAX_MSI_IRQS_PER_CTRL		1
#define MAX_MSI_CTRLS			(MAX_MSI_IRQS / MAX_MSI_IRQS_PER_CTRL)
#define MSI_DEF_NUM_VECTORS		512
#define BYTE_NUM_PER_MSI_VEC		4

// mango sideband signals
#define CDNS_PCIE_CFG_MANGO_APB     0x1800000
#define CDNS_PCIE_IRS_REG0400       0x0400
#define CDNS_PCIE_IRS_REG0404       0x0404
#define CDNS_PCIE_IRS_REG0418       0x0418
#define CDNS_PCIE_IRS_REG041C       0x041C
#define CDNS_PCIE_IRS_REG0804       0x0804
#define CDNS_PCIE_IRS_REG080C       0x080C
#define CDNS_PCIE_IRS_REG0810       0x0810
#define CDNS_PCIE_IRS_REG085C       0x085C
#define CDNS_PCIE_IRS_REG0860       0x0860
#define CDNS_PCIE_IRS_REG0864       0x0864
#define CDNS_PCIE_IRS_REG0868       0x0868
#define CDNS_PCIE_IRS_REG086C       0x086C

#define CDNS_PCIE_IRS_REG0804_CLR_LINK0_MSI_IN_BIT		2
#define CDNS_PCIE_IRS_REG0804_CLR_LINK1_MSI_IN_BIT		3
#define CDNS_PCIE_IRS_REG0810_ST_LINK0_MSI_IN_BIT		2
#define CDNS_PCIE_IRS_REG0810_ST_LINK1_MSI_IN_BIT		3

#define CDNS_PLAT_CPU_TO_BUS_ADDR       0xCFFFFFFFFF

struct cdns_pcie_database {
	void __iomem *pcie_reg_base;
};

static struct cdns_pcie_database cdns_pcie_db;

static inline void cdns_pcie_rp_writel(struct cdns_pcie *pcie,
				       u32 reg, u32 value)
{
	writel(value, pcie->reg_base + CDNS_PCIE_RP_BASE + reg);
}

static inline u32 cdns_pcie_rp_readl(struct cdns_pcie *pcie,
				       u32 reg)
{
	return readl(pcie->reg_base + CDNS_PCIE_RP_BASE + reg);
}

/**
 * struct cdns_mango_pcie_rc - private data for this PCIe Root Complex driver
 * @pcie: Cadence PCIe controller
 * @dev: pointer to PCIe device
 * @cfg_res: start/end offsets in the physical system memory to map PCI
 *           configuration space accesses
 * @bus_range: first/last buses behind the PCIe host controller
 * @cfg_base: IO mapped window to access the PCI configuration space of a
 *            single function at a time
 * @max_regions: maximum number of regions supported by the hardware
 * @no_bar_nbits: Number of bits to keep for inbound (PCIe -> CPU) address
 *                translation (nbits sets into the "no BAR match" register)
 * @vendor_id: PCI vendor ID
 * @device_id: PCI device ID
 */
struct cdns_mango_pcie_rc {
	struct cdns_pcie	pcie;
	struct device		*dev;
	struct resource		*cfg_res;
	struct resource		*bus_range;
	void __iomem		*cfg_base;
	u32			max_regions;
	u32			no_bar_nbits;
	u16			vendor_id;
	u16			device_id;
	u16			pcie_id;
	u16			link_id;
	u32			top_intc_used;
	struct irq_domain	*msi_domain;
	int			msi_irq;
	struct irq_domain	*irq_domain;
	dma_addr_t		msi_data;
	void			*msi_page;
	struct irq_chip		*msi_irq_chip;
	u32			num_vectors;
	u32			num_applied_vecs;
	u32			irq_mask[MAX_MSI_CTRLS];
	struct pci_bus		*root_bus;
	raw_spinlock_t		lock;
	DECLARE_BITMAP(msi_irq_in_use, MAX_MSI_IRQS);
};

static u64 cdns_mango_cpu_addr_fixup(struct cdns_pcie *pcie, u64 cpu_addr)
{
	return cpu_addr & CDNS_PLAT_CPU_TO_BUS_ADDR;
}

static const struct cdns_pcie_ops cdns_mango_ops = {
	.cpu_addr_fixup = cdns_mango_cpu_addr_fixup,
};

static void __iomem *cdns_mango_pci_map_bus(struct pci_bus *bus, unsigned int devfn,
					    int where)
{
	struct pci_host_bridge *bridge = pci_find_host_bridge(bus);
	struct cdns_mango_pcie_rc *rc = pci_host_bridge_priv(bridge);
	struct cdns_pcie *pcie = &rc->pcie;
	unsigned int busn = bus->number;
	u32 addr0, desc0;

	if (pci_is_root_bus(bus)) {
		/*
		 * Only the root port (devfn == 0) is connected to this bus.
		 * All other PCI devices are behind some bridge hence on another
		 * bus.
		 */
		if (devfn)
			return NULL;

		return pcie->reg_base + CDNS_PCIE_RP_BASE + (where & 0xfff);
	}
	/* Check that the link is up */
	if (!(cdns_pcie_readl(pcie, CDNS_PCIE_LM_BASE) & 0x1))
		return NULL;
	/* Clear AXI link-down status */
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_LINKDOWN, 0x0);

	/* Update Output registers for AXI region 0. */
	addr0 = CDNS_PCIE_AT_OB_REGION_PCI_ADDR0_NBITS(12) |
		CDNS_PCIE_AT_OB_REGION_PCI_ADDR0_DEVFN(devfn) |
		CDNS_PCIE_AT_OB_REGION_PCI_ADDR0_BUS(busn);
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_OB_REGION_PCI_ADDR0(0), addr0);

	/* Configuration Type 0 or Type 1 access. */
	desc0 = CDNS_PCIE_AT_OB_REGION_DESC0_HARDCODED_RID |
		CDNS_PCIE_AT_OB_REGION_DESC0_DEVFN(0);
	/*
	 * The bus number was already set once for all in desc1 by
	 * cdns_pcie_host_init_address_translation().
	 */
	if (busn == bridge->busnr + 1)
		desc0 |= CDNS_PCIE_AT_OB_REGION_DESC0_TYPE_CONF_TYPE0;
	else
		desc0 |= CDNS_PCIE_AT_OB_REGION_DESC0_TYPE_CONF_TYPE1;
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_OB_REGION_DESC0(0), desc0);

	return rc->cfg_base + (where & 0xfff);
}

int cdns_pcie_config_read(struct pci_bus *bus, unsigned int devfn,
			    int where, int size, u32 *val)
{
	unsigned long addr;
	unsigned int value, offset;
	void __iomem *aligned_addr;

	if ((bus->number != 0) && (bus->number != 0x40) &&
	    (bus->number != 0x80) && (bus->number != 0xc0))
		return pci_generic_config_read(bus, devfn, where, size, val);

	addr = (unsigned long)bus->ops->map_bus(bus, devfn, where);
	if (!addr) {
		*val = ~0;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	if (size == 1) {
		offset = addr & 0x3;
		aligned_addr = (void __iomem *)(addr & ~0x3UL);
		value = readl(aligned_addr);
		*val = (value >> (8 * offset)) & 0xff;
	} else if (size == 2) {
		WARN_ON((addr & 0x1) != 0); // address should be aligned to 2 bytes
		offset = addr & 0x3;
		aligned_addr = (void __iomem *)(addr & ~0x3UL);
		value = readl(aligned_addr);
		*val = (value >> (8 * offset)) & 0xffff;
	} else {
		WARN_ON((addr & 0x3) != 0); // address should be aligned to 4 bytes
		*val = readl((void __iomem *)(addr));
	}

	return PCIBIOS_SUCCESSFUL;
}

int cdns_pcie_config_write(struct pci_bus *bus, unsigned int devfn,
			     int where, int size, u32 val)
{
	unsigned long addr;
	unsigned int value, offset;
	void __iomem *aligned_addr;

	if ((bus->number != 0) && (bus->number != 0x40) &&
	    (bus->number != 0x80) && (bus->number != 0xc0))
		return pci_generic_config_write(bus, devfn, where, size, val);

	addr = (unsigned long)bus->ops->map_bus(bus, devfn, where);
	if (!addr)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (size == 1) {
		offset = addr & 0x3;
		aligned_addr = (void __iomem *)(addr & ~0x3UL);
		value = readl(aligned_addr);
		value &= ~(0xFF << (8 * offset));
		value |= ((val << (8 * offset)) & (0xFF << (8 * offset)));
		writel(value, aligned_addr);
	} else if (size == 2) {
		WARN_ON((addr & 0x1) != 0);
		offset = addr & 0x3;
		aligned_addr = (void __iomem *)(addr & ~0x3UL);
		value = readl(aligned_addr);
		value &= ~(0xFFFF << (8 * offset));
		value |= ((val << (8 * offset)) & (0xFFFF << (8 * offset)));
		writel(value, aligned_addr);
	} else {
		WARN_ON((addr & 0x3) != 0);
		writel(val, (void __iomem *)(addr));
	}

	return PCIBIOS_SUCCESSFUL;
}


static struct pci_ops cdns_pcie_host_ops = {
	.map_bus	= cdns_mango_pci_map_bus,
	.read		= cdns_pcie_config_read,
	.write		= cdns_pcie_config_write,
};

static const struct of_device_id cdns_pcie_host_of_match[] = {
	{ .compatible = "sophgo,cdns-pcie-host" },

	{ },
};

static int cdns_pcie_host_init_root_port(struct cdns_mango_pcie_rc *rc)
{
	struct cdns_pcie *pcie = &rc->pcie;
	u32 value, ctrl;
	u32 id;

	/*
	 * Set the root complex BAR configuration register:
	 * - disable both BAR0 and BAR1.
	 * - enable Prefetchable Memory Base and Limit registers in type 1
	 *   config space (64 bits).
	 * - enable IO Base and Limit registers in type 1 config
	 *   space (32 bits).
	 */
	ctrl = CDNS_PCIE_LM_BAR_CFG_CTRL_DISABLED;
	value = CDNS_PCIE_LM_RC_BAR_CFG_BAR0_CTRL(ctrl) |
		CDNS_PCIE_LM_RC_BAR_CFG_BAR1_CTRL(ctrl) |
		CDNS_PCIE_LM_RC_BAR_CFG_PREFETCH_MEM_ENABLE |
		CDNS_PCIE_LM_RC_BAR_CFG_PREFETCH_MEM_64BITS |
		CDNS_PCIE_LM_RC_BAR_CFG_IO_ENABLE |
		CDNS_PCIE_LM_RC_BAR_CFG_IO_32BITS;
	cdns_pcie_writel(pcie, CDNS_PCIE_LM_RC_BAR_CFG, value);

	/* Set root port configuration space */
	if (rc->vendor_id != 0xffff) {
		id = CDNS_PCIE_LM_ID_VENDOR(rc->vendor_id) |
			CDNS_PCIE_LM_ID_SUBSYS(rc->vendor_id);
		cdns_pcie_writel(pcie, CDNS_PCIE_LM_ID, id);
	}

	if (rc->device_id != 0xffff) {
		value = cdns_pcie_rp_readl(pcie, PCI_VENDOR_ID);
		value &= 0x0000FFFF;
		value |= (rc->device_id << 16);
		cdns_pcie_rp_writel(pcie, PCI_VENDOR_ID, value);
	}

	cdns_pcie_rp_writel(pcie, PCI_CLASS_REVISION, PCI_CLASS_BRIDGE_PCI << 16);

	return 0;
}

static int cdns_pcie_host_init_address_translation(struct cdns_mango_pcie_rc *rc)
{
	struct cdns_pcie *pcie = &rc->pcie;
	struct pci_host_bridge *bridge = pci_host_bridge_from_priv(rc);
	struct resource *cfg_res = rc->cfg_res;
	struct resource_entry *entry = NULL;
	u32 addr0, addr1, desc1;
	u64 cpu_addr;
	int r, busnr = 0;

	entry = resource_list_first_type(&bridge->windows, IORESOURCE_BUS);
	if (entry)
		busnr = entry->res->start;

	/*
	 * Reserve region 0 for PCI configure space accesses:
	 * OB_REGION_PCI_ADDR0 and OB_REGION_DESC0 are updated dynamically by
	 * cdns_pci_map_bus(), other region registers are set here once for all.
	 */
	addr1 = 0; /* Should be programmed to zero. */
	desc1 = CDNS_PCIE_AT_OB_REGION_DESC1_BUS(busnr);
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_OB_REGION_PCI_ADDR1(0), addr1);
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_OB_REGION_DESC1(0), desc1);

	cpu_addr = cfg_res->start;
	addr0 = CDNS_PCIE_AT_OB_REGION_CPU_ADDR0_NBITS(12) |
		(lower_32_bits(cpu_addr) & GENMASK(31, 8));
	addr1 = upper_32_bits(cpu_addr);
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_OB_REGION_CPU_ADDR0(0), addr0);
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_OB_REGION_CPU_ADDR1(0), addr1);

	r = 1;
	resource_list_for_each_entry(entry, &bridge->windows) {
		struct resource *res = entry->res;
		u64 pci_addr = res->start - entry->offset;

		if (resource_type(res) == IORESOURCE_IO)
			cdns_pcie_set_outbound_region(pcie, busnr, 0, r,
						      true,
						      pci_pio_to_address(res->start),
						      pci_addr,
						      resource_size(res));
		else
			cdns_pcie_set_outbound_region(pcie, busnr, 0, r,
						      false,
						      res->start,
						      pci_addr,
						      resource_size(res));

		r++;
	}

	/*
	 * Set Root Port no BAR match Inbound Translation registers:
	 * needed for MSI and DMA.
	 * Root Port BAR0 and BAR1 are disabled, hence no need to set their
	 * inbound translation registers.
	 */
	addr0 = CDNS_PCIE_AT_IB_RP_BAR_ADDR0_NBITS(rc->no_bar_nbits);
	addr1 = 0;
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_IB_RP_BAR_ADDR0(RP_NO_BAR), addr0);
	cdns_pcie_writel(pcie, CDNS_PCIE_AT_IB_RP_BAR_ADDR1(RP_NO_BAR), addr1);

	return 0;
}

static int cdns_pcie_msi_init(struct cdns_mango_pcie_rc *rc)
{
	struct device *dev = rc->dev;
	struct cdns_pcie *pcie = &rc->pcie;
	u32 apb_base = CDNS_PCIE_CFG_MANGO_APB;
	u64 msi_target = 0;
	u32 value = 0;

	// support 512 msi vectors
	rc->msi_page = dma_alloc_coherent(dev, 2048, &rc->msi_data,
					  (GFP_KERNEL|GFP_DMA32|__GFP_ZERO));
	if (rc->msi_page == NULL)
		return -1;

	dev_info(dev, "msi_data is 0x%llx\n", rc->msi_data);
	msi_target = (u64)rc->msi_data;

	if (rc->link_id == 1) {
		apb_base -= 0x800000;
		/* Program the msi_data */
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0868),
				 lower_32_bits(msi_target));
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG086C),
				 upper_32_bits(msi_target));

		value = cdns_pcie_readl(pcie, (apb_base + CDNS_PCIE_IRS_REG080C));
		value = (value & 0xffff0000) | MAX_MSI_IRQS;
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG080C), value);
	} else {
		/* Program the msi_data */
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0860),
				 lower_32_bits(msi_target));
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0864),
				 upper_32_bits(msi_target));

		value = cdns_pcie_readl(pcie, (apb_base + CDNS_PCIE_IRS_REG085C));
		value = (value & 0x0000ffff) | (MAX_MSI_IRQS << 16);
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG085C), value);
	}

	return 0;
}

static int cdns_pcie_host_init(struct device *dev, struct cdns_mango_pcie_rc *rc)
{
	int err;

	err = cdns_pcie_host_init_root_port(rc);
	if (err)
		return err;

	err = cdns_pcie_host_init_address_translation(rc);
	if (err)
		return err;

	if (rc->top_intc_used == 0) {
		rc->num_vectors = MSI_DEF_NUM_VECTORS;
		rc->num_applied_vecs = 0;
		if (IS_ENABLED(CONFIG_PCI_MSI)) {
			err = cdns_pcie_msi_init(rc);
			if (err)
				return err;
		}
	}
	return 0;
}


static void cdns_pcie_msi_ack_irq(struct irq_data *d)
{
	irq_chip_ack_parent(d);
}

static void cdns_pcie_msi_mask_irq(struct irq_data *d)
{
	pci_msi_mask_irq(d);
	irq_chip_mask_parent(d);
}

static void cdns_pcie_msi_unmask_irq(struct irq_data *d)
{
	pci_msi_unmask_irq(d);
	irq_chip_unmask_parent(d);
}

static struct irq_chip cdns_pcie_msi_irq_chip = {
	.name = "cdns-msi",
	.irq_ack = cdns_pcie_msi_ack_irq,
	.irq_mask = cdns_pcie_msi_mask_irq,
	.irq_unmask = cdns_pcie_msi_unmask_irq,
};

static struct msi_domain_info cdns_pcie_msi_domain_info = {
	.flags	= (MSI_FLAG_USE_DEF_DOM_OPS | MSI_FLAG_USE_DEF_CHIP_OPS),
	.chip	= &cdns_pcie_msi_irq_chip,
};

static int cdns_pcie_msi_setup_for_top_intc(struct cdns_mango_pcie_rc *rc, int intc_id)
{
	struct irq_domain *irq_parent = cdns_pcie_get_parent_irq_domain(intc_id);
	struct fwnode_handle *fwnode = of_node_to_fwnode(rc->dev->of_node);

	rc->msi_domain = pci_msi_create_irq_domain(fwnode,
						   &cdns_pcie_msi_domain_info,
						   irq_parent);
	if (!rc->msi_domain) {
		dev_err(rc->dev, "create msi irq domain failed\n");
		return -ENODEV;
	}

	return 0;
}

/* MSI int handler */
irqreturn_t cdns_handle_msi_irq(struct cdns_mango_pcie_rc *rc)
{
	u32 i, pos, irq;
	unsigned long val;
	u32 status, num_vectors;
	irqreturn_t ret = IRQ_NONE;

	num_vectors = rc->num_applied_vecs;
	for (i = 0; i <= num_vectors; i++) {
		status = readl((void *)(rc->msi_page + i * BYTE_NUM_PER_MSI_VEC));
		if (!status)
			continue;

		ret = IRQ_HANDLED;
		val = status;
		pos = 0;
		while ((pos = find_next_bit(&val, MAX_MSI_IRQS_PER_CTRL,
					    pos)) != MAX_MSI_IRQS_PER_CTRL) {
			irq = irq_find_mapping(rc->irq_domain,
					       (i * MAX_MSI_IRQS_PER_CTRL) +
					       pos);
			generic_handle_irq(irq);
			pos++;
		}
		writel(0, ((void *)(rc->msi_page) + i * BYTE_NUM_PER_MSI_VEC));
	}
	if (ret == IRQ_NONE) {
		ret = IRQ_HANDLED;
		for (i = 0; i <= num_vectors; i++) {
			for (pos = 0; pos < MAX_MSI_IRQS_PER_CTRL; pos++) {
				irq = irq_find_mapping(rc->irq_domain,
						       (i * MAX_MSI_IRQS_PER_CTRL) +
						       pos);
				if (!irq)
					continue;
				generic_handle_irq(irq);
			}
		}
	}

	return ret;
}

static irqreturn_t cdns_pcie_irq_handler(int irq, void *arg)
{
	struct cdns_mango_pcie_rc *rc = arg;
	struct cdns_pcie *pcie = &rc->pcie;
	u32 apb_base = CDNS_PCIE_CFG_MANGO_APB;
	u32 status = 0;
	u32 st_msi_in_bit = 0;
	u32 clr_msi_in_bit = 0;

	if (rc->link_id == 1) {
		apb_base -= 0x800000;
		st_msi_in_bit = CDNS_PCIE_IRS_REG0810_ST_LINK1_MSI_IN_BIT;
		clr_msi_in_bit = CDNS_PCIE_IRS_REG0804_CLR_LINK1_MSI_IN_BIT;
	} else {
		st_msi_in_bit = CDNS_PCIE_IRS_REG0810_ST_LINK0_MSI_IN_BIT;
		clr_msi_in_bit = CDNS_PCIE_IRS_REG0804_CLR_LINK0_MSI_IN_BIT;
	}

	status = cdns_pcie_readl(pcie, (apb_base + CDNS_PCIE_IRS_REG0810));
	if ((status >> st_msi_in_bit) & 0x1) {
		WARN_ON(!IS_ENABLED(CONFIG_PCI_MSI));

		//clear msi interrupt bit reg0810[2]
		status = cdns_pcie_readl(pcie, (apb_base + CDNS_PCIE_IRS_REG0804));
		status |= ((u32)0x1 << clr_msi_in_bit);
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0804), status);

		status &= ~((u32)0x1 << clr_msi_in_bit);
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0804), status);

		cdns_handle_msi_irq(rc);
	}

	return IRQ_HANDLED;
}

/* Chained MSI interrupt service routine */
static void cdns_chained_msi_isr(struct irq_desc *desc)
{
	struct irq_chip *chip = irq_desc_get_chip(desc);
	struct cdns_mango_pcie_rc *rc;
	struct cdns_pcie *pcie;
	u32 apb_base = CDNS_PCIE_CFG_MANGO_APB;
	u32 status = 0;
	u32 st_msi_in_bit = 0;
	u32 clr_msi_in_bit = 0;

	chained_irq_enter(chip, desc);

	rc = irq_desc_get_handler_data(desc);
	pcie = &rc->pcie;
	if (rc->link_id == 1) {
		apb_base -= 0x800000;
		st_msi_in_bit = CDNS_PCIE_IRS_REG0810_ST_LINK1_MSI_IN_BIT;
		clr_msi_in_bit = CDNS_PCIE_IRS_REG0804_CLR_LINK1_MSI_IN_BIT;
	} else {
		st_msi_in_bit = CDNS_PCIE_IRS_REG0810_ST_LINK0_MSI_IN_BIT;
		clr_msi_in_bit = CDNS_PCIE_IRS_REG0804_CLR_LINK0_MSI_IN_BIT;
	}

	status = cdns_pcie_readl(pcie, (apb_base + CDNS_PCIE_IRS_REG0810));
	if ((status >> st_msi_in_bit) & 0x1) {
		WARN_ON(!IS_ENABLED(CONFIG_PCI_MSI));

		//clear msi interrupt bit reg0810[2]
		status = cdns_pcie_readl(pcie, (apb_base + CDNS_PCIE_IRS_REG0804));
		status |= ((u32)0x1 << clr_msi_in_bit);
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0804), status);

		status &= ~((u32)0x1 << clr_msi_in_bit);
		cdns_pcie_writel(pcie, (apb_base + CDNS_PCIE_IRS_REG0804), status);

		cdns_handle_msi_irq(rc);
	}

	chained_irq_exit(chip, desc);
}

static int cdns_pci_msi_set_affinity(struct irq_data *d,
				   const struct cpumask *mask, bool force)
{
	return -EINVAL;
}

static void cdns_pci_bottom_mask(struct irq_data *d)
{
}

static void cdns_pci_bottom_unmask(struct irq_data *d)
{
}

static void cdns_pci_setup_msi_msg(struct irq_data *d, struct msi_msg *msg)
{
	struct cdns_mango_pcie_rc *rc = irq_data_get_irq_chip_data(d);
	u64 msi_target;

	msi_target = (u64)rc->msi_data;

	msg->address_lo = lower_32_bits(msi_target) + BYTE_NUM_PER_MSI_VEC * d->hwirq;
	msg->address_hi = upper_32_bits(msi_target);
	msg->data = 1;

	rc->num_applied_vecs = d->hwirq;

	dev_err(rc->dev, "msi#%d address_hi %#x address_lo %#x\n",
		(int)d->hwirq, msg->address_hi, msg->address_lo);
}

static void cdns_pci_bottom_ack(struct irq_data *d)
{
}

static struct irq_chip cdns_pci_msi_bottom_irq_chip = {
	.name = "CDNS-PCI-MSI",
	.irq_ack = cdns_pci_bottom_ack,
	.irq_compose_msi_msg = cdns_pci_setup_msi_msg,
	.irq_set_affinity = cdns_pci_msi_set_affinity,
	.irq_mask = cdns_pci_bottom_mask,
	.irq_unmask = cdns_pci_bottom_unmask,
};

static int cdns_pcie_irq_domain_alloc(struct irq_domain *domain,
				    unsigned int virq, unsigned int nr_irqs,
				    void *args)
{
	struct cdns_mango_pcie_rc *rc = domain->host_data;
	unsigned long flags;
	u32 i;
	int bit;

	raw_spin_lock_irqsave(&rc->lock, flags);

	bit = bitmap_find_free_region(rc->msi_irq_in_use, rc->num_vectors,
				      order_base_2(nr_irqs));

	raw_spin_unlock_irqrestore(&rc->lock, flags);

	if (bit < 0)
		return -ENOSPC;

	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_info(domain, virq + i, bit + i,
				    rc->msi_irq_chip,
				    rc, handle_edge_irq,
				    NULL, NULL);

	return 0;
}

static void cdns_pcie_irq_domain_free(struct irq_domain *domain,
				    unsigned int virq, unsigned int nr_irqs)
{
	struct irq_data *d = irq_domain_get_irq_data(domain, virq);
	struct cdns_mango_pcie_rc *rc = irq_data_get_irq_chip_data(d);
	unsigned long flags;

	raw_spin_lock_irqsave(&rc->lock, flags);

	bitmap_release_region(rc->msi_irq_in_use, d->hwirq,
			      order_base_2(nr_irqs));

	raw_spin_unlock_irqrestore(&rc->lock, flags);
}

static const struct irq_domain_ops cdns_pcie_msi_domain_ops = {
	.alloc	= cdns_pcie_irq_domain_alloc,
	.free	= cdns_pcie_irq_domain_free,
};

int cdns_pcie_allocate_domains(struct cdns_mango_pcie_rc *rc)
{
	struct fwnode_handle *fwnode = of_node_to_fwnode(rc->dev->of_node);

	rc->irq_domain = irq_domain_create_linear(fwnode, rc->num_vectors,
					       &cdns_pcie_msi_domain_ops, rc);
	if (!rc->irq_domain) {
		dev_err(rc->dev, "Failed to create IRQ domain\n");
		return -ENOMEM;
	}

	irq_domain_update_bus_token(rc->irq_domain, DOMAIN_BUS_NEXUS);

	rc->msi_domain = pci_msi_create_irq_domain(fwnode,
						   &cdns_pcie_msi_domain_info,
						   rc->irq_domain);
	if (!rc->msi_domain) {
		dev_err(rc->dev, "Failed to create MSI domain\n");
		irq_domain_remove(rc->irq_domain);
		return -ENOMEM;
	}

	return 0;
}

void cdns_pcie_free_msi(struct cdns_mango_pcie_rc *rc)
{
	if (rc->msi_irq) {
		irq_set_chained_handler(rc->msi_irq, NULL);
		irq_set_handler_data(rc->msi_irq, NULL);
	}

	irq_domain_remove(rc->msi_domain);
	irq_domain_remove(rc->irq_domain);

	if (rc->msi_page)
		dma_free_coherent(rc->dev, 1024, rc->msi_page, rc->msi_data);

}

static int cdns_pcie_msi_setup(struct cdns_mango_pcie_rc *rc)
{
	int ret = 0;

	raw_spin_lock_init(&rc->lock);

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		rc->msi_irq_chip = &cdns_pci_msi_bottom_irq_chip;

		ret = cdns_pcie_allocate_domains(rc);
		if (ret)
			return ret;

		if (rc->msi_irq)
			irq_set_chained_handler_and_data(rc->msi_irq, cdns_chained_msi_isr, rc);
	}

	return ret;
}

static int cdns_pcie_host_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct pci_host_bridge *bridge;
	struct cdns_mango_pcie_rc *rc;
	struct cdns_pcie *pcie;
	struct resource *res;
	int ret;
	int phy_count;
	int top_intc_id = -1;

	bridge = devm_pci_alloc_host_bridge(dev, sizeof(*rc));
	if (!bridge)
		return -ENOMEM;

	rc = pci_host_bridge_priv(bridge);
	rc->dev = dev;

	pcie = &rc->pcie;
	pcie->is_rc = true;
	pcie->ops = &cdns_mango_ops;

	rc->max_regions = 32;
	of_property_read_u32(np, "cdns,max-outbound-regions", &rc->max_regions);

	rc->no_bar_nbits = 32;
	of_property_read_u32(np, "cdns,no-bar-match-nbits", &rc->no_bar_nbits);

	rc->vendor_id = 0xffff;
	of_property_read_u16(np, "vendor-id", &rc->vendor_id);

	rc->device_id = 0xffff;
	of_property_read_u16(np, "device-id", &rc->device_id);

	rc->pcie_id = 0xffff;
	of_property_read_u16(np, "pcie-id", &rc->pcie_id);

	rc->link_id = 0xffff;
	of_property_read_u16(np, "link-id", &rc->link_id);

	rc->top_intc_used = 0;
	of_property_read_u32(np, "top-intc-used", &rc->top_intc_used);
	if (rc->top_intc_used == 1)
		of_property_read_u32(np, "top-intc-id", &top_intc_id);

	if (rc->link_id == 0) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "reg");
		pcie->reg_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(pcie->reg_base)) {
			dev_err(dev, "missing \"reg\"\n");
			return PTR_ERR(pcie->reg_base);
		}
		cdns_pcie_db.pcie_reg_base = pcie->reg_base;
	} else if (rc->link_id == 1) {
		pcie->reg_base = cdns_pcie_db.pcie_reg_base + 0x800000;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cfg");
	rc->cfg_base = devm_pci_remap_cfg_resource(dev, res);
	if (IS_ERR(rc->cfg_base)) {
		dev_err(dev, "missing \"cfg\"\n");
		return PTR_ERR(rc->cfg_base);
	}
	rc->cfg_res = res;

	ret = cdns_pcie_init_phy(dev, pcie);
	if (ret) {
		dev_err(dev, "failed to init phy\n");
		return ret;
	}
	platform_set_drvdata(pdev, pcie);

	pm_runtime_enable(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		dev_err(dev, "pm_runtime_get_sync() failed\n");
		goto err_get_sync;
	}

	ret = cdns_pcie_host_init(dev, rc);
	if (ret)
		goto err_init;

	if ((rc->top_intc_used == 0) && (IS_ENABLED(CONFIG_PCI_MSI))) {
		rc->msi_irq = platform_get_irq_byname(pdev, "msi");
		if (rc->msi_irq <= 0) {
			dev_err(dev, "failed to get MSI irq\n");
			goto err_init_irq;
		}

		ret = devm_request_irq(dev, rc->msi_irq, cdns_pcie_irq_handler,
				       IRQF_SHARED | IRQF_NO_THREAD,
				       "cdns-pcie-irq", rc);

		if (ret) {
			dev_err(dev, "failed to request MSI irq\n");
			goto err_init_irq;
		}
	}

	bridge->dev.parent = dev;
	bridge->ops = &cdns_pcie_host_ops;
	bridge->map_irq = of_irq_parse_and_map_pci;
	bridge->swizzle_irq = pci_common_swizzle;
	if (rc->top_intc_used == 0)
		bridge->sysdata = rc;

	if (rc->top_intc_used == 0) {
		ret = cdns_pcie_msi_setup(rc);
		if (ret < 0)
			goto err_host_probe;
	} else if (rc->top_intc_used == 1) {
		ret = cdns_pcie_msi_setup_for_top_intc(rc, top_intc_id);
		if (ret < 0)
			goto err_host_probe;
	}

	ret = pci_host_probe(bridge);
	if (ret < 0)
		goto err_host_probe;

	return 0;

 err_host_probe:
 err_init_irq:
	if ((rc->top_intc_used == 0) && pci_msi_enabled())
		cdns_pcie_free_msi(rc);

 err_init:
	pm_runtime_put_sync(dev);

 err_get_sync:
	pm_runtime_disable(dev);
	cdns_pcie_disable_phy(pcie);
	phy_count = pcie->phy_count;
	while (phy_count--)
		device_link_del(pcie->link[phy_count]);

	return ret;
}

static void cdns_pcie_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cdns_pcie *pcie = dev_get_drvdata(dev);
	int ret;

	ret = pm_runtime_put_sync(dev);
	if (ret < 0)
		dev_dbg(dev, "pm_runtime_put_sync failed\n");

	pm_runtime_disable(dev);
	cdns_pcie_disable_phy(pcie);
}

static struct platform_driver cdns_pcie_host_driver = {
	.driver = {
		.name = "cdns-pcie-host",
		.of_match_table = cdns_pcie_host_of_match,
		.pm	= &cdns_pcie_pm_ops,
	},
	.probe = cdns_pcie_host_probe,
	.shutdown = cdns_pcie_shutdown,
};
builtin_platform_driver(cdns_pcie_host_driver);
