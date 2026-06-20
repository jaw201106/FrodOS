#include "fat32.h"
#include "shell.h"

// External kernel dependencies from your video / shell / io layers
void put_char(char c);
void kprint(const char* str);
void kprint_int(int num);
int ata_read_sector(uint32_t lba, uint16_t* buffer);

// Internal state tracking
static uint32_t partition_start_lba = 0;
static struct fat32_bpb bpb;
static uint32_t current_dir_cluster = 0;

// Internal Helpers
static uint32_t get_lba_from_cluster(uint32_t cluster) {
    if (cluster < 2) return 0;
    uint32_t fat_start = partition_start_lba + bpb.reserved_sectors;
    uint32_t data_start = fat_start + (bpb.fat_count * bpb.fat_size_32);
    return ((cluster - 2) * bpb.sectors_per_cluster) + data_start;
}

static void internal_memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) d[i] = s[i];
}

static int str_case_cmp(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
        if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

// Fixed Partition LBA Cluster calculation
uint32_t fat32_get_next_cluster(uint32_t cluster) {
    uint32_t absolute_fat_start = partition_start_lba + bpb.reserved_sectors;
    uint32_t fat_sector = absolute_fat_start + (cluster / 128);
    uint32_t fat_offset = (cluster % 128);
    __attribute__((aligned(4))) uint8_t sector_buf[512];

    if (ata_read_sector(fat_sector, (uint16_t*)sector_buf) != 0) {
        kprint("FAT32 Error: Failed to read FAT table sector.\n");
        return 0x0FFFFFF8; // Return end of chain on hardware failure
    }
    uint32_t* fat_table = (uint32_t*)sector_buf;
    return fat_table[fat_offset] & 0x0FFFFFFF;
}

// Reconstructs Unicode bytes safely down to an ASCII buffer slot
static void parse_lfn_chunk(struct fat32_lfn_entry* lfn, char* lfn_name_buf) {
    int index = (lfn->sequence_num & 0x1F) - 1;
    if (index < 0 || index >= 20) return; // Buffer protection boundary
    int char_offset = index * 13;

    for (int i = 0; i < 5; i++) {
        uint16_t u = lfn->name_part1[i];
        if (u == 0x0000 || u == 0xFFFF) return;
        lfn_name_buf[char_offset++] = (char)(u & 0xFF);
    }
    for (int i = 0; i < 6; i++) {
        uint16_t u = lfn->name_part2[i];
        if (u == 0x0000 || u == 0xFFFF) return;
        lfn_name_buf[char_offset++] = (char)(u & 0xFF);
    }
    for (int i = 0; i < 2; i++) {
        uint16_t u = lfn->name_part3[i];
        if (u == 0x0000 || u == 0xFFFF) return;
        lfn_name_buf[char_offset++] = (char)(u & 0xFF);
    }
}

static void get_short_name(struct fat32_entry* entry, char* out_buf) {
    int p = 0;
    for (int i = 0; i < 8; i++) {
        if (entry->name[i] != ' ') out_buf[p++] = (char)entry->name[i];
    }
    if (!(entry->attributes & ATTR_DIRECTORY)) {
        out_buf[p++] = '.';
        for (int i = 0; i < 3; i++) {
            if (entry->ext[i] != ' ') out_buf[p++] = (char)entry->ext[i];
        }
        if (out_buf[p - 1] == '.') p--;
    }
    out_buf[p] = '\0';
}

/* HIGH-VALUE REFACTOR: Shared entry lookup module 
   Fills out 'found_entry' if a filename matches target.
*/
static int fat32_find_entry(uint32_t dir_cluster, const char* target_name, struct fat32_entry* found_entry) {
    uint32_t cluster = dir_cluster;
    __attribute__((aligned(4))) uint8_t sector_buf[512];
    char current_lfn[256] = {0};
    int has_lfn = 0;
    uint32_t safety_loop = 65536; // Protects kernel from infinite allocation loops

    while (cluster < 0x0FFFFFF8 && cluster >= 2 && safety_loop--) {
        uint32_t start_lba = get_lba_from_cluster(cluster);

        for (uint32_t sector = 0; sector < bpb.sectors_per_cluster; sector++) {
            if (ata_read_sector(start_lba + sector, (uint16_t*)sector_buf) != 0) {
                kprint("FAT32 Error: Directory sector read failed.\n");
                return 0;
            }
            struct fat32_entry* entries = (struct fat32_entry*)sector_buf;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) return 0; // Absolute End-Of-Directory
                
                if (entries[i].name[0] == 0xE5) { // Deleted Item
                    has_lfn = 0;
                    for (int j = 0; j < 256; j++) current_lfn[j] = '\0';
                    continue;
                }

                if (entries[i].attributes == ATTR_LONG_NAME) {
                    struct fat32_lfn_entry* lfn = (struct fat32_lfn_entry*)&entries[i];
                    parse_lfn_chunk(lfn, current_lfn);
                    has_lfn = 1;
                    continue;
                }

                if (entries[i].attributes & ATTR_VOLUME_ID) {
                    has_lfn = 0;
                    continue;
                }

                char short_name[13];
                get_short_name(&entries[i], short_name);

                if ((has_lfn && str_case_cmp(current_lfn, target_name) == 0) ||
                    (str_case_cmp(short_name, target_name) == 0)) {
                    internal_memcpy(found_entry, &entries[i], sizeof(struct fat32_entry));
                    return 1; // Found!
                }

                for (int idx = 0; idx < 256; idx++) current_lfn[idx] = '\0';
                has_lfn = 0;
            }
        }
        cluster = fat32_get_next_cluster(cluster);
    }
    return 0; // Not Found
}

