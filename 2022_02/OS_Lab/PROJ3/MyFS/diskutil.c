//
// @file : diskutil.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements disk utility functions.
//


#include "diskutil.h"

extern char disks[MAX_STRING_LEN][MAX_IMG_COUNT];
extern struct partition partitions[MAX_IMG_COUNT];
extern int disk_count;
extern struct entry_t* entries[MAX_IMG_COUNT];
extern uint16_t loaded_partitions;
extern unsigned char block_stats[MAX_BLOCK_COUNT][MAX_IMG_COUNT];
extern unsigned char inode_stats[MAX_INODE_COUNT][MAX_IMG_COUNT];


/**
 * A function that scans disks in current working directory.
 * The disks must have .img as its file extension. Otherwise, it will not be recognized as disk.
 * This function will set all disk list into the disks variable.
 * @return -1 if failure, 0 if successful.
 */
int scan_disks() {
    DIR* d;
    struct dirent *dir;
    disk_count = 0;

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, ".img")) { // If the file ends in .img, consider it a disk.
                strcpy(disks[disk_count], dir->d_name);
#ifdef DEBUG
                printf("[DEBUG] Found disk %d: %s\n", disk_count, disks[disk_count]);
#endif
                disk_count++;
            }
        }
        closedir(d);
        return disk_count == 0 ? -1 : 0; // If there were no disk available, return -1;
    } else {
        printf("[ERROR] Could not get information of current directory.\n");
        return -1;
    }
}


/**
 * A function that reads disk's super block.
 * @param disk_index The disk index to look super block from.
 * @return -1 if not successful, 0 if successful.
 */
int load_super_block(int disk_index) {
    struct partition* cur_p = &partitions[disk_index];
    FILE* fp = fopen(disks[disk_index], "rb");
#ifdef DEBUG
    printf("[INFO] Loading super block from disk %d: %s...\n", disk_index, disks[disk_index]);
#endif
    if (fp) {
        // Store temporary super block to dump it later.
        struct super_block tmp_sp = cur_p->s;

        unsigned int values[10];
        unsigned char buffer[4];
        unsigned int tmp_val;

        // Read header 4 bytes to check if it is valid.
        fseek(fp, 0, SEEK_SET);
        (void)! fread(buffer, sizeof(char) * 4, 1, fp); // Using (void)! for ignoring -Wunused-result.
        memcpy(&tmp_val, buffer, sizeof(char) * 4);
        if (tmp_val != SIMPLE_PARTITION) {
            printf("[ERROR] Disk %d (%s) is invalid partition.\n", disk_index, disks[disk_index]);
            fclose(fp);
            return -1;
        } else {
            values[0] = tmp_val;
        }

        // Instead of loading each values, let's just read all and dump it with memcpy.
        for (int i = 1 ; i < 10 ; i++) {
            fseek(fp, i * 4, SEEK_SET);
            (void)! fread(buffer, sizeof(char) * 4, 1, fp);
            memcpy(&tmp_val, buffer, sizeof(char) * 4);
            values[i] = tmp_val;
        }

        // Dump values into the struct
        memcpy(&tmp_sp, values, sizeof(unsigned int) * 10);
        (void)! fread(tmp_sp.volume_name, sizeof(char) * 24, 1, fp);
        (void)! fread(tmp_sp.padding, sizeof(char) * 960, 1, fp);

#ifdef DEBUG
        // Print partition information.
        printf("[DEBUG] Partition Type: %08x\n", tmp_sp.partition_type);
        printf("[DEBUG] Block Size: %08x\n", tmp_sp.block_size);
        printf("[DEBUG] Inode Size: %08x\n", tmp_sp.inode_size);
        printf("[DEBUG] First Inode: %08x\n", tmp_sp.first_inode);

        printf("[DEBUG] Number of Inodes: %08x\n", tmp_sp.num_inodes);
        printf("[DEBUG] Number of Inode blocks: %08x\n", tmp_sp.num_inode_blocks);
        printf("[DEBUG] Number of Free Inodes: %08x\n", tmp_sp.num_free_inodes);

        printf("[DEBUG] Number of Blocks: %08x\n", tmp_sp.num_blocks);
        printf("[DEBUG] Number of Free Blocks: %08x\n", tmp_sp.num_free_blocks);
        printf("[DEBUG] First Data Block: %08x\n", tmp_sp.first_data_block);
        printf("       └ Volume Name: %s\n", tmp_sp.volume_name);
#endif
        cur_p->disk_index = disk_index;
        fclose(fp);
        cur_p->s = tmp_sp;
        return 0;
    } else {
        return -1;
    }
}


/**
 * A function that loads inode table from the disk partition.
 * @param disk_index The disk index to look inode table from.
 * @return -1 if not successful, 0 if successful.
 */
int load_inode_table(int disk_index) {
    struct partition* cur_p = &partitions[disk_index];
    FILE *fp = fopen(disks[disk_index], "rb");
#ifdef DEBUG
    printf("[INFO] Loading inode table from disk %d: %s...\n", disk_index, disks[disk_index]);
#endif

    unsigned char buffer[sizeof(struct inode) * 224]; // Generate a buffer for inode table.
    if (fp) {
        fseek(fp, 0x400, SEEK_SET); // This is where the inode table starts
        (void)! fread(buffer, sizeof(struct inode) * 224, 1, fp);
        memcpy(&(cur_p->inode_table), buffer, sizeof(struct inode) * 224); // Dump values into inode table.
        fclose(fp);

#ifdef DEBUG
        // Print partition information.
        for (int i = 0 ; i < 224 ; i++) {
            printf("[DEBUG] Inode table %d: Mode %x, Locked %x, Date %x, Size %x, Indirect Block %d\n",
                   i, cur_p->inode_table[i].mode, cur_p->inode_table[i].locked, cur_p->inode_table[i].date,
                   cur_p->inode_table[i].size, cur_p->inode_table[i].indirect_inode);
        }
        printf("[INFO] Loaded inode table from disk %d: %s...\n", disk_index, disks[disk_index]);
#endif
        return 0;
    } else {
        return -1;
    }
}


/**
 * A function that loads data blocks from the disk partition.
 * @param disk_index The disk index to look data blocks from.
 * @return -1 if not successful, 0 if successful.
 */
int load_data_blocks(int disk_index) {
    struct partition* cur_p = &partitions[disk_index];
    FILE *fp = fopen(disks[disk_index], "rb");
#ifdef DEBUG
    printf("[INFO] Loading data blocks from disk %d: %s...\n", disk_index, disks[disk_index]);
#endif
    unsigned char buffer[sizeof(struct blocks) * 4088]; // Generate a buffer for inode table.
    if (fp) {
        fseek(fp, 0x2000, SEEK_SET); // This is where the inode table starts
        (void)! fread(buffer, sizeof(struct blocks) * 4088, 1, fp);
        memcpy(&(cur_p->data_blocks), buffer, sizeof(struct blocks) * 4088); // Dump values into inode table.
        fclose(fp);
#ifdef DEBUG
        printf("[INFO] Loaded data blocks from disk %d: %s...\n", disk_index, disks[disk_index]);
#endif
        return 0;
    } else {
        return -1;
    }
}


/**
 * A function that loads root directory
 * @param disk_index The disk index to load root directory from.
 * @return -1 if failure, 0 if success.
 */
int load_root(int disk_index) {
    struct partition* cur_p = &partitions[disk_index];
    entries[disk_index] = load_entries(*cur_p);
    if (entries[disk_index] == NULL) {
        return -1;
    } else {
        printf("[INFO] Successfully mounted disk %d: %s\n", disk_index, disks[disk_index]);
        loaded_partitions = loaded_partitions | (0x1 << disk_index); // Set current partition as loaded.
        return 0;
    }
}


/**
 * A function that generates struct dentry from inode.
 * @param disk_index The disk index.
 * @param inode_index The inode index.
 * @param ret The struct dentry* to return data into.
 * @return -1 if unsuccessful, 0 if successful.
 */
__attribute__((unused)) int generate_dentry(int disk_index, int inode_index, struct dentry* ret) {
    ret->inode = inode_index;
    struct partition cur_p = partitions[disk_index];
    // Parse file mode and set file type into dentry.
    switch (cur_p.inode_table[inode_index].mode & 0x10000) {
        case 0x10000: // Regular file
            ret->file_type = DENTRY_TYPE_REG_FILE;
            break;
        case 0x20000: // Directory file
            ret->file_type = DENTRY_TYPE_DIR_FILE;
            break;
        default:
            ret->file_type = 0x00;
            break;
    }
    return 0;
}


/**
 * A function that generates prefix string.
 * This will generate ls -al like output prefix and permission.
 * If the permission was 777, it will return rwxrwxrwx
 * If the file was a directory, it will put 'd' in front of the permission string.
 * @param mode The mode to convert into string.
 * @param ret The string to store converted data into.
 * @return 0.
 */
