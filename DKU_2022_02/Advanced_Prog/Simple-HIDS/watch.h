#include "common.h"


char* find_wd_path(int, int);
void handle_modify(const struct inotify_event*, int);
void handle_create(const struct inotify_event*, int);
void handle_delete(const struct inotify_event*, int);
struct entry* find_file(char* file, int);