// Initialize File System Context with expanded validation metrics
void fat32_init(uint32_t lba) {
    partition_start_lba = lba;
    __attribute__((aligned(4))) uint8_t sector_buf[512];

    if (ata_read_sector(partition_start_lba, (uint16_t*)sector_buf) != 0) {
        kprint("FAT32 Error: Hard Drive Boot sector unreadable.\n");
        return;
    }
    internal_memcpy(&bpb, sector_buf, sizeof(struct fat32_bpb));

    // Hardened Structural Sanity Bounds Validation
    if ((bpb.boot_signature != 0x29 && bpb.boot_signature != 0x28) ||
        bpb.bytes_per_sector != 512 || bpb.sectors_per_cluster == 0 || bpb.root_cluster < 2) {
        kprint("Error: Invalid or incompatible FAT32 filesystem parameters detected.\n");
        return;
    }

    current_dir_cluster = bpb.root_cluster;
    kprint("\n--- FAT32 Upgraded Driver Initialized Successfully ---\n");
}

// Upgraded LS Command
void fat32_ls(void) {
    uint32_t cluster = current_dir_cluster;
    __attribute__((aligned(4))) uint8_t sector_buf[512];
    char current_lfn[256] = {0};
    int has_lfn = 0;
    uint32_t safety = 65536;

    kprint("\nType    Size(Bytes)   Name\n");
    kprint("----------------------------------------\n");

    while (cluster < 0x0FFFFFF8 && cluster >= 2 && safety--) {
        uint32_t start_lba = get_lba_from_cluster(cluster);

        for (uint32_t sector = 0; sector < bpb.sectors_per_cluster; sector++) {
            if (ata_read_sector(start_lba + sector, (uint16_t*)sector_buf) != 0) return;
            struct fat32_entry* entries = (struct fat32_entry*)sector_buf;

            for (int i = 0; i < 16; i++) {
                if (entries[i].name[0] == 0x00) return; 
                if (entries[i].name[0] == 0xE5) {
                    has_lfn = 0;
                    for (int j = 0; j < 256; j++) current_lfn[j] = '\0';
                    continue;
                }

                if (entries[i].attributes == ATTR_LONG_NAME) {
                    struct fat32_lfn_entry* lfn = (struct fat32_lfn_entry*)&entries[i];
                    parse_lfn_chunk(lfn, current_lfn);
                    has_lfn = 1;
                    continue;
                }

                if (entries[i].attributes & ATTR_VOLUME_ID) {
                    has_lfn = 0;
                    continue;
                }

                if (entries[i].attributes & ATTR_DIRECTORY) kprint("<DIR>   -             ");
                else {
                    kprint("FILE    ");
                    kprint_int((int)entries[i].file_size);
                    kprint("             ");
                }

                if (has_lfn && current_lfn[0] != '\0') {
                    kprint(current_lfn);
                } else {
                    char short_name[13];
                    get_short_name(&entries[i], short_name);
                    kprint(short_name);
}
                kprint("\n");

                for (int idx = 0; idx < 256; idx++) current_lfn[idx] = '\0';
                has_lfn = 0;
            }
        }
        cluster = fat32_get_next_cluster(cluster);
    }
}

// Cleaned up CAT command using the unified helper block interface
void fat32_cat(char* filename) {
    struct fat32_entry file_entry;
    
    if (!fat32_find_entry(current_dir_cluster, filename, &file_entry)) {
        kprint("File not found.\n");
        return;
    }

    if (file_entry.attributes & ATTR_DIRECTORY) {
        kprint("Error: Target path name is a directory.\n");
        return;
    }

    uint32_t file_cluster = ((uint32_t)file_entry.cluster_high << 16) | file_entry.cluster_low;
    uint32_t bytes_remaining = file_entry.file_size;
    uint32_t safety = 65536;

    kprint("\n--- File Content ---\n");
    while (file_cluster < 0x0FFFFFF8 && file_cluster >= 2 && bytes_remaining > 0 && safety--) {
        uint32_t file_lba = get_lba_from_cluster(file_cluster);

        for (uint32_t s = 0; s < bpb.sectors_per_cluster && bytes_remaining > 0; s++) {
            __attribute__((aligned(4))) uint8_t file_buf[512];
            if (ata_read_sector(file_lba + s, (uint16_t*)file_buf) != 0) return;

            uint32_t chunk = (bytes_remaining > 512) ? 512 : bytes_remaining;
            for (uint32_t j = 0; j < chunk; j++) {
                put_char((char)file_buf[j]);
            }
            bytes_remaining -= chunk;
        }
        file_cluster = fat32_get_next_cluster(file_cluster);
    }
    kprint("\n");
}

// Cleaned up CD command using the unified helper block interface
void fat32_cd(char* dirname) {
    if (dirname[0] == '/' && dirname[1] == '\0') {
        current_dir_cluster = bpb.root_cluster;
        kprint("Returned to root.\n");
        return;
    }

    struct fat32_entry dir_entry;
    if (!fat32_find_entry(current_dir_cluster, dirname, &dir_entry)) {
        kprint("Directory not found.\n");
        return;
    }

    if (!(dir_entry.attributes & ATTR_DIRECTORY)) {
        kprint("Error: Target path name is a file.\n");
        return;
    }

    uint32_t target_cluster = ((uint32_t)dir_entry.cluster_high << 16) | dir_entry.cluster_low;
    current_dir_cluster = (target_cluster == 0) ? bpb.root_cluster : target_cluster;
    kprint("Navigated successfully.\n");
}
