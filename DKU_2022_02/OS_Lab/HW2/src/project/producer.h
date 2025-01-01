#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"


/**
 * A struct that will be used when producer handler function is called.
 * This is for sending arguments into producer handler.
 */
typedef struct producerHandlerArgs {
	int nWorkers; // Total number of producer workers.
	int nSharedObjects; // Total count of shared objects.
	int* globalStatus; // Global status on shared objects.
	int* workerJobs; // Status of producer workers. -1 means not assigned, -3 means finished, 1 means being used.
	int* isFinished; // To store if the file was finished or not.
	pthread_mutex_t* globalMutex; // For mutex between producer handler and consumer handler.
	pthread_cond_t* workerCond; // For waking up each producer workers.
    pthread_cond_t* p2cCond; // For sending signal to consumer from producer.
    pthread_mutex_t* producerMutex; // For mutexes between producer handler and producer workers.
    sharedObject* sharedObjects; // Shared objects.
} producerHandlerArgs;


/**
 * A struct that will be used when producer worker function was called.
 * This is for sending arguments into producer worker.
 */
typedef struct producerWorkerArgs {
	FILE* rFile; // The read FILE info.
	int myIndex; // This producer worker thread's index
	int* workerJobs; // The array of worker statuses
	int* isFinished; // If the producer was finished or not
    pthread_mutex_t* workerMutex; // The mutex between producer workers. This is used when reading file.
	pthread_cond_t* myCond; // The condition to wait for signal at.
	sharedObject* sharedObjects; // Shared objects.
    pthread_cond_t* p2cCond; // For sending signal into consumer from producer.
    pthread_mutex_t* producerMutex; // For mutexes between producer handler and producer consumer.
} producerWorkerArgs;

void* producerHandler(void*);
void* producerWorker(void*);
