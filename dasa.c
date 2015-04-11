/* chronos/dasa.c
 *
 * DASA Single-Core Scheduler Module for ChronOS
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

int task_cmp(void * field, struct list_head * a, struct list_head * b) {
	// Comparison function for list_sort
	return 0;
}

static inline int schedule_feasible(struct list_head * head, int i) {
	struct rt_info * it;
	struct timespec exec_ts = CURRENT_TIME;
	list_for_each_entry(it, head, task_list[i]) {
		add_ts(&exec_ts, &(it->left), &exec_ts);
		if (earlier_deadline(&(it->deadline), &exec_ts)) return 0;
	}
	return 1;
}

struct rt_info* sched_dasa(struct list_head *head, int flags)
{
	const int DENSITY_LIST = SCHED_LIST1;
	const int SCHEDULE_LIST = SCHED_LIST2;
	
	// ChronOS seems to want to do everything to an rt_info* rather
	// than the direct list_heads, while list.h expects a dummy list
	// node to act as the head.
	struct list_head density_list;
	struct list_head schedule;

	struct rt_info * it;

	struct list_head * lit;

	long ivd;
	
	struct timespec * t1;

	INIT_LIST_HEAD(&density_list);
	INIT_LIST_HEAD(&schedule);

	// for each task in ready tasks,
	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		// if a task is aborted, return it
		if (check_task_failure(it, flags)) return it;

		// The following call required adding an EXPORT_SYMBOL()
		// even though it was non-static and had public prototypes.
		// This probably won't work in stock ChronOS.

		// compute task's IVD
		livd(it, 0, flags);

		
		// initialize list heads
		initialize_lists(it);

		// add it to density list
		//list_add_after(&density_list, it, DENSITY_LIST);
		//insert_on_list(it, &density_list, DENSITY_LIST, SORT_KEY_LVD, 1);
		ivd = it->local_ivd;
		lit = &density_list;
		while (!list_is_last(lit, &density_list)) {
			if (list_first_entry(lit,
				struct rt_info,
				task_list[DENSITY_LIST]
				)->local_ivd > ivd)
				break;
			lit = lit->next;
		}
		list_add(&(it->task_list[DENSITY_LIST]), lit);
		
	}

	// quicksort tasks by IVD
	//quicksort(&density_list,
			//DENSITY_LIST, 
			//SORT_KEY_LVD, 1);

	// for each task, by value density
	list_for_each_entry(it, &density_list, task_list[DENSITY_LIST]) {
		// add it to the schedule, sorted by deadline
		//insert_on_list(it, &schedule, SCHEDULE_LIST, SORT_KEY_DEADLINE, 1);
		t1 = &(it->deadline);
		lit = &schedule;
		while (!list_is_last(lit, &schedule)) {
			if (earlier_deadline(&(list_first_entry(lit,
							struct rt_info,
							task_list[SCHEDULE_LIST]
							)->deadline), t1))
				break;
			lit = lit->next;
		}
		list_add(&(it->task_list[SCHEDULE_LIST]), lit);

		// check to see if schedule is feasible and, if not, remove it.
		// I think this one also required an EXPORT_SYMBOL.
		if (!schedule_feasible(&schedule, SCHEDULE_LIST))
			list_remove(it, SCHEDULE_LIST);
	}

	// If we ended up with an empty schedule, it means that
	// there are no feasible schedules, but nothing has yet blown
	// a deadline. Fall back to the highest-value-density task.
	// Otherwise, do what DASA is supposed to do (return the first
	// thing in the schedule)
	if (list_empty(&schedule))
		return list_first_entry(&density_list,
					struct rt_info,
					 task_list[DENSITY_LIST]);
	else
		return list_first_entry(&schedule,
					struct rt_info,
					task_list[SCHEDULE_LIST]);

}

struct rt_sched_local dasa = {
	.base.name = "DASA",
	.base.id = SCHED_RT_DASA,
	.flags = 0,
	.schedule = sched_dasa,
	.base.sort_key = SORT_KEY_PERIOD,
	.base.list = LIST_HEAD_INIT(dasa.base.list)
};

static int __init dasa_init(void)
{
	return add_local_scheduler(&dasa);
}
module_init(dasa_init);

static void __exit dasa_exit(void)
{
	remove_local_scheduler(&dasa);
}
module_exit(dasa_exit);

MODULE_DESCRIPTION("DASA Single-Core Scheduling Module for ChronOS");
MODULE_AUTHOR("Ben Weinstein-Raun <b@w-r.me>");
MODULE_LICENSE("GPL");

