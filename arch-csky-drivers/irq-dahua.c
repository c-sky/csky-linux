// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/io.h>

static unsigned int intc_reg;

#define DHC_PIC_BASE	intc_reg

/* register */
#define DHC_REG_PIC_MODE        0x0000
#define DHC_REG_PIC_PO          0x0004
#define DHC_REG_PIC_MASK        0x0008
#define DHC_REG_PIC_VECTOR      0x000C
#define DHC_REG_PIC_COW1        0x0010

#define DHC_REG_PIC_PRIOR0      0x0014
#define DHC_REG_PIC_PRIOR1      0x0018
#define DHC_REG_PIC_PRIOR2      0x001C
#define DHC_REG_PIC_PRIOR3      0x0020
#define DHC_REG_PIC_PRIOR4      0x0024
#define DHC_REG_PIC_PRIOR5      0x0028
#define DHC_REG_PIC_PRIOR6      0x002C
#define DHC_REG_PIC_PRIOR7      0x0030

#define DHC_REG_PIC_COW2        0x0034
#define DHC_REG_PIC_STATUS      0x0054

#define DHC_REG_PIC_STATUS1     0x0058
#define DHC_REG_PIC_MODE1       0x0060
#define DHC_REG_PIC_PO1         0x0064
#define DHC_REG_PIC_MASK1       0x0068
#define DHC_REG_PIC_PRIOR8      0x006c
#define DHC_REG_PIC_PRIOR9      0x0070
#define DHC_REG_PIC_PRIOR10     0x0074
#define DHC_REG_PIC_PRIOR11     0x0078
#define DHC_REG_PIC_PRIOR12     0x007c
#define DHC_REG_PIC_PRIOR13     0x0080
#define DHC_REG_PIC_PRIOR14     0x0084
#define DHC_REG_PIC_PRIOR15     0x0088

/* register map */
#define DHC_PR_PIC_MODE         ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_MODE ) )
#define DHC_PR_PIC_PO           ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PO ) )
#define DHC_PR_PIC_MASK         ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_MASK ) )
#define DHC_PR_PIC_VECTOR       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_VECTOR ) )
#define DHC_PR_PIC_COW1         ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_COW1 ) )

#define DHC_PR_PIC_PRIOR0       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR0 ) )
#define DHC_PR_PIC_PRIOR1       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR1 ) )
#define DHC_PR_PIC_PRIOR2       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR2 ) )
#define DHC_PR_PIC_PRIOR3       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR3 ) )
#define DHC_PR_PIC_PRIOR4       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR4 ) )
#define DHC_PR_PIC_PRIOR5       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR5 ) )
#define DHC_PR_PIC_PRIOR6       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR6 ) )
#define DHC_PR_PIC_PRIOR7       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR7 ) )

#define DHC_PR_PIC_COW2         ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_COW2 ) )
#define DHC_PR_PIC_STATUS       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_STATUS ) )


#define DHC_PR_PIC_STATUS1      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_STATUS1 ) )
#define DHC_PR_PIC_MODE1        ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_MODE1 ) )
#define DHC_PR_PIC_PO1          ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PO1 ) )
#define DHC_PR_PIC_MASK1        ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_MASK1 ) )
#define DHC_PR_PIC_PRIOR8       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR8 ) )
#define DHC_PR_PIC_PRIOR9       ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR9 ) )
#define DHC_PR_PIC_PRIOR10      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR10 ) )
#define DHC_PR_PIC_PRIOR11      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR11 ) )
#define DHC_PR_PIC_PRIOR12      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR12 ) )
#define DHC_PR_PIC_PRIOR13      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR13 ) )
#define DHC_PR_PIC_PRIOR14      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR14 ) )
#define DHC_PR_PIC_PRIOR15      ( *( volatile unsigned int *)( DHC_PIC_BASE + DHC_REG_PIC_PRIOR15 ) )

/* PIC_MODE */
#define DHC_REG_PIC_TRI_EDGE        ( 0x01 )
#define DHC_REG_PIC_TRI_EDGE_ALL    ( 0xFFFFFFFF )
#define DHC_REG_PIC_TRI_LEVEL       ( 0x00 )
#define DHC_REG_PIC_TRI_LEVEL_ALL   ( 0x00 )

/* PIC_PO */
#define DHC_REG_PIC_POS_H           ( 0x01 )        /* posedge or high level */
#define DHC_REG_PIC_POS_H_ALL       ( 0xFFFFFFFF )

#define DHC_REG_PIC_NEG_L           ( 0x00 )        /* negedge or low level */
#define DHC_REG_PIC_NEG_L_ALL       ( 0x00 )


/* PIC_MASK */
#define DHC_REG_PIC_MASK_BIT        ( 0x01 )
#define DHC_REG_PIC_MASK_ALL        ( 0xFFFFFFFF )