int generate_prefix_str(unsigned int mode, char* ret) {
    char tmp[11] = {'-'};
    // Check directory
    if ((mode & 0x20000) == INODE_MODE_DIR_FILE)
        tmp[0] = 'd';

    // Generate permission string according to the mode.
    tmp[1] = (mode & INODE_MODE_AC_USER_R) == INODE_MODE_AC_USER_R ? 'r' : '-';
    tmp[2] = (mode & INODE_MODE_AC_USER_W) == INODE_MODE_AC_USER_W ? 'w' : '-';
    tmp[3] = (mode & INODE_MODE_AC_USER_X) == INODE_MODE_AC_USER_X ? 'x' : '-';
    tmp[4] = (mode & INODE_MODE_AC_GRP_R) == INODE_MODE_AC_GRP_R ? 'r' : '-';
    tmp[5] = (mode & INODE_MODE_AC_GRP_W) == INODE_MODE_AC_GRP_W ? 'w' : '-';
    tmp[6] = (mode & INODE_MODE_AC_GRP_X) == INODE_MODE_AC_GRP_X ? 'x' : '-';
    tmp[7] = (mode & INODE_MODE_AC_OTHER_R) == INODE_MODE_AC_OTHER_R ? 'r' : '-';
    tmp[8] = (mode & INODE_MODE_AC_OTHER_W) == INODE_MODE_AC_OTHER_W ? 'w' : '-';
    tmp[9] = (mode & INODE_MODE_AC_OTHER_X) == INODE_MODE_AC_OTHER_X ? 'x' : '-';

    tmp[10] = 0;
    strcpy(ret, tmp);
    return 0;
}


/**
 * A function that prints out all entries in current directory.
 * This will print all siblings with current directory entry.
 * @param dir The current directory's first entry to start listing.
 * @param level The level of details to print out.
 *              level 3 will print out ls -al + inode and disk information
 *              level 2 will print out ls -al
 *              level 1 (default) will print out ls
 * @return 0.
 */
int impl_ls(struct entry_t* dir, unsigned char level) {
#ifdef DEBUG
    printf("[DEBUG] Current directory : %s\n", dir->name);
#endif
    struct entry_t* tmp = dir->child;
    unsigned int entry_count = 0;
    while(tmp != NULL) {
        // Skip .. and .
        if (((strcmp("..", tmp->name) != 0) && ((strcmp(".", tmp->name))) != 0) && (strlen(tmp->name) >= 1)) {
            char perm_info[11];
            struct inode in = partitions[tmp->disk_index].inode_table[tmp->inode_index];
            generate_prefix_str(in.mode, perm_info);
            if (level == 3 && (strlen(tmp->name) >= 1))
                printf("%s %10d %s(Inode %d @ disk %d)\n", perm_info, in.size, tmp->name,
                       tmp->inode_index, tmp->disk_index);
            else if (level == 2 && (strlen(tmp->name) >= 1))
                printf("%s %10d %s\n", perm_info, in.size, tmp->name);
            else {
                printf("%s  ", tmp->name);
                if ((entry_count % LS_SPLIT_COUNT) == 0 && (tmp->sibling != NULL) && entry_count != 0){
                    printf("\n");
                }
                entry_count++;
            }
        }
        tmp = tmp->sibling;
    }
    printf("\n");
    return 0;
}


/**
 * A function that mimics cat in bash.
 * @param cur_dir The current directory's entry.
 * @param target The target file's name.
 * @return -1 if unsuccessful, 0 if successful.
 */
int impl_cat(struct entry_t* cur_dir, char* target) {
    struct entry_t* res;
    if (find_entry(cur_dir, &res, target) == 0) {
        unsigned char* buf = NULL;
        if (read_file_data(res, &buf) == -1) {
            printf("cat: Could not read file\n");
            return -1;
        } else {
            struct inode in = partitions[cur_dir->disk_index].inode_table[res->inode_index];
            if ((in.mode & INODE_MODE_REG_FILE) == INODE_MODE_REG_FILE) { // rmdir cannot delete file.
                printf("%s", buf);
                free(buf);
            } else { // Disable 'cat'ing a directory.
                printf("cat: %s: is a directory\n", target);
            }
        }
    } else {
        printf("cat: %s: No such file or directory\n", target);
        return -1;
    }
    return 0;
}


/**
 * A function that performs chmod action to a specific file.
 * @param cur_dir The current directory's entry.
 * @param target The file name to change permission.
 * @param permission The permission to set the file as.
 * @return -1 if failure, 0 if success.
 */
int impl_chmod(struct entry_t* cur_dir, char* target, unsigned int permission) {
    struct entry_t* res = malloc(sizeof(struct entry_t));
    if (find_entry(cur_dir, &res, target) == 0) {
        handle_cow(res->disk_index, res); // Handle CoW.
        struct inode new_inode; // Generate temp inode for new permission.
        struct inode in = partitions[res->disk_index].inode_table[res->inode_index];
        unsigned int mode = 0xFF000 & in.mode; // Copy MSB 2 digits (file or dir)
        mode = mode | (permission & 0xFFF); // Set permission 3 digits.
        memcpy(&new_inode, &in, sizeof(struct inode));
        new_inode.mode = mode; // Store new permission.
        update_inode(res->disk_index, res->inode_index, &new_inode);
    } else {
        printf("chmod: cannot access '%s': No such file or directory\n", target);
        return -1;
    }
    return 0;
}


/**
 * A function that prints out stat of a specific file.
 * @param cur_dir The current directory's entry.
 * @param target The file name to get stats from.
 * @return -1 if failure, 0 if success.
 */
int impl_stat(struct entry_t* cur_dir, char* target) {
    struct entry_t* res = malloc(sizeof(struct entry_t));
    if (find_entry(cur_dir, &res, target) == 0) {
        char perm_info[11];
        struct inode in = partitions[res->disk_index].inode_table[res->inode_index];
        generate_prefix_str(in.mode, perm_info);
        unsigned char is_empty = (in.mode & INODE_MODE_EMPTY_FILE) == INODE_MODE_EMPTY_FILE;
        printf("   File: %s    Empty file: %d\n", res->name, is_empty);
        printf("   Size: %d    ", in.size);
        printf("IO Block: %d    \n", in.size / 1024 + 1);
        printf("   Disk: %s (%d)    ", disks[res->disk_index], res->disk_index);
        printf("Inode: %d    \n", res->inode_index);
        printf("   Access: (%04x/%s)\n", (in.mode & 0x0FFF), perm_info);
        if (in.indirect_inode != -1) {
            printf("   Indirection to: %d\n", in.indirect_inode);
        }

        // Print indirections into this inode.
        unsigned int indirections[MAX_INODE_COUNT] = {0};
        int indirection_count = find_indirection_inodes(cur_dir->disk_index, res->inode_index, indirections);
        if (indirection_count != 0) {
            printf("   Indirection(s) from: ");
            for (int i = 0; i < indirection_count; i++)
                printf("%d, ", indirections[i]);
            printf("\n");
        }

        return 0;
    } else {
        printf("cat: cannot stat '%s': No such file or directory\n", target);
        free(res);
        return -1;
    }
}


/**
 * A function that generates an empty file.
 * @param cur_dir The current directory.
 * @param target The target file name to generate
 * @return -1 if failure, 0 if successful.
 */
int impl_touch(struct entry_t* cur_dir, char* target) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, target) == -1)  // If file does not exist, create file.
        return create_file(cur_dir->disk_index, cur_dir, target, 0);
    else
        return 0;
}


/**
 * A function that generates an empty directory.
 * @param cur_dir The current directory.
 * @param target The target directory name to generate
 * @return -1 if failure, 0 if successful.
 */
int impl_mkdir(struct entry_t* cur_dir, char* target) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, target) == -1) { // If directory does not exist, create directory.
        return create_file(cur_dir->disk_index, cur_dir, target, 1);
    } else {
        printf("mkdir: cannot create directory ‘%s’: File exists\n", target);
        return -1;
    }
}


/**
 * A function that removes a single file.
 * This function is not for removing a directory file.
 * Use rmdir for removing directory file.
 * @param cur_dir The current directory.
 * @param target The target file to remove.
 * @return -1 if failure, 0 if successful.
 */
int impl_rm(struct entry_t* cur_dir, char* target) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, target) == -1) { // If directory does not exist, we can't delete it.
        printf("rm: cannot remove ‘%s’: No such file or directory\n", target);
        return -1;
    } else {
        struct inode in = partitions[cur_dir->disk_index].inode_table[res->inode_index];
        if ((in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE) { // rm cannot delete directory.
            printf("rm: cannot remove '%s': Is a directory\n", target);
            return -1;
        } else {
            handle_cow(res->disk_index, res); // Handle CoW.
            return delete_file(cur_dir->disk_index, cur_dir, target);
        }
    }
}


