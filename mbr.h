#ifndef MBR_H
#define MBR_H

#include "ata.h"

// Define the 16-byte structure for a single partition entry
struct mbr_partition {
    uint8_t  bootable;      // 0x80 = bootable, 0x00 = not bootable
    uint8_t  start_head;
    uint8_t  start_sector;  // Also contains high bits of cylinder
    uint8_t  start_cylinder;
    uint8_t  sys_id;        // 0x0C or 0x0B are usually FAT32
    uint8_t  end_head;
    uint8_t  end_sector;
    uint8_t  end_cylinder;
    uint32_t start_lba;     // THIS IS WHAT WE NEED: Starting sector of partition
    uint32_t total_sectors; // Total sectors in partition
} __attribute__((packed));

void mbr_parse();

#endif
