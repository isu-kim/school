//
// @file : threads.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that 
//


#include "threads.h"

/**
 * A function that is for producer worker thread.
 * @param args The arguments of producer_arg_t.
 * @return NULL
 */
void* producer_worker(void* args) {
    struct producer_arg_t* worker_args = (struct producer_arg_t*) args;
    struct so_t* target = worker_args->shared_objects;
    long offset = worker_args->offset;
    long end = offset + worker_args->total_bytes;
    int is_finished = 0;

    long* total_size = malloc(sizeof(long));
    *total_size = 0;

#ifdef DEBUG
    printf("[P%d] Started : %ld ~ %ld\n", worker_args->index, offset, end);
#endif

    // Unless the file was finished, keep reading.
    while (!is_finished) {
        int read_size;

        pthread_mutex_lock(&target->lock);
        // If the target was not empty, wait for it to become empty.
        if (target->empty == 0) {
            pthread_cond_wait(&target->cond, &target->lock);
        }

        // Check if remaining bytes are less than the block size.
        // If so, just read remaining part.
        if (end - offset <= BLOCK_SIZE) {
            read_size = (int) (end - offset); // This will be int.
            is_finished = 1; // After this read, this worker will end.
        } else read_size = BLOCK_SIZE;

        // Critical section starts here.

        read_file(target->content, offset, read_size);
        target->empty = is_finished ? -1 : 0; // If this was last, mark -1 instead of 0.
        target->size = read_size;

        pthread_mutex_unlock(&target->lock);
        pthread_cond_broadcast(&target->cond);
        // Critical Section ends here.

        offset += read_size;
        *total_size += read_size;
    }

#ifdef DEBUG
    printf("[P%d] Finished\n", worker_args->index);
#endif

    pthread_exit(total_size);
}


/**
 * A function that is for producer worker thread.
 * @param args The arguments of producer_arg_t.
 * @return NULL
 */
void* consumer_worker(void* args) {
    struct consumer_arg_t* worker_args = (struct consumer_arg_t*) args;
#ifdef DEBUG
    printf("[C%d] Started\n", worker_args->index);
#endif

    struct so_t* target = worker_args->shared_objects;
    int is_finished = 0;

    long* total_size = malloc(sizeof(long));
    *total_size = 0;

    // Unless the file was finished, keep reading.
    while (!is_finished) {

        // If the target was empty, wait for producer to fill something into the shared object.
        char tmp[BLOCK_SIZE];
        int read_size = 0;


        // Critical section starts here.
        pthread_mutex_lock(&target->lock);

        if (target->empty == 1)
            pthread_cond_wait(&target->cond, &target->lock);

        is_finished = target->empty == -1; // Check if this was the last element.
        strcpy(tmp, target->content); // copy content.
        target->empty = 1;
        read_size = target->size;

        pthread_mutex_unlock(&target->lock);
        pthread_cond_broadcast(&target->cond);
        // Critical section ends here.

        // Print out the result.
#ifdef DEBUG
        for (int i = 0 ; i < read_size ; i++) {
            printf("%c", tmp[i]);
        }
#endif
        process_string(tmp, worker_args->stats); // process string.
        *total_size += read_size;
    }

#ifdef DEBUG
    printf("[C%d] Finished\n", worker_args->index);
#endif

    pthread_exit(total_size);
}