/**
 * A function that prints out the volume status.
 * @param target_volume The index of volume to get stats from.
 * @return 0. This function will not fail.
 */
int impl_vstat(unsigned int target_volume) {
    struct partition cur_p = partitions[target_volume];
    printf("   Volume Name: %s\n", cur_p.s.volume_name);
    printf("   Used Inodes: %d   Free Inodes: %d\n",
           cur_p.s.num_inodes - cur_p.s.num_free_inodes, cur_p.s.num_free_inodes);
    printf("   Used Blocks: %d   Free Blocks: %d\n",
           cur_p.s.num_blocks - cur_p.s.num_free_blocks, cur_p.s.num_free_blocks);

    return 0;
}


/**
 * A function that removes a directory recursively.
 * This function is not for removing a single file.
 * Use rm for removing regular file.
 * @param cur_dir The current directory.
 * @param target The target directory to remove.
 * @return -1 if failure, 0 if successful.
 */
int impl_rmdir(struct entry_t* cur_dir, char* target) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, target) == -1) { // If directory does not exist, we can't delete directory.
        printf("rmdir: cannot remove ‘%s’: No such file or directory\n", target);
        return -1;
    } else {
        struct inode in = partitions[cur_dir->disk_index].inode_table[res->inode_index];
        if ((in.mode & INODE_MODE_REG_FILE) == INODE_MODE_REG_FILE) { // rmdir cannot delete file.
            printf("rmdir: failed to remove '%s': Not a directory\n", target);
            return -1;
        } else
            return delete_directory(cur_dir->disk_index, cur_dir, target);
    }
}


/**
 * A function that writes data into a file.
 * This will open file as W mode and overwrite everything.
 * This function works like "echo "hello world" > test.txt"
 * @param cur_dir The current directory.
 * @param target The target file to write.
 * @param context The data to write into a file. This shall have no " (quotation marks) surrounding the context.
 * @return -1 if failure, 0 if successful.
 */
int impl_write(struct entry_t* cur_dir, char* target, char* context) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, target) == -1) { // If the file does not exist, we can't write it.
        printf("write: cannot write to ‘%s’: No such file\n", target);
        return -1;
    } else {
        struct inode in = partitions[cur_dir->disk_index].inode_table[res->inode_index];
        if ((in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE) { // Write can't write directory files.
            printf("write: failed to write '%s': Is a directory\n", target);
            return -1;
        } else {
            handle_cow(res->disk_index, res); // Handle CoW.
            return write_file_data(res, strlen(context), (unsigned char *) context);
        }
    }
}


/**
 * A function that appends data into a file.
 * This will open file as A mode and append data.
 * This function works like "echo "hello world" >> test.txt"
 * @param cur_dir The current directory.
 * @param target The target file to append.
 * @param context The data to write into a file. This shall have no " (quotation marks) surrounding the context.
 * @return -1 if failure, 0 if successful.
 */
int impl_append(struct entry_t* cur_dir, char* target, char* context) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, target) == -1) { // If the file does not exist, we can't write it.
        printf("append: cannot append to ‘%s’: No such file\n", target);
        return -1;
    } else {
        struct inode in = partitions[cur_dir->disk_index].inode_table[res->inode_index];
        if ((in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE) { // Write can't write directory files.
            printf("append: failed to append '%s': Is a directory\n", target);
            return -1;
        } else {
            handle_cow(res->disk_index, res); // Handle CoW.
            return append_file_data(res, strlen(context), (unsigned char *) context);
        }
    }
}


/**
 * A function that performs file copy.
 * This will work as a CoW way.
 * When this function gets an existing file as dst, this will overwrite to the destination file.
 * @param cur_dir The current directory.
 * @param src The source to copy.
 * @param dst The destination to copy as.
 * @return -1 if failure, 0 if successful.
 */
int impl_cp(struct entry_t* cur_dir, char* src, char* dst) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, src) == -1) { // If the file does not exist, we can't copy it.
        printf("cp: cannot stat '%s': No such file or directory\n", src);
        return -1;
    } else {
        struct inode in = partitions[cur_dir->disk_index].inode_table[res->inode_index];
        if ((in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE) { // Write can't copy directory files.
            printf("cp: copying directory is not permitted; omitting directory '%s'\n", src);
            return -1;
        } else {
            struct entry_t *ignored;
            // Check if name with destination exists, if so, delete it.
            if (find_entry(cur_dir->child, &ignored, dst) != -1) {
                if (delete_file(cur_dir->disk_index, cur_dir, dst) == -1) {
                    printf("[ERROR] Could not delete %s\n", dst);
                    return -1;
                }
            }
            return copy_file(cur_dir->disk_index, res, dst); // Copy file.
        }
    }
}


/**
 * A function that performs file rename.
 * When this function gets an existing file as dst, this will overwrite to the destination file.
 * For moving files into another directory, use impl_move instead.
 * @param cur_dir The current directory.
 * @param src The source to copy.
 * @param dst The destination to copy as.
 * @return -1 if failure, 0 if successful.
 */
int impl_rename(struct entry_t* cur_dir, char* src, char* dst) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, src) == -1) { // If the file does not exist, we can't move it.
        printf("rename: cannot stat '%s': No such file or directory\n", src);
        return -1;
    } else {
        struct entry_t *dst_entry;
        // Check if name with destination exists, if so, delete it.
        if (find_entry(cur_dir->child, &dst_entry, dst) != -1) {
            handle_cow(dst_entry->disk_index, dst_entry); // Handle CoW.
            if (delete_file(cur_dir->disk_index, cur_dir, dst) == -1) {
                printf("[ERROR] Could not delete %s\n", dst);
                return -1;
            }
        }
        return rename_file(cur_dir->disk_index, res, dst); // Rename file.
    }
}


/**
 * A function that performs file move.
 * For renaming files, use impl_rename instead.
 * @param cur_dir The current directory.
 * @param src The source to copy.
 * @param dst The destination to copy as.
 * @return -1 if failure, 0 if successful.
 */
int impl_move(struct entry_t* cur_dir, char* src, char* dst) {
    struct entry_t* res;
    if (find_entry(cur_dir->child, &res, src) == -1) { // If the file does not exist, we can't move it.
        printf("mv: cannot stat '%s': No such file or directory\n", src);
        return -1;
    } else {
        struct entry_t *dst_entry;
        // Check if name with destination exists, if so, also check if it is directory file.
        if (find_entry(cur_dir->child, &dst_entry, dst) != -1 || (strcmp(dst, "..") == 0)) {
            if ((strcmp(dst, "..") == 0)) return move_file(cur_dir->disk_index, res, dst); // For ..
            struct inode in = partitions[dst_entry->disk_index].inode_table[dst_entry->inode_index]; // For normal case.
            if ((in.mode & INODE_MODE_DIR_FILE) != INODE_MODE_DIR_FILE) { // This was not a directory file, quit.
                printf("mv: cannot move '%s': '%s' is not a directory\n", src, dst);
                return -1;
            } else {
                return move_file(cur_dir->disk_index, res, dst); // move file.
            }
        } else { // If the target directory does not exist, we can't move the file.
            printf("mv: cannot move '%s': '%s': No such file or directory\n", src, dst);
            return -1;
        }
    }
}


/**
 * A function that writes data into a specific file.
 * This function provides following features:
 * 1. Write data to a specific file with provided buffer.
 * 2. If the written data's size was longer than the original blocks, this will automatically assign free blocks.
 *    Ex) If the original file had 1 block of data in it, however when the new written data was 3 blocks of data,
 *        This function will automatically get 2 free blocks.
 * 3. Update the inode of this file, this will update the file size and list of blocks.
 * @param target The target to write.
 * @param buffer_size The buffer size. Be aware that this buffer size should NOT pass more than 6 blocks.
 * @param buffer The buffer to write data.
 * @return -1 if failure, 0 if successful.
 */
