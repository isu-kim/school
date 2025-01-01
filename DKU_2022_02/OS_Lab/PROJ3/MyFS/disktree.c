//
// @file : disktree.c
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that implements left child right sibling structure and its tree elements.
//

#include "disktree.h"


extern char disks[MAX_STRING_LEN][MAX_IMG_COUNT];
extern struct partition partitions[MAX_IMG_COUNT];
extern int disk_count;
extern struct entry_t* entries[MAX_IMG_COUNT];
extern uint16_t loaded_partitions;
extern unsigned char block_stats[MAX_BLOCK_COUNT][MAX_IMG_COUNT];
extern unsigned char inode_stats[MAX_INODE_COUNT][MAX_IMG_COUNT];


/**
 * A function that loads entries from root directory recursively.
 * This function will generate all files according to the left child right sibling tree.
 * @param partition The partition to scan directories and files from.
 * @return The head entry.
 */
struct entry_t* load_entries(struct partition partition) {
    // Generate head of the entries. This will represent the root directory.
    struct entry_t *head = malloc(sizeof(struct entry_t));
    if (!head) return NULL;
    else {
        // Store root's inode.
        memset(head, 0, sizeof(struct entry_t));
        head->inode_index = partition.s.first_inode;

        struct inode root = partition.inode_table[partition.s.first_inode];
        struct blocks tmp_blocks[6];

        // Copy 6 blocks from the inode's block.
        unsigned short start_block = root.blocks[0];
        unsigned short block_count = root.size / sizeof(struct blocks) + 1; // Calculate the last block.

        // Copy tmp blocks from start to the end the inode's pointer.
        memcpy(tmp_blocks, &partition.data_blocks[start_block], sizeof(struct blocks) * block_count);

        // Start loading entries into the tree.
        insert_entry(head, head, (unsigned char *) tmp_blocks, 0, root.size, partition.disk_index, partition.inode_table, 1, 1);
        return head;
    }
}


/**
 * A function that inserts entry to the left child right sibling tree.
 * This will work recursively and load all siblings.
 * TODO: Add directory recursive support.
 * @param node The current node that we are going to put sibling or child into.
 * @param head The head of this whole entry system.
 * @param tmp_blocks The blocks that this directory is using in the disk.
 * @param block_offset The offset of current dirent information in the block.
 * @param size The total size to read off from the disk block.
 * @param disk_index The disk's index. This is to store which disk this entry is located at.
 * @param inode_table The inode table to look for from the partition.
 * @param is_first_child Whether if this entry is the first child for root directory.
 * @return -1 if unsuccessful, 0 if successful.
 */
