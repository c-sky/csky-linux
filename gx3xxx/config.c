#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <asm/machdep.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <mach/map.h>

extern void __init gx3xxx_init_IRQ(void);
extern unsigned int gx3201_get_irqno(void);

static int gx3201_hwclk(int set, struct rtc_time *t)
{
	t->tm_year = 1980;
	t->tm_mon  = 1;
	t->tm_mday = 1;
	t->tm_hour = 0;
	t->tm_min  = 0;
	t->tm_sec  = 0;

	return 0;
}

static void gx3201_tick(void)
{
	__raw_writel(1, GX3201_VA_COUNTER_1_STATUS);
}

static void gx3201_timer_reset(void)
{
	__raw_writel(0x1, GX3201_VA_COUNTER_1_CONTROL);
	__raw_writel(0x0, GX3201_VA_COUNTER_1_CONTROL);
	__raw_writel(0x3, GX3201_VA_COUNTER_1_CONFIG);

	__raw_writel(26,GX3201_VA_COUNTER_1_PRE);
	__raw_writel(0xFFFFD8EF,GX3201_VA_COUNTER_1_INI);
	__raw_writel(0x2, GX3201_VA_COUNTER_1_CONTROL);
}

extern irqreturn_t csky_timer_interrupt(int irq, void *dummy);
static struct irqaction gx3201_timer_irq = {
        .name           = "Gx3201 Timer Tick",
        .flags          = IRQF_SHARED,
        .handler        = csky_timer_interrupt,
};

void __init gx3201_timer_init(void)
{
	gx3201_timer_reset();
//	jiffies_64 = 0;
	setup_irq(10,&gx3201_timer_irq);
}

static unsigned long gx3201_timer_offset(void)
{
	return ((__raw_readl(GX3201_VA_COUNTER_1_VALUE) - 0xFFFFD8EF)/(0xFFFFFFFF - 0xFFFFD8EF))/10000;
}

void gx3xxx_halt(void)
{
	printk("%s.\n", __func__);
	/* close mmu */
	asm volatile (
		"mfcr   r7, cr18;"
		"bclri  r7, 0;"
		"bclri  r7, 1;"
		"mtcr   r7, cr18;"
		:
		:
		:"r7"
	);

	/* jmp */
	((void (*)(u32 WakeTime,u32 GpioMask,u32 GpioData,u32 key))0x00100000)
			(*(unsigned int *) (0x00103000 - 0x10),
			*(unsigned int *)  (0x00103000 - 0xc),
			*(unsigned int *)  (0x00103000 - 0x8),
			*(unsigned int *)  (0x00103000 - 0x4));
	while(1);
}

void gx3xxx_restart(void)
{
	printk("%s.\n", __func__);

	__raw_writel(
	((28800000/1000000 - 1) << 16)|(0x10000 - 10000),
	(void*) 0xa020b004
	);

	__raw_writel( 3, (void*) 0xa020b000);
	while(1);

}

extern int gxav_memhole_init(void);
void __init config_BSP(void)
{
	mach_time_init = gx3201_timer_init;
	mach_tick = gx3201_tick;
	mach_hwclk = gx3201_hwclk;
	mach_init_IRQ = gx3xxx_init_IRQ;
	mach_gettimeoffset = gx3201_timer_offset;
	mach_get_auto_irqno = gx3201_get_irqno;

	mach_reset = gx3xxx_restart;
	mach_halt = gx3xxx_halt;
}

#ifdef CONFIG_SERIAL_8250
static struct plat_serial8250_port gx3211_8250_uart0_data[] = {
	[0] = {
		.mapbase = 0x00403000,
		.membase = (char *)0xa0403000,
		.irq = 15,
		.flags = UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype = UPIO_MEM,
		.regshift = 2,
		.uartclk = 29491200,
	},
	[1] = {
		.mapbase = 0x00413000,
		.membase = (char *)0xa0413000,
		.irq = 26,
		.flags = UPF_SKIP_TEST | UPF_BOOT_AUTOCONF,
		.iotype = UPIO_MEM,
		.regshift = 2,
		.uartclk = 29491200,
	}, {
	},
};

static struct platform_device gx3211_8250_uart0 = {
	.name			= "serial8250",
	.id			= 0,
	.dev			= {
		.platform_data	= gx3211_8250_uart0_data,
	},
};

static int __init board_devices_init(void)
{

	*(volatile unsigned int *) 0xa030a14c |= (1 << 22) | (1 << 23);
	platform_device_register(&gx3211_8250_uart0);
	return 0;
}

arch_initcall(board_devices_init);
#endif
