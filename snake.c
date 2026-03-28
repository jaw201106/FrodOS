extern unsigned char keyboard_map[128];
// snake.c - Slow controllable snake
#include "snake.h"
#include "io.h"

static unsigned short* vga = (unsigned short*)0xB8000;
static int snake_x[100], snake_y[100];
static int snake_len;
static int dir;
static int food_x, food_y;
static int score;
static int gameover;

static unsigned int seed = 1;
int my_rand() {
    seed = seed * 1103515245 + 12345;
    return (unsigned int)(seed / 65536) % 32768;
}

void draw_char(int x, int y, char c, char color) {
    vga[y * 80 + x] = (unsigned short)c | (color << 8);
}

// Super slow delay
void super_slow_delay() {
    for(volatile int i = 0; i < 3000000; i++);
}

void init_game() {
    // Clear screen
    for(int i = 0; i < 80*25; i++) {
        vga[i] = (unsigned short)' ' | (0x07 << 8);
    }
    
    // Draw border
    for(int x = 5; x < 75; x++) {
        draw_char(x, 5, '#', 0x0C);
        draw_char(x, 20, '#', 0x0C);
    }
    for(int y = 5; y <= 20; y++) {
        draw_char(5, y, '#', 0x0C);
        draw_char(74, y, '#', 0x0C);
    }
    
    // Snake in center
    snake_len = 3;
    dir = 0;
    snake_x[0] = 40; snake_y[0] = 12;
    snake_x[1] = 39; snake_y[1] = 12;
    snake_x[2] = 38; snake_y[2] = 12;
    
    // Draw snake
    draw_char(snake_x[0], snake_y[0], 'O', 0x0A);
    draw_char(snake_x[1], snake_y[1], 'o', 0x0A);
    draw_char(snake_x[2], snake_y[2], 'o', 0x0A);
    
    // Food
    food_x = 50;
    food_y = 12;
    draw_char(food_x, food_y, 'F', 0x0E);
    
    score = 0;
    gameover = 0;
    
    // Display
    draw_char(60, 2, 'W', 0x0F);
    draw_char(61, 2, 'A', 0x0F);
    draw_char(62, 2, 'S', 0x0F);
    draw_char(63, 2, 'D', 0x0F);
    draw_char(64, 2, '=', 0x0F);
    draw_char(65, 2, 'M', 0x0F);
    draw_char(66, 2, 'O', 0x0F);
    draw_char(67, 2, 'V', 0x0F);
    draw_char(68, 2, 'E', 0x0F);
    
    draw_char(60, 4, 'S', 0x0F);
    draw_char(61, 4, 'c', 0x0F);
    draw_char(62, 4, 'o', 0x0F);
    draw_char(63, 4, 'r', 0x0F);
    draw_char(64, 4, 'e', 0x0F);
    draw_char(65, 4, ':', 0x0F);
    draw_char(66, 4, '0', 0x0F);
}

void update_score() {
    int tens = score / 10;
    int ones = score % 10;
    draw_char(66, 4, '0' + tens, 0x0F);
    draw_char(67, 4, '0' + ones, 0x0F);
}

void generate_food() {
    int valid;
    do {
        valid = 1;
        food_x = (my_rand() % 68) + 6;
        food_y = (my_rand() % 14) + 6;
        
        for(int i = 0; i < snake_len; i++) {
            if(snake_x[i] == food_x && snake_y[i] == food_y) {
                valid = 0;
                break;
            }
        }
    } while(!valid);
    
    draw_char(food_x, food_y, 'F', 0x0E);
}

