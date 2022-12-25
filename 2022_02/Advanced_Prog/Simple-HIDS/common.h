#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <sys/inotify.h>
#include <signal.h>
#include "thpool.h"

#define MAX_STRING 1024
#define NUM_OF_THREADS 8
#define MAX_WATCH_THREADS 10

struct entry {
    char name[MAX_STRING];       // name
    mode_t mode;                 // mode
    off_t size;                  // total size
    time_t atime;                // time of last access
    time_t mtime;                // time of last modification

    unsigned char hash[MD5_DIGEST_LENGTH];                  // md5 hash of file

    struct entry *sibling;
    struct entry *child;
};


struct wd_node {
	char path[MAX_STRING];       // path of this wd
	int wd;                      // wd value of this node
	struct wd_node* next;        // next node
};


struct hash_thpool_arg {
	char* path;
	unsigned char* hash;
};


struct thread_args {
	int index;
	char* target_dir;
};

struct entry *load_entries(struct entry *entries, char *dir, int);
int release_entries(struct entry *entries, int);
//unsigned char* md5(char* path);
void hash_func(void*);
int watch(int);

// hash function

// watch function
