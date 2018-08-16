/* For testing CPU lockup.
 * Copyright (c) 2018 Takayuki Nagata
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/kthread.h>

static unsigned int cpu = 0;
module_param(cpu, uint, 0);
MODULE_PARM_DESC(cpu, "Set a target CPU");

static unsigned long delay = 300;
module_param(delay, ulong, 0);
MODULE_PARM_DESC(delay, "Set delay for lockup in seconds");

MODULE_AUTHOR("Takayuki Nagata <tnagata@redhat.com>");
MODULE_LICENSE("GPL");

struct task_struct *task;

static void doit(void)
{
	printk(KERN_EMERG "lockup: !!CPU%d IS LOCKED!!\n", cpu);
	while (1) { 
	}
}

static int lockup_thread(void *data)
{
	unsigned long start = local_clock() >> 30LL;

	printk(KERN_EMERG "lockup: CPU%d will hang after %ld seconds. Unload the module to stop lockup.\n", cpu, delay);
	while (1) {
		unsigned long now;
		if (kthread_should_stop()) {
			printk("lockup: stopping lockup thread.\n");
			return 0;
		}
		now = local_clock() >> 30LL;
		if (time_after(now, start + delay)) {
			doit();
		}
		schedule();
	}
	return 0;
}

static int __init modlockup_init(void)
{
	task = kthread_create(lockup_thread, NULL, "lockup/%d", cpu);

	if (task) {
		kthread_bind(task, cpu);
		wake_up_process(task);
	} else {
		return -ENOMEM;
	}

	return 0;
}

static void __exit modlockup_exit(void)
{
	if (task) {
		kthread_stop(task);
	}
}

module_init(modlockup_init);
module_exit(modlockup_exit);
