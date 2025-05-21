#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "structs.h"

#define S_RECT_HEIGHT    8
#define S_RECT_WIDTH     8

#define TILE_WIDTH       S_RECT_WIDTH
#define TILE_HEIGHT      S_RECT_HEIGHT

#define GRID_WIDTH       (SCREEN_WIDTH / TILE_WIDTH)
#define GRID_HEIGHT      (SCREEN_HEIGHT / TILE_HEIGHT)

#define SNAKE_START_GRID_X   (GRID_WIDTH / 2)
#define SNAKE_START_GRID_Y   (GRID_HEIGHT / 2)

#define SNAKE_START_X   (SNAKE_START_GRID_X * TILE_WIDTH)
#define SNAKE_START_Y   (SNAKE_START_GRID_Y * TILE_HEIGHT)

#define FOOD_WIDTH       TILE_WIDTH
#define FOOD_HEIGHT      TILE_HEIGHT

#define MOVE_DELAY_FRAMES 6

#define MEM_VRAM        0x06000000

//colors
#define ODD_TILE_COLOR  0x46B1
#define EVEN_TILE_COLOR 0x4B12
#define FOOD_COLOR      0x1AFC

typedef u16             M3LINE[SCREEN_WIDTH];
typedef u16             COLOR;

#define m3_mem          ((M3LINE*)MEM_VRAM)

int current_dx = 1; // Current direction: 1 for right, -1 for left (X-axis)
int current_dy = 0; // Current direction: 1 for down, -1 for up (Y-axis)
int next_dx = 1;    // Next intended direction (from input)
int next_dy = 0;

int move_timer = MOVE_DELAY_FRAMES;

COLOR RGB15(u32 red, u32 green, u32 blue)
{   return red | (green<<5) | (blue<<10);   }

bool collision(struct rect* rect1, struct rect* rect2) {

    int x1 = rect1->x;
    int y1 = rect1->y;
    int width1 = rect1->width;
    int height1 = rect1->height;

    int x2 = rect2->x;
    int y2 = rect2->y;
    int width2 = rect2->width;
    int height2 = rect2->height;

    if (x1 + width1 > x2 &&
        x1 < x2 + width2 &&
        y1 + height1 > y2 &&
        y1 < y2 + height2) return true;
    return false;
}

void draw_pixel(int x, int y, COLOR color){
    m3_mem[y][x] = color;
}

void draw_rect(struct rect* rect, COLOR color){

    int x = rect->x;
    int y = rect->y;
    int h = rect->height;
    int w = rect->width;

    for(int i = x; i < x+w; i++){
        for(int j = y; j < y+h; j++){
            draw_pixel(i, j, color);
        }
    }
}

void draw_tile(int x, int y, int h, int w, COLOR color){

    for(int i = x; i < x+w; i++){
        for(int j = y; j < y+h; j++){
            draw_pixel(i, j, color);
        }
    }
}

void reset_snake_direction() {
    current_dx = 1; // Default to moving right
    current_dy = 0;
    next_dx = 1;
    next_dy = 0;
}

void reset_snake(struct rect* snake){
    snake->x = SNAKE_START_X;
    snake->y = SNAKE_START_Y;
    reset_snake_direction();
}

void check_position(struct rect* snake) {

    int max_x = (GRID_WIDTH - 1) * TILE_WIDTH;  // Max top-left x for a tile
    int max_y = (GRID_HEIGHT - 1) * TILE_HEIGHT; // Max top-left y for a tile

    if (snake->x < 0 || snake->x > max_x || snake->y < 0 || snake->y > max_y) {
        reset_snake(snake);
    }
}

int rand_int_fast(int min_val, int max_val) {

    if (min_val > max_val) {
        int temp = min_val;
        min_val = max_val;
        max_val = temp;
    }

    int range = max_val - min_val + 1;
    if (range <= 0) return min_val;
    return min_val + (rand() % range);
}

