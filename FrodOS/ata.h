#ifndef ATA_H
#define ATA_H

// Safe freestanding type wrappers for your compiler setup
typedef __UINT8_TYPE__   uint8_t;
typedef __UINT16_TYPE__  uint16_t;
typedef __UINT32_TYPE__  uint32_t;

// Standard IO Ports (Primary Channel)
#define ATA_REG_DATA          0x1F0
#define ATA_REG_ERROR         0x1F1
#define ATA_REG_SECCOUNT      0x1F2
#define ATA_REG_LBA_LOW       0x1F3
#define ATA_REG_LBA_MID       0x1F4
#define ATA_REG_LBA_HIGH      0x1F5
#define ATA_REG_DRIVE_SEL     0x1F6
#define ATA_REG_STATUS        0x1F7
#define ATA_REG_COMMAND       0x1F7
#define ATA_REG_ALT_STATUS    0x3F6

// Status Register Bits
#define ATA_STATUS_ERR        0x01
#define ATA_STATUS_DRQ        0x08
#define ATA_STATUS_DF         0x20
#define ATA_STATUS_RDY        0x40
#define ATA_STATUS_BSY        0x80

// Commands
#define ATA_CMD_READ          0x20
#define ATA_CMD_WRITE         0x30

// External dependency for error reporting
void kprint(const char* str);

// !!! ADD THESE LINES AT THE BOTTOM OF YOUR ATA.H !!!
int ata_read_sector(uint32_t lba, uint16_t* buffer);

#endif
