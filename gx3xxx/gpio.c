/* linux/arch/csky/gx3xxx/gpio.c
 *
 * Copyright (c) 2004-2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * GX3201 GPIO support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>

#include <asm/irq.h>
#include <asm/io.h>

#include <mach/map.h>

/* MACRO DEFINITION */
#define GX_GPIO_INPUT	0
#define GX_GPIO_OUTPUT	1

#define REG_OFFSET_EPDDR		0x00
#define REG_OFFSET_EPDR		0x04
#define REG_OFFSET_BSET		0x08
#define REG_OFFSET_BCLR		0x0c

struct gx_gpio_output_bak_s{
	unsigned int porta;
	unsigned int portb;
	unsigned int portc;
};
static struct gx_gpio_output_bak_s gx_gpio_output_bak;

void gx_gpio_cfgpin(unsigned int pin, unsigned int function)
{
	void __iomem *base;
	unsigned long mask;
	unsigned long con;
	unsigned long flags;

	if (pin < 32){
		base = (void __iomem *)GX3201_VA_GPIOA;
		mask = 1 << pin;
	}else if (pin < 64){
		base = (void __iomem *)GX3201_VA_GPIOB;
		mask = 1 << (pin - 32);
	}else if (pin < 96){
		base = (void __iomem *)GX3201_VA_GPIOC;
		mask = 1 << (pin - 64);
	}else
		return;

	switch (function) {
		case GX_GPIO_INPUT:
			function = 0;
			break;
		case GX_GPIO_OUTPUT:
			function = 1;
			break;
		default:
			break;
	}

	/* modify the specified register wwith IRQs off */

	local_irq_save(flags);

	con  = __raw_readl(base + REG_OFFSET_EPDDR);
	con &= ~mask;
	con |= function;

	__raw_writel(con, base + REG_OFFSET_EPDDR);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(gx_gpio_cfgpin);

unsigned int gx_gpio_getcfg(unsigned int pin)
{
	void __iomem *base;
	unsigned long val;
	unsigned long mask;

	if (pin < 32){
		base = (void __iomem *)GX3201_VA_GPIOA;
		mask = 1 << pin;
	}else if (pin < 64){
		base = (void __iomem *)GX3201_VA_GPIOB;
		mask = 1 << (pin - 32);
	}else if (pin < 96){
		base = (void __iomem *)GX3201_VA_GPIOC;
		mask = 1 << (pin - 64);
	}else
		return -1;

	val =  __raw_readl(base + REG_OFFSET_EPDDR);

	if (val & mask)
		return GX_GPIO_OUTPUT;
	else
		return GX_GPIO_INPUT;
}

EXPORT_SYMBOL(gx_gpio_getcfg);

void gx_gpio_setpin(unsigned int pin, unsigned int to)
{
	void __iomem *base;
	unsigned long offs;
	unsigned long flags;
	unsigned long dat;

	if (pin < 32){
		base = (void __iomem *)GX3201_VA_GPIOA;
		offs = pin;
		gx_gpio_output_bak.porta &= ~(1 << offs);
		gx_gpio_output_bak.porta |= to << offs;
	}else if (pin < 64){
		base = (void __iomem *)GX3201_VA_GPIOB;
		offs = pin - 32;
		gx_gpio_output_bak.portb &= ~(1 << offs);
		gx_gpio_output_bak.portb |= to << offs;
	}else if (pin < 96){
		base = (void __iomem *)GX3201_VA_GPIOC;
		offs = pin - 64;
		gx_gpio_output_bak.portc &= ~(1 << offs);
		gx_gpio_output_bak.portc |= to << offs;
	}else
		return;

	local_irq_save(flags);

	dat = 1 << offs;
	if (to == 0)
		__raw_writel(dat, base + REG_OFFSET_BCLR);
	else if (to == 1)
		__raw_writel(dat, base + REG_OFFSET_BSET);

	local_irq_restore(flags);
}

EXPORT_SYMBOL(gx_gpio_setpin);

unsigned int gx_gpio_getpin(unsigned int pin)
{
	void __iomem *base;
	unsigned long offs;
	unsigned long dir;
	unsigned long pin_value;
	unsigned long output_bak_tmp;

	if (pin < 32){
		base = (void __iomem *)GX3201_VA_GPIOA;
		offs = pin;
		output_bak_tmp = gx_gpio_output_bak.porta;
	}else if (pin < 64){
		base = (void __iomem *)GX3201_VA_GPIOB;
		offs = pin - 32;
		output_bak_tmp = gx_gpio_output_bak.portb;
	}else if (pin < 96){
		base = (void __iomem *)GX3201_VA_GPIOC;
		offs = pin - 64;
		output_bak_tmp = gx_gpio_output_bak.portc;
	}else
		return -1;

	dir = __raw_readl(base + REG_OFFSET_EPDDR) & (1<< offs);

	if (dir){
		pin_value = (output_bak_tmp >> offs) & 0x1;
	}
	else
		pin_value = (__raw_readl(base + REG_OFFSET_EPDR) >> offs) & 0x1;

	return pin_value;
}

EXPORT_SYMBOL(gx_gpio_getpin);

