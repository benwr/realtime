/* chronos/edf.c
 *
 * EDF Single-Core Scheduler Module for ChronOS
 *
 * Author(s)
 *	- Matthew Dellinger, mdelling@vt.edu
 *	- Ben Weinstein-Raun, bwr@vt.edu
 *
 * Copyright (C) 2009-2012 Virginia Tech Real Time Systems Lab
 */

#include <linux/module.h>
#include <linux/chronos_types.h>
#include <linux/chronos_sched.h>
#include <linux/list.h>

struct rt_info * sched_edf(struct list_head *head, int flags)
{
	struct rt_info * best = local_task(head->next);
	struct rt_info * um = local_task(head->next);

	struct list_head * node;
	list_for_each(node, head) {
		int secdiff;

		um = local_task(node);
		secdiff = um->deadline.tv_sec - best->deadline.tv_sec;
		if (secdiff < 0) best = um;
		else if (secdiff == 0 &&
			 um->deadline.tv_nsec - best->deadline.tv_nsec < 0) best = um;
	}

	//if(flags & SCHED_FLAG_PI)
	//	best = get_pi_task(best, head, flags);

	return best;
}

struct rt_sched_local edf = {
	.base.name = "EDF",
	.base.id = SCHED_RT_EDF,
	.flags = 0,
	.schedule = sched_edf,
	.base.sort_key = SORT_KEY_PERIOD,
	.base.list = LIST_HEAD_INIT(edf.base.list)
};

static int __init edf_init(void)
{
	return add_local_scheduler(&edf);
}
module_init(edf_init);

static void __exit edf_exit(void)
{
	remove_local_scheduler(&edf);
}
module_exit(edf_exit);

MODULE_DESCRIPTION("EDF Single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Matthew Dellinger <matthew@mdelling.com>");
MODULE_LICENSE("GPL");

