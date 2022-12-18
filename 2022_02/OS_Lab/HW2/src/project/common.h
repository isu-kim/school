#pragma once

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HANDLER_WAIT 1000
#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256


/**
 * A struct for shared object between producer workers and consumer workers.
 * This will actually store data between those two workers.
 */
typedef struct sharedObject {
	pthread_mutex_t* mutex;
	pthread_cond_t* cond;
	char* line;
	int isFull;
} sharedObject;


/**
 * A struct for storing stats of given string.
= */
typedef struct wordStat {
    int stringStat[MAX_STRING_LENGTH];
    int charStat[ASCII_SIZE];
} wordStat;

int getEmptyIndex(const int*, const sharedObject*, int);
int getFilledIndex(const int*, const sharedObject*, int);
int isAllWorkerOver(const int*, int);
void processString(char*, wordStat*);
wordStat* mergeStats(wordStat*, int);
void printStats(wordStat*);