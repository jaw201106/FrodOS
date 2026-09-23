#include "ata.h"

// 1. Hardware Glue
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

// Forces a 400-nanosecond hardware delay by reading the Alternate Status Register 4 times
static inline void ata_io_delay(void) {
    inb(ATA_REG_ALT_STATUS);
    inb(ATA_REG_ALT_STATUS);
    inb(ATA_REG_ALT_STATUS);
    inb(ATA_REG_ALT_STATUS);
}

// 2. Status Tracking Helpers with Loop Protections
static int ata_wait_ready(void) {
    uint32_t timeout = 100000; // Fast spin loop threshold
    while (timeout--) {
        uint8_t status = inb(ATA_REG_STATUS);

        // Return failure immediately if device fault or error flags trigger
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        // Success if Busy dropped and Ready bit rose
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY)) {
            return 0;
        }
    }
    kprint("ATA Error: Device Ready timeout expired.\n");
    return -2; // Device hang
}

static int ata_wait_drq(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ATA_REG_STATUS);

        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        // Success if Busy cleared and Data Request is high
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) {
            return 0;
        }
    }
    kprint("ATA Error: Data Request (DRQ) timeout expired.\n");
    return -2;
}

// 3. Robust Sector Reading Core Engine
int ata_read_sector(uint32_t lba, uint16_t* buffer) {
    // 1. Wait for controller to finish any previous hanging work
    if (ata_wait_ready() < 0) return -1;

    // 2. Select master drive (0xE0) and fill top 4 bits of 28-bit LBA addressing
    outb(ATA_REG_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    ata_io_delay(); // Give hardware time to adjust lines

    // 3. Pass processing parameters
    outb(ATA_REG_SECCOUNT, 1);
    outb(ATA_REG_LBA_LOW,  (uint8_t)lba);
    outb(ATA_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));

    // 4. Issue the read sector command execution line
    outb(ATA_REG_COMMAND,  ATA_CMD_READ);
    ata_io_delay(); // Mandatory 400ns hardware absorption window

    // 5. Wait for the hardware buffer data pool to populate
    if (ata_wait_drq() < 0) {
        kprint("ATA Error: Read operation aborted by device.\n");
        return -1;
    }

    // 6. Safely transfer the data stream chunk from PIO port
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_REG_DATA);
    }

    return 0; // Success
}

// 4. Extended Capability: Sector Writing Core Engine
int ata_write_sector(uint32_t lba, const uint16_t* buffer) {
    if (ata_wait_ready() < 0) return -1;

    outb(ATA_REG_DRIVE_SEL, 0xE0 | ((lba >> 24) & 0x0F));
    ata_io_delay();

    outb(ATA_REG_SECCOUNT, 1);
    outb(ATA_REG_LBA_LOW,  (uint8_t)lba);
    outb(ATA_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));

    outb(ATA_REG_COMMAND,  ATA_CMD_WRITE);
    ata_io_delay();

    // For writes, we must wait for DRQ *before* pushing data to disk
    if (ata_wait_drq() < 0) {
        kprint("ATA Error: Write operation aborted by device.\n");
        return -1;
    }

    for (int i = 0; i < 256; i++) {
        outb(ATA_REG_DATA, buffer[i]);
    }

    // Flush cache to ensure data hits physical platters/emulation block
    outb(ATA_REG_COMMAND, 0xE7); // Cache Flush Command
    ata_io_delay();
    ata_wait_ready();

    return 0;
}
