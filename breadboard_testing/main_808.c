// ROADMAP
/*
 * - learn how spi works [x]
 * - install and use esp_lcd [x]
 * - custom functions (ex. fill_screen(color)) [-]
 * - draw bitmaps [x]
 * - animations [x]
 * - game loop []
 */

//

#include <stdio.h>
#include <string.h> // for strlen 
#include <ctype.h> // for isalpha

// for random numbers
#include "esp_random.h"

// LCD LIBRARIES (7/27/26 @ 14:44)
// these are my guesses for what these files do just by taking a quick
// glance at them. i'll google it later, but for now i just want to get a quick
// color on the screen as my "hello world".
#include "esp_lcd_panel_ops.h" // operations (ex. esp_lcd_panel_init)
#include "esp_lcd_panel_io.h" // not a clue
#include "esp_lcd_panel_vendor.h" // including other headers for ease?

// APP ICONS AND OTHER IMAGES (8/4/26 @ 13:09)
// #include "./icons.h"

// for timer
#include "esp_timer.h"

// for delays
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// bomb sprites
#include "bombs.h"

// font spritesheet
#include "flowerfont.h"

// SPI DRIVER
#include "/home/aiden/esp-idf/components/esp_driver_spi/include/driver/spi_master.h"

// LOG
#include "/home/aiden/esp-idf/components/log/include/esp_log.h"

// GPIO (controls RST and maybe BL later (always driven high at the moment) )
#include "/home/aiden/esp-idf/components/esp_driver_gpio/include/driver/gpio.h"

// pin definitions (7/27/26 @ 15:03)
#define PIN_CS 10
#define PIN_RESET 4
#define PIN_DC 5
#define PIN_SDA_MOSI 11 // the st7789 uses weird pin labels. why SDA for MOSI?
#define PIN_SCL_SCK 12

#define MAX_BW 39
#define MAX_BH 91

#define BOMB_OFFSET_X 110
#define BOMB_OFFSET_Y 50

#define BLUEPIN 20
#define GREENPIN 21

#define FB_W 320
#define FB_H 240

typedef struct {
    int width; int height; char letter; int xindex; int yindex;
} large_font_letters;

typedef struct {
    char c; int x; int y;
} custom_char;

typedef struct {
    int pin; int state; int last_state; char name;
} button;

button buttons[2] = {
    {GREENPIN, 0, 0, 'M'},
    {BLUEPIN, 0, 0, 'W'}
};

// theres probably a better way to do this, given all characters
// have the same width and height. i used a quick python script
// to generate me a whole list of characters instead of manually
// typing everything out. 

// the indexes are gonna be a multiplier for the width and height,
// hopefully giving me the position of the letter i want to print out.

/*
import string

for letter in string.ascii_uppercase:
    print("{23, 37, '" + letter + "', 1, 1},")
    
for letter in string.ascii_lowercase:
    print("{23, 37, '" + letter + "', 1, 1},")
*/

