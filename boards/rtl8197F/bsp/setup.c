/*
 * Realtek Semiconductor Corp.
 *
 * bsp/setup.c
 *     bsp setup code
 *
 * Copyright (C) 2006-2012 Tony Wu (tonywu@realtek.com)
 */
#include <linux/console.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>

#include <asm/addrspace.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/time.h>
#include <asm/reboot.h>

#include "bspchip.h"

#ifdef CONFIG_NET
static void shutdown_netdev(void)
{
	struct net_device *dev;

	printk("Shutdown network interface\n");
	read_lock(&dev_base_lock);

	for_each_netdev(&init_net, dev)
	{
		if(dev->flags &IFF_UP) 
		{
			printk("%s:===>\n",dev->name);			
			rtnl_lock();
#if defined(CONFIG_COMPAT_NET_DEV_OPS)
			if(dev->stop)
				dev->stop(dev);
#else
			if ((dev->netdev_ops)&&(dev->netdev_ops->ndo_stop))
				dev->netdev_ops->ndo_stop(dev);
#endif
			rtnl_unlock();
		}
      	}
#if defined(CONFIG_RTL8192CD)
	{
		extern void force_stop_wlan_hw(void);
		force_stop_wlan_hw();
	}
#endif
#if defined(CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
	{	
	extern void rtl83XX_reset(void);
	rtl83XX_reset();
	}
#endif
	read_unlock(&dev_base_lock);
}
#endif

static void plat_machine_restart(char *command)
{
	REG32(BSP_GIMR)=0;
	REG32(BSP_GIMR2) = 0;
	local_irq_disable();
#ifdef CONFIG_NET
	shutdown_netdev();
#endif    
	REG32(BSP_WDTCNR) = 0; //enable watch dog
	while(1);
}

static void plat_machine_halt(void)
{
	printk("System halted.\n");
	while(1);
}

/* callback function */
//void __init bsp_setup(void)
void __init plat_mem_setup(void) // mips-ori
{
	/* define io/mem region */
	ioport_resource.start = 0x18000000;
	ioport_resource.end = 0x1fffffff;

	iomem_resource.start = 0x00000000;
	iomem_resource.end = 0x1fffffff;

	/* set reset vectors */
	_machine_restart = plat_machine_restart;
	_machine_halt = plat_machine_halt;
	pm_power_off = plat_machine_halt;
}