int write_file_data(struct entry_t* target, unsigned int buffer_size, unsigned char* buffer) {
    if(buffer_size > sizeof(struct blocks) * 6) { // The max that we can write is 6 blocks.
        printf("[ERROR] Write size is too big: %d\n", buffer_size);
        return -1;
    }

    struct inode *cur_in = &partitions[target->disk_index].inode_table[target->inode_index];
    unsigned int disk_index = target->disk_index;

    if (cur_in->locked) { // If the file was locked, we can't write it.
        printf("[ERROR] Could not write file %s: The file is locked\n", target->name);
        return -1;
    }

    // If the size of data we are writing right now exceeds current assigned blocks,
    // get more blocks and assign them. This method will not prevent fragmentation!!
    if (buffer_size > ((cur_in->size / 1024) + 1) * 1024) {
        // If the assigned blocks were small then the new one, allocate more blocks.
        unsigned int assigned_block[6] = {0};
        if (assign_empty_blocks(disk_index, 1, assigned_block) == -1) {
           printf("[ERROR] Could not allocate more disk blocks\n");
           return -1;
        }

        for (int i = 0 ; i < 6 ; i++) { // Store assigned block.
            if (cur_in->blocks[i] == 0) {
                cur_in->blocks[i] = assigned_block[0];
                break;
            }
        }
    }

    // Declare that this file is locked. (like fopen)
    cur_in->locked = 1;

    // For physical buffer.
    unsigned char* physical_buffer = (unsigned char*) malloc(sizeof(struct blocks) * 6);
    memset(physical_buffer, 0, sizeof(struct blocks) * 6);
    memcpy(physical_buffer, buffer, buffer_size); // Store data that was requested.

    // Iterate and physically emit change to disk.
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = cur_in->blocks[i];
        if (block == 0) continue; // If 0, it was just unassigned block. Therefore, just pass.
        write_data_block(disk_index, block, 1, &((struct blocks*) physical_buffer)[i]);
    }

    // Iterate and save change in the data table on the memory.
    struct partition *cur_p = &partitions[disk_index];
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = cur_in->blocks[i];
        if (block == 0) continue; // If 0, it was just unassigned block. Therefore, just pass.
        memcpy(cur_p->data_blocks + cur_in->blocks[i], &((struct blocks*) physical_buffer)[i], sizeof(struct blocks));
    }

    // Update inode information and also emit data to disk.
    unsigned char is_empty = (cur_in->mode & INODE_MODE_EMPTY_FILE) == INODE_MODE_EMPTY_FILE;
    cur_in->size = buffer_size;
    cur_in->mode = is_empty ? cur_in->mode ^ INODE_MODE_EMPTY_FILE : cur_in->mode; // Set this as non empty file.

    cur_in->locked = 0; // Unlock file since this is not being used. This works like fclose.
    write_inode(disk_index, target->inode_index, cur_in);
    free(physical_buffer);

    return 0;
}


/**
 * A function that appends data into a file.
 * The function will work like Append mode in fopen.
 * This will internally call write_file_data.
 * @param target The target to write.
 * @param buffer_size The buffer size. Be aware that this buffer size should NOT pass more than 6 blocks.
 * @param buffer The buffer to write data.
 * @return -1 if failure, 0 if successful.
 */
int append_file_data(struct entry_t* target, unsigned int buffer_size, unsigned char* buffer) {
    unsigned char* physical_buffer = (unsigned char*) malloc(sizeof(struct blocks) * 6);
    struct inode *cur_in = &partitions[target->disk_index].inode_table[target->inode_index];
    struct partition *cur_p = &partitions[target->disk_index];
    unsigned char is_empty_file = (cur_in->mode & INODE_MODE_EMPTY_FILE) == INODE_MODE_EMPTY_FILE;

    // Calculate new generated buffer size.
    // If the file was empty and was trying to append data into the file, set size as buffer size since the file is empty.
    unsigned int new_size = is_empty_file ? buffer_size : buffer_size + cur_in->size; //

    // Copy previous blocks in the memory to the physical buffer.
    // Also look for the last block that this file is stored at.
    memset(physical_buffer, 0, sizeof(struct blocks) * 6);
    unsigned int last_block_index = 0;
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = cur_in->blocks[i];
        if (block == 0) continue;
        memcpy(&((struct blocks*) physical_buffer)[i], cur_p->data_blocks + cur_in->blocks[i], sizeof(struct blocks));
        last_block_index = i;
    }

    // Copy the new appended data into the last block that was storing the data.
    unsigned int offset = is_empty_file ? 0 : cur_in->size % 1024; // Get last block's offset.
    memcpy(physical_buffer + last_block_index * sizeof(struct blocks) + offset, buffer, buffer_size);

    // Call write file data internally and emit data to disk.
    if (write_file_data(target, new_size, physical_buffer) == -1) {
        printf("[ERROR] Could not append data to file\n");
        free(physical_buffer);
        return -1;
    }

    // Copy data into the data block table in memory.
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = cur_in->blocks[i];
        if (block == 0) continue;
        memcpy(cur_p->data_blocks + cur_in->blocks[i], &((struct blocks*) physical_buffer)[i], sizeof(struct blocks));
    }

    // Store inode data back to the disk and to the memory.
    cur_in->size = new_size;
    write_inode(target->disk_index, target->inode_index, cur_in);

    free(physical_buffer);
    return 0;
}


/**
 * A function that reads data from file.
 * This function will physically read data from file and save it to the char buffer.
 * @param entry The entry to read data from.
 * @param ret The char* address to store return value into.
 * @return -1 if unsuccessful, 0 if successful.
 */
int read_file_data(struct entry_t* entry, unsigned char** ret) {
    struct partition cur_p = partitions[entry->disk_index];
    struct inode in = partitions[entry->disk_index].inode_table[entry->inode_index];

    unsigned char* buffer = (unsigned char*) malloc(sizeof (struct blocks) * 6);
    memset(buffer, 0, sizeof(struct blocks) * 6);
    // Generate buffer for loading data from disk.
    if (!buffer) return -1;

    // Read block one by one.
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = in.blocks[i];
        if (block == 0) continue;
        memcpy(&buffer[i], &cur_p.data_blocks[block], sizeof(struct blocks));
    }

    *ret = buffer;
    return 0;
}


/**
 * A function that updates logical inode with specific inode.
 * This will automatically update physical disk's data as well.
 * @param disk_index The disk index to write inode into.
 * @param inode_index The inode index to update.
 * @param data The inode data to write.
 * @return -1 if failure, 0 if successful.
 */
int update_inode(unsigned int disk_index, unsigned int inode_index, struct inode* data) {
    struct partition* cur_p = &partitions[disk_index];
    memcpy(&cur_p->inode_table[inode_index], data, sizeof(struct inode));
    return write_inode(disk_index, inode_index, data);
}


/**
 * A function that writes inode value to disk.
 * This physically writes data into the inode into the disk.
 * @param disk_index The disk index to write inode into.
 * @param inode_index The index of inode to write inode into.
 * @param data The inode data to write.
 * @return -1 if failure, 0 if successful.
 */
int write_inode(unsigned int disk_index, unsigned int inode_index, struct inode* data) {
    struct partition *cur_p = &partitions[disk_index];
    unsigned char* buffer = (unsigned char*) data; // Typecast into buffer.
    unsigned int inode_size = cur_p->s.inode_size;
    unsigned int real_offset = inode_size * inode_index + 0x400; // This is the offset of inode.

    // Open file for specific offset write, special thanks to https://stackoverflow.com/a/2623210/5716511
    FILE* fp = fopen(disks[disk_index], "rb+");
    fseek(fp, real_offset, SEEK_SET); // Set offset
    fwrite(buffer, sizeof(struct inode), 1, fp); // Write binary data.
    fclose(fp); // Close file.

    return 0;
}


/**
 * A function that creates a single empty file.
 * This will perform following actions sequentially.
 * 1. Get allocated 1 free block.
 * 2. Get allocated 1 free inode.
 * 3. Store inode to temporary inode value. (Size will be set as 1 and default permission is 777)
 * 4. Set first bit of assigned block as 1 for marking it being used.
 * 5. Generate set element_t value accordingly for future use in LCRS then connect to the main tree accordingly.
 * 6. Physically emit block and inode into disk.
 * 7. Add directory file so that the directory knows that there is a new file (which will be this)
 * @param disk_index The disk index to create new file into.
 * @param cur_dir The current directory.
 * @param file_name The file name.
 * @param is_dir Whether if this is a directory element or not.
 * @return -1 if failure, 0 if successful.
 */
