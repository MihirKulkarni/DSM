#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

static pthread_mutex_t global_mutex;		// you can access this mutex in both the thread function and also in main

void *function() {
	pthread_mutex_lock(&global_mutex);		// globally accessible mutex
	printf("I'm in function.. mutex is locked\n");
	usleep(1000000);
	printf("I'm in function.. unlocking mutex now..\n");
	pthread_mutex_unlock(&global_mutex);
	return 0;
}

main(int argc, char * argv[]) {
	pthread_t thread1;
	pthread_mutex_lock(&global_mutex);		// same mutex that is declared as global variable
	printf("I'm in main.. mutex locked..\n");
	int th = pthread_create(&thread1, NULL, function, NULL);
//	pthread_join(thread1, NULL);				// this is where the thread actually starts its execution.
																				// this now creates a deadlock because the execution is in thread but mutex is still locked by main.
	printf("Unlocking mutex in main..\n");
	pthread_mutex_unlock(&global_mutex);
	pthread_join(thread1, NULL);				// this is where the thread actually starts its execution
}
