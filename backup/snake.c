// snake.c - Snake game for FrodOS
#include "snake.h"
#include "shell.h"

#define WIDTH 60
#define HEIGHT 20
#define MAX_SNAKE 100

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position body[MAX_SNAKE];
    int length;
    int direction; // 0=right, 1=down, 2=left, 3=up
} Snake;

typedef struct {
    int x, y;
    int active;
} Food;

static Snake snake;
static Food food;
static int score;
static int game_over;

// VGA buffer
static unsigned short* vga = (unsigned short*)0xB8000;

// Draw border
void draw_border() {
    // Top border
    for(int i = 0; i < WIDTH + 2; i++) {
        vga[0 * 80 + i] = (unsigned short)'#' | (0x0C << 8);
    }
    
    // Bottom border
    for(int i = 0; i < WIDTH + 2; i++) {
        vga[(HEIGHT + 1) * 80 + i] = (unsigned short)'#' | (0x0C << 8);
    }
    
    // Left and right borders
    for(int i = 1; i <= HEIGHT; i++) {
        vga[i * 80 + 0] = (unsigned short)'#' | (0x0C << 8);
        vga[i * 80 + WIDTH + 1] = (unsigned short)'#' | (0x0C << 8);
    }
}

// Draw snake
void draw_snake() {
    for(int i = 0; i < snake.length; i++) {
        int pos = (snake.body[i].y + 1) * 80 + (snake.body[i].x + 1);
        if(i == 0) {
            // Snake head
            vga[pos] = (unsigned short)'O' | (0x0A << 8);
        } else {
            // Snake body
            vga[pos] = (unsigned short)'o' | (0x0A << 8);
        }
    }
}

// Draw food
void draw_food() {
    if(food.active) {
        int pos = (food.y + 1) * 80 + (food.x + 1);
        vga[pos] = (unsigned short)'F' | (0x0E << 8);
    }
}

// Generate new food
void generate_food() {
    do {
        food.x = (rand() % (WIDTH - 2)) + 1;
        food.y = (rand() % (HEIGHT - 2)) + 1;
        food.active = 1;
        
        // Check if food spawns on snake
        int collision = 0;
        for(int i = 0; i < snake.length; i++) {
            if(snake.body[i].x == food.x && snake.body[i].y == food.y) {
                collision = 1;
                break;
            }
        }
        if(!collision) break;
    } while(1);
}

// Initialize game
void init_game() {
    // Clear game area
    for(int y = 1; y <= HEIGHT; y++) {
        for(int x = 1; x <= WIDTH; x++) {
            vga[y * 80 + x] = (unsigned short)' ' | (0x07 << 8);
        }
    }
    
    // Initialize snake
    snake.length = 3;
    snake.direction = 0; // right
    snake.body[0].x = 5;
    snake.body[0].y = HEIGHT / 2;
    snake.body[1].x = 4;
    snake.body[1].y = HEIGHT / 2;
    snake.body[2].x = 3;
    snake.body[2].y = HEIGHT / 2;
    
    score = 0;
    game_over = 0;
    
    generate_food();
    draw_border();
    draw_snake();
    draw_food();
    
    // Show score
    const char* score_text = "Score: 0";
    for(int i = 0; score_text[i]; i++) {
        vga[0 * 80 + WIDTH + 5 + i] = (unsigned short)score_text[i] | (0x0F << 8);
    }
}

// Simple random number generator
static unsigned int rand_seed = 1;
int rand() {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (unsigned int)(rand_seed / 65536) % 32768;
}

// Move snake
int move_snake() {
    Position new_head = snake.body[0];
    
    switch(snake.direction) {
        case 0: new_head.x++; break; // right
        case 1: new_head.y++; break; // down
        case 2: new_head.x--; break; // left
        case 3: new_head.y--; break; // up
    }
    
    // Check collision with walls
    if(new_head.x < 1 || new_head.x > WIDTH || new_head.y < 1 || new_head.y > HEIGHT) {
        return 0; // Game over
    }
    
    // Check if food is eaten
    int ate_food = (new_head.x == food.x && new_head.y == food.y);
    
    // Move snake
    if(ate_food) {
        // Add new head and keep tail
        for(int i = snake.length; i > 0; i--) {
            snake.body[i] = snake.body[i-1];
        }
        snake.body[0] = new_head;
        snake.length++;
        
        // Update score
        score += 10;
        char score_str[20];
        int si = 0;
        int temp = score;
        if(temp == 0) {
            score_str[si++] = '0';
        } else {
            int digits = 0;
            while(temp > 0) { temp /= 10; digits++; }
            temp = score;
            for(int d = digits-1; d >= 0; d--) {
                score_str[d] = '0' + (temp % 10);
                temp /= 10;
            }
            si = digits;
        }
        score_str[si] = '\0';
        
        // Update score display
        const char* prefix = "Score: ";
        for(int i = 0; prefix[i]; i++) {
            vga[0 * 80 + WIDTH + 5 + i] = (unsigned short)prefix[i] | (0x0F << 8);
        }
        for(int i = 0; score_str[i]; i++) {
            vga[0 * 80 + WIDTH + 12 + i] = (unsigned short)score_str[i] | (0x0F << 8);
        }
        
        generate_food();
        
        // Win condition
        if(snake.length >= MAX_SNAKE) {
            return 2; // Win!
        }
    } else {
        // Remove tail and add new head
        Position old_tail = snake.body[snake.length - 1];
        for(int i = snake.length - 1; i > 0; i--) {
            snake.body[i] = snake.body[i-1];
        }
        snake.body[0] = new_head;
        
        // Clear old tail
        int pos = (old_tail.y + 1) * 80 + (old_tail.x + 1);
        vga[pos] = (unsigned short)' ' | (0x07 << 8);
    }
    
    // Check collision with self
    for(int i = 1; i < snake.length; i++) {
        if(snake.body[0].x == snake.body[i].x && snake.body[0].y == snake.body[i].y) {
            return 0; // Game over
        }
    }
    
    return 1; // Game continues
}

