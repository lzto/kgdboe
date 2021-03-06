/*
* A network interface for GDB for 3.x kernels.
* Inspired by the original 'kgdboe' by Jason Wessel <jason.wessel@windriver.com>
* that seems abandoned as of 2014.
*
* (c) 2014 Sysprogs OU <sysprogs@sysprogs.com>
*
* Supported kernel versions: 3.8.x - 3.15.x
*
* Usage:
*	1. Build the module: make -C /lib/modules/$(uname -r)/build M=$(pwd)
*   2. Load the module: insmod kgdboe.ko [device_name=ethX] [udp_port=Y]
*	3. Check the load status: dmesg | tail
*	4. Connect from gdb: target remote udp:<ip>:<port>
*
* WARNING! Using a network driver as a transport on multi-core systems is tricky! 
*		   KGDB does not fully know what spinlocks/resources the network driver needs
*		   and if another core has been stopped while holding them, the debugger will hang.
*		   This module does its best to avoid it by hooking resources that are likely required
*		   by the network driver, but there is no 100% guarantee that your driver does not take
*		   anything extra. It has been tested on the following network drivers:
*				pcnet32
*				e1000
*				r8169
*		   Nonetheless, by default it will disable all cores except #0 and will not do the hooking.
*		   If you absolutely need SMP while debugging, use the force_single_core=0 parameter to override,
*          but be ready to troubleshoot your network driver if it is different from the ones listed above. 
*
* This file is licensed under the terms of the GNU General Public License
* version 2. This program is licensed "as is" without any warranty of any
* kind, whether express or implied.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/socket.h>
#include "netpoll_wrapper.h"
#include "kgdboe_io.h"

MODULE_LICENSE("GPL");

#include <linux/timer.h>


/////////////////////////////////////
//set_memory_rw is not exported, search through symbol and findout where it is
#include <linux/errno.h>
#include <linux/kallsyms.h>
#include <linux/string.h>
int (*k_set_memory_rw)(unsigned long addr, int numpages) = NULL;
////////////////////////////////////

static int udp_port = 31337;
module_param(udp_port, int, 0444);

static char *device_name = "eth0";
module_param(device_name, charp, 0444);

static char *local_ip = NULL;
module_param(local_ip, charp, 0444);

static int force_single_core = 1;
module_param(force_single_core, int, 0444);

static int symbol_walk_callback(void *data, const char *name, struct module *mod, 
			unsigned long addr)
{
	/* Skip the symbol if it belongs to a module rather than to 
	 * * 	 * the kernel proper. */
	if (mod != NULL) 
		return 0;
	
	if (strcmp(name, "set_memory_rw") == 0) {
		if (k_set_memory_rw != NULL) {
			pr_warning("Found two set_memory_rw unable to continue\n");
			return -EFAULT;
		}
		k_set_memory_rw = (typeof(k_set_memory_rw))addr;
		return 0;
	}
	return 0;
}

static int __init kgdboe_init(void)
{
	int ret = kallsyms_on_each_symbol(symbol_walk_callback, NULL);
	if (ret)
		return ret;

	if (k_set_memory_rw == NULL)
	{
		pr_warning("Unable to find set_memory_rw function\n");
		return -EFAULT;
	}
	return kgdboe_io_init(device_name, udp_port, local_ip, force_single_core != 0);
}

static void __exit kgdboe_exit(void)
{
	kgdboe_io_cleanup();
}

module_init(kgdboe_init);
module_exit(kgdboe_exit);
