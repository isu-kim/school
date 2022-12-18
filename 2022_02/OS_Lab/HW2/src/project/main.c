#include <stdio.h>

#include "common.h"
#include "producer.h"
#include "consumer.h"


int main(int argc, char* argv[]) {
	int nProducers = 20;
	int nConsumers = 20;
	int nSharedObjects = 20;

    // ==== Parse argument STARTS ====
    // The original code came from "prod_cons.c"
    if (argc == 1) {
        printf("usage: ./project <readfile> #Producer #Consumer #SharedObjects\n");
        exit (0);
    }
    FILE* rFile = fopen((char *) argv[1], "r");  // open file with "r" mode.
    if (rFile == NULL) {
        printf("[-] Could not find file.");
        exit(0);
    }
    if (argv[2] != NULL) { // Get producer count
        nProducers = atoi(argv[2]);
        if (nProducers > 100) nProducers = 100; // max 100
        if (nProducers == 0) nProducers = 1;
    } else nProducers = 1;

    if (argv[3] != NULL) { // Get consumer count
        nConsumers = atoi(argv[3]);
        if (nConsumers > 100) nConsumers = 100;  // max 100
        if (nConsumers == 0) nConsumers = 1;
    } else nConsumers = 1;

    if (argv[3] != NULL) {
        nSharedObjects = atoi(argv[4]);
        if (nSharedObjects > 100) nSharedObjects = 100;  // max 100
        if (nSharedObjects == 0) nSharedObjects = 1;
    } else nSharedObjects = 1;
    // ==== Parse argument ENDS ====

    if ((nSharedObjects < nConsumers) || (nSharedObjects < nProducers)) {
        printf("[-] Shared objects must be greater than or equal to thread counts\n");
        printf("   Ex) #consumer 10, #producer 20 -> #Shared objects >= 20\n");
        return 0;
    }

    // ==== Prepare arguments STARTS ====
    // Yes, this is a lot :b
    pthread_t* pws = (pthread_t*) malloc(sizeof(pthread_t) * nProducers);
	pthread_t* cws = (pthread_t*) malloc(sizeof(pthread_t) * nConsumers);
	pthread_t* ph = (pthread_t*) malloc(sizeof(pthread_t));
	pthread_t* ch = (pthread_t*) malloc(sizeof(pthread_t));

	producerHandlerArgs* pha = (producerHandlerArgs*) malloc(sizeof(producerHandlerArgs));
	producerWorkerArgs* pwa = (producerWorkerArgs*) malloc(sizeof(producerWorkerArgs) * nProducers);
	consumerHandlerArgs* cha = (consumerHandlerArgs*) malloc(sizeof(consumerHandlerArgs));
	consumerWorkerArgs* cwa = (consumerWorkerArgs*) malloc(sizeof(consumerWorkerArgs) * nConsumers);

    wordStat* stats = (wordStat*) malloc(sizeof(wordStat) * nConsumers);
    for (int i = 0 ; i < nConsumers ; i++) {
        for (int j = 0 ; j < ASCII_SIZE ; j++) stats[i].charStat[j] = 0;
        for (int j = 0 ; j < MAX_STRING_LENGTH ; j++) stats[i].stringStat[j] = 0;
    }


    // For convention, if global status was set 0, this means the shared object is empty and is good to be used.
    // So, at first initialize all global statuses as 0 so that they are empty.
	int* globalStatus = (int*) malloc(sizeof(int) * nSharedObjects);
    for (int i = 0 ; i < nSharedObjects ; i++) globalStatus[i] = 0;

    // For convention, if worker job is set -1, this means the worker is ready to be used
    // So, at first initialize all worker jobs as -1 so that they are ready to be used.
	int* producerWorkerJobs = (int*) malloc(sizeof(int) * nProducers);
    for (int i = 0 ; i < nProducers ; i++) producerWorkerJobs[i] = -1;

    // For convention, if worker job is set -1, this means the worker is ready to be used
    // So, at first initialize all worker jobs as -1 so that they are ready to be used.
    int* consumerWorkerJobs = (int*) malloc(sizeof(int) * nConsumers);
    for (int i = 0 ; i < nConsumers ; i++) consumerWorkerJobs[i] = -1;

	int* producerIsFinished = (int*) malloc(sizeof(int));

	//FILE* rFile = fopen("./LICENSE", "r");
	//FILE* rFile = fopen("./ObsoleteFiles.inc100", "r"); // 100 Lines
	//FILE* rFile = fopen("./ObsoleteFiles.inc", "r"); // 1000 Lines
	//FILE* rFile = fopen("./ObsoleteFiles.incOld", "r");

	pthread_mutex_t globalMutex;
	pthread_mutex_t consumerWorkerMutex;
    pthread_mutex_t producerWorkerMutex;
    pthread_mutex_t consumerMutex;
    pthread_mutex_t producerMutex;
    pthread_cond_t globalCond;
    pthread_cond_t p2cCond;
    pthread_cond_t c2pCond;
	pthread_cond_t* producerWorkerCond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * nProducers);
	pthread_cond_t* consumerWorkerCond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * nConsumers);

	pthread_mutex_init(&globalMutex, NULL);
    pthread_mutex_init(&producerWorkerMutex, NULL);
	pthread_mutex_init(&consumerWorkerMutex, NULL);
	pthread_mutex_init(&consumerMutex, NULL);
	pthread_mutex_init(&producerMutex, NULL);
	pthread_cond_init(&globalCond, NULL);
    pthread_cond_init(&p2cCond, NULL);
    pthread_cond_init(&c2pCond, NULL);

	sharedObject* sharedObjects = (sharedObject*) malloc(sizeof(sharedObject) * nSharedObjects);
    pthread_mutex_t* sharedObjectMutexes = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * nSharedObjects);
    pthread_cond_t* sharedObjectConditions = (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * nSharedObjects);

    for (int i = 0 ; i < nSharedObjects ; i++) {
        pthread_mutex_init(sharedObjectMutexes + i, NULL);
        pthread_cond_init(sharedObjectConditions + i, NULL);
        sharedObjects[i].mutex = sharedObjectMutexes + i;
        sharedObjects[i].cond = sharedObjectConditions + i;
    }

	pha->nWorkers = nProducers;
	pha->nSharedObjects = nSharedObjects;
	pha->globalStatus = globalStatus;
	pha->workerJobs = producerWorkerJobs;
	pha->isFinished = producerIsFinished;
	pha->globalMutex = &globalMutex;
	pha->workerCond = producerWorkerCond;
    pha->p2cCond = &p2cCond;
    pha->producerMutex = &producerMutex;
    pha->sharedObjects = sharedObjects;

    cha->nWorkers = nConsumers;
    cha->nSharedObjects = nSharedObjects;
    cha->globalStatus = globalStatus;
    cha->workerJobs = consumerWorkerJobs;
    cha->globalMutex = &globalMutex;
    cha->workerCond = consumerWorkerCond;
    cha->p2cCond = &p2cCond;
    cha->c2pCond = &c2pCond;
    cha->consumerMutex = &consumerMutex;
    cha->sharedObjects = sharedObjects;


	for (int i = 0 ; i < nProducers ; i++) {
		pwa[i].rFile = rFile;
		pwa[i].myIndex = i;
		pwa[i].workerJobs = producerWorkerJobs;
		pwa[i].isFinished = producerIsFinished;
		pwa[i].workerMutex = &producerWorkerMutex;

		pthread_cond_init(&producerWorkerCond[i], NULL);
		pwa[i].myCond = &producerWorkerCond[i];
		pwa[i].sharedObjects = sharedObjects;
        pwa[i].p2cCond = &p2cCond;
        pwa[i].producerMutex = &producerMutex;
	}

    for (int i = 0 ; i < nConsumers ; i++) {
        cwa[i].myIndex = i;
        cwa[i].workerJobs = consumerWorkerJobs;

        pthread_cond_init(&consumerWorkerCond[i], NULL);
        cwa[i].myCond = &consumerWorkerCond[i];
        cwa[i].sharedObjects = sharedObjects;
        cwa[i].c2pCond = &c2pCond;
        cwa[i].consumerMutex = &consumerMutex;
        cwa[i].stat = &stats[i];
    }
    // ==== Prepare arguments ENDS ====


    // Real job starts here!
    printf("[+] Starting job...\n");
    printf("[+] Reading file : %s\n", argv[1]);
    printf("[+] Job info : %d Producers / %d Consumers / % d Shared objects\n", nProducers, nConsumers, nSharedObjects);
    printf("[+] If this process get stuck, kill it. Maybe an unexpected deadlock might happened.\n");
    clock_t begin = clock();

	for (int i = 0 ; i < nProducers ; i++) { // Generate producer workers
		pthread_create(pws + i, NULL, producerWorker, pwa + i);
	}

    for (int i = 0 ; i < nConsumers ; i++) { // Generate consumer workers
        pthread_create(cws + i, NULL, consumerWorker, cwa + i);
    }

    pthread_create(ph, NULL, producerHandler, pha); // Create producer handler
    pthread_create(ch, NULL, consumerHandler, cha); // Create consumer handler

    int producerWorkCount = 0;
    int consumerWorkCount = 0;

    // ===== Join producer STARTS =====
    int* ret1;
    pthread_join(*ph, (void**) &ret1); // Join producer handler.
    printf("[PH] Joined\n");

	for (int i = 0 ; i < nProducers ; i++) { // Join producer workers.
		int* ret2;
		pthread_join(pws[i], (void**) &ret2);
		printf("[PW%02d] Read %d lines\n", i, *ret2);
        producerWorkCount += *ret2;
	}
    // ===== Join producer ENDS =====

    printf("[+] Producer read %d lines\n", producerWorkCount);

    // ===== Join consumer STARTS =====
    for (int i = 0 ; i < nConsumers ; i++) {
        int* ret4;
        pthread_join(cws[i], (void**) &ret4);
        printf("[CW%02d] Processed %d lines\n", i, *ret4);
        consumerWorkCount += *ret4;
    }

    int* ret3;
    pthread_join(*ch, (void**) &ret3);
    printf("[CH] Joined\n");

    printf("[+] Consumer read %d lines\n", consumerWorkCount);
    // ===== Join consumer ENDS =====

    wordStat* merged = mergeStats(stats, nConsumers); // Merge wordStatus from all consumer workers.
    printStats(merged); // Print result.

    clock_t end = clock();
    double timeSpent = (double)(end - begin) / CLOCKS_PER_SEC;

    printf("[+] Elapsed time : %f\n", timeSpent);
    printf("[+] Processed : %d / %d\n", consumerWorkCount, producerWorkCount);

	return 0;	
}