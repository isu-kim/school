//
// @file : utils.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that has all those utility functions.
//


#include "utils.h"


extern long segments[];
extern long segment_lengths[];

/**
 * A function that gets file size in bytes
 * @param file_name The name of file to read.
 * @return -1 if the file was not readable,
 *         otherwise, will return total byte length of file.
 */
long get_file_size(const char* file_name) {
    FILE* fp = fopen(file_name, "rb");
    if (!fp) return -1;
    else {
        fseek(fp, 0L, SEEK_END);
        return ftell(fp);
    }
}


/**
 * A function that parses arguments.
 * @param argc The argc for main function.
 * @param argv The argv for main function.
 * @param file_name The pointer to string to store file name into.
 * @param thread_count The pointer to integer to store total thread number into.
 * @return returns -1 if invalid, 0 if valid.
 */
int parse_args(int argc, char* argv[], char* file_name, int* thread_count) {
    // ==== Parse argument STARTS ====
    // The original code came from "prod_cons.c"
    if (argc == 1) {
        printf("usage: ./hw2 <readfile> #thread_count\n");
        exit(0);
    }

    FILE* rFile = fopen((char *) argv[1], "r");  // Open file with "rb" mode.
    if (rFile == NULL) {
        printf("[-] Could not find file.");
        return -1;
    } else strcpy(file_name, argv[1]);

    if (argv[2] != NULL) { // Get producer count
        *thread_count = (int) strtol(argv[2], NULL, 10);
        if (*thread_count > 100) *thread_count = 100; // max 100
        if (*thread_count == 0) *thread_count = 1;
    } else *thread_count = 1;

    if (*thread_count < 0) {
        printf("[-] Total thread count must be positive number.\n");
        return -1;
    }

    return 0;
}


/**
 * A function that generates segments from total length.
 * For example, this function will make 10 with 3 segment_count to -> 3 / 3 / 4
 * Original code : https://stackoverflow.com/a/32543184/5716511
 * @param total The total bytes of file.
 * @param segment_count The total count of segments.
 * @return The segment size array.
 */
long* split_segments(long total, int segment_count) {
    long* segments_tmp = malloc(sizeof(long) * segment_count);
    memset(segments_tmp, 0, sizeof(long) * segment_count);

    long remain = total;
    int partsLeft = segment_count;

    for (int i = 0; partsLeft > 0; i++) {
        long size = (remain + partsLeft - 1) / partsLeft; // rounded up, aka ceiling
        segments_tmp[i] = size;
        remain -= size;
        partsLeft--;
    }

    return segments_tmp;
}


/**
 * A function that reads file from disk with length.
 * If this function could not read file from disk, it will exit(0)
 * @param data The char* to store data into.
 * @param offset The offset of current file read.
 * @param length The length of current file read.
 */
void read_file(char* data, long offset, long length) {
    char tmp[BLOCK_SIZE];    // Open with rb option.
    extern char file_name[];
    FILE* fp = fopen(file_name, "rb");
    if (!fp) {
        printf("\n[ERROR] Could not open file : %s\n", strerror(errno));
        exit(0);
    }

    fseek(fp, offset, SEEK_SET);
    fread(tmp, length, 1, fp);
    fclose(fp); // Don't forget to close file lol.
    strcpy(data, tmp);
}


/**
 * A function that sets segments and segment_lengths using sizes splitted.
 * @param size The total size of the file to be splitted.
 * @param producer_num The total count of segments to be processed.
 */
void set_segments(long size, int producer_num) {
    // Split segments with respect to producer counts.
    long *tmp_lengths = split_segments(size, producer_num);
    long cur_sum = 0;
    for (int i = 0; i < producer_num; i++) {
        segments[i] = cur_sum;
        segment_lengths[i] = tmp_lengths[i];
        cur_sum += tmp_lengths[i];
    }
}


/**
 * A function that initializes shared objects.
 * This will malloc and return the allocated array.
 * @param shared_object_num The total count of shared objects.
 * @return malloced array of shared objects.
 */