large_font_letters large_letters[] = {
    {23, 37, 'A', 1, 1},
    {23, 37, 'B', 2, 1},
    {23, 37, 'C', 3, 1},
    {23, 37, 'D', 4, 1},
    {23, 37, 'E', 5, 1},
    {23, 37, 'F', 6, 1},
    {23, 37, 'G', 7, 1},
    {23, 37, 'H', 8, 1},
    {23, 37, 'I', 1, 2},
    {23, 37, 'J', 2, 2},
    {23, 37, 'K', 3, 2},
    {23, 37, 'L', 4, 2},
    {23, 37, 'M', 5, 2},
    {23, 37, 'N', 6, 2},
    {23, 37, 'O', 7, 2},
    {23, 37, 'P', 8, 2},
    {23, 37, 'Q', 1, 3},
    {23, 37, 'R', 2, 3},
    {23, 37, 'S', 3, 3},
    {23, 37, 'T', 4, 3},
    {23, 37, 'U', 5, 3},
    {23, 37, 'V', 6, 3},
    {23, 37, 'W', 7, 3},
    {23, 37, 'X', 8, 3},
    {23, 37, 'Y', 1, 4},
    {23, 37, 'Z', 2, 4},
    {23, 37, 'a', 3, 4},
    {23, 37, 'b', 4, 4},
    {23, 37, 'c', 5, 4},
    {23, 37, 'd', 6, 4},
    {23, 37, 'e', 7, 4},
    {23, 37, 'f', 8, 4},
    {23, 37, 'g', 1, 5},
    {23, 37, 'h', 2, 5},
    {23, 37, 'i', 3, 5},
    {23, 37, 'j', 4, 5},
    {23, 37, 'k', 5, 5},
    {23, 37, 'l', 6, 5},
    {23, 37, 'm', 7, 5},
    {23, 37, 'n', 8, 5},
    {23, 37, 'o', 1, 6},
    {23, 37, 'p', 2, 6},
    {23, 37, 'q', 3, 6},
    {23, 37, 'r', 4, 6},
    {23, 37, 's', 5, 6},
    {23, 37, 't', 6, 6},
    {23, 37, 'u', 7, 6},
    {23, 37, 'v', 8, 6},
    {23, 37, 'w', 1, 7},
    {23, 37, 'x', 2, 7},
    {23, 37, 'y', 3, 7},
    {23, 37, 'z', 4, 7}
};

spi_bus_config_t busconf = {
    .sclk_io_num = PIN_SCL_SCK,
    .mosi_io_num = PIN_SDA_MOSI,
    .miso_io_num = -1, // no miso pin, no need to send data back (no touch screen)
    .quadwp_io_num = -1, // st7789 doesnt use quad spi
    .quadhd_io_num = -1, // st7789 doesnt use quad spi
    .max_transfer_sz = 240 * 320 * 2 // (w * h * sizeof rgb565)
};

// io config (7/27/26 @ 15:12)
esp_lcd_panel_io_spi_config_t ioconf = {
    .dc_gpio_num = PIN_DC,
    .cs_gpio_num = PIN_CS,
    .pclk_hz = 40000000,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .trans_queue_depth = 10
};

// create io handle (7/27/26 @ 15:16)
esp_lcd_panel_io_handle_t io_handle;

// create panel (7/27/26 @ 15:20)
esp_lcd_panel_dev_config_t panelconf = {
    .reset_gpio_num = PIN_RESET,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16
};
esp_lcd_panel_handle_t panel;

// framebuffer and drawing a color to the screen (7/27/26 @ 15:24)
uint16_t fb[240 * 320];

// leaving this here so i remember to use pointers.
// int get_btn_press(button b){
//     b.last_state = b.state;
//     b.state = gpio_get_level(b.pin);
//     // below expands to (in lua)...
//     // if b.last_state == 1 && b.state == 0 then return 1 else return 0 end
//     return (b.last_state == 1 && b.state == 0);
// }

int get_btn_press(button *b){
    b->last_state = b->state;
    b->state = gpio_get_level(b->pin);
    // below expands to (in lua)...
    // if b.last_state == 1 && b.state == 0 then return 1 else return 0 end
    return (b->last_state == 1 && b->state == 0);
}

// bomb frame
int bomb_frame = 0;

void draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= FB_W || y < 0 || y >= FB_H) return;

    fb[y * FB_W + x] = __builtin_bswap16(color);;
}

// returns a random number in the range of [-range, range]
int rand_offset(int range) {
    return (esp_random() % (2 * range + 1)) - range;
}

void draw_line_horizontal(int sx, int dx, int y, uint16_t color){
    for (int i = sx; i<dx; i++){
        draw_pixel(i, y, color);
    }
}

void draw_line_vertical(int sy, int dy, int x, uint16_t color){
    for (int i = sy; i<dy; i++){
        draw_pixel(x, i, color);
    }
}

void draw_rect(int x, int y, int w, int h, uint16_t color){
    draw_line_horizontal(x, x + w, y, color);
    draw_line_horizontal(x, x + w, y + h, color);
    draw_line_vertical(y, y + h, x, color);
    draw_line_vertical(y, y + h, x + w, color);
}

