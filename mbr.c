#include "mbr.h"
#include "shell.h"

void mbr_parse() {
    uint16_t sector[256]; // 512-byte buffer
    ata_read_sector(0, sector);

    // The partition table starts 446 bytes into the sector
    // We cast the buffer to our struct starting at that offset
    struct mbr_partition* partitions = (struct mbr_partition*)((uint8_t*)sector + 446);

    kprint("\n--- Partition Table ---\n");

    for (int i = 0; i < 4; i++) {
        if (partitions[i].sys_id == 0) continue; // Skip empty slots

        kprint("Partition "); kprint_int(i);
        kprint(": Type 0x"); kprint_int(partitions[i].sys_id);
        kprint(" Start LBA: "); kprint_int(partitions[i].start_lba);
        kprint("\n");

        if (partitions[i].sys_id == 0x0C || partitions[i].sys_id == 0x0B) {
            kprint("Found FAT32 Partition!\n");
            // Store partitions[i].start_lba for your FAT32 driver later
        }
    }
}
