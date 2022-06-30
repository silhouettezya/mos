#include "lib.h"
#include <error.h>
#include <mmu.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void * (*start_rountine)(void *), void *arg) {
	int newthreadid = syscall_thread_alloc();
	if (newthreadid < 0) {
		thread = 0;
		return newthreadid;
	}
	struct Tcb *t = &env->env_threads[newthreadid & 0x7];
	t->tcb_tf.pc = start_rountine;
	t->tcb_tf.regs[4] = arg;
	t->tcb_tf.regs[31] = exit;
	syscall_set_thread_status(t->thread_id, ENV_RUNNABLE);
	*thread = t->thread_id;
	return 0;
}

void pthread_exit(void *value_ptr) {
	u_int threadid = syscall_getthreadid();
	struct Tcb *t = &env->env_threads[threadid & 0x7];
	t->tcb_exit_ptr = value_ptr;
	syscall_thread_destroy(threadid);
}

int pthread_cancel(pthread_t thread) {
	struct Tcb *t = &env->env_threads[thread & 0x7];
	if ((t->thread_id != thread) || (t->tcb_status == ENV_FREE)) {
		return -E_THREAD_NOTFOUND;
	}
	if (t->tcb_cancelstate == PTHREAD_CANCEL_ENABLE) {
	    t->tcb_exit_value = -THREAD_CANCELED_EXIT;
	    if (t->tcb_canceltype == PTHREAD_CANCEL_ASYCHRONOUS) {
		    syscall_thread_destroy(thread);
	    } else {
		    t->tcb_canceled = 1;
	    }
    }
	return 0;
}

int pthread_setcancelstate(int state, int *oldvalue) {
	int threadid = syscall_getthreadid();
	struct Tcb *t = &env->env_threads[threadid & 0x7];
	if ((t->thread_id != threadid) || (t->tcb_status == ENV_FREE)) {
		return -E_THREAD_NOTFOUND;
	}
	if (oldvalue != NULL) {
		*oldvalue = t->tcb_cancelstate;
	}
	t->tcb_cancelstate = state;
	return 0;
}

int pthread_setcanceltype(int type, int *oldvalue) {
	int threadid = syscall_getthreadid();
	struct Tcb *t = &env->env_threads[threadid & 0x7];
	if ((t->thread_id != threadid) || (t->tcb_status == ENV_FREE)) {
		return -E_THREAD_NOTFOUND;
	}
	if (oldvalue != NULL) {
		*oldvalue = t->tcb_canceltype;
	}
	t->tcb_canceltype = type;
	return 0;
}

void pthread_testcancel() {
	int threadid = syscall_getthreadid();
	struct Tcb *t = &env->env_threads[threadid & 0x7];
	if (t->thread_id != threadid) {
		user_panic("panic at pthread_testcancel!\n");
	}
	if ((t->tcb_canceled == 1) && (t->tcb_cancelstate == PTHREAD_CANCEL_ENABLE) && (t->tcb_canceltype == PTHREAD_CANCEL_DEFFERED)) {
		t->tcb_exit_value = -THREAD_CANCELED_EXIT;// challenge delete all this
		syscall_thread_destroy(t->thread_id);
	}
}

int pthread_join(pthread_t thread, void **retval) {
	int r = syscall_thread_join(thread, retval);
	return r;
}

int pthread_detach(pthread_t thread) {
	struct Tcb *t = &env->env_threads[thread & 0x7];
	if (t->thread_id != thread) {
		return -E_THREAD_NOTFOUND;
	}
	if (t->tcb_status != ENV_FREE) {
		t->tcb_detach = 1;
	}
	return 0;
}
