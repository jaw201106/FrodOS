#ifndef MBR_H
#define MBR_H

typedef __UINT8_TYPE__   uint8_t;
typedef __UINT16_TYPE__  uint16_t;
typedef __UINT32_TYPE__  uint32_t;

__attribute__((packed)) struct mbr_partition {
    uint8_t  boot_indicator;
    uint8_t  starting_chs[3]; // MUST be an array of 3, NOT a single byte!
    uint8_t  sys_id;
    uint8_t  ending_chs[3];   // MUST be an array of 3, NOT a single byte!
    uint32_t start_lba;
    uint32_t total_sectors;
}; // Exactly 16 bytes

__attribute__((packed)) struct master_boot_record {
    uint8_t              bootstrap_code[446]; // MUST be an array of 446!
    struct mbr_partition partitions[4];       // MUST be an array of 4!
    uint16_t             boot_signature;      // 2 bytes. Total = 446 + 64 + 2 = Exactly 512
};

uint32_t mbr_parse(void);

#endif
