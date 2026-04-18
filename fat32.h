#ifndef FAT32_H
#define FAT32_H

#include "ata.h"

struct fat32_bpb {
    uint8_t  jmp;
    uint8_t  oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_ver;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot;
    uint8_t  res[12];
    uint8_t  drive_num;
    uint8_t  res1;
    uint8_t  boot_sig;
    uint32_t vol_id;
    uint8_t  vol_label[11];
    uint8_t  fs_type[8];
} __attribute__((packed));

struct fat32_entry {
    uint8_t  name[8];
    uint8_t  ext[3];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  creation_time_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t cluster_high;
    uint16_t mod_time;
    uint16_t mod_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed));

void fat32_init(uint32_t lba);
void fat32_ls();
void fat32_cd(char* dirname);
void fat32_cat(char* filename);

#endif
