#include "lib.h"
#include <error.h>

int sem_init(sem_t *sem,int shared,unsigned int value) {
	if (sem == 0) {
		return -E_SEM_ERROR;
	}
	sem->sem_envid = env->env_id;
	sem->sem_value = value;
	sem->sem_shared = shared;
	sem->sem_status = SEM_VALID;
	sem->sem_wait_count = 0;
    sem->sem_first = 0;
    sem->sem_last = 0;
	int i;
	for(i = 0; i < THREAD_MAX; i++) {
		sem->sem_wait_list[i] = NULL;
	}
	return 0;
}

int sem_destroy(sem_t *sem) {
	sem->sem_value = SEM_FREE;
	return 0;	
}

int sem_wait(sem_t *sem) {
	return syscall_sem_wait(sem);
}

int sem_trywait(sem_t *sem) {
	return syscall_sem_trywait(sem);
}

int sem_post(sem_t *sem) {
	return syscall_sem_post(sem);
}

int sem_getvalue(sem_t *sem,int *sval) {
	return syscall_sem_getvalue(sem,sval);
}
