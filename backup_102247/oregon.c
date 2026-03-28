extern unsigned char keyboard_map[128];
// oregon.c - Oregon Trail game for FrodOS
#include "oregon.h"
#include "io.h"

static unsigned short* vga = (unsigned short*)0xB8000;
static unsigned int seed = 1;

// Game variables
static int miles_to_go;
static int food;
static int money;
static int health;
static int day;
static int game_over;
static int win;

static int my_rand() {
    seed = seed * 1103515245 + 12345;
    return (unsigned int)(seed / 65536) % 32768;
}

static void draw_char(int x, int y, char c, char color) {
    if(x >= 0 && x < 80 && y >= 0 && y < 25) {
        vga[y * 80 + x] = (unsigned short)c | (color << 8);
    }
}

static void draw_text(int x, int y, const char* text, char color) {
    for(int i = 0; text[i]; i++) {
        draw_char(x + i, y, text[i], color);
    }
}

static void clear_area(int x1, int y1, int x2, int y2) {
    for(int y = y1; y <= y2; y++) {
        for(int x = x1; x <= x2; x++) {
            draw_char(x, y, ' ', 0x07);
        }
    }
}

static void wait_key() {
    while(1) {
        if(inb(0x64) & 0x01) {
            unsigned char sc = inb(0x60);
            if(!(sc & 0x80)) {
                return;
            }
        }
    }
}

