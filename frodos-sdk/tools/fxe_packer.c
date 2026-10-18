#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../fxe.h"
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: ./fxe_packer <text.bin> <data.bin> <output.fxe>\n");
        return 1;
    }
    FILE* f_text = fopen(argv[1], "rb");
    FILE* f_data = fopen(argv[2], "rb");
    FILE* f_out  = fopen(argv[3], "wb");
    if (!f_text || !f_out) {
        printf("Error opening input/output files.\n");
        if (f_text) fclose(f_text);
        if (f_data) fclose(f_data);
        if (f_out) fclose(f_out);
        return 1;
    }
    fseek(f_text, 0, SEEK_END);
    uint32_t text_sz = ftell(f_text);
    fseek(f_text, 0, SEEK_SET);

    uint32_t data_sz = 0;
    if (f_data && strcmp(argv[2], "none") != 0) {
        fseek(f_data, 0, SEEK_END);
        data_sz = ftell(f_data);
        fseek(f_data, 0, SEEK_SET);
    }

    fxe_header_t header;
    header.magic[0] = FXE_MAGIC_0;
    header.magic[1] = FXE_MAGIC_1;
    header.magic[2] = FXE_MAGIC_2;
    header.magic[3] = FXE_MAGIC_3;
    header.entry_point = 0x6000000;
    header.text_offset = sizeof(fxe_header_t);
    header.text_size   = text_sz;
    header.data_offset = sizeof(fxe_header_t) + text_sz;
    header.data_size   = data_sz;
    header.bss_size    = 4096;
    header.stack_size  = 16384;

    fwrite(&header, 1, sizeof(fxe_header_t), f_out);
    uint8_t* buf = malloc(text_sz);
    if (buf) {
        size_t read_bytes = fread(buf, 1, text_sz, f_text);
        (void)read_bytes; // Suppress compiler warning safely
        fwrite(buf, 1, text_sz, f_out);
        free(buf);
    }

    if (data_sz > 0 && f_data) {
        buf = malloc(data_sz);
        if (buf) {
            size_t read_bytes = fread(buf, 1, data_sz, f_data);
            (void)read_bytes;
            fwrite(buf, 1, data_sz, f_out);
            free(buf);
        }
    }
    fclose(f_text); if (f_data) fclose(f_data); fclose(f_out);
    printf("[PACKER] Generated: %s\n", argv[3]);
    return 0;
}