int insert_entry(struct entry_t* node, struct entry_t* head, unsigned char* tmp_blocks, unsigned int block_offset,
                 unsigned int size, unsigned int disk_index, struct inode* inode_table, unsigned char is_first_child,
                 unsigned char is_child) {
    if (block_offset + 0x20 > size) return 0;
    else {
        // Generate a new node and init its values.
        struct entry_t* new_entry = malloc(sizeof(struct entry_t));
        unsigned char* entry_val = malloc(sizeof(unsigned char) * 0x20);
        if ((void*) new_entry == (void*) entry_val) return -1; // If malloc failed, return -1.
        memset(new_entry, 0, sizeof(struct entry_t));
        memset(entry_val, 0, sizeof(unsigned char) * 0x20);

        // Store values from the binary data.
        memcpy(entry_val, tmp_blocks + block_offset, 0x20); // Dump values from block.
        memcpy(new_entry->name, entry_val +  0x10, 0x10); // Store current entry's name;
        new_entry->disk_index = disk_index; // Store disk index value.

        // If this entry was .. or ., just skip this.
        if (strcmp(new_entry->name, "..") == 0 || strcmp(new_entry->name, ".") == 0) {
            // TODO HERE
            insert_entry(node, head, tmp_blocks, block_offset + 0x20, size, disk_index, inode_table, is_first_child, 1);
            return 0;
        }

        // Store inode's information.
        unsigned int inode_index;
        memcpy(&inode_index, entry_val, sizeof(unsigned char) * 2);
        new_entry->inode_index = inode_index;

        // Check if current node was a directory, and take action accordingly.
        struct inode node_in = partitions[disk_index].inode_table[node->inode_index];
        if (((node_in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE) && is_child){
            // If the node was directory and is setting a child
            // The reason of checking if this node was setting child is that we do not have any method of knowing
            // why the current node's child is NULL or not. That means, if we do not check the condition,
            // loading an empty directory and loading a sibling will be treated together,
            // therefore will be losing directories.
            node->child = new_entry;
            new_entry->parent = is_first_child ? head : node;
            //printf("%s is first child of %s\n", new_entry->name, node->name);
        } else { // If node was just a regular file, set entry as sibling.
            new_entry->parent = node->parent;
            node->sibling = new_entry;
            //printf("%s is sibling of %s\n", new_entry->name, node->name);
        }

        struct inode cur_in = partitions[disk_index].inode_table[inode_index];
        // If this sibling was a directory file, go recursive and find more entries.
        // However, if the name was . or .., do not go recursion, otherwise it will get stuck.
        if (((cur_in.mode & INODE_MODE_DIR_FILE) == INODE_MODE_DIR_FILE)){
#ifdef DEBUG
        printf("[DEBUG] Registering directory %s: (Inode %d) - parent %s\n", new_entry->name, inode_index, new_entry->parent->name);
#endif
            // Copy directory file's data and go recursive.
            struct blocks dir_blocks[6];
            unsigned int start_block = cur_in.blocks[0];
            unsigned short block_count = cur_in.size / sizeof(struct blocks) + 1; // Calculate the last block.
            memcpy(dir_blocks, &partitions[disk_index].data_blocks[start_block], sizeof(struct blocks) * block_count);
            insert_entry(new_entry, head, (unsigned char *) dir_blocks, 0, cur_in.size, disk_index, inode_table, 0, 1);
#ifdef DEBUG
        printf("[DEBUG] Registered directory %s: (Inode %d) - parent %s\n", new_entry->name, inode_index, new_entry->parent->name);
#endif
        }
#ifdef DEBUG
        // for (int i = 0 ; i < 0x20 ; i++) printf("%x", entry_val[i]);
        printf("[DEBUG] Registered file - %s: (Inode %d) - parent %s\n", new_entry->name, inode_index, new_entry->parent->name);
#endif
        // Store sibling information and look for other siblings recursively and free read value.
        free(entry_val);
        insert_entry(new_entry, head, tmp_blocks, block_offset + 0x20, size, disk_index, inode_table, 0, 0);
        return 0;
    }
}



/**
 * A function that releases entries from lcrs tree.
 * This will recursively look for all entries and delete the elements.
 * @param head The head of the tree. This shall always be the root directory.
 * @return -1 if unsuccessful, 0 if successful.
 */
int release_entries(struct entry_t* head) {
    if (head == NULL) {
        return -1;
    }
    struct entry_t *peer = head->child;
    while (peer != NULL) {
        struct entry_t *temp = peer;
        struct inode in = partitions[peer->disk_index].inode_table[peer->inode_index];
        if ((in.mode & 0xF0000) == INODE_MODE_DIR_FILE) { // If this was a directory, release children as well.
            release_entries(temp->child);
        }
        free(temp);
        peer = peer->sibling;
    }
    return 0;
}


/**
 * A function that finds entry struct that matches target file's name.
 * @param cur_dir Current directory's entry.
 * @param ret The struct* to store found data into.
 * @param target The target file or directory to look for.
 * @return -1 if The file was not found, 0 if file was found.
 */
int find_entry(struct entry_t* cur_dir, struct entry_t** ret, char* target) {
    struct entry_t* peer = cur_dir;
    *ret = NULL;
    char* file_name = strrchr(target, '/'); // Get last file name.
    if (file_name == NULL) file_name = target;
    while (peer != NULL) {
        if (!(strcmp(peer->name, file_name))) { // Entry found.
            *ret = peer;
        }
        peer = peer->sibling;
    }
    return *ret == NULL ? -1 : 0;
}