int create_file(unsigned int disk_index, struct entry_t* cur_dir, char* file_name, unsigned char is_dir) {
    // Create new inode and entry structs.
    struct inode *new_in = malloc(sizeof(struct inode));
    struct entry_t *new_ent = malloc(sizeof(struct entry_t));
    memset(new_in, 0, sizeof(struct inode));
    memset(new_ent, 0, sizeof(struct entry_t));

    if ((void*) new_in == (void*) new_ent) return -1; // Both malloc failed
    else {
        unsigned int assigned_blocks[6] = {0xFFFF}; // Initialize with 0xFFFF since this index is impossible to be assigned.
        unsigned int assigned_inode_index = 0xFFFF; // Initialize with 0xFFFF since this index is impossible to be assigned.

        // Get allocated 1 free block and 1 free inode for creating file.
        int ret = assign_empty_blocks(disk_index, 1, assigned_blocks); // Assign empty assigned_blocks.
        if (ret == -1) { // Assigning empty assigned_blocks failed.
            printf("[ERROR] Could not get free assigned_blocks assigned when creating file\n");
            return -1;
        }
        ret = assign_empty_inodes(disk_index, &assigned_inode_index); // Assign empty inode.
        if (ret == -1) { // Assigning empty inode failed.
            printf("[ERROR] Could not get free inodes assigned when creating file\n");
            return -1;
        }
#ifdef DEBUG
        printf("[DEBUG] create_file: got assigned assigned_blocks: %d %d %d %d %d %d\n", assigned_blocks[0], assigned_blocks[1], assigned_blocks[2],
               assigned_blocks[3], assigned_blocks[4], assigned_blocks[5]);
        printf("[DEBUG] create_file: got assigned inode: %d\n", assigned_inode_index);
#endif
        // Generate mode including permission.
        unsigned int mode = 0x00000;
        if (is_dir) mode = mode | INODE_MODE_DIR_FILE;
        else mode = mode | INODE_MODE_REG_FILE;
        mode = mode | 0x00777; // Set default permission as 777.

        // Store initial inode data.
        new_in->date = 0;
        new_in->indirect_inode = -1;
        new_in->locked = 0;
        new_in->blocks[0] = assigned_blocks[0]; // Store block info.

        // Store initial block data to actually emit to the disk.
        // When creating a directory, default size is 0x40 since it includes . and ..
        // When creating a file, default size is set 0x3 since it just has \n + padding.
        struct blocks tmp_block[1];
        memset(tmp_block, 0, sizeof(struct blocks));
        if (is_dir) { // For directory, add . and .. initially.
            unsigned char initial_str[0x40] = {0};
            initial_str[0x10] = '.';
            initial_str[0x30] = '.';
            initial_str[0x31] = '.';
            memcpy(tmp_block, initial_str, sizeof(unsigned char) * 0x40);
            new_in->size = 0x40; // Since this has two entries.
        } else { // For file, set it as \n.
            ((unsigned char*) tmp_block)[0] = '\n';
            new_in->size = 3; // Since this is just one file with \n.
            mode = mode | INODE_MODE_EMPTY_FILE; // Set file as empty file.
        }

        new_in->mode = mode;
        struct inode* in_table = partitions[disk_index].inode_table;
        memcpy(in_table + assigned_inode_index, new_in, sizeof(struct inode)); // Apply it to the inode table.
        struct blocks* block_table = partitions[disk_index].data_blocks;

        // Apply it to the disk block table.
        memcpy(block_table + assigned_blocks[0], tmp_block, sizeof(struct blocks));

        // Store new entry's information.
        strcpy(new_ent->name, file_name);
        new_ent->disk_index = disk_index;
        new_ent->inode_index = assigned_inode_index;
        new_ent->sibling = NULL;
        new_ent->child = NULL;
        new_ent->parent = cur_dir;

        // Set inode and block as being used.
        inode_stats[assigned_inode_index][disk_index] = 1;
        for (int i = 0 ; i < 6 ; i++) {
            block_stats[assigned_blocks[i]][disk_index] = 1;
        }

        // Insert entry into the LCRS tree for easy indexing in the future.
        if (cur_dir->child == NULL) { // If current directory was an empty directory, just link this as child.
            cur_dir->child = new_ent;
            // printf("This is the first element: %s, parent %s\n", new_ent->name, cur_dir->name);
        } else { // If current directory was not empty directory, find the last sibling and attach this as sibling.
            struct entry_t *peer;
            peer = cur_dir->child;
            while (peer->sibling != NULL) { // Find the last sibling of current directory.
                peer = peer->sibling;
            }
            peer->sibling = new_ent;
        }

        // Physically emit data into the disk.
        write_inode(disk_index, assigned_inode_index, new_in); // Store inode.
        write_data_block(disk_index, assigned_blocks[0], 1, tmp_block); // Store data block.
        dir_add_child(disk_index, new_ent, cur_dir); // Add entry into the current directory file.

        free(new_in);
        return 0;
    }
}


/**
 * A function that writes data into specific block in disk.
 * @param disk_index The disk index to write.
 * @param block_index The index of disk block.
 * @param block_count The count of disk blocks to write. For example, if we are writing 3.5 blocks, this shall be 4.
 * @param data The struct blocks that contains data to write.
 * @return -1 if failure, 0 if successful.
 */
int write_data_block(unsigned int disk_index, unsigned int block_index, unsigned int block_count, struct blocks* data) {
    unsigned char* buffer = (unsigned char*) data; // Typecast into buffer.
    unsigned int real_offset = 0x2000 + block_index * sizeof(struct blocks); // This is the offset of the whole disk.

#ifdef DEBUG
    printf("[DEBUG] Writing data from %x to %x (%d bytes)\n", real_offset, real_offset + block_count * sizeof(struct blocks), block_count * 400);
#endif
    // Open file for specific offset write, special thanks to https://stackoverflow.com/a/2623210/5716511
    FILE* fp = fopen(disks[disk_index], "rb+");
    fseek(fp, real_offset, SEEK_SET); // Set offset
    fwrite(buffer, block_count * sizeof(struct blocks), 1, fp); // Write binary data.
    fclose(fp); // Close file.

    return 0;
}


/**
 * A function that assigns empty blocks from disk.
 * @param disk_index The index of disk to look for empty blocks.
 * @param block_count The count of empty blocks to get assigned.
 * @param ret The array of unsigned int* to set assigned blocks to.
 *            The array MUST have at 6 elements. Size must be greater than sizeof(unsigned int) * 6.
 * @return -1 if failure, 0 if success
 *         If the disk had no more space, this will return -1.
 *         Even if it had 'some' disk space left, this will return -1.
 *         Ex) Disk space left 3 blocks, required blocks 4 -> returns -1.
 */
int assign_empty_blocks(unsigned int disk_index, unsigned int block_count, unsigned int* ret) {
    struct partition *cur_p = &partitions[disk_index];
    unsigned int free_blocks = cur_p->s.num_free_blocks;
    unsigned int tmp_ret[6] = {0xFFFF};
    unsigned int cur_count = 0;
    if (free_blocks >= block_count) {
        // Iterate over blocks till we satisfy the block count.
        for (int i = 0 ; (i < MAX_BLOCK_COUNT) && (cur_count < block_count) ; i++) {
            if (block_stats[i][disk_index] == 0) {
                block_stats[i][disk_index] = 1; // Set as being used.
                tmp_ret[cur_count] = i;
                cur_count++;
            }
        }
        for (int i = 0 ; i < 6 ; i++) // Copy values to ret.
            ret[i] = tmp_ret[i];
        //memcpy(*ret, tmp_ret, sizeof(unsigned int) * 6);
        return 0;
    } else { // When disk space was not enough.
        printf("[ERROR] No space left on device\n");
        return -1;
    }
}


/**
 * A function that assigns empty inodes from disk.
 * This function will just simply grab an empty inode from the disk that was available in the fastest order.
 * @param disk_index The index of disk to look for empty inodes.
 * @param ret The pointer to unsigned int to store empty inode to.
 * @return -1 if failure (no free inodes), 0 if success.
 */
int assign_empty_inodes(unsigned int disk_index, unsigned int* ret) {
    struct partition *cur_p = &partitions[disk_index];
    unsigned int free_inodes = cur_p->s.num_free_inodes;
    if (free_inodes != 0) { // We at least had an inode to assign.
        // Iterate over all inodes and look for an empty inode.
        // This will be an O(n) operation.
        for (int i = 0 ; i < MAX_INODE_COUNT ; i++) {
            if (inode_stats[i][disk_index] == 0) { // If the inode was free, assign it.
                inode_stats[i][disk_index] = 1; // Set the inode as being used.
                cur_p->s.num_free_inodes = cur_p->s.num_free_inodes - 1; // Reduce one free inode.
                *ret = i; // Set assigned inode.
                return 0;
            }
        }
        return -1; // Could not find any empty space.
    } else { // When disk space was not enough.
        printf("[ERROR] No space left on device\n");
        return -1;
    }
}


/**
 * A function that scans disk blocks and look marks each block is being used or not to the block_stats
 * @param disk_index The disk index to look for.
 * @return -1 if failure, 0 if success.
 */
int scan_disk_blocks(unsigned int disk_index) {
    if(disk_index >= disk_count) return -1;
    else {
        struct partition *cur_p = &partitions[disk_index];
        for (int i = 0 ; i < MAX_INODE_COUNT ; i++) {
            struct inode cur_i = cur_p->inode_table[i];
            for (int j = 0 ; j < 6 ; j++) {
                block_stats[cur_i.blocks[j]][disk_index] = 1; // Set the block as being used.
            }
        }
        unsigned int count = 0;
        for (int i = 0 ; i < MAX_BLOCK_COUNT ; i++) {
            count += block_stats[i][disk_index] == 1; // Add total count of empty blocks.
        }
        cur_p->s.num_free_blocks = cur_p->s.num_blocks - count; // Store free block count
        return 0;
    }
}


