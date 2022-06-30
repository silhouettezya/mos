#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
void sched_yield(void)//challenge
{
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
	struct Tcb *t = curtcb;    
    /*  hint:
     *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
     *     using LIST_REMOVE and LIST_INSERT_TAIL.
     *  2. if (env_sched_list[point] is empty), point = 1 - point;
     *     then search through `env_sched_list[point]` for a runnable env `e`, 
     *     and set count = e->env_pri
     *  3. count--
     *  4. env_run()
     *
     *  functions or macros below may be used (not all):
     *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
     */
	if (count == 0 || t == NULL || t->tcb_status != ENV_RUNNABLE)
	{
		if (t != NULL)
		{
			LIST_REMOVE(t, tcb_sched_link);
			if (t->tcb_status != ENV_FREE) {
				LIST_INSERT_TAIL(&tcb_sched_list[1 - point], t, tcb_sched_link);
			}
		}
		while (1)
		{
			while (LIST_EMPTY(&tcb_sched_list[point]))
				point = 1 - point;
			t = LIST_FIRST(&tcb_sched_list[point]);
			if (t->tcb_status == ENV_FREE) {
				LIST_REMOVE(t, tcb_sched_link);
			} else if (t->tcb_status == ENV_NOT_RUNNABLE) {
				LIST_REMOVE(t, tcb_sched_link);
				LIST_INSERT_HEAD(&tcb_sched_list[1 - point], t, tcb_sched_link);
			} else {
				count = t->tcb_pri;
				break;
			}
		}
	}
	count--;
	env_run(t);
}

