#include <linux/reboot.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_power_off(void)
{
	local_irq_disable();
	if (pm_power_off)
		pm_power_off();
	while(1);
}

void machine_halt(void)
{
	local_irq_disable();
	if (pm_power_off)
		pm_power_off();
	while(1);
}

void machine_restart(char *cmd)
{
	local_irq_disable();
	do_kernel_restart(cmd);
	while(1);
}