/**
 * A function that scans disk blocks and look marks each block is being used or not to the block_stats
 * @param disk_index The disk index to look for.
 * @return -1 if failure, 0 if success.
 */
int scan_disk_inodes(unsigned int disk_index) {
    if(disk_index >= disk_count) return -1;
    else {
        struct partition *cur_p = &partitions[disk_index];
        for (int i = 0 ; i < MAX_INODE_COUNT ; i++) { // Mark inodes that are being used.
            struct inode cur_i = cur_p->inode_table[i];
            inode_stats[i][disk_index] = (cur_i.size != 0 || i < 3);
        }

        unsigned int count = 0; // Store count value.
        for (int i = 0 ; i < MAX_INODE_COUNT ; i++) {
            count += inode_stats[i][disk_index] == 1;
        }
        cur_p->s.num_free_inodes = cur_p->s.num_inodes - count; // Store free inode count.
        return 0;
    }
}


/**
 * A function that adds a file into a specific directory.
 * This function will add entry to the directory file.
 * @param disk_index The disk index that current directory is located at
 * @param child The child entry to add.
 * @param dir The current directory to add new file into.
 * @return -1 if failure, 0 if successful.
 */
int dir_add_child(unsigned int disk_index, struct entry_t* child, struct entry_t* dir) {
    struct inode *in = &partitions[dir->disk_index].inode_table[dir->inode_index];
    struct partition *cur_p = &partitions[disk_index];
    unsigned block_count = (in->size / sizeof(struct blocks)) + 1; // Calculate total block count.
    unsigned int buffer_size = sizeof(struct blocks) * block_count; // Calculate total buffer size.

    // Generate and copy buffer from original data.
    unsigned char *buffer = malloc(sizeof(unsigned char) * buffer_size);
    memcpy(buffer, cur_p->data_blocks + in->blocks[0], buffer_size);

    // Search for empty space with 0x20 bytes to place our new entry into.
    unsigned int offset = 0;
    while (offset <= buffer_size) {
        // Seek throughout the directory file and look for empty element.
        unsigned char tmp_ent[0x20] = {0};
        memcpy(tmp_ent, buffer + offset, sizeof(unsigned char) * 0x20);

        // Check if the current entry that we are seeking in the file is empty.
        unsigned int cur_sum = 0 ;
        for (int i = 0 ; i < 0x20 ; i++) {
            cur_sum += tmp_ent[i];
        }

        if (cur_sum == 0) break; // If we have 0x20 bytes of empty space to write new directory entry into.
        offset = offset + 0x20; // Otherwise, update offset and look for the next one.
    }

    if (offset == buffer_size) { // This means that there is not enough room for new file in this entry.
        printf("[ERROR] Could not add more files to current directory\n");
        return -1;
    }

    // Prepare new entry buffer.
    unsigned char new_entry[0x20] = {0};
    unsigned int inode_index = child->inode_index;
    memcpy(new_entry, &inode_index, sizeof(char) * 4); // Copy inode index.
    memcpy(new_entry + 0x10, child->name, sizeof(char) * 16); // Copy name into the entry.
    memcpy(buffer + offset, new_entry, sizeof(char) * 0x20); // Store data into the buffer.
    memcpy(cur_p->data_blocks + in->blocks[0], buffer, buffer_size);

#ifdef DEBUG
    printf("[DEBUG] Found empty entry place from %0x to %0x.\n", offset, offset + 0x20 - 1);
    printf("[DEBUG] Writing data:\n");
    printf("[DEBUG] New Entry : ");
    for (int i = 0 ; i < 0x20 ; i++)
        printf("%x", new_entry[i]);
    printf("\n");
    printf("[DEBUG] Buffer : ");
    for (int i = 0 ; i < 0x20 ; i++)
        printf("%x", buffer[i + offset]);
    printf("\n");
    printf("[DEBUG] Block count : %d\n", block_count);
#endif

    in->size = in->size + 0x20;
    //partitions[disk_index].inode_table[dir->inode_index].size = in->size + 0x20; // Update directory's size.

    // Physically emit data into the disk file.
    // We also need to update the inode metadata for the directory as well.
    write_data_block(disk_index, in->blocks[0], block_count, (struct blocks*) buffer);
    write_inode(disk_index, dir->inode_index, in);
    free(buffer); // OS is my garbage collector but...

    return 0;
}


/**
 * A function that removes specific child entry from the directory.
 * When deleting a file, there shall be two things that must be performed:
 * 1. Set the entry which spans over length of 0x20 as 0.
 * 2. Then shift all entries after the target to the target point.
 *    Ex) 1 2 3 4 was the file and we removed 3
 *        1 2 4 shall be the directory entry value.
 * 3. Update inode size for the directory.
 * @param disk_index The disk index to search for the target.
 * @param dir The directory entry that the target is residing in.
 * @param target The name of the target.
 * @return -1 if failure, 0 if successful.
 */
int dir_remove_child(unsigned int disk_index, struct entry_t* dir, char* target) {
    struct inode *in = &partitions[dir->disk_index].inode_table[dir->inode_index];
    struct partition *cur_p = &partitions[disk_index];
    unsigned block_count = (in->size / sizeof(struct blocks)) + 1; // Calculate total block count.
    unsigned int buffer_size = sizeof(struct blocks) * block_count; // Calculate total buffer size.
    unsigned char found_target = 0;

    // Generate and copy buffer from original data.
    unsigned char *buffer = malloc(buffer_size);
    memcpy(buffer, cur_p->data_blocks + in->blocks[0], buffer_size);

    // Search for the place where our target is located at.
    unsigned int offset = 0;
    while (offset <= buffer_size) {
        // Seek throughout the directory file and look for the file name
        char file_name[0x10] = {0};
        memcpy(file_name, buffer + offset + 0x10, sizeof(unsigned char) * 0x10);
        if (!strcmp(file_name, target)) { // Meaning that we have found the name.
            // If we have found the entry, there are two things that we shall perform.
            // 1. Set the entry as 0.
            // 2. Shift all entries after the found entry 0x20.
            found_target = 1;
            memset(buffer + offset, 0, sizeof(unsigned char) * 0x20); // Remove file entry.
            //printf("shifting from %x to %x %x(%d -> %d)\n", offset, offset + 0x20, in->size - offset - 0x20, in->size - offset - 0x20, (int) (in->size - offset - 0x20) > 0 ? in->size - offset - 0x20 : 0);
            // If the size is negative, set it as 0 since it will cause SEFGAULT.
            memcpy(buffer + offset, buffer + offset + 0x20, (int) (in->size - offset - 0x20) > 0 ? in->size - offset - 0x20 : 0); // TODO Fix pointer.
            break;
        }
        offset = offset + 0x20; // Otherwise, update offset and look for the next one.
    }

    if (!found_target) { // This means that there is not enough room for new file in this entry.
        printf("[ERROR] Could not find entry %s\n", target);
        return -1;
    }

    in->size = in->size - 0x20; // Remove size 0x20 since the file size was changed.

    // Physically emit data into the disk file.
    // We also need to update the inode metadata for the directory as well.
    memcpy(cur_p->data_blocks + in->blocks[0], buffer, buffer_size);
    write_data_block(disk_index, in->blocks[0], block_count, (struct blocks*) buffer);
    write_inode(disk_index, dir->inode_index, in);
    free(buffer); // OS is my garbage collector but...

    return 0;
}


/**
 * A function that deletes a single file or directory file.
 * @param disk_index The disk index that this file is located.
 * @param dir The directory that this file is located at.
 * @param target The target file.
 * @return -1 if failure, 0 if successful.
 */
