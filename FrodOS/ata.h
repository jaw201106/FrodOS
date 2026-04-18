#ifndef ATA_H
#define ATA_H

// Manually define types if stdint.h isn't available
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// Standard Ports
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_SEL   0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7

#define ATA_CMD_READ    0x20
#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRQ  0x08

void ata_read_sector(uint32_t lba, uint16_t* buffer);

#endif