void fill_screen(uint16_t color){
    for (int i=0;i<320*240;i++){
        fb[i] =  __builtin_bswap16(color);
    }
}

void draw_rect_filled(int x, int y, int w, int h, uint16_t color){
    for (int i = y; i<=y + h;i++){
        for (int j = x; j<=x+w; j++){
            draw_pixel(j, i, color);
        }
    }
}

// void draw_playspace(){ // helper function
//     draw_rect(5-1, 5-1, 315+1, 235+1, 0x0000);
//     draw_rect(5, 5, 315, 235, 0x0000);
//     draw_rect(5+1, 5+1, 315-1, 235-1, 0x0000);
// }

float counter = 0;
void update(float dt){
    /*
    counter += dt;
    if (counter >= 1.0f){
        bomb_frame = (bomb_frame + 1) % 4;
        counter=0;
    */
}

void render(){
    // clear screen/fill framebuffer
    // for (int i = 0; i< 320* 240; i++){
    //     fb[i] = 0xffff;
    // }
    
    /*
    for (int y = 0; y < MAX_BH; y++) {
        for (int x = 0; x < MAX_BW; x++) {
            uint16_t px = epd_bitmap_allArray[bomb_frame][y * MAX_BW + x];
            if (px != 0xf81f) {
                fb[(y + BOMB_OFFSET_Y) * FB_W + (x + BOMB_OFFSET_X)] = __builtin_bswap16(px);
            }
        }
    }
    */

    draw_pixel(5, 5, 0x0000);
    draw_line_horizontal(5, FB_W-5, 10, 0x0000);
    draw_line_vertical(15, FB_H-5, 5, 0x0000);
    draw_rect(10, 20, FB_W-5, 25, 0x0000);
    draw_rect_filled(10, 50, FB_W-5, 25, 0x0000);

    if (get_btn_press(&buttons[0])==1){
        fill_screen(0x07e0);
    }

    if (get_btn_press(&buttons[1])==1){
        fill_screen(0x001f);
    }

    // display fb
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);
}

void app_main(void)
{
    // setting button configurations
    // green
    gpio_reset_pin(buttons[0].pin);
    gpio_set_direction(buttons[0].pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttons[0].pin, GPIO_PULLUP_ONLY);// enables internal pullup (active low)
    // blue
    gpio_reset_pin(buttons[1].pin);
    gpio_set_direction(buttons[1].pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttons[1].pin, GPIO_PULLUP_ONLY);// enables internal pullup (active low)


    // SPI_DMA_CH_AUTO means use DMA
    spi_bus_initialize(SPI2_HOST, &busconf, SPI_DMA_CH_AUTO);
    // create io handle (continued)
    esp_lcd_new_panel_io_spi(SPI2_HOST, &ioconf, &io_handle);
    // create panel (continued)
    esp_lcd_new_panel_st7789(io_handle, &panelconf, &panel);

    // sends st7789 commands for you instead of creating your own handler
    esp_lcd_panel_reset(panel);
    esp_lcd_panel_init(panel);
    esp_lcd_panel_disp_on_off(panel, true);

    esp_lcd_panel_invert_color(panel, true);

    // 90 degree panel rotation
    esp_lcd_panel_swap_xy(panel, true);
    esp_lcd_panel_mirror(panel, true, false); // mirror x = true, mirror y = false


    // // framebuffer and drawing a color to the screen (continued)
    // for (int i = 0; i < 240 * 320; i++){
    //     fb[i] = 0xDEAD; // one of the only rgb565 colors i know. red/0xF800 is boring.
    // }

    int64_t last_time = esp_timer_get_time() / 1000;

    for (int i = 0; i< 320* 240; i++){
        fb[i] = 0xffff;
    }

    // loop()
    while (1){
        uint32_t current_time = esp_timer_get_time() / 1000;
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        update(delta_time);
        render();
        vTaskDelay(1);
    }
}