void move_snake() {
    int new_x = snake_x[0];
    int new_y = snake_y[0];
    
    switch(dir) {
        case 0: new_x++; break;
        case 1: new_y++; break;
        case 2: new_x--; break;
        case 3: new_y--; break;
    }
    
    // Wall collision
    if(new_x <= 5 || new_x >= 74 || new_y <= 5 || new_y >= 20) {
        gameover = 1;
        draw_char(35, 12, 'G', 0x0C);
        draw_char(36, 12, 'A', 0x0C);
        draw_char(37, 12, 'M', 0x0C);
        draw_char(38, 12, 'E', 0x0C);
        draw_char(39, 12, ' ', 0x0C);
        draw_char(40, 12, 'O', 0x0C);
        draw_char(41, 12, 'V', 0x0C);
        draw_char(42, 12, 'E', 0x0C);
        draw_char(43, 12, 'R', 0x0C);
        draw_char(35, 14, 'P', 0x0E);
        draw_char(36, 14, 'R', 0x0E);
        draw_char(37, 14, 'E', 0x0E);
        draw_char(38, 14, 'S', 0x0E);
        draw_char(39, 14, 'S', 0x0E);
        draw_char(40, 14, ' ', 0x0E);
        draw_char(41, 14, 'R', 0x0E);
        draw_char(42, 14, ' ', 0x0E);
        draw_char(43, 14, 'T', 0x0E);
        draw_char(44, 14, 'O', 0x0E);
        draw_char(45, 14, ' ', 0x0E);
        draw_char(46, 14, 'R', 0x0E);
        draw_char(47, 14, 'E', 0x0E);
        draw_char(48, 14, 'S', 0x0E);
        draw_char(49, 14, 'T', 0x0E);
        draw_char(50, 14, 'A', 0x0E);
        draw_char(51, 14, 'R', 0x0E);
        draw_char(52, 14, 'T', 0x0E);
        return;
    }
    
    int ate = (new_x == food_x && new_y == food_y);
    
    if(ate) {
        // Grow
        for(int i = snake_len; i > 0; i--) {
            snake_x[i] = snake_x[i-1];
            snake_y[i] = snake_y[i-1];
        }
        snake_x[0] = new_x;
        snake_y[0] = new_y;
        snake_len++;
        score += 10;
        update_score();
        generate_food();
    } else {
        // Move - clear tail
        int old_x = snake_x[snake_len-1];
        int old_y = snake_y[snake_len-1];
        draw_char(old_x, old_y, ' ', 0x07);
        
        for(int i = snake_len-1; i > 0; i--) {
            snake_x[i] = snake_x[i-1];
            snake_y[i] = snake_y[i-1];
        }
        snake_x[0] = new_x;
        snake_y[0] = new_y;
    }
    
    // Self collision
    for(int i = 1; i < snake_len; i++) {
        if(snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
            gameover = 1;
            draw_char(35, 12, 'G', 0x0C);
            draw_char(36, 12, 'A', 0x0C);
            draw_char(37, 12, 'M', 0x0C);
            draw_char(38, 12, 'E', 0x0C);
            draw_char(39, 12, ' ', 0x0C);
            draw_char(40, 12, 'O', 0x0C);
            draw_char(41, 12, 'V', 0x0C);
            draw_char(42, 12, 'E', 0x0C);
            draw_char(43, 12, 'R', 0x0C);
            return;
        }
    }
    
    // Redraw snake
    for(int i = 0; i < snake_len; i++) {
        char c = (i == 0) ? 'O' : 'o';
        draw_char(snake_x[i], snake_y[i], c, 0x0A);
    }
}

void start_snake_game() {
    // Save screen
    unsigned short saved[80*25];
    for(int i = 0; i < 80*25; i++) {
        saved[i] = vga[i];
    }
    
    init_game();
    int running = 1;
    int move_counter = 0;
    
    while(running) {
        // Keyboard input
        if(inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if(!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                
                if(!gameover) {
                    if(c == 'w' || c == 'W') {
                        if(dir != 1) dir = 3;
                    } else if(c == 's' || c == 'S') {
                        if(dir != 3) dir = 1;
                    } else if(c == 'a' || c == 'A') {
                        if(dir != 0) dir = 2;
                    } else if(c == 'd' || c == 'D') {
                        if(dir != 2) dir = 0;
                    } else if(c == 27) {
                        running = 0;
                    }
                } else {
                    if(c == 'r' || c == 'R') {
                        init_game();
                        gameover = 0;
                        move_counter = 0;
                    } else if(c == 27) {
                        running = 0;
                    }
                }
            }
        }
        
        // Move snake - very slow
        if(!gameover) {
            move_counter++;
            if(move_counter > 30) {  // Even slower movement
                move_counter = 0;
                move_snake();
            }
        }
        
        // Super slow delay
        super_slow_delay();
    }
    
    // Restore screen
    for(int i = 0; i < 80*25; i++) {
        vga[i] = saved[i];
    }
}
