#include "fat32.h"
#include "shell.h"

void put_char(char c);

static uint32_t partition_start_lba = 0;
static struct fat32_bpb bpb;
static uint32_t current_dir_cluster = 0;

// 1. Math Helpers
uint32_t get_lba_from_cluster(uint32_t cluster) {
    uint32_t fat_start = partition_start_lba + bpb.reserved_sectors;
    uint32_t data_start = fat_start + (bpb.fat_count * bpb.fat_size_32);
    return ((cluster - 2) * bpb.sectors_per_cluster) + data_start;
}

uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t fat_sector = bpb.reserved_sectors + (cluster / 128);
    uint32_t fat_offset = (cluster % 128);
    uint16_t buf[256];
    ata_read_sector(partition_start_lba + fat_sector, buf);
    uint32_t* fat_table = (uint32_t*)buf;
    return fat_table[fat_offset] & 0x0FFFFFFF;
}

// 2. Initialize
void fat32_init(uint32_t lba) {
    partition_start_lba = lba;
    uint16_t buffer[256];
    ata_read_sector(partition_start_lba, buffer);
    struct fat32_bpb* temp_bpb = (struct fat32_bpb*)buffer;
    bpb = *temp_bpb;
    current_dir_cluster = bpb.root_cluster;

    kprint("\n--- FAT32 Initialized ---\n");
    kprint("OEM Name: ");
    for(int i=0; i<8; i++) put_char(((char*)bpb.oem)[i]);
    kprint("\n");
}

// 3. LS Command
void fat32_ls() {
    uint16_t buffer[256];
    ata_read_sector(get_lba_from_cluster(current_dir_cluster), buffer);
    struct fat32_entry* entry = (struct fat32_entry*)buffer;

    kprint("\nName        Type    Size\n");
    for (int i = 0; i < 16; i++) {
        if (entry[i].name[0] == 0x00) break;
        if (entry[i].name[0] == 0xE5 || entry[i].attributes == 0x0F) continue;

        for(int j=0; j<8; j++) if(entry[i].name[j] != ' ') put_char(entry[i].name[j]);
        if(entry[i].attributes & 0x10) kprint("    <DIR>");
        else {
            kprint(".");
            for(int j=0; j<3; j++) if(entry[i].ext[j] != ' ') put_char(entry[i].ext[j]);
            kprint("    FILE");
        }
        kprint("\n");
    }
}

// 4. CAT Command Logic
void fat32_cat(char* filename) {
    uint16_t buffer[256];
    ata_read_sector(get_lba_from_cluster(current_dir_cluster), buffer);
    struct fat32_entry* entry = (struct fat32_entry*)buffer;

    kprint("\n--- Content ---\n");

    for (int i = 0; i < 16; i++) {
        if (entry[i].name[0] == filename[0] || entry[i].name[0] == (filename[0] - 32)) {
            uint32_t cluster = (entry[i].cluster_high << 16) | entry[i].cluster_low;

            while (cluster < 0x0FFFFFF8 && cluster >= 2) {
                uint16_t file_buf[256];
                ata_read_sector(get_lba_from_cluster(cluster), file_buf);
                char* data = (char*)file_buf;

                for(int j = 0; j < 512; j++) {
                    if (data[j] == '\0') continue; // Skip nulls
                    put_char(data[j]);
                }
                cluster = fat32_get_next_cluster(cluster);
            }
            kprint("\n");
            return;
        }
    }
    kprint("File not found or empty.\n");
}

void fat32_cd(char* dirname) {
    uint16_t buffer[256];
    ata_read_sector(get_lba_from_cluster(current_dir_cluster), buffer);
    struct fat32_entry* entry = (struct fat32_entry*)buffer;

    // Convert first char of dirname to uppercase for matching
    char target = dirname[0];
    if (target >= 'a' && target <= 'z') target -= 32;

    for (int i = 0; i < 16; i++) {
        if (entry[i].name[0] == 0x00) break;

        // Check if first letter matches AND it's a directory (attribute 0x10)
        if (entry[i].name[0] == target && (entry[i].attributes & 0x10)) {
            // Update the global current_dir_cluster to the new folder's cluster
            current_dir_cluster = (entry[i].cluster_high << 16) | entry[i].cluster_low;

            kprint("Entered directory: ");
            kprint(dirname);
            kprint("\n");
            return;
        }
    }
    kprint("Directory not found or is a file.\n");
}
