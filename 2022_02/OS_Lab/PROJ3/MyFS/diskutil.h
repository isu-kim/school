//
// @file : diskutil.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines disk utility functions.
//


#ifndef MYFS_DISKUTIL_H
#define MYFS_DISKUTIL_H
#pragma once

#include <dirent.h>

#include "common.h"
#include "fs.h"
#include "utils.h"
#include "disktree.h"

#define LS_SPLIT_COUNT 10

// For initializing disk.
int scan_disks(void);
int load_super_block(int);
int load_inode_table(int);
int load_data_blocks(int);
int load_root(int);

// For utility.
int generate_prefix_str(unsigned int, char*);

// For implementing user commands.
int impl_ls(struct entry_t*, unsigned char);
int impl_cat(struct entry_t*, char*);
int impl_chmod(struct entry_t*, char*, unsigned int);
int impl_stat(struct entry_t*, char*);
int impl_touch(struct entry_t*, char*);
int impl_mkdir(struct entry_t*, char*);
int impl_rm(struct entry_t*, char*);
int impl_rmdir(struct entry_t*, char*);
int impl_write(struct entry_t*, char*, char*);
int impl_append(struct entry_t*, char*, char*);
int impl_vstat(unsigned int);
int impl_cp(struct entry_t*, char*, char*);
int impl_rename(struct entry_t*, char*, char*);
int impl_move(struct entry_t*, char*, char*);

// For abstract interface for file operation.
int write_file_data(struct entry_t*, unsigned int, unsigned char*);
int append_file_data(struct entry_t*, unsigned int, unsigned char*);
int read_file_data(struct entry_t*, unsigned char**);

// For abstract interface for inode update.
int update_inode(unsigned int, unsigned int, struct inode*);
int write_inode(unsigned int, unsigned int, struct inode*);

// For abstract interface for creating a single file.
int create_file(unsigned int, struct entry_t*, char*, unsigned char);

// For scanning empty disk blocks and empty inodes.
int scan_disk_blocks(unsigned int);
int scan_disk_inodes(unsigned int);

// For physical write in disk.
int write_data_block(unsigned int, unsigned int, unsigned int, struct blocks*);

// For low level operations.
int assign_empty_blocks(unsigned int, unsigned int, unsigned int*);
int assign_empty_inodes(unsigned int, unsigned int*);

// For directory operations.
int dir_add_child(unsigned int, struct entry_t*, struct entry_t*);
int dir_remove_child(unsigned int, struct entry_t*, char* target);

// For delete operations.
int delete_file(unsigned int, struct entry_t*, char*);
int delete_directory(unsigned int, struct entry_t*, char*);
void r_delete_directory(unsigned int, struct entry_t*, struct entry_t*);

// For copy operations especially CoW.
int copy_file(unsigned int, struct entry_t*, char*);
void find_original_inode(unsigned int, unsigned int, unsigned int*);
int process_cow(unsigned int, struct entry_t*);
int find_indirection_inodes(unsigned int, unsigned int, unsigned int*);
int handle_cow(unsigned int, struct entry_t*);

// For other operations (especially mv).
int move_file(unsigned int, struct entry_t*, char*);
int rename_file(unsigned int, struct entry_t*, char*);

#endif //MYFS_DISKUTIL_H
