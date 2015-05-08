/* chronos/icpp.c
 *
 * ICPP Single-Core Scheduler Module for ChronOS
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
#include <linux/list_sort.h>

static const int DYNAMIC_SCHEDULE = SCHED_LIST1;

int task_cmp_by_dynamic_priority(void * _, struct list_head * a, struct list_head * b) {
	struct rt_info * a_task, * b_task;

	a_task = list_entry(a, struct rt_info, task_list[DYNAMIC_SCHEDULE]);
	b_task = list_entry(b, struct rt_info, task_list[DYNAMIC_SCHEDULE]);

	return ((signed long) b_task->dynamic_priority) - ((signed long) a_task->dynamic_priority);
}


struct rt_info* sched_icpp(struct list_head *head, int flags)
{
	struct rt_info * it, * best;

	struct list_head dynamic_schedule;

	
	unsigned long csstar;

	INIT_LIST_HEAD(&dynamic_schedule);
	csstar = ULONG_MAX;
	best = NULL;

	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		if (it->dynamic_priority == 0) 
			it->dynamic_priority = timespec_to_long(&it->period);
		
		// if (check_task_failure(it, flags)) return it;

		if (it->dynamic_priority < csstar) {
			if(it->locks_held > 0)
				csstar = it->dynamic_priority;

			printk("setting best with priority %lu\n", it->dynamic_priority);
			best = it;
		}

		// list_add(&(it->task_list[DYNAMIC_SCHEDULE]), &dynamic_schedule);
	}
	
	if (best != NULL) {
		return best;
	}

	// list_sort(NULL, &dynamic_schedule, task_cmp_by_dynamic_priority);

	//list_for_each_entry(it, &dynamic_schedule, task_list[DYNAMIC_SCHEDULE]) {
		//if (it->requested_resource == NULL || it->dynamic_priority < csstar) {
			//printk("returning a task due to dynamic priority\n");
			//return it;
		//}
	//}

	printk("falling back to first item in schedule\n");
	return local_task(head->next);
}

struct rt_sched_local icpp= {
	.base.name = "ICPP",
	.base.id = SCHED_RT_ICPP,
	.flags = 0,
	.schedule = sched_icpp,
	.base.sort_key = SORT_KEY_PERIOD,
	.base.list = LIST_HEAD_INIT(icpp.base.list)
};

static int __init icpp_init(void)
{
	return add_local_scheduler(&icpp);
}
module_init(icpp_init);

static void __exit icpp_exit(void)
{
	remove_local_scheduler(&icpp);
}
module_exit(icpp_exit);

MODULE_DESCRIPTION("ICPP Single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Ben Weinstein-Raun <b@w-r.me>");
MODULE_LICENSE("GPL");

