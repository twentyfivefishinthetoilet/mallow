#ifndef DRAW_FUNCTIONS_H
#define DRAW_FUNCTIONS_H

#include <stdint.h>

extern uint16_t fb[320*240];

// @brief   converts rgb to rgb565, what the lcd display uses
// @param   {uint8_t r} red
// @param   {uint8_t g} green
// @param   {uint8_t b} blue
// @return  uint16_t / RGB565 color (ex. (0,0,255) --> 0x008f)
uint16_t RGB_to_RGB565(uint8_t r, uint8_t g, uint8_t b);

// @brief   places a pixel in the framebuffer.
// @param   {int x} coordinate on the x axis
// @param   {int y} coordinate on the y axis. remember that the y axis is flipped (positive shifts down)
// @param   {uint16_t color} the color of that dot you just placed
void draw_pixel(int x, int y, uint16_t color);

// @brief   draws a line horizontally 
// @param   {int sx} source x, the starting point
// @param   {int dx} destination x, the ending point 
// @param   {int y} position on the y axis.
// @param   {uint16_t color} color of the line you just drew
void draw_line_horizontal(int sx, int dx, int y, uint16_t color);

// @brief   draws a line vertically 
// @param   {int sy} source y, the starting point
// @param   {int dy} destination y, the ending point 
// @param   {int x} position on the x axis.
// @param   {uint16_t color} color of the line you just drew
void draw_line_vertical(int sy, int dy, int x, uint16_t color);

// @brief   draws a rectangle 
// @param   {int x} coordinate on the x axis
// @param   {int y} coordinate on the y axis. remember that the y axis is flipped (positive shifts down)
// @param   {int w} width of the rectangle
// @param   {int h} height of the rectangle
// @param   {uint16_t color} outline color of the rectangle in RGB565 format
void draw_rect(int x, int y, int w, int h, uint16_t color);

// @brief   draws a rectangle... but filled 
// @param   {int x} coordinate on the x axis
// @param   {int y} coordinate on the y axis. remember that the y axis is flipped (positive shifts down)
// @param   {int w} width of the rectangle
// @param   {int h} height of the rectangle
// @param   {uint16_t color} color of the rectangle in RGB565 format
void draw_rect_filled(int x, int y, int w, int h, uint16_t color);

void fill_screen(uint16_t color);

#endif