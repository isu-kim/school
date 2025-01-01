#include "producer.h"


/**
 * A function that is for producer handler.
 * This function will handle producer worker threads.
 * This will do following things:
 * 1. Check if shared object is available.
 * 2. If so, grab available producer worker and make it to read line and store data into shared object.
 * @param args The argument that was provided
 * @return Returns pthread_exit value.
 */
void* producerHandler(void* args) {
	// printf("[PH] Generated\n");
    // Init variables.
	int* ret = (int*) malloc(sizeof(int));
	producerHandlerArgs* pha = args;

	int emptyIndex;
	int curWorker;

	while(1) {
        // First, check if the job was over.
        // == Critical Section between producer worker and handler STARTS
        pthread_mutex_lock(pha->producerMutex);
        int isFinished = *pha->isFinished;
        pthread_mutex_unlock(pha->producerMutex);
        // == Critical Section between producer worker and handler STARTS

        // If the job was over, break loop.
        if (isFinished) break;

        // If the shared object was filled, meaning that it was not empty, broadcast to consumer handler.
        // This is a backup signal just in case consumer handler is waiting for signal that the shared object is empty.
        if (getFilledIndex(pha->globalStatus, pha->sharedObjects, pha->nSharedObjects) != -2)
            pthread_cond_broadcast(pha->p2cCond);

        // == Critical Section between consumer handler and producer handler STARTS
		pthread_mutex_lock(pha->globalMutex);

        // Get shared object's empty index.
		emptyIndex = getEmptyIndex(pha->globalStatus, pha->sharedObjects, pha->nSharedObjects);

		// If the shared object was full, wait for any of consumer worker's signal.
        // However, this will wait with timeout since the signal might get lost (or desynchronized)
        if (emptyIndex == -2) {
            struct timeval now;
            struct timespec ts;

            // Set timeout info. Wait 1 second.
            gettimeofday(&now, NULL);
            ts.tv_sec = now.tv_sec + 1;
            ts.tv_nsec = now.tv_usec * 1000;

            // Wait for signal with timeout.
            // If signal arrived, go back to the start of the loop.
            pthread_cond_timedwait(pha->p2cCond, pha->globalMutex, &ts);
            pthread_mutex_unlock(pha->globalMutex);
            // == Critical Section between consumer handler and producer handler ENDS
            continue;
        }

        // == Critical Section between producer worker and handler STARTS
        pthread_mutex_lock(pha->producerMutex);

        // Now, select which worker thread to make it work this time.
        // This will make workers go like in order of : 0 -> 1 -> 2 -> 3 -> ... -> 0
        if (curWorker == pha->nWorkers) curWorker = 0;
        if (pha->workerJobs[curWorker] != -1) {
            pthread_mutex_unlock(pha->producerMutex);
            pthread_mutex_unlock(pha->globalMutex);
            curWorker++;
            continue;
        }

        pthread_mutex_unlock(pha->producerMutex);
        // == Critical Section between producer worker and handler ENDS

        pha->globalStatus[emptyIndex] = 1; // Set shared object as filled.
        pha->workerJobs[curWorker] = emptyIndex; // Let worker know where to put data at.

        pthread_mutex_unlock(pha->globalMutex);
        // == Critical Section between consumer handler and producer handler ENDS

        pthread_cond_signal(&pha->workerCond[curWorker]); // Tell worker to wake the fuck up and get to work.
	}

    int allWorkerDone = 0;
    // Keep sending signals to producer workers until all producer workers are finished.
    while (!allWorkerDone) {
        usleep(HANDLER_WAIT);
        pthread_mutex_lock(pha->producerMutex);
        allWorkerDone = isAllWorkerOver(pha->workerJobs, pha->nWorkers);

        pthread_mutex_unlock(pha->producerMutex);

        // Send signal to all worker threads
        // If we do not send signals to worker threads, they will literally wait until they get signal.
        // That will make deadlock and worker threads not finish properly.
        for (int i = 0 ; i < pha->nWorkers ; i++)
            pthread_cond_signal(&pha->workerCond[i]);
    }

    // When the producer workers are over, there might be some shared objects that are empty.
    // In order for consumer workers to terminate without any issues, pretend those empty objects as filled.
    pthread_mutex_lock(pha->globalMutex);
    for (int i = 0 ; i < pha->nSharedObjects ; i++) {
        if (pha->sharedObjects[i].isFull == 1) continue;
        else {
            pha->sharedObjects[i].line = NULL;
            pha->sharedObjects[i].isFull = -1;
            pha->globalStatus[i] = 1;
        }
    }
    pthread_mutex_unlock(pha->globalMutex);

    // Exit with status 0.
	*ret = 0;
	pthread_exit(ret);
}


