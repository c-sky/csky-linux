/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_CSKY_PCI_H
#define __ASM_CSKY_PCI_H

#define PCIBIOS_MIN_IO		0
#define PCIBIOS_MIN_MEM		0

/* C-SKY shim does not initialize PCI bus */
#define pcibios_assign_all_busses()	1

extern int isa_dma_bridge_buggy;

#ifdef CONFIG_PCI
static inline int pci_get_legacy_ide_irq(struct pci_dev *dev, int channel)
{
	/* no legacy IRQ on csky */
	return -ENODEV;
}

static inline int pci_proc_domain(struct pci_bus *bus)
{
	/* always show the domain in /proc */
	return 1;
}
#endif  /* CONFIG_PCI */

#define PCI_DMA_BUS_IS_PHYS	(1)

#endif /* __ASM_CSKY_PCI_H */
