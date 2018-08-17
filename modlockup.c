/* For testing CPU lockup.
 * Copyright (c) 2018 Takayuki Nagata
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/kthread.h>

static unsigned int cpu = 0;
module_param(cpu, uint, 0);
MODULE_PARM_DESC(cpu, "Set a target CPU (default: 0)");

static unsigned long delay = 300;
module_param(delay, ulong, 0);
MODULE_PARM_DESC(delay, "Set delay for lockup in seconds (default: 300)");

static unsigned long duration = 10;
module_param(duration, ulong, 0);
MODULE_PARM_DESC(duration, "Set locup duration in seconds (default: 10)");

MODULE_AUTHOR("Takayuki Nagata <tnagata@redhat.com>");
MODULE_LICENSE("GPL");

struct task_struct *task;

static void doit(void)
{
	unsigned long start = local_clock() >> 30LL;

	printk(KERN_EMERG "lockup: !!CPU%d WILL BE LOCKING FOR %ld SECONDS!!\n", cpu, duration);
	while (1) {
		unsigned long now;
		now = local_clock() >> 30LL;
		if (time_after(now, start + duration)) {
			break;
		}
	}
	printk("lockup: CPU%d will be released\n", cpu);

	return;
}

static int lockup_thread(void *data)
{
	unsigned long start = local_clock() >> 30LL;

	printk(KERN_EMERG "lockup: CPU%d will be locked for %ld seconds after %ld seconds."
		 "Unload the module to stop lockup.\n", cpu, duration, delay);
	while (!kthread_should_stop()) {
		unsigned long now;

		now = local_clock() >> 30LL;
		if (time_after(now, start + delay)) {
			doit();
			break;
		} else {
			cond_resched();
		}
	}
	printk("lockup: stopping lockup thread.\n");

	task = NULL;
	return 0;
}

static int __init modlockup_init(void)
{
	int ret = 0;

	task = kthread_create(lockup_thread, NULL, "lockup/%d", cpu);
	if (task) {
		kthread_bind(task, cpu);
		wake_up_process(task);
	} else {
		ret = -ENOMEM;
	}

	return ret;
}

static void __exit modlockup_exit(void)
{
	if (task) {
		kthread_stop(task);
	}
}

module_init(modlockup_init);
module_exit(modlockup_exit);
