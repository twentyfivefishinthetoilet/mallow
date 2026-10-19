#include "draw_functions.h"

/*
uint16_t RGB_to_RGB565(uint8_t r, uint8_t g, uint8_t b);

void draw_pixel(int x, int y, uint16_t color);

void draw_line_horizontal(int sx, int dx, int y, uint16_t color);

void draw_line_vertical(int sy, int dy, int x, uint16_t color);

void draw_rect(int x, int y, int w, int h, uint16_t color);

void draw_rect_filled(int x, int y, int w, int h, uint16_t color);
*/

uint16_t fb[320*240];

uint16_t RGB_to_RGB565(uint8_t r, uint8_t g, uint8_t b){
    return ((r >> 3) << 11 | ((g >> 2) << 5) | (b>>3));
}

void draw_pixel(int x, int y, uint16_t color){
    if (x < 0 || x > 320 || y < 0 || y > 240) return;
    fb[y * 320 + x] = color;
}

void draw_line_horizontal(int sx, int dx, int y, uint16_t color){
    for (int i=sx;i<=dx;i++){
        draw_pixel(i, y, color);
    }
}

void draw_line_vertical(int sy, int dy, int x, uint16_t color){
    for (int i=sy;i<=dy;i++){
        draw_pixel(x, i, color);
    }
}

void draw_rect(int x, int y, int w, int h, uint16_t color){
    draw_line_horizontal(x, x+w, y, color);
    draw_line_horizontal(x, x+w, y+h, color);
    draw_line_vertical(y, y+h, x, color);
    draw_line_vertical(y, y+h, x+w, color);
}

void draw_rect_filled(int x, int y, int w, int h, uint16_t color){
    for (int j=y; j<=y+h; j++){
        for (int i=x; i<=x+w; i++){
            draw_pixel(i, j, color);
        }
    }
}

void fill_screen(uint16_t color){
    for (int i=0;i<320*240;i++) fb[i] = color;
}