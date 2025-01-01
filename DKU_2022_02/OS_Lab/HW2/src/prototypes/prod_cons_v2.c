#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
	FILE* rfile;
	int linenum;
	char* line;
	pthread_mutex_t globalLock; // Lock for between producer and consumers
	pthread_mutex_t consumerLock; // Lock between consumers
	pthread_cond_t globalCond; // Condition between producer and consumers
	pthread_cond_t consumerCond; // Lock between consumers
	int full;
} so_t;


/**
 * A function that is for producer.
 * @param void* arg The arguments when called with fp.
 */
void* producer(void* arg) {
	// Store values from arg when first called.
	so_t* so = arg;
	int* ret = malloc(sizeof(int));
	FILE* rfile = so->rfile;
	int i = 0;
	char* line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		// Critical section
		pthread_mutex_lock(&so->globalLock); // mutex lock
		
		// If shared buffer is full, wait for signal that consumer used it.
		// pthread_cond_wait will automatically unlock mutex as well.
		// This means that mutex is unlocked and consumer is able to use shared buffer.
		if (so->full) pthread_cond_wait(&so->globalCond, &so->globalLock);
		
		// Try reading a new line
		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) { // Fail to read line (including EOF)
			so->full = 1; // Finished reading line
			so->line = NULL;
			// This means producer no longer has any more lines to read!
			// Producer ended, so unlock and send signal to consumer process.
			pthread_mutex_unlock(&so->globalLock);
			pthread_cond_broadcast(&so->globalCond);

			break; // Then break.
		}

		// If normal case, just produce data into shared buffer.
		so->linenum = i; 
		so->line = strdup(line);
		i++;
		so->full = 1; // Set it full so that other thread can access it.

		// Critical section ends here.
		// Unlock mutex and signal.
		pthread_mutex_unlock(&so->globalLock);
		pthread_cond_broadcast(&so->globalCond);
	}

	free(line); // Need to free this since this is malloced.
	// printf("[+] Prod_%x: %d lines\n", (unsigned int) pthread_self(), i);
	*ret = i;

	// Exit thread
	pthread_exit(ret);
}


/**
 * A function that consumes data from shared buffer. (Consumer)
 * @param void* arg The arguments when called with fp.
 */
void* consumer(void* arg) { // A function for consumer
	so_t* so = arg;
	int* ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char* line;

	while (1) {
		// Critical section starts here!
		// Since we have multiple consumers, we need a mutex between consumers.
		// Otherwise consumer threads will have a race condition which can break our shared memory!
		pthread_mutex_lock(&so->consumerLock); // Tell other consumer threads that mutex is locked.
		pthread_mutex_lock(&so->globalLock); // Lock mutex for producer and consumers
		// If the shared buffer is not full, wait till this thread gets signal.
		// Also wait will unlock mutex so that producer can use it.
		if (!so->full) pthread_cond_wait(&so->globalCond, &so->globalLock);

		line = so->line;
		if (line == NULL) { // Meaning that producer ended.
			break;
		}

		len = strlen(line);
		printf("[+] Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line);
		i++;
		so->full = 0;

		// Critical secion ended, unlock and signal
		
		// Unlocking global lock and broadcasting globalCond will make producer's wait over.
		// Even if all consumers are waiting for globalLock, since they are locked by consumerLock currently.
		// Meaning that they will not respond to the signal that this thread sent.
		pthread_mutex_unlock(&so->globalLock);
		pthread_cond_broadcast(&so->globalCond);
		// Finally unlock mutex between all consumers.
		pthread_mutex_unlock(&so->consumerLock);
	}

	// When the process was done, unlock mutex between consumers.
	// Also don't forget to unlock mutex between consumers and producers.
	pthread_mutex_unlock(&so->consumerLock);
	pthread_mutex_unlock(&so->globalLock);
	*ret = i;
	pthread_exit(ret);
}


int main (int argc, char *argv[]) {
	pthread_t prod[100];
	pthread_t cons[100];
	int Nprod, Ncons;
	int rc;   long t;
	int* ret;
	int i;
	FILE* rfile;
	if (argc == 1) {
		printf("[-] usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit (0);
	}
	so_t* share = malloc(sizeof(so_t)); // heap with so_t
	memset(share, 0, sizeof(so_t)); // set it 0 for share (same as calloc)
	rfile = fopen((char *) argv[1], "r");  // open file with "r" mode.
	if (rfile == NULL) {
		perror("rfile");
		exit(0);
	}
	if (argv[2] != NULL) { // Get producer count
		Nprod = atoi(argv[2]);
		if (Nprod > 100) Nprod = 100; // max 100
		if (Nprod == 0) Nprod = 1;
	} else Nprod = 1;
	if (argv[3] != NULL) { // Get consumer count
		Ncons = atoi(argv[3]);
		if (Ncons > 100) Ncons = 100;  // max 100
		if (Ncons == 0) Ncons = 1;
	} else Ncons = 1;

	share->rfile = rfile;
	share->line = NULL;

	pthread_mutex_init(&share->globalLock, NULL); // Generate mutex for pthread
	pthread_mutex_init(&share->consumerLock, NULL); // Generate mutex for pthread
	pthread_cond_init(&share->globalCond, NULL);
	pthread_cond_init(&share->consumerCond, NULL);

	for (i = 0 ; i < Nprod ; i++) {
		printf("[+] Producer %d created !\n", i);
		pthread_create(&prod[i], NULL, producer, share); // create pthread for producer
	}
	for (i = 0 ; i < Ncons ; i++) {
		printf("[+] Consumer %d created !\n", i);
		pthread_create(&cons[i], NULL, consumer, share); // create pthread for consumer
	}
	
	printf("[+] main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("[+] consumer_%d joined with %d\n", i, *ret); // join pthread for consumer
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("[+] producer_%d joined with %d\n", i, *ret); // join pthread for producer
	}
	pthread_exit(NULL);
	exit(0);
}

