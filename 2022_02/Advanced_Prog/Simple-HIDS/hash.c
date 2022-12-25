#include "hash.h"

//int hash_func(char *path, unsigned char *hash)
void hash_func(void* args)
{
    // Get argument and typecast to our needs.
    struct hash_thpool_arg* th_args = (struct hash_thpool_arg*) args;
    char* path = th_args->path;
    unsigned char* hash = th_args->hash;

#ifdef DEBUG
    printf("[DEBUG] Thread %u working on %s\n", (int)pthread_self(), path);
#endif

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("Failed to open %s\n", path);
    }

    MD5_CTX ctx;
    MD5_Init(&ctx);

    int bytes;
    unsigned char data[1024];

    while ((bytes = fread(data, 1, 1024, fp)) != 0)
        MD5_Update(&ctx, data, bytes);

    MD5_Final(hash, &ctx);
    fclose (fp);
}
