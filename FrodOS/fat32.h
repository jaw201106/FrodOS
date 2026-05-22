#ifndef FAT32_H
#define FAT32_H

// Safe freestanding types using compiler builtins
typedef __UINT8_TYPE__   uint8_t;
typedef __UINT16_TYPE__  uint16_t;
typedef __UINT32_TYPE__  uint32_t;

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

__attribute__((packed)) struct fat32_bpb {
    uint8_t  jump_code[3];
    uint8_t  oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
};

__attribute__((packed)) struct fat32_entry {
    uint8_t  name[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved_win_nt;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_accessed_date;
    uint16_t cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;
    uint32_t file_size;
};

__attribute__((packed)) struct fat32_lfn_entry {
    uint8_t  sequence_num;     // Position in LFN chain (ORed with 0x40 for last entry)
    uint16_t name_part1[5];    // Chars 1-5 (Unicode)
    uint8_t  attributes;       // Always 0x0F
    uint8_t  type;             // Always 0x00
    uint8_t  checksum;         // Checksum matching 8.3 alias
    uint16_t name_part2[6];    // Chars 6-11 (Unicode)
    uint16_t first_cluster;    // Always 0x0000
    uint16_t name_part3[2];    // Chars 12-13 (Unicode)
};

// API Functions
void fat32_init(uint32_t lba);
void fat32_ls(void);
void fat32_cat(char* filename);
void fat32_cd(char* dirname);
uint32_t fat32_get_next_cluster(uint32_t cluster);

#endif
