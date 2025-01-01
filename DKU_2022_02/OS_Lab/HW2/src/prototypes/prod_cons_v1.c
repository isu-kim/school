#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct sharedobject {
	FILE* rfile;
	int linenum;
	char* line;
	pthread_mutex_t lock;
	pthread_cond_t cond;
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
		pthread_mutex_lock(&so->lock); // mutex lock
		
		// If shared buffer is full, wait for signal.
		// pthread_cond_wait will automatically unlock mutex as well.
		// This means that mutex is unlocked and consumer is able to use shared buffer.
		if (so->full) pthread_cond_wait(&so->cond, &so->lock);
		
		read = getdelim(&line, &len, '\n', rfile);
		if (read == -1) { // Fail to read line (including EOF)
			so->full = 1; // Finished reading line
			so->line = NULL;
			// This means producer no longer has any more lines to read!
			// Producer ended, so unlock and send signal to consumer process.
			pthread_mutex_unlock(&so->lock);
			pthread_cond_signal(&so->cond);

			break; // Then break.
		}

		// If normal case, just produce data into shared buffer.
		so->linenum = i; 
		so->line = strdup(line);
		i++;
		so->full = 1; // Set it full so that other thread can access it.

		// Critical section ends here.
		// Unlock mutex and signal.
		pthread_mutex_unlock(&so->lock);
		pthread_cond_signal(&so->cond);
	}

	free(line); // Need to free this since this is malloced.
	printf("Prod_%x: %d lines\n", (unsigned int) pthread_self(), i);
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
		// So lock mutex.
		pthread_mutex_lock(&so->lock);
		// If the shared buffer is not full, wait till this thread gets signal.
		// Also wait will unlock mutex so that producer can use it.
		if (!so->full) pthread_cond_wait(&so->cond, &so->lock);
		line = so->line;
		if (line == NULL) { // Meaning that producer ended.
			pthread_mutex_unlock(&so->lock);
			break;
		}
		len = strlen(line);
		printf("Cons_%x: [%02d:%02d] %s",
			(unsigned int)pthread_self(), i, so->linenum, line);
		free(so->line);
		i++;
		so->full = 0;

		// Critical secion ended, unlock and signal
		pthread_mutex_unlock(&so->lock);
		pthread_cond_signal(&so->cond);
	}
	printf("Cons: %d lines\n", i);
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
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
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

	pthread_mutex_init(&share->lock, NULL); // Generate mutex for pthread
	pthread_cond_init(&share->cond, NULL);

	for (i = 0 ; i < Nprod ; i++)
		pthread_create(&prod[i], NULL, producer, share); // create pthread for producer
	for (i = 0 ; i < Ncons ; i++)
		pthread_create(&cons[i], NULL, consumer, share); // create pthread for consumer
	
	printf("main continuing\n");

	for (i = 0 ; i < Ncons ; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret); // join pthread for consumer
	}
	for (i = 0 ; i < Nprod ; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret); // join pthread for producer
	}
	
	pthread_mutex_destroy(&share->lock);
	pthread_cond_destroy(&share->cond);

	pthread_exit(NULL);
	exit(0);
}

