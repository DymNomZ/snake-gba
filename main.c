#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "structs.h"
#include "graphics_funcs.h"
#include "configs.h"

#define SNAKE_START_GRID_X   (GRID_WIDTH / 2)
#define SNAKE_START_GRID_Y   (GRID_HEIGHT / 2)

#define SNAKE_START_X   (SNAKE_START_GRID_X * TILE_WIDTH)
#define SNAKE_START_Y   (SNAKE_START_GRID_Y * TILE_HEIGHT)

#define MOVE_DELAY_FRAMES 6

int current_dx = 1; // Current direction: 1 for right, -1 for left (X-axis)
int current_dy = 0; // Current direction: 1 for down, -1 for up (Y-axis)
int next_dx = 1;    // Next intended direction (from input)
int next_dy = 0;

int score = 0;

int move_timer = MOVE_DELAY_FRAMES;

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

void reset_snake_direction() {
    current_dx = 1; // Default to moving right
    current_dy = 0;
    next_dx = 1;
    next_dy = 0;
}

void reset_snake(struct rect* snake){
    snake->x = SNAKE_START_X;
    snake->y = SNAKE_START_Y;
    score = 0;
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
        .color = WHITE
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

                //increment score
                score++;
                
                food.x = rand_int_fast(0, GRID_WIDTH - 1) * TILE_WIDTH;
                food.y = rand_int_fast(0, GRID_HEIGHT - 1) * TILE_HEIGHT;
            }

            move_timer = MOVE_DELAY_FRAMES;
        }

        draw_rect(&s_rect, s_rect.color);
        draw_rect(&food, food.color);
        /////
        display_number(score);
    }
}