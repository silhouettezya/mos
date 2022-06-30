#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>
#include <error.h>

extern char *KERNEL_SP;
extern struct Env *curenv;
//challenge
extern struct Tcb *curtcb;

u_int sys_getthreadid(void)
{
	return curtcb->thread_id;
}

int sys_thread_alloc(void)
{
	int r;
	struct Tcb *t;
	
	if ((r = thread_alloc(curenv, &t)) < 0) {
		return r;
	}

	t->tcb_pri = curenv->env_threads[0].tcb_pri;
	t->tcb_status = ENV_NOT_RUNNABLE;
	t->tcb_tf.regs[2] = 0;
	t->tcb_tf.pc = t->tcb_tf.cp0_epc;
	return t->thread_id;
	
}

int sys_thread_destroy(int sysno, u_int threadid)
{
	int r;
	struct Tcb *t;

	if ((r = threadid2tcb(threadid,&t)) < 0) {
		return r;
	}
	
	//if joined
	struct Tcb *tmp;
	while (!LIST_EMPTY(&t->tcb_joined_list)) {
		tmp = LIST_FIRST(&t->tcb_joined_list);
		LIST_REMOVE(tmp,tcb_joined_link);
		*(tmp->tcb_join_retval) = t->tcb_exit_ptr;
		tmp->tcb_status = ENV_RUNNABLE;
	}
	printf("[%08x] destroying tcb %08x\n", curenv->env_id, t->thread_id);
	thread_destroy(t);
	return 0;
}

int sys_set_thread_status(int sysno, u_int threadid, u_int status)
{
	int ret;
	struct Tcb *tcb;

	if ((status != ENV_RUNNABLE)&&(status != ENV_NOT_RUNNABLE)&&(status != ENV_FREE)) {
		return -E_INVAL;
	}
	ret = threadid2tcb(threadid, &tcb);
	if (ret < 0) {
		return ret;
	}
	if ((status == ENV_RUNNABLE) && (tcb->tcb_status != ENV_RUNNABLE)) {
		LIST_INSERT_HEAD(tcb_sched_list, tcb, tcb_sched_link);	
	}
	tcb->tcb_status = status;
	return 0;
}

int sys_thread_join(int sysno, u_int threadid, void **retval)
{
	int r;
	struct Tcb *t;

	r = threadid2tcb(threadid, &t);
	if (r < 0)
		return r;
	if (t->tcb_detach) {
		return -E_THREAD_JOIN_FAIL;
	}
	//if isend
	if (t->tcb_status == ENV_FREE) {
		if (retval != NULL) {
			*retval = t->tcb_exit_ptr;
		}
		return 0;
	}
	LIST_INSERT_HEAD(&t->tcb_joined_list, curtcb, tcb_joined_link);
	curtcb->tcb_join_retval = retval;
	curtcb->tcb_status = ENV_NOT_RUNNABLE;
	
	sys_yield();
	return 0;
}

int sys_sem_wait(int sysno, sem_t *sem)
{
	if (sem->sem_status == SEM_FREE) {
		return -E_SEM_ERROR;
	}
	int i;
	if (sem->sem_value > 0) {
		sem->sem_value--;
	} else {
		sem->sem_wait_list[sem->sem_last] = curtcb;
		sem->sem_last = (sem->sem_last + 1) % THREAD_MAX;
		sem->sem_wait_count++;
		curtcb->tcb_status = ENV_NOT_RUNNABLE;
		
		sys_yield();
	}
	return 0;
}

int sys_sem_trywait(int sysno, sem_t *sem)
{
	if (sem->sem_status == SEM_FREE) {
		return -E_SEM_ERROR;
	}
	if (sem->sem_value > 0) {
		sem->sem_value--;
		return 0;
	}
	return -E_SEM_EAGAIN;
}

int sys_sem_post(int sysno, sem_t *sem)
{
	struct Tcb *t;

	if (sem->sem_status == SEM_FREE) {
		return -E_SEM_ERROR;
	}
	if (sem->sem_value > 0) {
		sem->sem_value++;
	} else {
		if (sem->sem_wait_count == 0) {
			sem->sem_value++;
		} else {
			t = sem->sem_wait_list[sem->sem_first];
			sem->sem_wait_list[sem->sem_first] = 0;
			sem->sem_first = (sem->sem_first + 1) % THREAD_MAX;
			t->tcb_status = ENV_RUNNABLE;
			sem->sem_wait_count--;
		}
	}
	return 0;
}

int sys_sem_getvalue(int sysno, sem_t *sem, int *sval)
{
	if (sem->sem_status == SEM_FREE) {
		return -E_SEM_ERROR;
	}
	if (sval != NULL) {
		*sval = sem->sem_value;
	}
	return 0;
}

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current process. This function will never return.
 *
 * Note:
 *  For convenience, you can just give up the current time slice.
 */
