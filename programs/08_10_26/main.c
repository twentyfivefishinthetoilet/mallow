// TODO:
// FLASH MEMORY [] - https://www.pjrc.com/teensy/W25Q128FV.pdf
// ESP_LCD [x]
// DRAW FUNCTIONS/VISUALS [-]
// LUA INTEGRATION []
// BUTTONS [x]

// GENERAL
#include <stdio.h>

// ESP-IDF DRIVERS
#include "/home/aiden/esp-idf/components/esp_driver_spi/include/driver/spi_master.h" // spi driver
#include "/home/aiden/esp-idf/components/log/include/esp_log.h" 
#include "/home/aiden/esp-idf/components/esp_driver_gpio/include/driver/gpio.h" // handles gpio

// FOR ST7789
#include "/home/aiden/esp-idf/components/esp_lcd/include/esp_lcd_panel_ops.h"
#include "/home/aiden/esp-idf/components/esp_lcd/include/esp_lcd_panel_io.h"
#include "/home/aiden/esp-idf/components/esp_lcd/include/esp_lcd_panel_vendor.h"

// FOR LUA 
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

// lua functions

// visuals/graphics
#include "draw_functions/draw_functions.h"

// PIN DEFINITIONS
// SCREEN
#define LCD_CS 10
#define LCD_RESET 4
#define LCD_DC 5
#define LCD_MOSI 11
#define LCD_SCK 12 

// FLASH
#define FLASH_CS 8
#define FLASH_MOSI 48 
#define FLASH_MISO 18
#define FLASH_SCK 17 

// BUTTON PINS
// M = GREEN
// W = BLUE 
// these are going to be the naming conventions for mallow.
// i plan on adding 6 total buttons (UP DOWN LEFT RIGHT + W AND M)
// pin definition names might change since controllers are going to be modular.
#define M_PIN 20
#define W_PIN 21

// OTHER
#define LCD_WIDTH 320
#define LCD_HEIGHT 240

// button struct for organizing variables.
// i should probably do this for the drawing functions...
typedef struct {
    int pin; int state; int last_state; char name;
} Button;

// setting the default states to 1 since the buttons are going to be an 
// active low. i dont know if this does anything, but it's worth a shot
Button buttons[2] = {
    {M_PIN, 1, 1, 'M'},
    {W_PIN, 1, 1, 'W'}
};

// gets button press
// @note i might rewrite this later without the debouncing. what if a game wants the user to press and hold the button
//       for a set amount of time?
// @param button Button *b
int get_button_press(Button *b){
    b->last_state = b->state;
    b->state = gpio_get_level(b->pin);
    // if button was last high and now low, then return 1. else, return 0
    // helps with preventing ghost presses.
    return (b->last_state == 1 && b->state == 0);
}

// BASE SPI CONFIG
spi_bus_config_t bus_config = {
    .sclk_io_num = LCD_SCK,
    .mosi_io_num = LCD_MOSI,
    .miso_io_num = -1, // no miso pin for st7789 since this one doesnt have touch
    .quadwp_io_num = -1, // st7789 doesnt use quadspi
    .quadhd_io_num = -1, // st7789 doesnt use quadspi
    .max_transfer_sz = 320*240*2 // screen width, height, and size of rgb565, the color system the st7789 uses
};

// LCD SPI BUS
esp_lcd_panel_io_spi_config_t lcd_io_config = {
    .dc_gpio_num = LCD_DC, 
    .cs_gpio_num = LCD_CS,
    .pclk_hz = 40000000,
    .lcd_cmd_bits = 8, // a byte for the command, just like flash memory
    .lcd_param_bits = 8, // a following byte parameter for every command 
    .trans_queue_depth = 10
};

esp_lcd_panel_io_handle_t lcd_io_handle;

esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = LCD_RESET,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16 // rgb565, 2 bytes per pixel
};

esp_lcd_panel_handle_t panel; 

// FRAMEBUFFER
uint16_t fb[320*240];

// FLASH SPI BUS
spi_bus_config_t flash_bus_config = {
    .sclk_io_num = FLASH_SCK,
    .mosi_io_num = FLASH_MOSI,
    .miso_io_num = FLASH_MISO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 256 // the max size that should ever be sent. this is a full page of information right here
};

spi_device_interface_config_t flash_dev_config = {
    .clock_speed_hz = 20000000,
    .mode = 0, // spi mode 0
    .spics_io_num = FLASH_CS,
    .queue_size = 4,
    .command_bits = 8, // instruction byte
    .address_bits = 24 // trailing parameter bytes
};

spi_device_handle_t flash_handle;

// FLASH HELPER FUNCTIONS
// ...

// LUA SETUP
// ...

void app_main(void)
{
    // BUTTON GPIO INIT
    // sets all buttons to it's original state.
    // improved from the last script. insetad of doing a call for each
    // index in the array, i just have a for loop do it for me.
    for (int i=0; i<2; i++){
        gpio_reset_pin(buttons[i].pin);
        gpio_set_direction(buttons[i].pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(buttons[i].pin, GPIO_PULLUP_ONLY);
    }

    // LCD SPI BUS INIT
    spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO); // tells the spi line to use DMA, direct memory access.
                                                                 // since it bypasses something, i forgot, it writes directly to memory.
                                                                 // this lack of delay makes it display stuff faster
    esp_lcd_new_panel_io_spi(SPI2_HOST, &lcd_io_config, &lcd_io_handle);
    esp_lcd_new_panel_st7789(lcd_io_handle, &panel_config, &panel);

    // LCD INIT COMMANDS
    esp_lcd_panel_reset(panel);
    esp_lcd_panel_init(panel);
    esp_lcd_panel_disp_on_off(panel, true);
    esp_lcd_panel_invert_color(panel, true); // i dont know why but my screen needs to be inverted with the bits swapped 
                                             // for the colors to look right.
    // panel rotation
    // todo: make a better panel rotation function for the lua api
    // it's not a priority but idk somebody might want it
    esp_lcd_panel_swap_xy(panel, true);
    esp_lcd_panel_mirror(panel, true, false); 

    // LUA INIT
    //...
    
    //FLASH INIT
    //...

    printf("(%d, %d, %d) -> 0x%02X\n", 255, 255, 0, RGB_to_RGB565(255, 255, 0));
    for (int i = 0; i<320*240;i++){
        fb[i] = __builtin_bswap16(RGB_to_RGB565(255, 255, 0));
    }
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 320 ,240, fb);
}