int delete_file(unsigned int disk_index, struct entry_t* dir, char* target) {
    // Store necessary data.
    struct inode *inode_table = partitions[disk_index].inode_table;
    struct blocks *block_table = partitions[disk_index].data_blocks;
    struct entry_t *target_entry = NULL;
    find_entry(dir->child, &target_entry, target);
    if (target_entry == NULL) return -1; // Could not find entry.
    struct inode target_in = partitions[target_entry->disk_index].inode_table[target_entry->inode_index];

    // Remove blocks from block table.
    unsigned short block_arr[0x6] = {0};
    memcpy(block_arr, target_in.blocks, sizeof(unsigned short) * 6);
    for (int i = 0; i < 6; i++) {
        if (block_arr[i] == 0) continue; // if block was 0 skip since this is unassigned block.
        memset(block_table + block_arr[i], 0, sizeof(struct blocks)); // Clear block data in memory.
        write_data_block(disk_index, block_arr[i], 1, block_table + block_arr[i]); // Emit change to disk.
    }

    // Remove inode from the inode table.
    unsigned int inode_index = target_entry->inode_index;
    memset(inode_table + inode_index, 0, sizeof(struct inode)); // Clock inode data in memory.
    write_inode(disk_index, inode_index, &inode_table[inode_index]); // Emit change to disk.
    inode_stats[inode_index][disk_index] = 0; // Set inode as available.

    // Remove entry from the LCRS tree.
    struct entry_t *peer = dir->child;
    if (peer == target_entry) { // If this was the first child in the directory.
        dir->child = peer->sibling; // Just unlink it.
    } else { // If this was not the first child in the directory.
        while (peer->sibling != NULL) { // Find the sibling that is connected to current entry.
            if (peer->sibling == target_entry) {
                peer->sibling = target_entry->sibling; // Connect before and next.
                break;
            }
            peer = peer->sibling;
        }
    }

    // Remove file from the parent directory file.
    if (dir_remove_child(disk_index, dir, target) == -1) {
        printf("[ERROR] Could not delete file %s\n", target);
        return -1;
    }

    return 0;
}


/**
 * A function that deletes directory recursively.
 * @param disk_index The volume index that this directory is located at.
 * @param cur_dir The current directory.
 * @param target_dir The target directory to delete
 * @return -1 if failure, 0 if successful.
 */
int delete_directory(unsigned int disk_index, struct entry_t* cur_dir, char* target_dir) {
    struct entry_t *target_entry = NULL;
    find_entry(cur_dir->child, &target_entry, target_dir);
    if (target_entry == NULL) return -1; // Could not find entry.

    r_delete_directory(disk_index, cur_dir, target_entry); // Delete directory recursively.
    return 0;
}


/**
 * A recursive function that deletes all subdirectories and the files.
 * @param disk_index The index of disk.
 * @param cur_dir The current directory's entry.
 * @param child The child entry to delete.
 */
void r_delete_directory(unsigned int disk_index, struct entry_t* cur_dir, struct entry_t* child) {
    if (child->child == NULL) { // If the child was regular file, delete file.
        delete_file(disk_index, cur_dir, child->name);
        return;
    } else { // If the child was a directory
        struct entry_t *peer = child->child;
        while (peer != NULL) { // For all s
            struct inode tmp_in = partitions[disk_index].inode_table[peer->inode_index];
            if ((tmp_in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE) { // If this peer was inode
                // Delete subdirectories and files, delete them all.
                r_delete_directory(disk_index, child, peer);

                // Once all the files in directory was deleted, delete the directory file as well.
                struct partition *cur_p = &partitions[disk_index];
                struct inode *in = &cur_p->inode_table[child->inode_index];

                // Clear blocks that this directory was using.
                unsigned block_count = (in->size / sizeof(struct blocks)) + 1; // Calculate total block count.
                struct blocks empty_block;
                memset(&empty_block, 0, sizeof(struct blocks));
                for (int i = 0 ; i < block_count ; i++) { // Physically emit deletion.
                    if (in->blocks[i] == 0) continue;
                    else write_data_block(disk_index, in->blocks[i], 1, &empty_block);
                }
                // Delete entry in memory
                memset(cur_p->data_blocks + in->blocks[0], 0, sizeof(struct blocks) * block_count);

                // Delete inode that this directory was using.
                struct inode empty_inode;
                memset(&empty_inode, 0, sizeof(struct inode));
                write_inode(disk_index, child->inode_index, &empty_inode); // Physically emit deletion.
                memset(in, 0,sizeof(struct inode)); // Delete inode in the memory.
            }
            delete_file(disk_index, child, peer->name); // Delete entry from the higher directory.
            peer = peer->sibling;
        }
        delete_file(disk_index, cur_dir, child->name); // Delete entry from the higher directory.
    }
}


/**
 * A function that copies a file.
 * Since this program uses CoW method, this will just generate inode and directory entry.
 * @param disk_index The volume index that this directory is located at.
 * @param src The source entry to copy.
 * @param dst The destination file name to copy as.
 * @return -1 if failure, 0 if successful.
 */
int copy_file(unsigned int disk_index, struct entry_t* src, char* dst) {
    struct partition *cur_p = &partitions[disk_index];

    // Find the original inode's index.
    // This is for cases when copying a copied entry.
    // If we were copying a copied entry, we need to find the original entry.
    unsigned int original_inode_index = 0xFFFF;
    find_original_inode(disk_index, src->inode_index, &original_inode_index);
    if (original_inode_index == 0xFFFF) { // We could not find original inode, quit
        printf("[ERROR] Could not find original inode\n");
        return -1;
    }

    struct inode *src_in = &cur_p->inode_table[original_inode_index];
    struct inode *dst_in = NULL;

    // Get an empty inode.
    unsigned int assigned_inode = 0;
    if (assign_empty_inodes(disk_index, &assigned_inode) == -1) {
        printf("[ERROR] Could not assign inode\n");
        return -1;
    }

    // Since we are performing CoW, just copy the whole inode into the new one.
    dst_in = &cur_p->inode_table[assigned_inode];
    memcpy(dst_in, src_in, sizeof(struct inode));
    dst_in->indirect_inode = (int) original_inode_index; // Set source inode as indirection.
    write_inode(disk_index, assigned_inode, dst_in); // Emit change to the disk.

    // Make new entry in the tree for the copied one.
    struct entry_t* parent = src->parent;
    struct entry_t* new_ent = malloc(sizeof(struct entry_t));
    memset(new_ent, 0, sizeof(struct entry_t));

    // Set values accordingly.
    new_ent->inode_index = assigned_inode;
    new_ent->disk_index = disk_index;
    new_ent->child = NULL;
    new_ent->sibling = NULL;
    new_ent->parent = parent;
    strcpy(new_ent->name, dst);

    // Find last sibling and attach new entry to the new entry.
    struct entry_t *peer = src;
    while(peer->sibling != NULL) {
        peer = peer->sibling;
    }
    peer->sibling  = new_ent;

    // add entry to the directory explicitly.
    dir_add_child(disk_index, new_ent, parent);

    return 0;
}


/**
 * A function that finds original inode.
 * When we have recursive indirection, we need to find the original inode that is actually storing the disk data.
 * Ex) inode 3 (indirection) -> inode 2 (indirection) -> inode 1 (actual)
 * This function will works as a recursive function and find the index of the actual inode.
 * @param disk_index The disk index to look inode table from.
 * @param cur_inode The current inode that is directing another inode.
 * @param ret The pointer to store original inode's index.
 */
void find_original_inode(unsigned int disk_index, unsigned int cur_inode, unsigned int* ret) {
    struct inode in = partitions[disk_index].inode_table[cur_inode];
    if (in.indirect_inode == -1) *ret = cur_inode; // We found the actual inode that is being redirected.
    else {
        find_original_inode(disk_index, in.indirect_inode, ret);
    }
}


/**
 * A function that processes CoW (Copy on Write).
 * In this program, CoW will be triggered with following conditions:
 * 1. chmod of the copied file.
 * 2. append of the copied file.
 * 3. write of the copied file.
 * 4. rm of the copied file.
 * 5. mv of the copied file.
 * In order to process CoW, this program will perform following operations.
 * 1. Assign free blocks with the same amount that the original one had.
 * 2. Dump everything to the new block.
 * 3. Store data block physically.
 * 4. Update inode information.
 * 5. Store inode physically.
 * @param disk_index The disk index to process CoW.
 * @param copied_entry The entry that was copied and is directing another inode.
 * @return -1 if failure, 0 if successful.
 */
int process_cow(unsigned int disk_index, struct entry_t* copied_entry) {
#ifdef DEBUG
    printf("[DEBUG] CoW called for %s\n", copied_entry->name);
#endif
    struct inode *copied_in = &partitions[copied_entry->disk_index].inode_table[copied_entry->inode_index];
    struct inode *original_in = &partitions[disk_index].inode_table[copied_in->indirect_inode];
    struct partition *cur_p = &partitions[disk_index];
    unsigned int block_count = (original_in->size / 1024) + 1;

    // Assign free blocks just like the original source had.
    struct blocks buffer[6];
    memset(buffer, 0, sizeof(struct blocks) * 6);
    unsigned int assigned_blocks[6] = {0};
    if (assign_empty_blocks(disk_index, block_count, assigned_blocks) == -1) {
        printf("[ERROR] Could not assign empty blocks\n");
        return -1;
    }

    // With the assigned free blocks, dump everything from the original source's blocks.
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = assigned_blocks[i];
        if (block == 0) continue;
        memcpy(&buffer[i], &cur_p->data_blocks[original_in->blocks[i]], sizeof(struct blocks));
    }

    // Store data block physically and into the memory as well.
    for (int i = 0 ; i < 6 ; i++) {
        unsigned int block = assigned_blocks[i];
        if (block == 0) continue;
        memcpy(&cur_p->data_blocks[block], &buffer[i], sizeof(struct blocks));
        write_data_block(disk_index, block, 1, &buffer[i]);
    }

    // Update inode information then store it to the disk as well.
    for (int i = 0 ; i < 6 ; i++)
        copied_in->blocks[i] = assigned_blocks[i]; // Can't use memcpy since this is unsigned short vs unsigned int.
    copied_in->indirect_inode = -1; // This is no longer indirection inode.
    write_inode(disk_index, copied_entry->inode_index, copied_in);

    return 0;
}


