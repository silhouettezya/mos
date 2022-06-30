#include "lib.h"

int count = 0;//reader num
int full = 15;//yvliang
sem_t sem_mutex;//count huchi
sem_t sem_rw;//duxiehuchi
sem_t *mutex = &sem_mutex;
sem_t *rw = &sem_rw;


void *B(void *args) {//writer
    int id = *((int *)args);
	while(1) {
	    sem_wait(rw);
	    if(full > 0) {//if > 0
		    full--;//write
            writef("B%d bug ticket\n", id);
	    } else if (full == 0) {
            break;
        }
	    sem_post(rw);
	}
	sem_post(rw);
}

void *Q(void *args) {//reader
    int id = *((int *)args);
	while(1) {
	    sem_wait(mutex);//read count
	    if(count==0) {//if == 0
	    	sem_wait(rw);//stop write
	    }
	    count++;//reader++
	    sem_post(mutex);//free full
	    writef("Q%d: now have %d ticket\n", id, full);//read
		if (full == 0) break;
	    sem_wait(mutex);//read full
	    count--;//reader--
	    if(count==0) {//last leave
	    	sem_post(rw);//wake write
	    }
	    sem_post(mutex);//free mutex
	}
	sem_wait(mutex);//read full
    count--;//reader--
    if(count==0) {//last leave
        sem_post(rw);//wake write
     }
    sem_post(mutex);//free mutex
}

void umain() {
	u_int id1 = 1;
	u_int id2 = 2;
	u_int id3 = 3;
	u_int id4 = 4;
	sem_init(mutex,0,1);
    sem_init(rw, 0, 1);
	pthread_t thread1;
	pthread_t thread2;
	pthread_t thread3;
	pthread_t thread4;
	pthread_create(&thread1, NULL, B, (void *)&id1);
	//pthread_create(&thread2, NULL, Q, (void *)&id2);
	pthread_create(&thread3, NULL, B, (void *)&id3);
	pthread_create(&thread4, NULL, Q, (void *)&id4);
}
