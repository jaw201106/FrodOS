// fs.h - Simple file system
#ifndef FS_H
#define FS_H

#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
} file_t;

typedef struct {
    file_t files[MAX_FILES];
    int file_count;
} filesystem_t;

extern filesystem_t fs;

void fs_init();
int fs_create(const char* name);
int fs_write(const char* name, const char* data);
int fs_read(const char* name, char* buffer, int max_size);
void fs_list();
int fs_delete(const char* name);

#endif