// Delay function
void delay(int ms) {
    // Rough delay loop
    for(volatile int i = 0; i < ms * 10000; i++);
}

// Show game over screen
void show_game_over() {
    int center_x = (80 - 20) / 2;
    int center_y = 12;
    
    const char* game_over_text = "GAME OVER!";
    const char* final_score = "Final Score: ";
    char score_str[10];
    int si = 0;
    int temp = score;
    if(temp == 0) {
        score_str[si++] = '0';
    } else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = score;
        for(int d = digits-1; d >= 0; d--) {
            score_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    score_str[si] = '\0';
    
    // Clear center area
    for(int y = 10; y <= 14; y++) {
        for(int x = center_x - 5; x <= center_x + 30; x++) {
            vga[y * 80 + x] = (unsigned short)' ' | (0x07 << 8);
        }
    }
    
    // Draw game over text
    for(int i = 0; game_over_text[i]; i++) {
        vga[center_y * 80 + center_x + i] = (unsigned short)game_over_text[i] | (0x0C << 8);
    }
    
    // Draw final score
    for(int i = 0; final_score[i]; i++) {
        vga[(center_y + 2) * 80 + center_x + i] = (unsigned short)final_score[i] | (0x0F << 8);
    }
    for(int i = 0; score_str[i]; i++) {
        vga[(center_y + 2) * 80 + center_x + 13 + i] = (unsigned short)score_str[i] | (0x0F << 8);
    }
    
    const char* restart = "Press R to restart, ESC to exit";
    int restart_x = (80 - 35) / 2;
    for(int i = 0; restart[i]; i++) {
        vga[(center_y + 4) * 80 + restart_x + i] = (unsigned short)restart[i] | (0x0B << 8);
    }
}

// Show win screen
void show_win_screen() {
    int center_x = (80 - 20) / 2;
    int center_y = 12;
    
    const char* win_text = "YOU WIN!";
    const char* final_score = "Final Score: ";
    char score_str[10];
    int si = 0;
    int temp = score;
    if(temp == 0) {
        score_str[si++] = '0';
    } else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = score;
        for(int d = digits-1; d >= 0; d--) {
            score_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    score_str[si] = '\0';
    
    // Clear center area
    for(int y = 10; y <= 14; y++) {
        for(int x = center_x - 5; x <= center_x + 30; x++) {
            vga[y * 80 + x] = (unsigned short)' ' | (0x07 << 8);
        }
    }
    
    // Draw win text
    for(int i = 0; win_text[i]; i++) {
        vga[center_y * 80 + center_x + i] = (unsigned short)win_text[i] | (0x0A << 8);
    }
    
    // Draw final score
    for(int i = 0; final_score[i]; i++) {
        vga[(center_y + 2) * 80 + center_x + i] = (unsigned short)final_score[i] | (0x0F << 8);
    }
    for(int i = 0; score_str[i]; i++) {
        vga[(center_y + 2) * 80 + center_x + 13 + i] = (unsigned short)score_str[i] | (0x0F << 8);
    }
    
    const char* restart = "Press R to restart, ESC to exit";
    int restart_x = (80 - 35) / 2;
    for(int i = 0; restart[i]; i++) {
        vga[(center_y + 4) * 80 + restart_x + i] = (unsigned short)restart[i] | (0x0B << 8);
    }
}

// Main game loop
void start_snake_game() {
    // Save current screen
    unsigned short saved_screen[80*25];
    for(int i = 0; i < 80*25; i++) {
        saved_screen[i] = vga[i];
    }
    
    init_game();
    
    int game_running = 1;
    int game_result = 1;
    int last_move = 0;
    
    while(game_running) {
        // Check keyboard
        if(inb(0x64) & 0x01) {
            unsigned char scancode = inb(0x60);
            if(!(scancode & 0x80)) {
                char c = keyboard_map[scancode];
                
                if(!game_over) {
                    // Game controls
                    if(c == 'w' || c == 'W') {
                        if(snake.direction != 1) snake.direction = 3;
                    } else if(c == 's' || c == 'S') {
                        if(snake.direction != 3) snake.direction = 1;
                    } else if(c == 'a' || c == 'A') {
                        if(snake.direction != 0) snake.direction = 2;
                    } else if(c == 'd' || c == 'D') {
                        if(snake.direction != 2) snake.direction = 0;
                    } else if(c == 27) { // ESC
                        game_running = 0;
                    }
                } else {
                    // Game over screen controls
                    if(c == 'r' || c == 'R') {
                        init_game();
                        game_over = 0;
                    } else if(c == 27) { // ESC
                        game_running = 0;
                    }
                }
            }
        }
        
        // Move snake every few frames
        if(!game_over && last_move++ > 5) {
            last_move = 0;
            int move_result = move_snake();
            
            if(move_result == 0) {
                game_over = 1;
                show_game_over();
            } else if(move_result == 2) {
                game_over = 1;
                show_win_screen();
            } else {
                // Redraw game
                draw_snake();
                draw_food();
            }
        }
        
        // Small delay
        delay(1);
    }
    
    // Restore saved screen
    for(int i = 0; i < 80*25; i++) {
        vga[i] = saved_screen[i];
    }
}