#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"


/**
 * A struct that will be used when consumer handler function was called.
 * This is for sending arguments into consumer handler.
 */
typedef struct consumerHandlerArgs {
	int nWorkers; // Total count of consumer workers.
	int nSharedObjects; // Total count of shared objects.
	int* globalStatus; // Global status on shared objects.
	int* workerJobs; // Status of consumer workers. -1 means not assigned, -3 means finished, 1 means is being used.
    pthread_mutex_t* globalMutex; // For mutex between producer handler and consumer handler.
	pthread_cond_t* workerCond; // For waking up each consumer workers.
    pthread_cond_t* c2pCond; // For sending signal to producer from consumer.
    pthread_cond_t* p2cCond; // For waiting signal from consumer when the shared object was empty.
    pthread_mutex_t* consumerMutex; // For mutex between consumer handler and consumer workers.
    sharedObject* sharedObjects; // Shared objects.
} consumerHandlerArgs;


/**
 * A struct that will be used when consumer worker function was called.
 * This is for sending arguments into consumer worker.
 */
typedef struct consumerWorkerArgs {
	int myIndex; // This consumer worker thread's index
	int* workerJobs; // The array of worker statuses.
	pthread_cond_t* myCond; // The condition to wait for signal at.
	sharedObject* sharedObjects; // Shared objects.
    pthread_cond_t* c2pCond; // For sending signal to producer from consumer.
    pthread_mutex_t* consumerMutex; // For mutex between consumer handler and consumer workers.
    wordStat* stat; // For storing word stat after processing current line.
} consumerWorkerArgs;


void* consumerHandler(void*);
void* consumerWorker(void*);
