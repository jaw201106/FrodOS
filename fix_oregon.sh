#!/bin/bash
cd ~/FrodOS

# Add extern keyboard_map to oregon.c
if ! grep -q "extern unsigned char keyboard_map" oregon.c; then
    sed -i '1i extern unsigned char keyboard_map[128];' oregon.c
fi

# Add extern keyboard_map to snake.c
if ! grep -q "extern unsigned char keyboard_map" snake.c; then
    sed -i '1i extern unsigned char keyboard_map[128];' snake.c
fi

# Add extern to kernel.c if needed
if ! grep -q "extern void print_welcome" kernel.c; then
    sed -i '1i extern void print_welcome();\nextern void launch_shell();' kernel.c
fi