void draw_background(){

    for(int i = 0; i < GRID_WIDTH; i++){
        for(int j = 0; j < GRID_HEIGHT; j++){

            if((j % 2 == 0 && i % 2 == 0) || (j % 2 != 0 && i % 2 != 0)){
                draw_tile(
                    i * TILE_WIDTH, j * TILE_HEIGHT,
                    TILE_WIDTH, TILE_HEIGHT, ODD_TILE_COLOR
                );
            }
            else{
                draw_tile(
                    i * TILE_WIDTH, j * TILE_HEIGHT,
                    TILE_WIDTH, TILE_HEIGHT, EVEN_TILE_COLOR
                );
            }
        }
    }
}

void restore_tile(struct rect* rect) {
    int grid_x = rect->x / TILE_WIDTH;
    int grid_y = rect->y / TILE_HEIGHT;
    COLOR tile_color;

    if ((grid_y % 2 == 0 && grid_x % 2 == 0) 
    || (grid_y % 2 != 0 && grid_x % 2 != 0)) {
        tile_color = ODD_TILE_COLOR;
    } else {
        tile_color = EVEN_TILE_COLOR;
    }

    draw_tile(rect->x, rect->y, rect->width, rect->height, tile_color);
}

int main(void) {

    //seed onw time
    srand(time(NULL));

    // interrupt handlers
    irqInit();

    // enable Vblank Interrupt, allow VblankIntrWait
    irqEnable(IRQ_VBLANK);

    //Set screen to mode 3
    SetMode( MODE_3 | BG2_ON );

    draw_background();

    int start_x = SNAKE_START_X;
    int start_y = SNAKE_START_Y;

    struct rect s_rect = { 
        .x = start_x, 
        .y = start_y, 
        .width = TILE_WIDTH, 
        .height = TILE_HEIGHT,
        .color = 0x7FFF
    };

    struct rect food = {
        .x = rand_int_fast(0, GRID_WIDTH - 1) * TILE_WIDTH,
        .y = rand_int_fast(0, GRID_HEIGHT - 1) * TILE_HEIGHT,
        .width = FOOD_WIDTH,
        .height = FOOD_HEIGHT,
        .color = FOOD_COLOR
    };

    reset_snake_direction();

    //game loop
    while (1) {
        VBlankIntrWait();

        scanKeys();
        int key_input = keysDown();

        if (key_input & KEY_UP) {
            if (current_dy != 1) {
                next_dx = 0;
                next_dy = -1;
            }
        } else if (key_input & KEY_DOWN) {
            if (current_dy != -1) {
                next_dx = 0;
                next_dy = 1;
            }
        } else if (key_input & KEY_LEFT) {
            if (current_dx != 1) {
                next_dx = -1;
                next_dy = 0;
            }
        } else if (key_input & KEY_RIGHT) {
            if (current_dx != -1) {
                next_dx = 1;
                next_dy = 0;
            }
        }

        move_timer--;
        if (move_timer <= 0) {

            restore_tile(&s_rect);

            // Update current direction from the next intended direction
            current_dx = next_dx;
            current_dy = next_dy;

            // Update snake position by one tile
            if (current_dx != 0 || current_dy != 0) { // Only move if there's a direction
                 s_rect.x += current_dx * TILE_WIDTH;
                 s_rect.y += current_dy * TILE_HEIGHT;
            }


            check_position(&s_rect);

            if (collision(&s_rect, &food)) {
                
                restore_tile(&s_rect);
                
                food.x = rand_int_fast(0, GRID_WIDTH - 1) * TILE_WIDTH;
                food.y = rand_int_fast(0, GRID_HEIGHT - 1) * TILE_HEIGHT;
            }

            move_timer = MOVE_DELAY_FRAMES;
        }

        draw_rect(&s_rect, s_rect.color);
        draw_rect(&food, food.color);

    }
}