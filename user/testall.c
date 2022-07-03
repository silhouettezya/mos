#include "lib.h"

int count = 0;//reader num
int full = 40;//yvliang
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
            printf("B%d bug ticket\n", id);
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
	    printf("Q%d: now have %d ticket\n", id, full);//read
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

void *work(void *args) {
    int id;
    id = *((int *)args);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFFERED, NULL);
    int i;
    for (i = 0; i < 5; i++) {
        printf("id : %d put %d\n", id, i);
    }
    while (1) {
        printf("id : %d wait to be canceled\n", id);
        pthread_testcancel();
    }
}

void umain() {
	int id1 = 1;
	int id2 = 2;
	int id3 = 3;
	int id4 = 4;
	sem_init(mutex,0,1);
    sem_init(rw, 0, 1);
	pthread_t thread1;
	pthread_t thread2;
	pthread_t thread3;
	pthread_t thread4;
	pthread_create(&thread1, NULL, Q, (void *)&id1);
	pthread_create(&thread2, NULL, B, (void *)&id2);
	pthread_create(&thread3, NULL, B, (void *)&id3);
	pthread_create(&thread4, NULL, Q, (void *)&id4);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    printf("sem test finish!\n");

    int id5 = 5;
    pthread_t thread5;
    int revalue;
    pthread_create(&thread5, NULL, work, (void *)&id5);
    printf("send cancel message\n");
    pthread_cancel(thread5);
    pthread_join(thread5, NULL);
    printf("cancel test finish!\n");
}