/*** exercise 4.6 ***/
void sys_yield(void)
{
	bcopy((void *) KERNEL_SP - sizeof(struct Trapframe),
          (void *) TIMESTACK - sizeof(struct Trapframe),
          sizeof(struct Trapframe));
    sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 *
 * Pre-Condition:
 * 	xstacktop points to one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
/*** exercise 4.12 ***/
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;
	
	if ((ret = envid2env(envid, &env, 0)) < 0) return ret;
    env->env_pgfault_handler = func;
    env->env_xstacktop = xstacktop;

	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 *
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
/*** exercise 4.3 ***/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;

	if (!(perm & PTE_V) || (perm & PTE_COW)) {
		return -E_INVAL;
	}	
    if (va >= UTOP) {
		return -E_INVAL;
	}
    ret = envid2env(envid, &env, 1);
    if (ret < 0) {
		return ret;
	}
    ret = page_alloc(&ppage);
    if (ret < 0) {
		return ret;
	}
    ret = page_insert(env->env_pgdir, ppage, va, perm);
    if (ret < 0) {
		return ret;
	}	
    return 0;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm must have PTE_V to be valid.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
/*** exercise 4.4 ***/
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

    //your code here
	if (srcva >= UTOP || dstva >= UTOP) {
		return -E_INVAL;
	}
    if (!(perm & PTE_V)) {
		return -E_INVAL;
	}

    if ((ret = envid2env(srcid, &srcenv, 0)) < 0) {
		return ret;
	}
	if ((ret = envid2env(dstid, &dstenv, 0)) < 0) {
		return ret;
	}

    ppage = page_lookup(srcenv->env_pgdir, round_srcva, &ppte);
    if (ppage == NULL) {
		return -E_INVAL;
	}

    if (!(*ppte & PTE_R) && (perm & PTE_R)) {
		return -E_INVAL;
	}

    if ((ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm)) < 0) {
		return ret;
	}

    return 0;
	
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
/*** exercise 4.5 ***/
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret;
	struct Env *env;

	if (va >= UTOP) return -E_INVAL;
    if ((ret = envid2env(envid, &env, 1)) < 0) return ret;

    page_remove(env->env_pgdir, va);

	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
/*** exercise 4.8 ***/
int sys_env_alloc(void)//challenge
{
	// Your code here.
	int r;
	struct Env *e;

	if ((r = env_alloc(&e, curenv->env_id)) < 0) {
		return r;
	}
    bcopy((void *) KERNEL_SP - sizeof(struct Trapframe),
          (void *) &(e->env_threads[0].tcb_tf), sizeof(struct Trapframe));
    e->env_threads[0].tcb_status = ENV_NOT_RUNNABLE;
    e->env_threads[0].tcb_pri = curenv->env_threads[0].tcb_pri;
	e->env_threads[0].tcb_tf.pc = e->env_threads[0].tcb_tf.cp0_epc;
    e->env_threads[0].tcb_tf.regs[2] = 0; // $v0 <- return value

	return e->env_id;
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
/*** exercise 4.14 ***/
int sys_set_env_status(int sysno, u_int envid, u_int status)//challenge
{
	// Your code here.
	struct Env *env;
	int ret;
	struct Tcb *tcb;

	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE && status != ENV_FREE) {
		return -E_INVAL;
	}
	ret = envid2env(envid, &env, 0);
	tcb = &env->env_threads[0];
	if (ret) {
		return ret;
	}
	if (status == ENV_RUNNABLE && tcb->tcb_status != ENV_RUNNABLE) {
		LIST_INSERT_HEAD(&tcb_sched_list[0], tcb, tcb_sched_link);
	}
	tcb->tcb_status = status;
	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from
 * other process. To be more specific, it will flag
 * the current process so that other process could send
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: 0 is also a valid value for `dstva`).
 *
 * Post-Condition:
 * 	This syscall will set the current process's status to
 * ENV_NOT_RUNNABLE, giving up cpu.
 */
/*** exercise 4.7 ***/
void sys_ipc_recv(int sysno, u_int dstva)//challenge
{
	if (dstva >= UTOP) return;
    curenv->env_ipc_recving = 1;
    curenv->env_ipc_dstva = dstva;
	curenv->env_ipc_waiting_threadid = curtcb->thread_id;
    curtcb->tcb_status = ENV_NOT_RUNNABLE;
    sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: You need to call `envid2env`.
 */
/*** exercise 4.7 ***/
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)//challenge
{

	int r;
	struct Env *e;
	struct Page *p;
	
	if (srcva >= UTOP) return -E_INVAL;
    if ((r = envid2env(envid, &e, 0)) < 0) return r;
    if (e->env_ipc_recving == 0) return -E_IPC_NOT_RECV;
    e->env_ipc_value = value;
    e->env_ipc_from = curenv->env_id;
    e->env_ipc_perm = perm;
    e->env_ipc_recving = 0;
	int threadid = e->env_ipc_waiting_threadid;
    e->env_threads[threadid & 0x7].tcb_status = ENV_RUNNABLE;

    if (srcva != 0) {
        Pte *pte;
        p = page_lookup(curenv->env_pgdir, srcva, &pte);
        if (p == NULL) return -E_INVAL;
        page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm);
    }

	return 0;
}

