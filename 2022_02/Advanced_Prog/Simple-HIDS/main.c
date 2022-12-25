#include "common.h"


int flag = 1;
struct entry* entries[MAX_WATCH_THREADS] = {NULL};
int fd[MAX_WATCH_THREADS] = {0};
//struct wd_node* wds[MAX_WATCH_THREADS];
struct wd_node* wds[MAX_WATCH_THREADS] = {NULL};
threadpool thpool[MAX_WATCH_THREADS] = {NULL};
pthread_mutex_t global_mutex;
pthread_cond_t global_cond;
int active_thread_count = 0;


/**
 * @brief A function that is for each threads.
 *        For maxmium concurrency, each directory watching threads will generate 
 *        thread pools, entries and watch directory informations repectively.
 *        
 * @param args void* argument that is an argument for this current thread.
 * @returns NULL
 */
void* dir_thread(void* args) {
	struct thread_args* t_args = (struct thread_args*) args;
	char target_dir[MAX_STRING];
	strcpy(target_dir, t_args->target_dir);

	// Start setup
    fd[t_args->index] = inotify_init(); // Have a separate inotify_init().
	wds[t_args->index] = NULL; // Have a separate list of watch directories.
    thpool[t_args->index] = thpool_init(NUM_OF_THREADS); // Have a separate threadpool.

	printf("[DEBUG] Watcher thread %d started: watching %s\n", t_args->index, t_args->target_dir); 

    if (target_dir[strlen(target_dir)-1] == '/') { // Remove last / from directory
    	target_dir[strlen(target_dir)-1] = 0;
    }

	entries[t_args->index] = NULL; // Initialize entries of this thread.
    entries[t_args->index] = load_entries(entries[t_args->index], target_dir, t_args->index);
	watch(t_args->index);

#ifdef DEBUG
	printf("[DEBUG] Watcher thread %d terminated\n", t_args->index);
#endif

	pthread_exit(NULL);
}


/**
 * @brief A function that handles exit signal. 
 *        This will take care of SIGINT.
 *
 * @param ignored The SIGINT. So will be ignored.
 */
void exit_handler(int ignored) {
#ifdef DEBUG
	printf("[DEBUG] Exit handler called\n");
#endif
	flag = 0;
	// Iterate over all thread's data and remove watch and pools.
	for (int i = 0 ; i < active_thread_count ; i++) {
		struct wd_node* tmp_wd = wds[i];
		while(tmp_wd) {
			inotify_rm_watch(fd[i], tmp_wd->wd);
			tmp_wd = tmp_wd->next;
		}
		release_entries(entries[i], i);
#ifdef DEBUG
		printf("[DEBUG] Watcher thread %d released entries.\n", i);
#endif
		thpool_destroy(thpool[i]);
#ifdef DEBUG
		printf("[DEBUG] Watcher thread %d destroyed thread pool.\n", i);
#endif
	}
}


/**
 * @brief The all mighty main function.
 *
 * @param argc The argc
 * @param argv The argv
 * @return int -1 if failure, 0 if successfully executed.
 */
int main(int argc, char **argv) {
    if (argc == 1) {
        printf("Usage: %s [target directory]\n", argv[0]);
        return -1;
    }

    int ret = pthread_mutex_init(&global_mutex, NULL);
    if (ret == -1) {
	    printf("[ERROR] Failed to initialize mutex.\n");
		return -1;
	}
    ret = pthread_cond_init(&global_cond, NULL);
	if (ret == -1) {
	    printf("[ERROR] Failed to initialize condition.\n");
		return -1;
	}

	active_thread_count = argc - 1;

    printf("[INFO] Generated thread pool with %d threads\n", NUM_OF_THREADS);
	// register signal handler
    signal(SIGINT, exit_handler);

	pthread_t threads[MAX_WATCH_THREADS]; // Store threads

    for (int i = 1 ; i < argc ; i++) {
		struct thread_args* tmp = malloc(sizeof(struct thread_args)); // Need heap since this is multithread.
		tmp->index = i - 1;
		tmp->target_dir = argv[i];
#ifdef DEBUG
		printf("[DEBUG] Assigning directory %s to thread %d\n", tmp->target_dir, tmp->index);
#endif 
		pthread_create(&threads[i], NULL, dir_thread, (void*) tmp);	// Create thread with args.
	}

	void* ignored = NULL;
    for (int i = 1 ; i < argc ; i++) {
		pthread_join(threads[i], ignored);	// Join thread.
	}

#ifdef DEBUG
	printf("[DEBUG] All threads terminated.\n");
#endif

    return 0;
}
