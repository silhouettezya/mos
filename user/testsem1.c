#include "lib.h"

int full = 50;//yvliang
sem_t sem_rw;//duxiehuchi
sem_t *rw = &sem_rw;

void *B(void *args) {//writer
    int id = (int)((u_int *)args)[1];
    while (1) {
	    sem_wait(rw);
	    if(full > 0) {//if > 0
		    full--;//write
            writef("B%d write, now have %d\n", id, full);
	    } else if (full == 0) {
            sem_post(rw);
			break;
        }
	    sem_post(rw);
    }
	pthread_exit(0);
}

void umain() {
	u_int id0 = 0;
	u_int arg1[2];
	u_int arg2[2];
	u_int arg3[2];
    sem_init(rw, 0, 1);
	pthread_t thread1;
	pthread_t thread2;
	pthread_t thread3;
	arg1[1] = 1;
	pthread_create(&thread1, NULL, B, (void *)arg1);
	arg2[1] = 3;
	pthread_create(&thread2, NULL, B, (void *)arg2);
	arg3[1] = 2;
	pthread_create(&thread3, NULL, B, (void *)arg3);
}
