#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include "structs.h"

#define S_RECT_HEGIHT    4
#define S_RECT_WIDTH     4
#define FOOD_SIZE        2

#define MEM_VRAM        0x06000000

typedef u16             M3LINE[SCREEN_WIDTH];
typedef u16             COLOR;

#define m3_mem          ((M3LINE*)MEM_VRAM)

COLOR RGB15(u32 red, u32 green, u32 blue)
{   return red | (green<<5) | (blue<<10);   }

/* Bounding Box Collision Detection */
bool collision(
    int x1, int y1, int width1, int height1,
    int x2, int y2, int width2, int height2) {
    if (x1 + width1 > x2 &&
        x1 < x2 + width2 &&
        y1 + height1 > y2 &&
        y1 < y2 + height2) return true;
    return false;
}

void draw_pixel(int x, int y, COLOR color){
    m3_mem[y][x] = color;
}

void draw_rect(struct rect* s_rect, COLOR color){

    int x = s_rect->x;
    int y = s_rect->y;
    int h = s_rect->height;
    int w = s_rect->width;

    for(int i = x; i < x+w; i++){
        for(int j = y; j < y+h; j++){
            draw_pixel(i, j, color);
        }
    }
}

void clear_previous(struct rect* s_rect){
    draw_rect(s_rect, 0x0000);
}

int main(void) {

    // Interrupt handlers
    irqInit();

    // Enable Vblank Interrupt, Allow VblankIntrWait
    irqEnable(IRQ_VBLANK);

    /* Set screen to mode 3 */
    SetMode( MODE_3 | BG2_ON );

    struct rect s_rect = { 
        .x = (SCREEN_WIDTH/2) - (S_RECT_WIDTH/2), 
        .y = (SCREEN_HEIGHT/2) - (S_RECT_HEGIHT/2), 
        .width = S_RECT_WIDTH, 
        .height = S_RECT_HEGIHT,
        .color = 0x7FFF
    };

    int velocity = 0;
    int axis = 1;

    /* Main Game Loop */
    while (1) {
        VBlankIntrWait();

        clear_previous(&s_rect);

        scanKeys();
        int key_pressed = keysDown();

        if((key_pressed == KEY_UP) || (key_pressed == KEY_DOWN)){

            axis = 1;

            if((key_pressed == KEY_UP) && s_rect.y >= 0){
                velocity = -2;
            }
            else if((key_pressed == KEY_DOWN) && s_rect.y <= SCREEN_HEIGHT - S_RECT_HEGIHT){
                velocity = 2;
            }
        }
        else if((key_pressed == KEY_LEFT) || (key_pressed == KEY_RIGHT)){

            axis = 2;

            if((key_pressed == KEY_LEFT) && s_rect.x >= 0){
                velocity= -2;
            }
            else if((key_pressed == KEY_RIGHT) && s_rect.x <= SCREEN_WIDTH - S_RECT_WIDTH){
                velocity = 2;
            }
        }

        if(axis == 2) s_rect.x += velocity;
        else s_rect.y += velocity;

        draw_rect(&s_rect, s_rect.color);

    }
}