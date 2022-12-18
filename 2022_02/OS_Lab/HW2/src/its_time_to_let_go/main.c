#include "common.h"
#include "utils.h"
#include "threads.h"

long segments[MAX_THREADS] = {0};
long segment_lengths[MAX_THREADS] = {0};
char file_name[MAX_STRING];

pthread_mutex_t handler_mutex;
pthread_cond_t handler_cond;


/**
 * The almighty main function.
 * @param argc Argc
 * @param argv Argv
 * @return 0 if successful, -1 if not.
 */
int main(int argc, char* argv[]) {
    int thread_count;
    if (parse_args(argc, argv, file_name, &thread_count) == -1) return -1; // Parse arguments
    long size = get_file_size(file_name);

    printf("[+] Total file size : %ld / Thread count : %d\n", size, thread_count);
#ifdef DEBUG
    printf("[DEBUG] File Size : %ld\n", size);
#endif

    // Set segments by size.
    set_segments(size, thread_count);

#ifdef DEBUG
    printf("[DEBUG] Segments : \n");
    for (int i = 0 ; i < thread_count ; i++) {
        printf("From %ld ~ %ld : %ld bytes\n", segments[i], segments[i] + segment_lengths[i] - 1, segment_lengths[i]);
    }
#endif

    // Check if handler mutexes and conditions were correctly initialized.
    if ((pthread_mutex_init(&handler_mutex, NULL) || pthread_cond_init(&handler_cond, NULL))) {
        printf("[-] Could not initialize mutexes and conditions.\n");
        return -1;
    }

    // For storing word stats.
    struct word_stat_t* ws = malloc(sizeof(struct word_stat_t) * thread_count);
    memset(ws, 0, sizeof(struct word_stat_t) * thread_count);

    // For arguments with thread callback functions.
    struct so_t* so = init_shared_objects(thread_count);
    struct producer_arg_t* pw_a = init_producer_args(thread_count, so);
    struct consumer_arg_t* cw_a = init_consumer_args(thread_count, so, ws);

    pthread_t prods[MAX_THREADS];
    pthread_t cons[MAX_THREADS];

    long prod_size = 0;
    long cons_size = 0;
    long* ret;

    // Start all threads.
    for (int i = 0 ; i < thread_count ; i++)
        pthread_create(&prods[i], NULL, producer_worker, pw_a + i); // create pthread for producer
    for (int i = 0 ; i < thread_count ; i++)
        pthread_create(&cons[i], NULL, consumer_worker, cw_a + i); // create pthread for consumer

    // Join all threads.
    for (int i = 0 ; i < thread_count ; i++) {
        pthread_join(cons[i], (void **) &ret);
        printf("[+] Producer %d read %ld bytes\n", i, *ret);
        prod_size += *ret;
    }
    for (int i = 0 ; i < thread_count ; i++) {
        pthread_join(prods[i], (void **) &ret);
        printf("[+] Consumer %d processed %ld bytes\n", i, *ret);
        cons_size += *ret;
    }

    printf("[+] Total : %ld / %ld\n", cons_size, prod_size);

    struct word_stat_t* merged = merge_stats(ws, thread_count); // Merge wordStatus from all consumer workers.
    print_stats(merged); // Print result.

    // OS is my garbage collector lol
    free(so);
    free(pw_a);
    free(cw_a);
    return 0;
}