#define DHC_REG_PIC_UNMASK_BIT      ( 0x00 )
#define DHC_REG_PIC_UNMASK_ALL      ( 0x00 )

/* COW1 */
#define DHC_REG_PIC_COW1_EOI	    ( 0x02 )
#define DHC_REG_PIC_COW1_FEOI	    ( 0x03 )

#define DHC_NR_IRQS                 ( 64 )

static void ck_irq_mask(struct irq_data *d)
{
	if(d->irq < 32)
	{
		DHC_PR_PIC_MASK |= ( 1 << d->irq );
	}
	else
	{
		DHC_PR_PIC_MASK1 |= ( 1 << (d->irq - 32) );
	}
}

static void ck_irq_unmask(struct irq_data *d)
{
	if(d->irq < 32)
	{
		DHC_PR_PIC_MASK &= ~( 1 << d->irq );
	}
	else
	{
		DHC_PR_PIC_MASK1 &= ~( 1 << (d->irq - 32) );
	}

}

static struct irq_chip ck_irq_chip = {
	.name		= "dahua_intc_v1",
	.irq_mask	= ck_irq_mask,
	.irq_unmask	= ck_irq_unmask,
};

static int ck_irq_map(struct irq_domain *h, unsigned int virq,
				irq_hw_number_t hw_irq_num)
{
	irq_set_chip_and_handler(virq, &ck_irq_chip, handle_level_irq);
	return 0;
}

static const struct irq_domain_ops ck_irq_ops = {
	.map	= ck_irq_map,
	.xlate	= irq_domain_xlate_onecell,
};

static unsigned int ck_get_irqno(void)
{
	return DHC_PR_PIC_STATUS & 0x7f;
};

static int __init
__intc_init(struct device_node *np, struct device_node *parent, int ave)
{
	struct irq_domain *root_domain;

	if (parent)
		panic("pic not a root intc\n");

	intc_reg = (unsigned int)of_iomap(np, 0);
	if (!intc_reg)
		panic("%s, of_iomap err.\n", __func__);

	csky_get_auto_irqno = ck_get_irqno;

	/* set all pic_mode to edge */
	DHC_PR_PIC_MODE  = DHC_REG_PIC_TRI_EDGE_ALL;
	DHC_PR_PIC_MODE1 = DHC_REG_PIC_TRI_EDGE_ALL;

	/* set posedge */
	DHC_PR_PIC_PO  = DHC_REG_PIC_POS_H_ALL;
	DHC_PR_PIC_PO1 = DHC_REG_PIC_POS_H_ALL;

	/* set mask */
	DHC_PR_PIC_MASK  = DHC_REG_PIC_MASK_ALL;
	DHC_PR_PIC_MASK1 = DHC_REG_PIC_MASK_ALL;
	DHC_PR_PIC_COW2  = 0x42;

	/*
	 * Initial the Interrupt source priority level registers
	 */
	DHC_PR_PIC_PRIOR0  = 0x03020100 + 0x40404040;
	DHC_PR_PIC_PRIOR1  = 0x07060504 + 0x40404040;
	DHC_PR_PIC_PRIOR2  = 0x0b0a0908 + 0x40404040;
	DHC_PR_PIC_PRIOR3  = 0x0f0e0d0c + 0x40404040;

	DHC_PR_PIC_PRIOR4  = 0x13121110 + 0x40404040;
	DHC_PR_PIC_PRIOR5  = 0x17161514 + 0x40404040;
	DHC_PR_PIC_PRIOR6  = 0x1b1a1918 + 0x40404040;
	DHC_PR_PIC_PRIOR7  = 0x1f1e1d1c + 0x40004040;

	DHC_PR_PIC_PRIOR8  = 0x03020100 + 0x40404040;
	DHC_PR_PIC_PRIOR9  = 0x07060504 + 0x40404040;
	DHC_PR_PIC_PRIOR10 = 0x0b0a0908 + 0x40404040;
	DHC_PR_PIC_PRIOR11 = 0x0f0e0d0c + 0x40404040;
	DHC_PR_PIC_PRIOR12 = 0x13121110 + 0x40404040;
	DHC_PR_PIC_PRIOR13 = 0x17161514 + 0x40404040;
	DHC_PR_PIC_PRIOR14 = 0x1b1a1918 + 0x40404040;
	DHC_PR_PIC_PRIOR15 = 0x1f1e1d1c + 0x40404040;

	root_domain = irq_domain_add_legacy(np, DHC_NR_IRQS, 0, 0, &ck_irq_ops, NULL);
	if (!root_domain)
		panic("root irq domain not available\n");

	irq_set_default_host(root_domain);

	return 0;
}

static int __init
intc_init(struct device_node *np, struct device_node *parent)
{
	return __intc_init(np, parent, true);
}
IRQCHIP_DECLARE(dahua_intc_v1, "dahua,intc-v1", intc_init);