/**
 * A function for producer worker.
 * This actually reads file and gets data from the file.
 * With the data it got, this will put data into the designated shared object.
 * This thread will terminate if the read line was NULL.
 * @param args The argument that was provided
 * @return Returns pthread_exit value.
 */
void* producerWorker(void* args) {
    // Init variables.
	int* ret = (int*) malloc(sizeof(int));
	int readCount = 0;
    char* line = NULL;
	size_t len = 0;
    ssize_t read;
	producerWorkerArgs* pwa = args;

	// printf("[PW%02d] Generated\n", pwa->myIndex);

	while(1) {
        // Need to set line as null.
        line = NULL;

        // Wait for producer handler's signal.
        pthread_mutex_t useless;
        pthread_cond_wait(pwa->myCond, &useless);

        // == Critical Section among producer handlers STARTS
        pthread_mutex_lock(pwa->workerMutex);
        read = getdelim(&line, &len, '\n', pwa->rFile);  // Read line from file.
        *pwa->isFinished = read == -1 ? 1 : 0; // If the file was finished, set it -1.
        pthread_mutex_unlock(pwa->workerMutex);
        // == Critical Section among producer handlers ENDS

        // If this worker thread was called without any provided shared object, meaning that the job was done, break.
        if (pwa->workerJobs[pwa->myIndex] == -1) {
            // == Critical Section among producer workers and handler STARTS
            pthread_mutex_lock(pwa->producerMutex);
            pwa->workerJobs[pwa->myIndex] = -3;
            pthread_mutex_unlock(pwa->producerMutex);
            // == Critical Section among producer workers and handler ENDS
            break;
        }

        // If the worker thread was not over, get target shared object.
        sharedObject* targetObject = &pwa->sharedObjects[pwa->workerJobs[pwa->myIndex]];

        // If it was last line, set it finished and break
        if (read == -1) {
            pthread_mutex_lock(targetObject->mutex);
            targetObject->line = NULL;
            targetObject->isFull = -1;
            pthread_mutex_unlock(targetObject->mutex);
            pthread_cond_signal(targetObject->cond);

            // == Critical Section among producer workers and handler STARTS
            pthread_mutex_lock(pwa->producerMutex);
            pwa->workerJobs[pwa->myIndex] = -3;
            pthread_mutex_unlock(pwa->producerMutex);
            // == Critical Section among producer workers and handler ENDS

            break;
        }

        // == Critical Section among consumer workers and producer worker STARTS
		pthread_mutex_lock(targetObject->mutex);

		// Store data into shared object.
		targetObject->line = line;
		targetObject->isFull = 1;

		pthread_mutex_unlock(targetObject->mutex);
        // == Critical Section among consumer workers and producer worker ENDS
        pthread_cond_broadcast(targetObject->cond); // When it is done using the shared object, send signal.

        // printf("[PW%02d] Store %d : %s\n", pwa->myIndex, pwa->workerJobs[pwa->myIndex], line);

        // Set worker job as -1 so that producer handler knows this thread is ready.
        // == Critical Section among producer workers and handler STARTS
        pthread_mutex_lock(pwa->producerMutex);
        pwa->workerJobs[pwa->myIndex] = -1;
        pthread_mutex_unlock(pwa->producerMutex);
        // == Critical Section among producer workers and handler ENDS

        // Send signal p2cCond just in case consumer handler is waiting for producer worker's signal.
        pthread_cond_broadcast(pwa->p2cCond); // Broadcast to consumer handler thread.
		readCount++; // Increment read count.
	}

    // Return with read count.
    *ret = readCount;
    pthread_cond_broadcast(pwa->p2cCond);
    pthread_exit(ret);
}
