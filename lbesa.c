/* chronos/lbesa.c
 *
 * LBESA Single-Core Scheduler Module for ChronOS
 *
 * Author(s)
 *	- Matthew Dellinger, mdelling@vt.edu
 *
 * Copyright (C) 2009-2012 Virginia Tech Real Time Systems Lab
 */

#include <linux/module.h>
#include <linux/chronos_types.h>
#include <linux/chronos_sched.h>
#include <linux/list.h>
//#include <limits.h>

static inline int schedule_feasible(struct list_head * head, int i) {
	struct rt_info * it;
	struct timespec exec_ts = CURRENT_TIME;
	list_for_each_entry(it, head, task_list[i]) {
		add_ts(&exec_ts, &(it->left), &exec_ts);
		if (earlier_deadline(&(it->deadline), &exec_ts)) return 0;
	}
	return 1;
}

struct rt_info* sched_lbesa(struct list_head *head, int flags)
{
	struct list_head schedule;
	const int SCHEDULE_LIST = SCHED_LIST1;

	struct rt_info * it;
	struct list_head * lit;

	long ivd;
	struct rt_info * unworthy;

	struct timespec * t1;

	INIT_LIST_HEAD(&schedule);

	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		// if a task is aborted, return it
		if (check_task_failure(it, flags)) return it;

		// Calculate the inverse value density
		livd(it, 0, flags);

		initialize_lists(it);

		// Insert into the schedule in EDF order
		t1 = &it->deadline;
		lit = &schedule;
		while (!list_is_last(lit, &schedule)) {
			if (earlier_deadline(&(list_first_entry(
					lit,
					struct rt_info,
					task_list[SCHEDULE_LIST]
					)->deadline), t1))
				break;
			lit = lit->next;
		}
		list_add(&(it->task_list[SCHEDULE_LIST]), lit);
	}

	while (!list_empty(&schedule)) {
		if (schedule_feasible(&schedule, SCHEDULE_LIST))
			return list_first_entry(&schedule,
						struct rt_info,
						task_list[SCHEDULE_LIST]);
		ivd = LONG_MIN;
		unworthy = NULL;
		list_for_each_entry(it, &schedule, task_list[SCHEDULE_LIST]) {
			if (it->local_ivd >= ivd) {
				ivd = it->local_ivd;
				unworthy = it;
			}
		}

		list_remove(unworthy, SCHEDULE_LIST);
	}

	return list_first_entry(&schedule,
				struct rt_info,
				task_list[SCHEDULE_LIST]);
}

struct rt_sched_local lbesa = {
	.base.name = "LBESA",
	.base.id = SCHED_RT_LBESA,
	.flags = 0,
	.schedule = sched_lbesa,
	.base.sort_key = SORT_KEY_PERIOD,
	.base.list = LIST_HEAD_INIT(lbesa.base.list)
};

static int __init lbesa_init(void)
{
	return add_local_scheduler(&lbesa);
}
module_init(lbesa_init);

static void __exit lbesa_exit(void)
{
	remove_local_scheduler(&lbesa);
}
module_exit(lbesa_exit);

MODULE_DESCRIPTION("LBESA Single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Matthew Dellinger <matthew@mdelling.com>");
MODULE_LICENSE("GPL");

