#include "watch.h"
#include <error.h>
#include <errno.h>
#include <string.h>


extern int flag;  // For stopping watch loop.
extern struct entry* entries[MAX_WATCH_THREADS]; // For storing entries.
extern int fd[MAX_WATCH_THREADS];
extern struct wd_node* wds[MAX_WATCH_THREADS];
extern threadpool thpool[MAX_WATCH_THREADS]; // For threadpool


static void __handle_inotify_event(const struct inotify_event *event, int index) {
	if (event->len > 0){
    	if (event->mask & IN_MODIFY)
			handle_modify(event, index);
    	if (event->mask & IN_CREATE)
			handle_create(event, index);
   		if (event->mask & IN_DELETE)
			handle_delete(event, index);
	}
}



/**
 * @brief A function that retrieves path information from wd value.
 *        Since we registered each directories individually, we need to concat full path.
 *        Therfore, this function will find out which directory this watch was issued by wd.
 *        This works in a recursive way.
 * @param wd The wd value from inotify_event 
 * @return String that represents path
 */
char* find_wd_path(int wd, int index) {
	struct wd_node* tmp_wd = wds[index];
	while (tmp_wd) {
		if (tmp_wd->wd == wd)
			return tmp_wd->path;
		tmp_wd = tmp_wd->next;
	}
	return NULL;
}



/**
 * @brief A function that finds out the entry before target entry.
 * 		  This is needed since we need to unlink an entry when a file has been deleted.
 * 		  This is actually a spaghetti code as well.
 * 		  This works in a recursive way.
 * @param current The current entry that we are looking right at the moment..
 * @param path The target entry to look for.
 * @param result The pointer to entry* to store entry before into.
 */
void find_entry_before(struct entry* current, struct entry* target, struct entry** result) {
	if (current == NULL) return;
	if (S_ISDIR(current->mode)) { // If directory, dive deeper.
		if (current->child == target) { // If child was the one, set result and return.
			*result = current;
			return;
		}
		find_entry_before(current->child, target, result);
	} else { // If this was not directory, look for siblings.
		struct entry* tmp = current;
		while (tmp->sibling) {
			if (S_ISDIR(tmp->mode)) {
				if (tmp->child == target) { // If child was the one, set result and return.
					*result = tmp;
					return;
				}
				find_entry_before(tmp->child, target, result);
			} else {
				if (tmp->sibling == target) { // If next entry was target, record current one.
					*result = tmp;
					return;
				}
			}
			tmp = tmp->sibling;
		}
	}
}

/**
 * @brief A function that finds out which entry this provided file is.
 *        This function will look for all entries and find for the entry with same name.
 * @param ent The entry point to start looking for.
 * @param path The file to look for.
 * @param result The pointer to entry* to store data into.
 */
void find_entry(struct entry* ent, char* path, struct entry** result) {
	if (ent == NULL) return;

	if (S_ISDIR(ent->mode)) {
		if (!strcmp(ent->name, path)) {
			*result = ent;
			return;
		}
		find_entry(ent->child, path, result);
	} else {
		struct entry* tmp = ent;
		while (tmp) {
			if (!strcmp(tmp->name, path)) {
				*result = tmp;
				return;
			}
			if (S_ISDIR(tmp->mode)) 
				find_entry(tmp->child, path, result);
			tmp = tmp->sibling;
		}
	}
}




/**
 * @brief A function that handlees IN_MODIFY event.
 *        This will find out the file and look for its hash.
 *        Then this will compare hash value.
 *        If it was different, it will alert and update hash value.
 *
 * @param event The inotify_event to handle.
 */
void handle_modify(const struct inotify_event* event, int index) {
	// inotify_add_watch was by relative path.
	// Therefore, we need to retrive new absolute path.
	char full_path[MAX_STRING];
	strcpy(full_path, find_wd_path(event->wd, index));
	strcat(full_path, "/");
	strcat(full_path, event->name);
	struct entry* tmp = NULL;
	
	// Find the struct entry that represents the path file.
	find_entry(entries[index], full_path, &tmp);
	// Generate hash for the new file.
	unsigned char new_hash[MD5_DIGEST_LENGTH]; // Store new hash.
	struct hash_thpool_arg tmp_arg={.path = full_path, .hash = new_hash};
		
#ifdef DEBUG
	printf("[DEBUG] Watcher thread %d adding task to threadpool: %s\n", index, full_path);
#endif
	thpool_add_work(thpool[index], hash_func, (void*)(&tmp_arg));
    	thpool_wait(thpool[index]);

	int is_changed = 0;
	if (tmp == NULL) return; // This means the file was not tracked.
	// Find out if hash value was changed.
	for (int i = 0 ; i < MD5_DIGEST_LENGTH ; i++) {
		if (new_hash[i] != tmp->hash[i]) {
			is_changed = 1;
			break;
		}
	}

	if (is_changed) { // Hash was changed!
		printf("[ALERT] File %s was modified and hash changed.\n        ", full_path);
		for (int i = 0 ; i < MD5_DIGEST_LENGTH ; i++) 
			printf("%02x", tmp->hash[i]);
		printf(" -> ");
		for (int i = 0 ; i < MD5_DIGEST_LENGTH ; i++) 
			printf("%02x", new_hash[i]);
		printf("\n");

		// Store new hash to the entry.
		for (int i = 0 ; i < MD5_DIGEST_LENGTH ; i++) 
			tmp->hash[i] = new_hash[i];
	} else { // Hash did not change.
		printf("[ALERT] File %s was modified without hash change.\n", full_path);
	}
}


