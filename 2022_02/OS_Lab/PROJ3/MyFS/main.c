//
// @file : main.c
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that implements main function.
//


#include "diskutil.h"
#include "common.h"
#include "ui.h"

char disks[MAX_STRING_LEN][MAX_IMG_COUNT];
struct partition partitions[MAX_IMG_COUNT];
struct entry_t* entries[MAX_IMG_COUNT];
unsigned char block_stats[MAX_BLOCK_COUNT][MAX_IMG_COUNT] = {0};
unsigned char inode_stats[MAX_INODE_COUNT][MAX_IMG_COUNT] = {0};
int disk_count;
uint16_t loaded_partitions = 0x00;

/**
 * The almighty main function.
 * @return 0 if terminated without any error, -1 if not.
 */
int main() {
    // Scan disks in directory
    if (scan_disks() == -1) {
        return -1;
    }
    print_title();

    // Load super block, inode table, data blocks and root directory from partition.
    // Also scan blocks and scan inodes.
    if (load_super_block(0) || load_inode_table(0) || load_data_blocks(0)
        || load_root(0) || scan_disk_blocks(0) || scan_disk_inodes(0)) {
        return -1; // Something went wrong.
    }

    main_loop();
    return 0;
}


