#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>


void p1_handler(int num) {
	printf("[P1] Got signal %s\n", strsignal(num));
	return;
}

void* p1(void* args){
	printf("[P1] Created\n");
	signal(SIGUSR1, p1_handler);
	while(1);
}

void p2_handler(int num) {
	printf("[P2] Got signal %s\n", strsignal(num));
	return;
}

void* p2(void* args){
	printf("[P2] Created\n");
	// This might be ignored. There shall be just one signal handler
	// For a signal.
	signal(SIGUSR2, p2_handler);
	while(1);
}

void p3_handler(int num) {
	printf("[P3] Got signal %s\n", strsignal(num));
	return;
}

void* p3(void* args){
	printf("[P3] Created\n");
	// This might be ignored. There shall be just one signal handler
	// For a signal.
	signal(SIGUSR2, p3_handler);
	while(1);
}


int main(void) {
	pthread_t p[3];
	printf("[+] PID : %d\n", getpid());

	printf("[+] Creating threads\n");
	pthread_create(&p[0], NULL, p1, NULL);
	pthread_create(&p[1], NULL, p2, NULL);
	pthread_create(&p[2], NULL, p3, NULL);

	printf("[+] Joining\n");
	pthread_join(p[0], NULL);
	pthread_join(p[1], NULL);
	pthread_join(p[2], NULL);

	return 0;
}






