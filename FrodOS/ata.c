#include "ata.h"

// Hardware Glue for ata.c
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ( "inw %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

static void ata_wait_bsy() {
    while (inb(ATA_STATUS) & ATA_STATUS_BSY);
}

static void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ));
}

void ata_read_sector(uint32_t lba, uint16_t* buffer) {
    ata_wait_bsy();
    outb(ATA_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)lba);
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND,  ATA_CMD_READ);

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_DATA);
    }
}
