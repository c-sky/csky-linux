// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>

void pcibios_fixup_bus(struct pci_bus *bus)
{
}
EXPORT_SYMBOL(pcibios_fixup_bus);

resource_size_t pcibios_align_resource(void *data, const struct resource *res,
				resource_size_t size, resource_size_t align)
{
	return res->start;
}
EXPORT_SYMBOL(pcibios_align_resource);
