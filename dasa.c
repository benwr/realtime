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

int task_cmp(void * arg, struct list_head * a, struct list_head * b) {
	// Comparison function for list_sort
	struct rt_info * a_task, * b_task;
	int * field = (int *) arg;

	a_task = list_entry(a, struct rt_info, task_list[*field]);
	b_task = list_entry(b, struct rt_info, task_list[*field]);

	if (*field == SCHED_LIST1) {
		return a_task->local_ivd - b_task->local_ivd;
	} else if (*field == SCHED_LIST3) {
		return compare_ts(&(a_task->temp_deadline), &(b_task->temp_deadline)); // difference between temporary deadlines
	}

	// assume field is SCHED_LIST2
	return compare_ts(&(a_task->deadline), &(b_task->deadline));// difference between deadlines
}

static inline void list_copy(struct list_head * a, int alist, struct list_head * b, int blist) {
	struct list_head * astart, * bstart;

	astart = a;
	bstart = b;
	
	while (a->next != astart) {
		// set b->next and b->next->prevb
		b->next = &(list_entry(a->next, struct rt_info, task_list[alist])->task_list[blist]);
		list_entry(a->next, struct rt_info, task_list[alist])->task_list[blist].prev = b;
		a = a->next;
		b = b->next;
	}

	b->next = bstart;
	bstart->prev = b;
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

static inline struct rt_info * first_dep(struct rt_info * task) {
	if (task->requested_resource == NULL) return NULL;
	else return task->requested_resource->owner_t;
}

static inline bool mark_deps_and_deadlocks(struct rt_info * task) {
	struct rt_info * it = task;
	while ((it->dep = first_dep(it)) != NULL && !task_check_flag(it, DEADLOCKED)) {
		if (task_check_flag(it, MARKED))
			task_set_flag(task, DEADLOCKED);
		task_set_flag(it, MARKED);
		it = it->dep;
	}

	it = task;

	while (it != NULL && task_check_flag(it, MARKED)) {
		task_clear_flag(it, MARKED);
		it = it->dep;
	}

	return task_check_flag(it, DEADLOCKED);
}

static void compute_dep_tentative_deadlines(struct rt_info * task) {
	struct timespec earliest = task->deadline;
	task->temp_deadline = task->deadline;

	while ((task = first_dep(task))) {
		if (compare_ts(&(task->deadline), &earliest) < 0) {
			earliest = task->deadline;
		}
		task->temp_deadline = earliest;
	}
}

struct rt_info* sched_dasa(struct list_head *head, int flags)
{
	int DENSITY_LIST = SCHED_LIST1;
	int SCHEDULE_LIST = SCHED_LIST2;
	int TENTATIVE_LIST = SCHED_LIST3;
	
	struct list_head density_list, schedule, tentative;

	struct rt_info * it, * task;

	INIT_LIST_HEAD(&density_list);
	INIT_LIST_HEAD(&schedule);
	INIT_LIST_HEAD(&tentative);

	// for each task in ready tasks,
	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		// compute dependency list
		// and check for deadlocks (setting DEADLOCKED flag; livd knows what to do with that)
		mark_deps_and_deadlocks(it);

		// compute task's LIVD, aborting deadlocks
		livd(it, true, flags);
	}

	list_for_each_entry(it, head, task_list[LOCAL_LIST]) {
		// if a task is aborted or failed, return it so it can finish aborting
		if (check_task_failure(it, flags)) return it;

		// initialize list heads
		initialize_lists(it);

		// add it to density list
		list_add(&(it->task_list[DENSITY_LIST]), &density_list);
	}

	// sort tasks by descending VD
	list_sort(&DENSITY_LIST, &density_list, task_cmp);

	// for each task, by value density
	list_for_each_entry(it, &density_list, task_list[DENSITY_LIST]) {
		// make a tentative copy of the schedule
		list_copy(&schedule, SCHEDULE_LIST, &tentative, TENTATIVE_LIST);

		// compute tentative deadlines (deadline tightening)
		compute_dep_tentative_deadlines(it);

		// add task and its dependents to tentative list, in
		// deadline/dependency-order
		for (task = it; task != NULL; task = first_dep(task)) {
			list_del_init(&(it->task_list[TENTATIVE_LIST]));
			list_add(&(it->task_list[TENTATIVE_LIST]), &tentative);
		}

		list_sort(&TENTATIVE_LIST, &tentative, task_cmp);
		
		// if the tentative schedule is feasible, copy it to the final schedule
		if (schedule_feasible(&tentative, TENTATIVE_LIST)) {
			list_copy(&tentative, TENTATIVE_LIST, &schedule, SCHEDULE_LIST);
			list_sort(&SCHEDULE_LIST, &schedule, task_cmp);
		}

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
