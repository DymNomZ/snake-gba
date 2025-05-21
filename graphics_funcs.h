#include <gba_video.h>
#include "structs.h"
#include "configs.h"
#include "characters.h"

#define MEM_VRAM        0x06000000
typedef u16             M3LINE[SCREEN_WIDTH];
#define m3_mem          ((M3LINE*)MEM_VRAM)

#define NUM_CHARS_LINE  10

//colors
#define ODD_TILE_COLOR  0x46B1
#define EVEN_TILE_COLOR 0x4B12
#define FOOD_COLOR      0x1AFC
#define BLACK           0x0000
#define WHITE           0x7FFF

typedef u16             COLOR;

COLOR RGB15(u32 red, u32 green, u32 blue)
{   return red | (green<<5) | (blue<<10);   }

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

void print_char(bool characterArray[64], int x, int y) {

    int color = BLACK;

    for (int i=0; i<8; i++) {
        for (int j=0; j<8; j++) {
            if (characterArray[i*8+j]){
                m3_mem[y + i][x + j] = color;
            }
        }
    }
}

void print_num(bool characterArray[64], int x, int y) {

    int color = BLACK;

    for (int i=0; i<8; i++) {
        for (int j=0; j<8; j++) {
            color = BLACK;
            if (characterArray[i*8+j]) color = WHITE;
            m3_mem[y + i][x + j] = color;
        }
    }
}

void display_number(int num){
    
    if(num == 0){
        print_num(number[0], TILE_WIDTH, TILE_HEIGHT);
        return;
    }

    int len = 0;
    int lennum = num;

    while(lennum != 0){
        lennum /= 10;
        len++;
    }

    for(int i = len; i > 0; i--){

        int digit = num % 10;
        num /= 10;

        print_num(number[digit - 1], TILE_WIDTH, TILE_HEIGHT);
    }
}

void display_text(char textBuffer[], int x, int y) {
    for (int i=0; i<NUM_CHARS_LINE; i++) {
        // Space
        if (textBuffer[i] == 0x20) { 										
            print_char(selector[0], x+i*8, y);
        // Exclamation point
        } else if (textBuffer[i] == 0x21) { 								
            print_char(punctuation[1], x+i*8, y);
        // Period
        } else if (textBuffer[i] == 0x2E) { 								
            print_char(punctuation[0], x+i*8, y);
        // Numbers
        } else if (textBuffer[i] >= 0x30 && textBuffer[i] <= 0x39) { 		
            print_char(number[textBuffer[i] - 0x30], x+i*8, y);
        // Letters
        } else {
            print_char(alphabet[textBuffer[i] - 0x41], x+i*8, y); 			
        }
    }
}