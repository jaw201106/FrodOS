#include "ramfs.h"

static uintptr_t ramfs_start = 0;
static uintptr_t ramfs_end = 0;

// Simple custom strcmp since standard library might not be fully present
static int ramfs_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Converts an ASCII octal string from the TAR header to an integer
static size_t octal_to_int(const char* str, int max_chars) {
    size_t result = 0;
    for (int i = 0; i < max_chars; i++) {
        // TAR fields are padded with spaces or nulls
        if (str[i] == '\0' || str[i] == ' ') {
            break;
        }
        if (str[i] < '0' || str[i] > '7') {
            continue;
        }
        result = (result << 3) + (str[i] - '0');
    }
    return result;
}

void ramfs_init(uintptr_t initrd_start, uintptr_t initrd_end) {
    ramfs_start = initrd_start;
    ramfs_end = initrd_end;
}

uint8_t* ramfs_read(const char* filename, size_t* out_size) {
    if (ramfs_start == 0 || ramfs_start >= ramfs_end) {
        return NULL;
    }

    uintptr_t current_address = ramfs_start;

    while (current_address < ramfs_end) {
        tar_header_t* header = (tar_header_t*)current_address;

        // An empty filename means we reached the end of the archive stream
        if (header->filename[0] == '\0') {
            break;
        }

        // Parse file size from the header (12-byte octal field)
        size_t file_size = octal_to_int(header->size, 12);

        // Check if this is the file we are looking for (typeflag '0' or '\0' is a regular file)
        if ((header->typeflag == '0' || header->typeflag == '\0') &&
            ramfs_strcmp(header->filename, filename) == 0) {

            if (out_size) {
                *out_size = file_size;
            }
            // The actual file contents begin right after the 512-byte header
            return (uint8_t*)(current_address + 512);
        }

        // Jump past the header + the file contents.
        // TAR files pad file data to the next 512-byte boundary.
        size_t aligned_size = ((file_size + 511) / 512) * 512;
        current_address += 512 + aligned_size;
    }

    return NULL; // File not found
}
