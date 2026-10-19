#include "draw_functions.h"

/*
uint16_t RGB_to_RGB565(uint8_t r, uint8_t g, uint8_t b);

void draw_pixel(int x, int y, uint16_t color);

void draw_line_horizontal(int sx, int dx, int y, uint16_t color);

void draw_line_vertical(int sy, int dy, int x, uint16_t color);

void draw_rect(int x, int y, int w, int h, uint16_t color);

void draw_rect_filled(int x, int y, int w, int h, uint16_t color);
*/

uint16_t RGB_to_RGB565(uint8_t r, uint8_t g, uint8_t b){
    return ((r >> 3) << 11 | ((g >> 2) << 5) | (b>>3));
}

