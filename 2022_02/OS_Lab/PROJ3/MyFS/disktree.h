//
// @file : disktree.h
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that defines left child right sibling structure and its tree elements.
//

#ifndef MYFS_DISKTREE_H
#define MYFS_DISKTREE_H
#pragma once

#include "common.h"


/**
 * A struct that implements Left child right sibling tree.
 */
struct entry_t {
    char name[16];       // name

    unsigned int disk_index; // For storing which disk this entry is stored at.
    unsigned int inode_index;
    //struct inode in; // For storing this entry's inode.

    struct entry_t *sibling;
    struct entry_t *child;
    struct entry_t *parent;
};


struct entry_t* load_entries(struct partition);
int release_entries(struct entry_t*);
int insert_entry(struct entry_t*, struct entry_t*, unsigned char*, unsigned int, unsigned int,
                 unsigned int, struct inode*, unsigned char, unsigned char);
int find_entry(struct entry_t*, struct entry_t**, char*);

#endif //MYFS_DISKTREE_H
