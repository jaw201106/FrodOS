#include "ramfs.h"

static uintptr_t ramfs_start = 0;
static uintptr_t ramfs_end = 0;

// Bounded string comparison to prevent walking off unmapped memory
static int ramfs_strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;

    while (n > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Converts an ASCII octal string from the TAR header to an integer safely
static size_t octal_to_int(const char* str, int max_chars) {
    size_t result = 0;

    // Size_t max value sanity check threshold to prevent parsing ridiculous overflows
    const size_t max_threshold = ((size_t)-1) >> 3;

    for (int i = 0; i < max_chars; i++) {
        // TAR fields are padded with spaces or nulls
        if (str[i] == '\0' || str[i] == ' ') {
            break;
        }
        if (str[i] < '0' || str[i] > '7') {
            // Strictly fail or continue. Continuing mirrors traditional implementations,
            // but returning 0 or a failure status on corrupted character is safer.
            continue;
        }

        if (result > max_threshold) {
            return 0; // Prevent integer wrap-around on corrupt header sizes
        }

        result = (result << 3) + (str[i] - '0');
    }
    return result;
}

void ramfs_init(uintptr_t initrd_start, uintptr_t initrd_end) {
    if (initrd_start >= initrd_end) {
        ramfs_start = 0;
        ramfs_end = 0;
        return;
    }
    ramfs_start = initrd_start;
    ramfs_end = initrd_end;
}

uint8_t* ramfs_read(const char* filename, size_t* out_size) {
    if (ramfs_start == 0 || ramfs_end <= ramfs_start || filename == NULL) {
        return NULL;
    }

    uintptr_t current_address = ramfs_start;

    // Ensure we have at least room to fit one full header sector
    while (current_address + 512 <= ramfs_end) {
        tar_header_t* header = (tar_header_t*)current_address;

        // An empty filename means we reached the end of the archive stream
        if (header->filename[0] == '\0') {
            break;
        }

        // Parse file size safely from the header (12-byte octal field)
        size_t file_size = octal_to_int(header->size, 12);

        // Check if this is the file we are looking for safely limiting comparison length
        if ((header->typeflag == '0' || header->typeflag == '\0') &&
            ramfs_strncmp(header->filename, filename, sizeof(header->filename)) == 0) {

            // Ensure the data payload fits entirely within the RAMFS memory boundaries
            if (current_address + 512 + file_size > ramfs_end) {
                return NULL; // Corrupt payload size boundary overrun
            }

            if (out_size) {
                *out_size = file_size;
            }
            return (uint8_t*)(current_address + 512);
            }

            // Calculate size aligned to 512 byte boundaries
            // Handled cleanly via masking to prevent overflow integer traps
            size_t aligned_size = (file_size + 511) & ~(size_t)511;

            // Overflow check: make sure current_address doesn't wrap around or jump bounds
            uintptr_t next_address = current_address + 512 + aligned_size;
            if (next_address <= current_address || next_address > ramfs_end) {
                break; // Terminate tracking to prevent out-of-bounds indexing loops
            }

            current_address = next_address;
    }

    return NULL; // File not found or corrupt filesystem context
}
