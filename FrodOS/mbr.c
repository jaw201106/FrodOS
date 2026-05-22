#include "ata.h"
#include "mbr.h"
#include "shell.h"

void kprint(const char* str);
void kprint_int(int num);
void kprint_hex8(uint8_t val);

uint32_t mbr_parse(void) {
    // A C Union forces the compiler to treat the stack chunk
    // as exactly the structure block or a raw 512-byte array safely.
    union {
        uint8_t raw[512];
        struct master_boot_record data;
    } sector_buf;

    uint32_t discovered_fat32_lba = 0;

    kprint("\n--- Scanning Master Boot Record (Sector 0) ---\n");

    // Clear array block memory strings
    for (int i = 0; i < 512; i++) {
        sector_buf.raw[i] = 0;
    }

    // Call hardware disk reader to populate raw buffer slice
    ata_read_sector(0, (uint16_t*)sector_buf.raw);

    // Read elements cleanly directly out of union layout definitions
    if (sector_buf.data.boot_signature != 0xAA55) {
        kprint("MBR Error: Invalid boot sector magic signature!\n");
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        struct mbr_partition* part = &sector_buf.data.partitions[i];

        if (part->sys_id == 0x00) {
            continue;
        }

        kprint("Partition [");
        kprint_int(i);
        kprint("]: Type 0x");
        kprint_hex8(part->sys_id);
        kprint(" | Start LBA: ");
        kprint_int((int)part->start_lba);

        if (part->sys_id == 0x0C || part->sys_id == 0x0B) {
            kprint(" <-- [FAT32 Validated]");
            if (discovered_fat32_lba == 0) {
                discovered_fat32_lba = part->start_lba;
            }
        }
        kprint("\n");
    }

    return discovered_fat32_lba;
}