struct so_t* init_shared_objects(int shared_object_num) {
    struct so_t* so = malloc(sizeof(struct so_t) * shared_object_num);
    for (int i = 0 ; i < shared_object_num ; i++) {
        pthread_cond_init(&so->cond, NULL);
        pthread_mutex_init(&so->lock, NULL);
        so->empty = 1;
    }

    return so;
}


/**
 * A function that initializes producer_arg_t array.
 * @param producer_num The total number of producers.
 * @param shared_objects The shared object count.
 * @return The allocated array of producer_arg_t.
 */
struct producer_arg_t* init_producer_args(int producer_num, struct so_t* shared_objects) {
    // Malloc and memset heap variable.
    struct producer_arg_t* pw_a = malloc(sizeof(struct producer_arg_t) * producer_num);
    memset(pw_a, 0, sizeof(struct producer_arg_t) * producer_num);

    // Init variables
    for (int i = 0 ; i < producer_num ; i++) {
        pw_a[i].index = i;
        pw_a[i].shared_objects = shared_objects + i;
        pw_a[i].offset = segments[i];
        pw_a[i].total_bytes = segment_lengths[i];
    }

    return pw_a;
}


/**
 * A function that initializes consumer_arg_t array.
 * @param producer_worker_num The total number of consumers.
 * @param shared_objects The shared object count.
 * @param stats The pointer to word_stats_t* to store initialized word_stat into.
 * @return The allocated array of consumer_arg_t.
 */
struct consumer_arg_t* init_consumer_args(int consumer_num, struct so_t* shared_objects, struct word_stat_t* stats) {
    // Malloc and memset heap variable.
    struct consumer_arg_t* cw_a = malloc(sizeof(struct consumer_arg_t) * consumer_num);
    memset(cw_a, 0, sizeof(struct consumer_arg_t) * consumer_num);

    // Init variables
    for (int i = 0 ; i < consumer_num ; i++) {
        cw_a[i].index = i;
        cw_a[i].shared_objects = shared_objects + i;
        cw_a[i].stats = stats + i;
    }

    return cw_a;
}


/**
 * A function that processes single line and stores status into wordStat.
 * The original code came from "char_stat.c" which professor gave us.
 * Needed to modify a bit of code to make it work in here.
 * @param line The line to be processed.
 * @param stat The wordStat to store data into.
 */
void process_string(char* line, struct word_stat_t* stat) {
    char *cptr = NULL;
    char *substr = NULL;
    char *brka = NULL;
    char *sep = "{}()[],;\" \n\t^";
    //long length = 0;
    // For each line,
    char* ptr;

    for (ptr = strtok(line, "\n") ; ptr != NULL; ptr = strtok(NULL, "\n")) {
        cptr = ptr;
        for (substr = strtok_r(cptr, sep, &brka); substr; substr = strtok_r(NULL, sep, &brka)) {
            int tmp_length = strlen(substr);
            // update stats
            // length of the sub-string
            if (tmp_length >= 30) tmp_length = 30;
#ifdef _IO_
            printf("length: %d\n", (int)length);
#endif
            stat->stringStat[tmp_length - 1]++;

            // number of the character in the sub-string
            for (int i = 0; i < tmp_length; i++) {
                if (*cptr < 256 && *cptr > 1) {
                    stat->charStat[*cptr]++;
#ifdef _IO_
                    printf("# of %c(%d): %d\n", *cptr, *cptr, stat2[*cptr]);
#endif
                }
                cptr++;
            }
            cptr++;
            if (*cptr == '\0') break;
        }
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
struct word_stat_t* merge_stats(struct word_stat_t* stats, int count) {
    // Generate wordStat and initialize it with 0.
    struct word_stat_t* tmp = (struct word_stat_t*) malloc(sizeof(struct word_stat_t));
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
void print_stats(struct word_stat_t* argStat){
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