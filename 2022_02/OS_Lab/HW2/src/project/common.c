#include "common.h"


/**
 * A function that gets empty shared object's index.
 * This will find the fastest index that it can find from the shared objects.
 * @param globalStatus The array of global status.
 * @param sharedObjects The shared objects.
 * @param nSharedObjects The total count of shared objects (which is length of array)
 * @return -2 if shared object is full, index number if something was empty.
 */
int getEmptyIndex(const int* globalStatus, const sharedObject* sharedObjects, int nSharedObjects) {
	int ret = nSharedObjects;

    // Iterate and find empty shared object.
    for (int i = 0 ; i < nSharedObjects ; i++) {
        // Look for any not used shared object.
		if (globalStatus[i] == 0 && (sharedObjects[i].isFull == 0)) {
            ret = i;
            break;
        }
	}

    // Return -2 if shared object was full, otherwise return found index
    return ret == nSharedObjects ? -2 : ret;
}


/**
 * A function that gets filled shared object's index.
 * This will find the fastest index that it can find from the shared objects.
 * @param globalStatus The array of global status.
 * @param sharedObjects The shared objects.
 * @param nSharedObjects The total count of shared objects (which is length of array)
 * @return -2 if shared object is empty, index number if something was filled.
 */
int getFilledIndex(const int* globalStatus, const sharedObject* sharedObjects, int nSharedObjects) {
    int ret = -2;

    // Iterate and find filled shared object.
    for (int i = 0 ; i < nSharedObjects ; i++) {
        if (globalStatus[i] == 1 && (sharedObjects[i].isFull != 0)) {
            ret = i;
            break;
        }
    }

    // Return -2 if shared object was empty, otherwise return found index
    return ret;
}


/**
 * A function that checks if all workers are over.
 * This function is meant to be used when handler threads are waiting for worker threads to be finished.
 * @param workers The array of workerJobs.
 * @param count The total count of workers.
 * @return 1 if all workers are terminated, 0 if not.
 */
int isAllWorkerOver(const int* workers, int count) {
    // Iterate over all workerJobs.
    for (int i = 0 ; i < count ; i++) {
        // Workers will store -3 if they were finished.
        if (workers[i] != -3) {
            return 0;
        }
    }
    return 1;
}


/**
 * A function that processes single line and stores status into wordStat.
 * The original code came from "char_stat.c" which professor gave us.
 * Needed to modify a bit of code to make it work in here.
 * @param line The line to be processed.
 * @param stat The wordStat to store data into.
 */
void processString(char* line, wordStat* stat) {
    char *cptr = NULL;
    char *substr = NULL;
    char *brka = NULL;
    char *sep = "{}()[],;\" \n\t^";
    int length = 0;
    // For each line,

    cptr = line;
    for (substr = strtok_r(cptr, sep, &brka);
         substr;
         substr = strtok_r(NULL, sep, &brka)) {
        length = strlen(substr);
        // update stats
        cptr = cptr + length + 1;
        if (length >= 30) length = 30;
        stat->stringStat[length-1]++;
        if (*cptr == '\0') break;
    }
    cptr = line;
    for (int i = 0 ; i < length ; i++) {
        if (*cptr < 256 && *cptr > 1) {
            stat->charStat[*cptr]++;
        }
        cptr++;
    }
}


/**
 * A function that merges multiple wordStat into one wordStat.
 * Since consumer threads use different wordStat, this function will merge all wordStat into one wordStat.
 * This will malloc wordStat. So don't forget to free it!
 * @param stats The array of wordStat.
 * @param count The length of array.
 * @return Returns merged wordStat.
 */
wordStat* mergeStats(wordStat* stats, int count) {
    // Generate wordStat and initialize it with 0.
    wordStat* tmp = (wordStat*) malloc(sizeof(wordStat));
    for (int i = 0 ; i < MAX_STRING_LENGTH ; i++) tmp->stringStat[i] = 0;
    for (int i = 0 ; i < ASCII_SIZE ; i++) tmp->charStat[i] = 0;

    // Iterate over all wordStat
    for (int i = 0 ; i < count ; i++) {
        // Store stringStats
        for (int j = 0 ; j < MAX_STRING_LENGTH ; j++) {
            tmp->stringStat[j] += stats[i].stringStat[j];
        }
        // Store charStats
        for (int j = 0 ; j < ASCII_SIZE ; j++) {
            tmp->charStat[j] += stats[i].charStat[j];
        }
    }
    return tmp;
}


/**
 * A function that prints out wordStat.
 * The original code came from "char_stat.c" which professor gave us.
 * @param argStat The wordStat to print it out.
 */
void printStats(wordStat* argStat){
    int* stat2 = argStat->charStat;
    int* stat = argStat->stringStat;

    // print out distributions
    printf("*** print out distributions *** \n");
    printf("  #ch  freq \n");
    int sum = 0;

    for (int i = 0 ; i < 30 ; i++) {
        sum += stat[i];
    }

    for (int i = 0 ; i < 30 ; i++) {
        int j = 0;
        int num_star = stat[i] * 80/sum;
        printf("[%3d]: %4d \t", i+1, stat[i]);
        for (j = 0 ; j < num_star ; j++)
            printf("*");
        printf("\n");
    }
    printf("       A        B        C        D        E        F        G        H        I        J        K        L        M        N        O        P        Q        R        S        T        U        V        W        X        Y        Z\n");
    printf("%8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d %8d\n",
           stat2['A']+stat2['a'], stat2['B']+stat2['b'],  stat2['C']+stat2['c'],  stat2['D']+stat2['d'],  stat2['E']+stat2['e'],
           stat2['F']+stat2['f'], stat2['G']+stat2['g'],  stat2['H']+stat2['h'],  stat2['I']+stat2['i'],  stat2['J']+stat2['j'],
           stat2['K']+stat2['k'], stat2['L']+stat2['l'],  stat2['M']+stat2['m'],  stat2['N']+stat2['n'],  stat2['O']+stat2['o'],
           stat2['P']+stat2['p'], stat2['Q']+stat2['q'],  stat2['R']+stat2['r'],  stat2['S']+stat2['s'],  stat2['T']+stat2['t'],
           stat2['U']+stat2['u'], stat2['V']+stat2['v'],  stat2['W']+stat2['w'],  stat2['X']+stat2['x'],  stat2['Y']+stat2['y'],
           stat2['Z']+stat2['z']);
}