static void show_status() {
    draw_text(2, 2, "=== OREGON TRAIL ===", 0x0E);
    draw_text(2, 4, "Miles to Oregon:", 0x0F);
    char miles_str[10];
    int si = 0;
    int temp = miles_to_go;
    if(temp == 0) miles_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = miles_to_go;
        for(int d = digits-1; d >= 0; d--) {
            miles_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    miles_str[si] = '\0';
    draw_text(25, 4, miles_str, 0x0F);
    
    draw_text(2, 5, "Food:", 0x0F);
    char food_str[10];
    si = 0;
    temp = food;
    if(temp == 0) food_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = food;
        for(int d = digits-1; d >= 0; d--) {
            food_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    food_str[si] = '\0';
    draw_text(12, 5, food_str, 0x0F);
    draw_text(20, 5, "lbs", 0x08);
    
    draw_text(2, 6, "Money:", 0x0F);
    char money_str[10];
    si = 0;
    temp = money;
    if(temp == 0) money_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = money;
        for(int d = digits-1; d >= 0; d--) {
            money_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    money_str[si] = '\0';
    draw_text(14, 6, money_str, 0x0F);
    
    draw_text(2, 7, "Health:", 0x0F);
    if(health > 70) draw_text(12, 7, "Good", 0x0A);
    else if(health > 40) draw_text(12, 7, "Fair", 0x0E);
    else if(health > 20) draw_text(12, 7, "Poor", 0x0C);
    else draw_text(12, 7, "Critical", 0x04);
    
    draw_text(2, 8, "Day:", 0x0F);
    char day_str[5];
    si = 0;
    temp = day;
    if(temp == 0) day_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = day;
        for(int d = digits-1; d >= 0; d--) {
            day_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    day_str[si] = '\0';
    draw_text(10, 8, day_str, 0x0F);
}

static void hunt() {
    clear_area(2, 10, 78, 20);
    draw_text(2, 10, "You go hunting...", 0x0B);
    
    int result = my_rand() % 100;
    int caught = 0;
    
    if(result < 50) {
        caught = 20 + (my_rand() % 50);
        draw_text(2, 12, "You caught", 0x0A);
        char caught_str[10];
        int si = 0;
        int temp = caught;
        if(temp == 0) caught_str[si++] = '0';
        else {
            int digits = 0;
            while(temp > 0) { temp /= 10; digits++; }
            temp = caught;
            for(int d = digits-1; d >= 0; d--) {
                caught_str[d] = '0' + (temp % 10);
                temp /= 10;
            }
            si = digits;
        }
        caught_str[si] = '\0';
        draw_text(20, 12, caught_str, 0x0A);
        draw_text(25, 12, "lbs of food!", 0x0A);
        food += caught;
    } else if(result < 80) {
        caught = 5 + (my_rand() % 20);
        draw_text(2, 12, "You got a small catch:", 0x0E);
        char caught_str[10];
        int si = 0;
        int temp = caught;
        if(temp == 0) caught_str[si++] = '0';
        else {
            int digits = 0;
            while(temp > 0) { temp /= 10; digits++; }
            temp = caught;
            for(int d = digits-1; d >= 0; d--) {
                caught_str[d] = '0' + (temp % 10);
                        temp /= 10;
            }
            si = digits;
        }
        caught_str[si] = '\0';
        draw_text(30, 12, caught_str, 0x0E);
        draw_text(35, 12, "lbs", 0x0E);
        food += caught;
    } else {
        draw_text(2, 12, "You found nothing...", 0x0C);
    }
    
    draw_text(2, 18, "Press any key to continue...", 0x07);
    wait_key();
}

static void travel() {
    clear_area(2, 10, 78, 20);
    
    int travel_miles = 50 + (my_rand() % 70);
    if(travel_miles > miles_to_go) travel_miles = miles_to_go;
    
    miles_to_go -= travel_miles;
    
    int food_used = (travel_miles / 20) + 1 + (my_rand() % 3);
    if(food_used > food) food_used = food;
    food -= food_used;
    
    draw_text(2, 10, "You travel", 0x0B);
    char miles_str[10];
    int si = 0;
    int temp = travel_miles;
    if(temp == 0) miles_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = travel_miles;
        for(int d = digits-1; d >= 0; d--) {
            miles_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    miles_str[si] = '\0';
    draw_text(20, 10, miles_str, 0x0B);
    draw_text(25, 10, "miles", 0x0B);
    
    draw_text(2, 12, "Used", 0x0F);
    char food_str[10];
    si = 0;
    temp = food_used;
    if(temp == 0) food_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = food_used;
        for(int d = digits-1; d >= 0; d--) {
            food_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    food_str[si] = '\0';
    draw_text(12, 12, food_str, 0x0F);
    draw_text(17, 12, "lbs of food", 0x0F);
    
    int event = my_rand() % 100;
    if(event < 20 && miles_to_go > 0) {
        draw_text(2, 14, "You had a rough day!", 0x0C);
        health -= 5;
    } else if(event < 30) {
        draw_text(2, 14, "You found some berries!", 0x0A);
        food += 10;
    } else if(event < 35) {
        draw_text(2, 14, "You found some money!", 0x0A);
        money += 20;
    } else {
        draw_text(2, 14, "The journey continues...", 0x07);
    }
    
    day++;
    
    if(food <= 0) {
        game_over = 1;
        draw_text(2, 16, "You ran out of food!", 0x0C);
    }
    if(health <= 0) {
        game_over = 1;
        draw_text(2, 16, "You died from exhaustion!", 0x0C);
    }
    
    draw_text(2, 18, "Press any key to continue...", 0x07);
    wait_key();
}

static void buy_food() {
    clear_area(2, 10, 78, 20);
    draw_text(2, 10, "How much food to buy? ($1 per lb)", 0x0F);
    draw_text(2, 11, "You have $", 0x0F);
    char money_str[10];
    int si = 0;
    int temp = money;
    if(temp == 0) money_str[si++] = '0';
    else {
        int digits = 0;
        while(temp > 0) { temp /= 10; digits++; }
        temp = money;
        for(int d = digits-1; d >= 0; d--) {
            money_str[d] = '0' + (temp % 10);
            temp /= 10;
        }
        si = digits;
    }
    money_str[si] = '\0';
    draw_text(18, 11, money_str, 0x0F);
    
    draw_text(2, 13, "Press 1 for 20 lbs ($20)", 0x0E);
    draw_text(2, 14, "Press 2 for 50 lbs ($50)", 0x0E);
    draw_text(2, 15, "Press 3 to cancel", 0x0E);
    
    while(1) {
        if(inb(0x64) & 0x01) {
            unsigned char sc = inb(0x60);
            if(!(sc & 0x80)) {
                char c = keyboard_map[sc];
                if(c == '1' && money >= 20) {
                    food += 20;
                    money -= 20;
                    draw_text(2, 17, "Bought 20 lbs of food!", 0x0A);
                    wait_key();
                    return;
                } else if(c == '2' && money >= 50) {
                    food += 50;
                    money -= 50;
                    draw_text(2, 17, "Bought 50 lbs of food!", 0x0A);
                    wait_key();
                    return;
                } else if(c == '3') {
                    return;
                } else {
                    draw_text(2, 17, "Not enough money!", 0x0C);
                    wait_key();
                    clear_area(2, 17, 78, 17);
                }
            }
        }
    }
}

static void show_menu() {
    clear_area(2, 10, 78, 20);
    draw_text(2, 10, "What do you want to do?", 0x0E);
    draw_text(2, 12, "1. Travel", 0x0F);
    draw_text(2, 13, "2. Hunt for food", 0x0F);
    draw_text(2, 14, "3. Buy food", 0x0F);
    draw_text(2, 15, "4. Quit game", 0x0F);
    draw_text(2, 17, "Choice: ", 0x0B);
}

static void show_ending() {
    clear_area(2, 10, 78, 20);
    if(win) {
        draw_text(30, 12, "YOU WIN!", 0x0A);
        draw_text(20, 14, "You reached Oregon!", 0x0F);
        draw_text(20, 15, "You survived", 0x0F);
        char day_str[10];
        int si = 0;
        int temp = day;
        if(temp == 0) day_str[si++] = '0';
        else {
            int digits = 0;
            while(temp > 0) { temp /= 10; digits++; }
            temp = day;
            for(int d = digits-1; d >= 0; d--) {
                day_str[d] = '0' + (temp % 10);
                temp /= 10;
            }
            si = digits;
        }
        day_str[si] = '\0';
        draw_text(35, 15, day_str, 0x0F);
        draw_text(38, 15, "days!", 0x0F);
    } else {
        draw_text(30, 12, "GAME OVER", 0x0C);
        draw_text(25, 14, "You didn't make it to Oregon...", 0x0F);
    }
    draw_text(25, 18, "Press ESC to exit", 0x0E);
    
    while(1) {
        if(inb(0x64) & 0x01) {
            unsigned char sc = inb(0x60);
            if(!(sc & 0x80)) {
                char c = keyboard_map[sc];
                if(c == 27) break;
            }
        }
    }
}

void start_oregon_trail() {
    // Save screen
    unsigned short saved[80*25];
    for(int i = 0; i < 80*25; i++) {
        saved[i] = vga[i];
    }
    
    // Initialize game
    miles_to_go = 2000;
    food = 500;
    money = 200;
    health = 100;
    day = 1;
    game_over = 0;
    win = 0;
    
    while(!game_over && !win) {
        for(int i = 0; i < 80*25; i++) {
            vga[i] = (unsigned short)' ' | (0x07 << 8);
        }
        show_status();
        
        if(miles_to_go <= 0) {
            win = 1;
            break;
        }
        
        show_menu();
        
        int choice = 0;
        while(choice == 0) {
            if(inb(0x64) & 0x01) {
                unsigned char sc = inb(0x60);
                if(!(sc & 0x80)) {
                    char c = keyboard_map[sc];
                    if(c == '1') choice = 1;
                    else if(c == '2') choice = 2;
                    else if(c == '3') choice = 3;
                    else if(c == '4') game_over = 1;
                }
            }
        }
        
        if(choice == 1) travel();
        else if(choice == 2) hunt();
        else if(choice == 3) buy_food();
        
        if(food <= 0 || health <= 0) game_over = 1;
    }
    
    show_ending();
    
    // Restore screen
    for(int i = 0; i < 80*25; i++) {
        vga[i] = saved[i];
    }
}