/**
 * @brief A function that handles IN_CREATE event.
 *        This function will:
 *        	- register new entry to the lsrc tree.
 *        	- register new directory
 *
 * @param event The inotify_event to handle.
 */
void handle_create(const struct inotify_event* event, int index) {
	// inotify_add_watch was by relative path.
	// Therefore, we need to retrive new absolute path.
	char full_path[MAX_STRING];
	char path[MAX_STRING]; // Store the directorh where this event happened.
	strcpy(path, find_wd_path(event->wd, index));
	strcpy(full_path, find_wd_path(event->wd, index));
	strcat(full_path, "/");
	strcat(full_path, event->name);

	printf("[ALERT] File %s was created.\n", full_path);

	// Get the entry that represents current directory.
	struct entry* tmp = NULL;
	find_entry(entries[index], path, &tmp);

	// Generate a new entry for the created file.
	struct entry* new_node = malloc(sizeof(struct entry));	
	struct stat info = {0};
	if (stat(full_path, &info) != 0) {
		printf("Failed to get the stat of %s\n", full_path);
		return;
	}

	new_node->mode = info.st_mode;
	new_node->size = info.st_size;
	new_node->atime = info.st_atime;
	new_node->mtime = info.st_mtime;
	strcpy(new_node->name, full_path);

	new_node->sibling = NULL;
	new_node->child = NULL;

	struct hash_thpool_arg tmp_arg={.path = full_path, .hash = new_node->hash};
		
#ifdef DEBUG
	printf("[DEBUG] Watcher thread %d adding task to threadpool: %s\n", index, full_path);
#endif
	thpool_add_work(thpool[index], hash_func, (void*)(&tmp_arg));
    thpool_wait(thpool[index]);
	if (S_ISDIR(new_node->mode)) { // If created one was a directory, set inotify_add_watch
		int wd = inotify_add_watch(fd[index], full_path, IN_MODIFY | IN_CREATE | IN_DELETE);
		struct wd_node* new_wd = malloc(sizeof(struct wd_node));
 		strcpy(new_wd->path, full_path);
        new_wd->wd = wd;
        new_wd->next = wds[index];
        wds[index] = new_wd;
	}

	// Insert new entry into the tree.
	if (!tmp->child) {// Meaning that there was no child, set this as child
		tmp->child = new_node;
		//printf("last element (directory) : %s\n", tmp->name);
	} else { // Meaning that in child directory there were some files, then find last sibling.
		tmp = tmp->child;
		// Iterate through the end of the siblings. 
		while (tmp->sibling) { 
			//printf("seacrhing %s\n", tmp->name);
			tmp = tmp->sibling;
		}
		// Append new one.
		//printf("last element %s\n", tmp->name);
		tmp->sibling = new_node;
	}

	struct entry* before = NULL;
	find_entry_before(entries[index], new_node, &before); 
	//printf("before : %s\n", before->name);
}


/**
 * @brief A function that handlees IN_DELETE event.
 * 		  This will remove entry from the entry tree by unlinking current deleted one.
 *
 * @param event The inotify_event to handle.
 */
void handle_delete(const struct inotify_event* event, int index) {
	// inotify_add_watch was by relative path.
	// Therefore, we need to retrive new absolute path.
	char full_path[MAX_STRING];
	strcpy(full_path, find_wd_path(event->wd, index));
	strcat(full_path, "/");
	strcat(full_path, event->name);
	struct entry* target = NULL;
	struct entry* before = NULL;

	// Find the struct entry that represents the path file.
	
	find_entry(entries[index], full_path, &target);
	find_entry_before(entries[index], target, &before);

	// If target and entry before target exists, delete current entry.
	// With linking, there shall be two cases.
	// - Deleted entry was a child of some other entry.
	// - Deleted entry was a sibling of some other entry.
	// Therefore, we need to separate both cases accordingly.
	printf("[ALERT] File %s was deleted.\n", full_path);
	if (target && before) {
		if (before->child == target)
			before->child = target->sibling;
		else 
			before->sibling = target->sibling;
	}
}


int watch(int index) {
    char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;

#ifdef DEBUG
	printf("[DEBUG] Starting loop for %d\n", index);
#endif
    while (flag) {
        ssize_t size = read(fd[index], buf, sizeof(buf));
        if (size == -1 && errno != EAGAIN) {
            printf("Failed to read an event\n");
            return -1;
        } else if (size <= 0) {
            break;
        }

        char *ptr;
        for (ptr = buf; ptr < buf + size; ptr += sizeof(struct inotify_event) + event->len) {
            event = (struct inotify_event *)ptr;
            __handle_inotify_event(event, index);
        }
    }
    return 0;
}
