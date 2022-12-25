#include "entry.h"

extern int fd[MAX_WATCH_THREADS];
extern struct wd_node* wds[MAX_WATCH_THREADS];
extern threadpool thpool[MAX_WATCH_THREADS];
extern pthread_mutex_t global_mutex;
extern pthread_cond_t global_cond;

int update_entry_info(struct entry *node, char *path, int index)
{
    struct stat info = {0};

    strcpy(node->name, path);

    if (stat(path, &info) != 0) {
        printf("Failed to get the stat of %s\n", path);
        return -1;
    }

    node->mode = info.st_mode;
    node->size = info.st_size;
    node->atime = info.st_atime;
    node->mtime = info.st_mtime;

    struct hash_thpool_arg args = {.path = node->name, .hash = node->hash};

#ifdef DEBUG
	printf("[DEBUG] Watcher thread %d adding task to threadpool: %s\n", index, node->name);
#endif
	// To avoid locking a single thread pool, each directory watcher threads will be having different thread pool.
	// If we share one single thread pool and mutex lock and unlock the threadpook, that will be useless in terms of concurrency.
    thpool_add_work(thpool[index], hash_func, (void*)(&args));
    thpool_wait(thpool[index]);  // This waits for all jobs in thread pool, thus add work and wait
#ifdef DEBUG
    printf("[DEBUG] Thread pool wait over: %s\n", node->name);
#endif

    node->sibling = NULL;
    node->child = NULL;

    return 0;
}

struct entry * update_entries(struct entry *head, char *current_dir, int index)
{
    DIR *dp = opendir(current_dir);
    if (dp == NULL) {
        printf("Failed to open %s\n", current_dir);
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name)) {
            continue;
        }

        char path[1024] = {0};
        sprintf(path, "%s/%s", current_dir, entry->d_name);

        struct entry *new_node = (struct entry *)malloc(sizeof(struct entry));
        if (new_node == NULL) {
            printf("Failed to allocate a new node\n");
            continue;
        }
        update_entry_info(new_node, path, index);

        char atime[MAX_STRING];
        sprintf(atime, "%s", ctime(&new_node->atime));
        atime[strlen(atime)-1] = 0;

        char mtime[MAX_STRING];
        sprintf(mtime, "%s", ctime(&new_node->mtime));
        mtime[strlen(mtime)-1] = 0;

        if (S_ISDIR(new_node->mode)) {
            printf("d: %s | %lld | %s | %s | ", path, (long long)new_node->size, atime, mtime);
			for (int i = 0 ; i < MD5_DIGEST_LENGTH ; i++)
					printf("%02x", new_node->hash[i]);
			printf("\n");
            new_node->child = update_entries(new_node->child, path, index);
			int wd = inotify_add_watch(fd[index], path, IN_MODIFY | IN_CREATE | IN_DELETE); // Watch directory usinga add_watch
			// Insert new wd node for future use.
			struct wd_node* new_wd = malloc(sizeof(struct wd_node));
			strcpy(new_wd->path, path);

			// Critical section starts
//			pthread_mutex_lock(&global_mutex);
			new_wd->wd = wd;
			new_wd->next = wds[index];
			wds[index] = new_wd;
//			pthread_mutex_unlock(&global_mutex);
			// Crital section ends

        } else {
            printf("f: %s | %lld | %s | %s | ", path, (long long)new_node->size, atime, mtime);
			for (int i = 0 ; i < MD5_DIGEST_LENGTH ; i++)
					printf("%02x", new_node->hash[i]);
			printf("\n");
        }

        if (head == NULL) {
            head = new_node;
        } else {
            new_node->sibling = head;
            head = new_node;
        }
    }

    closedir(dp);

    return head;
}

struct entry *load_entries(struct entry *head, char *dir, int index)
{
    head = (struct entry *)malloc(sizeof(struct entry));
    if (head == NULL) {
        printf("Failed to allocate a new node\n");
        return NULL;
    }
    update_entry_info(head, dir, index);
	int wd = inotify_add_watch(fd[index], dir, IN_MODIFY | IN_CREATE | IN_DELETE); // Watch directory usinga add_watch
	// Insert new wd node for future use.
	//printf("Inserting wd %d : %s\n", wd, dir);
	struct wd_node* new_wd = malloc(sizeof(struct wd_node));
	strcpy(new_wd->path, dir);

	// Critical section starts
//	printf("here\n");
//	pthread_mutex_lock(&global_mutex);
//	printf("locked\n");
	new_wd->wd = wd;
	new_wd->next = wds[index];
	wds[index] = new_wd;
//	pthread_mutex_unlock(&global_mutex);
//	printf("unlocked\n");
	// Critical section ends.

    head->child = update_entries(head->child, dir, index);

    return head;
}

int release_entries(struct entry *head, int index)
{
    if (head == NULL) {
        return -1;
    }

    struct entry *peer = head;
    while (peer != NULL) {
        struct entry *temp = peer;
        peer = peer->sibling;

        if (S_ISDIR(temp->mode)) {
            release_entries(temp->child, index);
            printf("[INFO] Watcher thread %d released dir: %s\n", index, temp->name);
        } else {
            printf("[INFO] Watcher thread %d released file: %s\n", index, temp->name);
        }

        free(temp);
    }

    return 0;
}
