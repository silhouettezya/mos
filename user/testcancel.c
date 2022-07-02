#include "lib.h"

void *work(void *args) {
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFFERED, NULL);
	int i;
	for (i = 0; i < 5; i++) {
    	writef("son put %d\n", i);
	}
	while (1) {
		writef("wait to be canceled\n");
		pthread_testcancel();
	}
}

void umain() {
	u_int id1 = 1;
	pthread_t thread1;
	int revalue;
	pthread_create(&thread1, NULL, work, NULL);
	writef("send cancel message\n");
    pthread_cancel(thread1);
    writef("finish!\n");
}