/**
 * A function that finds all inodes that has indirection to a specific inode.
 * Since this looks through all the entries in the inode table, this is O(n) operation.
 * Therefore, avoid using this function as much as possible.
 * @param disk_index The disk index to look for all inodes.
 * @param inode_index The target inode that we would like to indirections to.
 * @param ret The unsigned int array to store indirection information to.
 * @return The total count of indirections to this inode.
 */
int find_indirection_inodes(unsigned int disk_index, unsigned int inode_index, unsigned int* ret) {
    struct inode *inode_table = partitions[disk_index].inode_table;
    int cnt = 0;
    // For all inodes in inode table search for all inodes that are directing target inode.
    for (int i = 0 ; i < MAX_INODE_COUNT ; i++) {
        if (inode_table[i].indirect_inode == inode_index) {
            ret[cnt] = i;
            cnt++;
        }
    }
    return cnt;
}


/**
 * A function that handles CoW.
 * This function will check if CoW is required in the specific target.
 * When the target needs to be processed with CoW, this function will also perform CoW automatically.
 * There are two cases that this function will trigger CoW:
 * 1. The target was the original entry that some other inodes have indirection to:
 *    -> In this case, this function will perform CoW to all inodes that has indirection to the target.
 * 2. The target entry was just an indirection to an original file:
 *    -> In this case, this function will just perform CoW to the target entry.
 * @param disk_index The disk index to check CoW.
 * @param target The target entry to perform CoW.
 * @return -1 if failure, 0 if CoW was not performed, 1 if CoW was performed.
 */
int handle_cow(unsigned int disk_index, struct entry_t* target) {
    struct inode *in = &partitions[disk_index].inode_table[target->inode_index];
    if (in->indirect_inode != -1) {
        // This means that this target entry directs to another inode.
        if (process_cow(disk_index, target) == -1) { // CoW failed.
            printf("[ERROR] CoW of %s failed\n", target->name);
            return -1;
        }
        return 1; // CoW was performed.
    } else {
        // This means that this target entry is the original inode.
        // Thus, check if there are indirections to this inode.
        unsigned int inodes[MAX_INODE_COUNT] = {0};
        int cnt = find_indirection_inodes(disk_index, target->inode_index, inodes);
        if (cnt == 0) { // This means we do not need CoW
            return 0;
        } else { // This means we need CoW for all the indirections. (This is a bad case)
            // TODO: Add recursive finding for entry search in other directories.
            // This will find only the entries having the target as indirection inode within the same directory.
            // Also, this works very stupid. O(n^2) will be the time complexity, this must be improved in the future.
            struct entry_t *peer = target->parent->child; // Get all peers in the "SAME" directory.
            while (peer != NULL) {
                for (int i = 0 ; i < cnt ; i++) { // Look for matching cases.
                    if (inodes[i] == peer->inode_index) { // If the peer has indirection to the target inode,
                        if (process_cow(disk_index, peer) == -1) { // Perform CoW
                            printf("[ERROR] CoW of %s failed\n", peer->name);
                            return -1;
                        }
                    }
                }
                peer = peer->sibling;
            }
            return 1; // CoW was performed.
        }
    }
}


/**
 * A function that renames file.
 * This will delete file name including directory files.
 * This function will modify the name in the parent directory file.
 * @param disk_index The disk index that this directory is located at.
 * @param target The target file.
 * @param dst The name to set.
 * @return -1 if failure, 0 if successful.
 */
int rename_file(unsigned int disk_index, struct entry_t* target, char* dst) {
    struct entry_t *parent = target->parent;
    struct inode *parent_in = &partitions[disk_index].inode_table[parent->inode_index];
    struct partition *cur_p = &partitions[disk_index];

    // Generate temp buffer for storing the parent directory file.
    unsigned char *buffer = (unsigned char*) malloc(sizeof (struct blocks) * 6);
    memset(buffer, 0, sizeof(struct blocks) * 6);

    for(int i = 0 ; i < 6 ; i++) {
        unsigned int block = parent_in->blocks[i];
        if ((block == 0) && target->parent->parent != NULL) continue; // If this was not a root directory, pass block #0
        memcpy(&((struct blocks*) buffer)[i], cur_p->data_blocks + parent_in->blocks[i], sizeof(struct blocks));
    }

    // Search for the entry and change its name.
    unsigned int offset = 0;
    while (offset <= parent_in->size) {
        // Get entry's name
        unsigned char tmp_ent[0x20] = {0};
        unsigned char name[0x10] = {0};
        memcpy(tmp_ent, (unsigned char*) buffer + offset, sizeof(unsigned char) * 0x20);
        memcpy(name, tmp_ent + 0x10, sizeof(unsigned char) * 0x10);

        // If name matches, change name.
        if (!strcmp((char*) name, target->name)) {
            strcpy((char*) tmp_ent + 0x10, dst);
            memcpy(buffer + offset, tmp_ent, sizeof(unsigned char) * 0x20);
            break;
        }
        offset += 0x20;
    }

    // Apply changes to the data blocks in the memory and store it to disk.
    for (int i = 0 ; i < ((parent_in->size / 1024) + 1) ; i++) {
        unsigned int block = parent_in->blocks[i];
        if ((block == 0) && target->parent->parent != NULL) continue; // If this was not a root directory, pass block #0
        memcpy(&cur_p->data_blocks[block], &((struct blocks*) buffer)[i], sizeof(struct blocks));
        write_data_block(disk_index, block, 1, &((struct blocks*) buffer)[i]);
    }

    strcpy(target->name, dst); // Rename entry.
    free(buffer);
    return 0;
}


/**
 * A function that moves file.
 * This will remove the entry from the parent and will set a new parent record for the child.
 * This function will perform following operations sequentially:
 * 1. Remove entry from the original directory (parent) where it was at.
 * 2. Create entry into the new directory's file.
 * @param disk_index The disk index that this target is located at.
 * @param target The target file to move.
 * @param dst The destination directory to move file into.
 * @return -1 if failure, 0 if successful.
 */
int move_file(unsigned int disk_index, struct entry_t* target, char* dst) {
    // Remove entry from the original parent directory.
    dir_remove_child(disk_index, target->parent, target->name);
    // Unlink original entry from the LCRS tree.
    if (target->parent->child == target) {
        // If this was the first child in the directory, we need to update the child.
        target->parent->child = target->sibling; // Set it as sibling of the target.
    } else { // If this was an internal sibling, just disconnect this.
        struct entry_t *peer = target->parent->child;
        while (peer->sibling != NULL) { // Iterate and find the sibling that is connecting to the target
            if (peer->sibling == target) break;
            peer = peer->sibling;
        }
        if (peer->sibling == NULL) {
            printf("[ERROR] Could not find sibling for %s\n", target->name);
            return -1;
        }
        peer->sibling = target->sibling; // Unlink target
    }
    target->sibling = NULL;

    // Add directory entry to the new directory.
    struct entry_t* dst_entry = NULL;
    if (strcmp(dst, "..") == 0) { // For .. directory
        dst_entry = target->parent->parent;
        if (dst_entry == NULL) { // If .. does not exist.
            printf("[ERROR] Could not move into ..\n");
            return -1;
        }
    }
    else { // For other cases.
        if (find_entry(target->parent->child, &dst_entry, dst) ==
            -1) { // Meaning that we could not find the target directory. // TODO FIX HERE
            printf("[ERROR] Could not find destination entry. Inconsistency occurred.\n");
            return -1;
        }
    }
    dir_add_child(disk_index, target, dst_entry);

    // Link new entry into the LCRS tree.
    if (dst_entry->child == NULL) { // If the target directory had no child, set this as first child.
        dst_entry->child = target;
    } else { // Otherwise, find the last sibling and connect current entry to it.
        struct entry_t *peer = dst_entry->child;
        while (peer->sibling != NULL) {
            peer = peer->sibling;
        }
        peer->sibling = target; // Link target.
    }

    return 0;
}