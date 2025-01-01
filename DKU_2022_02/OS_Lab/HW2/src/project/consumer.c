#include "consumer.h"


/**
 * A function that is for consumer handler.
 * This is a handler function for consumer workers.
 * This will:
 * 1. Check which shared object is filled.
 * 2. Wake a consumer worker up and tell it to process it.
 * This thread will terminate if all worker threads are terminated.
 * @param args The argument that was provided
 * @return Returns pthread_exit value.
 */
void* consumerHandler(void* args) {
    // printf("[CH] Generated\n");
    // Init variables.
	int* ret = (int*) malloc(sizeof(int));
	consumerHandlerArgs* cha = args;

	int filledIndex;
	int curWorker;

	while(1) {
        // First, check if the job was over.
        // == Critical Section between consumer worker and handler STARTS
        pthread_mutex_lock(cha->consumerMutex);
        int allWorkerDone = isAllWorkerOver(cha->workerJobs, cha->nWorkers);
        pthread_mutex_unlock(cha->consumerMutex);
        // == Critical Section between consumer worker and handler ENDS

        // If all consumer workers are over, break.
        if (allWorkerDone) break;

        // If the shared object was filled, meaning that it was not empty, broadcast to producer handler.
        // This is just a backup if producer handler is waiting for signal that shared object is not full.
        if (getEmptyIndex(cha->globalStatus, cha->sharedObjects, cha->nSharedObjects) != 2)
            pthread_cond_broadcast(cha->c2pCond);

        // == Critical Section between consumer handler and producer handler STARTS
		pthread_mutex_lock(cha->globalMutex);

		// Get index of filled data in shared object.
        filledIndex = getFilledIndex(cha->globalStatus, cha->sharedObjects, cha->nSharedObjects);

		// If the shared object was empty, wait for any of producer worker's signal.
        // However, this will wait with timeout since the signal might get lost (or desynchronized).
		if (filledIndex == -2) {
            struct timeval now;
            struct timespec ts;

            // Set timeout info. Wait 1 second.
            gettimeofday(&now, NULL);
            ts.tv_sec = now.tv_sec + 1;
            ts.tv_nsec = now.tv_usec * 1000;

            // Wait for signal with timeout.
            // If signal arrived, go back to the start of the loop.
            pthread_cond_timedwait(cha->p2cCond, cha->globalMutex, &ts);
            pthread_mutex_unlock(cha->globalMutex);
            // == Critical Section between consumer handler and producer handler ENDS
            continue;
        }

        // == Critical Section between consumer worker and handler STARTS
        pthread_mutex_lock(cha->consumerMutex);
        // Now, select which worker thread to make it work this time.
        // This will make workers go like in order of : 0 -> 1 -> 2 -> 3 -> ... -> 0
        if (curWorker == cha->nWorkers) curWorker = 0;
        if (cha->workerJobs[curWorker] != -1) {
            pthread_mutex_unlock(cha->consumerMutex);
            pthread_mutex_unlock(cha->globalMutex);
            curWorker++;
            continue;
        }
        pthread_mutex_unlock(cha->consumerMutex);
        // == Critical Section between consumer worker and handler ENDS

        cha->globalStatus[filledIndex] = 0; // Set the shared object's status as empty.
        cha->workerJobs[curWorker] = filledIndex; // Let worker know where to put data at.

        pthread_mutex_unlock(cha->globalMutex);
        // == Critical Section between consumer handler and producer handler ENDS

		pthread_cond_signal(&cha->workerCond[curWorker]); // Tell worker to wake the fuck up and work.
	}

    // Exit with return value 0
    *ret = 0;
	pthread_exit(ret);
}

/**
 * A function that is for consumer worker.
 * The thread will terminate if and only if the shared object that this thread consumed string of NULL.
 * @param args The argument that was provided
 * @return Returns pthread_exit value.
 */
void* consumerWorker(void* args) {
    // Init variables.
	int* ret = (int*) malloc(sizeof(int));
	int readCount = 0;
    int isAwake = 0;
    char* line = NULL;
	consumerWorkerArgs* cwa = args;

    // printf("[CW%02d] Generated\n", cwa->myIndex);

    while(1) {
		// Wait for consumer handler's signal.
        if (!isAwake) {
            pthread_mutex_t useless;
            pthread_cond_wait(cwa->myCond, &useless);
            isAwake = 1;
        }

        // If the worker thread was not over, get target shared object.
        sharedObject* targetObject = &cwa->sharedObjects[cwa->workerJobs[cwa->myIndex]];

        // == Critical Section among consumer workers and producer worker STARTS
        pthread_mutex_lock(targetObject->mutex);

        // If the shared object was not filled in yet, wait for producer worker to put data in.
        // Also wait with timeout since this might cause deadlock if not timeout wait.
        if (targetObject->isFull == 0) {
            //printf("[CW%02d] Not filled yet\n", cwa->myIndex);
            struct timeval now;
            struct timespec ts;

            gettimeofday(&now, NULL);
            ts.tv_sec = now.tv_sec + 1;
            ts.tv_nsec = now.tv_usec * 1000;

            pthread_cond_timedwait(targetObject->cond, targetObject->mutex, &ts);
            pthread_mutex_unlock(targetObject->mutex);
            // == Critical Section among consumer workers and producer worker ENDS

            continue;
        }

        // If shared object's line was NULL, that this was the last line.
        // Let consumer handler know that this was last line and break loop.
        if (targetObject->line == NULL) {
            pthread_mutex_unlock(targetObject->mutex);
            // == Critical Section among consumer workers and producer worker ENDS

            // == Critical Section among consumer workers and handler STARTS
            pthread_mutex_lock(cwa->consumerMutex);
            cwa->workerJobs[cwa->myIndex] = -3; // Set current worker status -3.
            pthread_mutex_unlock(cwa->consumerMutex);
            // == Critical Section among consumer workers and handler ENDS
            // printf("[CW%02d] Terminated - NULL\n", cwa->myIndex);
            break;
        }

        // If this was normal case, just read and store data from shared object.
		line = targetObject->line;
		targetObject->isFull = 0; // Mark it empty.

        // Unlock mutex and send signal to producer.
		pthread_mutex_unlock(targetObject->mutex);
        pthread_cond_broadcast(targetObject->cond);

        // printf("[CW%02d] Read %d : %s\n", cwa->myIndex, cwa->workerJobs[cwa->myIndex], line);
        processString(line, cwa->stat); // Process string.

        // == Critical Section among consumer workers and handler STARTS
        pthread_mutex_lock(cwa->consumerMutex);
        cwa->workerJobs[cwa->myIndex] = -1; // Mark current worker job as not assigned (-1)
        pthread_mutex_unlock(cwa->consumerMutex);
        // == Critical Section among consumer workers and handler ENDS

        pthread_cond_broadcast(cwa->c2pCond); // Broadcast to producer handler thread.
		readCount++; // Increment read count.
        isAwake = 0; // Set is not awake.
	}

    // Return with read count.
    *ret = readCount;
    pthread_exit(ret);
}