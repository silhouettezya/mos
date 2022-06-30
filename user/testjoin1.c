#include "lib.h"

void *work(void *args) {
    int i;
    for (i = 0; i < 10; i++) {
        writef("put: %d\n", i);
    }
	pthread_exit(0);
}

void umain() {
	u_int id1 = 1;
	pthread_t thread1;
	int revalue;
	pthread_create(&thread1, NULL, work, NULL);
    pthread_join(thread1, revalue);
    writef("finish!\n");
}
