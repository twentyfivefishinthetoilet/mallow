// ROADMAP
/*
- learn how spi works []
- install and use esp_lcd []
- custom functions (ex. fill_screen(color)) []
- draw bitmaps []
- animations []
- game loop []
*/

// FOR FORGERS (the people of forge.hackclub.com? idk)
/*
im going to have my code in sections and write down timestamps of when i started and completed it. 
im going to commit after every day's work, check commit descriptions or my journal for an even better description about what i did that day.
if you have any questions, if im doing anything wrong, or you just want to chat, message me on slack (.aiden) or instagram (@mallow_dev)
*/

/*
NOTES:

*/

#include <stdio.h>

// LCD LIBRARIES (7/27/26 @ 14:44)
// these are my guesses for what these files do just by taking a quick
// glance at them. i'll google it later, but for now i just want to get a quick
// color on the screen as my "hello world".
#include "esp_lcd_panel_ops.h" // operations (ex. esp_lcd_panel_init)
#include "esp_lcd_panel_io.h" // not a clue
#include "esp_lcd_panel_vendor.h" // including other headers for ease?

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

void app_main(void)
{
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

    // framebuffer and drawing a color to the screen (continued)
    for (int i = 0; i < 240 * 320; i++){
        fb[i] = 0xDEAD; // one of the only rgb565 colors i know. red/0xF800 is boring.
    }

    // display fb 
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 240, 320, fb);
}
