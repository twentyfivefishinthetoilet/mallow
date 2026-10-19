// TODO:
// FLASH MEMORY [x] - https://www.pjrc.com/teensy/W25Q128FV.pdf
// ESP_LCD [x]
// DRAW FUNCTIONS/VISUALS [-]
// LUA INTEGRATION [x]
// BUTTONS [x]

// GENERAL
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ESP-IDF DRIVERS
#include "/home/aiden/esp-idf/components/esp_driver_gpio/include/driver/gpio.h" // handles gpio
#include "/home/aiden/esp-idf/components/esp_driver_spi/include/driver/spi_master.h" // spi driver
#include "/home/aiden/esp-idf/components/log/include/esp_log.h"

// FOR ST7789
#include "/home/aiden/esp-idf/components/esp_lcd/include/esp_lcd_panel_io.h"
#include "/home/aiden/esp-idf/components/esp_lcd/include/esp_lcd_panel_ops.h"
#include "/home/aiden/esp-idf/components/esp_lcd/include/esp_lcd_panel_vendor.h"

// FOR LUA
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

// FOR SLEEP FUNCTIONS
#include "/home/aiden/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos/FreeRTOS.h"
#include "/home/aiden/esp-idf/components/freertos/FreeRTOS-Kernel/include/freertos/task.h"

// FOR DELTA TIME
#include "esp_timer.h"

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
#define FLASH_MOSI 18
#define FLASH_MISO 17
#define FLASH_SCK 48

#define SCRIPT_BASE_ADDR 0x000000
#define SCRIPT_LEN_ADDR 0x000000 // 4 bytes for length
#define SCRIPT_DATA_ADDR 0x000004 // script starts right after

// BUTTON PINS
// M = GREEN
// W = BLUE
// these are going to be the naming conventions for mallow.
// i plan on adding 6 total buttons (UP DOWN LEFT RIGHT + W AND M)
// pin definition names might change since controllers are going to be modular.
#define M_PIN 6
#define W_PIN 21

// OTHER
#define LCD_WIDTH 320
#define LCD_HEIGHT 240

// button struct for organizing variables.
// i should probably do this for the drawing functions...
typedef struct {
  int pin;
  int state;
  int last_state;
  char name;
} Button;

// setting the default states to 1 since the buttons are going to be an
// active low. i dont know if this does anything, but it's worth a shot
Button buttons[2] = {{M_PIN, 1, 1, 'M'}, {W_PIN, 1, 1, 'W'}};

// gets button press
// @note i might rewrite this later without the debouncing. what if a game wants
// the user to press and hold the button for a set amount of time?
// @param button Button *b
int get_button_press(Button *b) {
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
    .max_transfer_sz = 320 * 240 * 2 // screen width, height, and size of
                                     // rgb565, the color system the st7789 uses
};

// LCD SPI BUS
esp_lcd_panel_io_spi_config_t lcd_io_config = {
    .dc_gpio_num = LCD_DC,
    .cs_gpio_num = LCD_CS,
    .pclk_hz = 40000000,
    .lcd_cmd_bits = 8,   // a byte for the command, just like flash memory
    .lcd_param_bits = 8, // a following byte parameter for every command
    .trans_queue_depth = 10
};

// FLASH SPI BUS
spi_bus_config_t flash_bus_config = {
  .sclk_io_num = FLASH_SCK,
  .mosi_io_num = FLASH_MOSI,
  .miso_io_num = FLASH_MISO,
  .quadwp_io_num = -1,
  .quadhd_io_num = -1,
  .max_transfer_sz = 256 // size of page
};

spi_device_interface_config_t flash_dev_config = {
  .clock_speed_hz = 20 * 1000 * 1000,
  .mode = 0,
  .spics_io_num = FLASH_CS,
  .queue_size = 4,
  .command_bits = 8, // instruction byte
  .address_bits = 24 // the w25q128 uses 24 bit addresses
};

// handles
esp_lcd_panel_io_handle_t lcd_io_handle;
esp_lcd_panel_handle_t panel;
spi_device_handle_t flash_handle;

esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = LCD_RESET,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16 // rgb565, 2 bytes per pixel
};

// FLASH FUNCTIONS
// commands like 02h are taken from documentation 
// link to documentation: https://www.pjrc.com/teensy/W25Q128FV.pdf

// sets up and attaches flash memory to the bus
void flash_init(void){
  esp_err_t err; // used for error handling. ive learned that this makes your life
                 // so much easier when debugging.
  err = spi_bus_initialize(SPI3_HOST, &flash_bus_config, SPI_DMA_CH_AUTO);
  printf("spi bus init: %s\n", esp_err_to_name(err));
  err = spi_bus_add_device(SPI3_HOST, &flash_dev_config, &flash_handle);
  printf("spi bus add device: %s\n", esp_err_to_name(err));
}


/*
// enables writing to the chip.
// this write enable (06h) command needs to be called before every
// page program or erase call. 


from documentation, 8.2.1 (page 30 of pdf):

The Write Enable instruction is entered by driving /CS low, 
shifting the instruction code “06h” into the Data Input (DI)
pin on the rising edge of CLK, and then driving /CS high.
*/
void flash_write_enable(void){
  spi_transaction_ext_t t = {0};
  t.base.cmd = 0x06;
  t.base.flags = SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
}

/*
read status register

command: 0x05

from documentation, 8.2.4 (page 31 of pdf):

The Read Status Register instructions allow the 8-bit Status Registers to be read
...
The Read Status Register instruction may be used at any time, even while a Program, Erase or Write
Status Register cycle is in progress
*/
uint8_t flash_read_status(void){
  spi_transaction_ext_t t = {0};
  t.base.cmd = 0x05;
  t.base.length = 8;
  t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
  return t.base.rx_data[0];
}

/*
read jedec id 

command: (0x9F)

returns C8 40 18 for my flash chip.
if it returns FF FF FF or 00 00 00, something is wired up
wrong or broken.

with a quick google search, i found that a jedec id is an 
identification code returned by flash chips when you try to
do anything with it with a microcontroller. it tells the
chip manufacturer, memory type, and capacity respectively
*/
void flash_read_jedec_id(uint8_t output[3]){
  spi_transaction_ext_t t = {0};
  t.base.cmd = 0x9F;
  t.base.length = 24;
  t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
  memcpy(output, t.base.rx_data, 3);
}

// helper function to check if any transactions are going through,
// or in other words, if the flash is busy. delays if it is
void flash_wait_busy(void){
  while(flash_read_status() & 0x01){
    vTaskDelay(1);
  }
}

/*
flash read data

command : 0x03

from documentation, 8.2.6 (page 35)

The Read Data instruction allows one or more data bytes to be sequentially 
read from the memory. The instruction is initiated by driving the /CS pin low 
and then shifting the instruction code “03h” followed by a 24-bit address (A23-A0) 
into the [MOSI] pin
*/
void flash_read_data(uint32_t address, uint8_t *buf, size_t len){
  spi_transaction_t t = {0};
  t.cmd = 0x03;
  t.addr = address;
  t.length = len * 8; // driver wants bits, not bytes
  t.rx_buffer = buf;
  spi_device_polling_transmit(flash_handle, &t);
}

/*
page program 

command: 0x02

from documentation, 8.2.15 (page 50 of pdf):
The Page Program instruction allows from one byte to 256 bytes (a page) of data to be programmed at
previously erased (FFh) memory locations. A Write Enable instruction must be executed before the
device will accept the Page Program Instruction (Status Register bit WEL= 1).

something else to keep in mind (from the same page of documentation)

If an entire 256 byte page is to be programmed, the last address byte (the 8 least significant address bits)
should be set to 0. If the last address byte is not zero, and the number of clocks exceeds the remaining
page length, the addressing will wrap to the beginning of the page
*/
void flash_page_program(uint32_t address, const uint8_t *buf, size_t len){
  flash_write_enable();
  spi_transaction_t t = {0};
  t.cmd = 0x02;
  t.addr = address;
  t.length = len * 8;
  t.tx_buffer = buf;
  spi_device_polling_transmit(flash_handle, &t);
  flash_wait_busy();
}

/*
flash sector erase 

comand: 0x20

from documentation, 8.2.17 (page 53 of pdf)

The Sector Erase instruction sets all memory within a specified sector (4K-bytes) 
to the erased state of all 1s (FFh). 
*/
void flash_sector_erase(uint32_t address){
  flash_write_enable();
  spi_transaction_t t = {0};
  t.cmd = 0x20;
  t.addr = address;
  spi_device_polling_transmit(flash_handle, &t);
  flash_wait_busy();
}

void flash_erase_range(uint32_t address, size_t len){
  uint32_t start = address & ~(0xFFF); // round down to 4kb boundry
  uint32_t end = (address + len + 0xFFF) & ~(0xFFF);
  for (uint32_t a = start; a < end; a+=4096){
    flash_sector_erase(a);
  }
}

// flash write data
void flash_write_data(uint32_t address, const uint8_t *buf, size_t len){
  size_t written = 0;
  while (written < len){
    uint32_t current_address = address + written;
    // how many bytes are left until the next 256 byte page boundry
    size_t space_in_page = 256 - (current_address % 256);
    size_t chunk = len - written;
    if (chunk > space_in_page) chunk = space_in_page;

    flash_page_program(current_address, buf+written, chunk);
    written += chunk;
  }
}

// flash write script 
void flash_write_script(const char *script){
  uint32_t len = strlen(script);
  // erase enough sectors to cover te header and the script itself
  flash_erase_range(SCRIPT_LEN_ADDR, 4 + len);
  // write the 4 byte length header first
  uint8_t len_bytes[4] = {
    (len >> 24) & 0xFF, (len >> 16) & 0xFF,
    (len >> 8) & 0xFF, len & 0xFF
  };
  flash_write_data(SCRIPT_LEN_ADDR, len_bytes, 4);
  flash_write_data(SCRIPT_DATA_ADDR, (const uint8_t*)script, len);
}

// flash read script
// make sure to free() buffer when done
char *flash_read_script(void){
  uint8_t len_bytes[4];
  flash_read_data(SCRIPT_LEN_ADDR, len_bytes, 4);
  printf("LEN_BYTES: %02X %02X %02X %02X\n", len_bytes[0], len_bytes[1], len_bytes[2], len_bytes[3]);
  uint32_t len = (len_bytes[0] << 24) | (len_bytes[1] << 16) | (len_bytes[2] << 8) | len_bytes[3];
  // a sanity check that flash was written. if it wasnt written, everything is 0xFF,
  // which would decode to basically garbage
  if (len == 0xFFFFFFFF || len > 64 * 1024){
    return NULL;
  } 

  char *buf = malloc(len + 1);
  if (!buf) return NULL;

  flash_read_data(SCRIPT_DATA_ADDR, (uint8_t*)buf, len);
  buf[len] = '\0'; // terminates the string
  return buf;
}

// LUA SETUP

// defining the variable that will be used for
// the lua interpreter. this variable is used
// in almost every lua function call
lua_State* L; 

// i'll probably need to store the length of the script somewhere.
// a script struct is probably going to be used for organization 
const char *script = "local counter = 0\n"
                     "function update() end\n"
                     "function render()\n"
                     "if counter >= 0.5 then\n"
                     "draw_rect_filled(5, 5, 310, 230, 0xf81f)\n"
                     "display_fb()\n"
                     "counter = 0\n"
                     "end\n"
                     "counter = counter + delta_time\n"
                     "end\n";

// LUA FUNCTIONS 

// graphics / draw functions

// for easier registering
typedef struct {
  int (*lua_function)(lua_State *L);
  char *function_name;
} LuaFunction; 

// lua draw pixel
// params: int x, int y, uint16_t color
int lua_draw_pixel(lua_State* L){
  int x = luaL_checkinteger(L, 1); // param 1
  int y = luaL_checkinteger(L, 2); // param 2
  lua_Integer color = lua_tointeger(L, 3);

  draw_pixel(x, y, (uint16_t)color); 

  return 0;
}

// lua draw horizontal line
// params: int sx, int dx, int y, uint16_t color
int lua_draw_line_h(lua_State* L){
  int sx = luaL_checkinteger(L, 1);
  int dx = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  lua_Integer color = lua_tointeger(L, 4);

  draw_line_horizontal(sx, dx, y, (uint16_t)color); 

  return 0;
}

// lua draw vertical line 
// params: int sy, int dy, int x, uint16_t color
int lua_draw_line_v(lua_State* L){
  int sy = luaL_checkinteger(L, 1);
  int dy = luaL_checkinteger(L, 2);
  int x = luaL_checkinteger(L, 3);
  lua_Integer color = lua_tointeger(L, 4);

  draw_line_vertical(sy, dy, x, (uint16_t)color);

  return 0;
}

// lua draw rect
// params: int x, int y, int w, int h, uint16_t color
int lua_draw_rect(lua_State* L){
  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int w = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  lua_Integer color = lua_tointeger(L, 5);

  draw_rect(x, y, w, h, (uint16_t)color);

  return 0;
}

// lua draw filled rect
// params: int x, int y, int w, int h, uint16_t color
int lua_draw_rect_filled(lua_State* L){
  int x = luaL_checkinteger(L, 1);
  int y = luaL_checkinteger(L, 2);
  int w = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  lua_Integer color = lua_tointeger(L, 5);

  draw_rect_filled(x, y, w, h, (uint16_t)color);

  return 0;
}

// lua display fb
// params: none
int lua_display_fb(lua_State* L){
  esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);

  return 0;
}

LuaFunction functions[6] = {
  {lua_draw_pixel, "draw_pixel"},
  {lua_draw_line_h, "draw_line_h"},
  {lua_draw_line_v, "draw_line_v"},
  {lua_draw_rect, "draw_rect"},
  {lua_draw_rect_filled, "draw_rect_filled"},
  {lua_display_fb, "display_fb"}
};

void app_main(void) {
  // BUTTON GPIO INIT
  // sets all buttons to it's original state.
  // improved from the last script. insetad of doing a call for each
  // index in the array, i just have a for loop do it for me.
  for (int i = 0; i < 2; i++) {
    gpio_reset_pin(buttons[i].pin);
    gpio_set_direction(buttons[i].pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttons[i].pin, GPIO_PULLUP_ONLY);
  }

  // LCD SPI BUS INIT
  spi_bus_initialize(
      SPI2_HOST, &bus_config,
      SPI_DMA_CH_AUTO); // tells the spi line to use DMA, direct memory access.
                        // since it bypasses something, i forgot, it writes
                        // directly to memory. this lack of delay makes it
                        // display stuff faster
  esp_lcd_new_panel_io_spi(SPI2_HOST, &lcd_io_config, &lcd_io_handle);
  esp_lcd_new_panel_st7789(lcd_io_handle, &panel_config, &panel);

  // LCD INIT COMMANDS
  esp_lcd_panel_reset(panel);
  esp_lcd_panel_init(panel);
  esp_lcd_panel_disp_on_off(panel, true);
  esp_lcd_panel_invert_color(
      panel, true); // i dont know why but my screen needs to be inverted with
                    // the bits swapped for the colors to look right.
  // panel rotation
  // todo: make a better panel rotation function for the lua api
  // it's not a priority but idk somebody might want it
  esp_lcd_panel_swap_xy(panel, true);
  esp_lcd_panel_mirror(panel, true, false);

  // LUA INIT
  L = luaL_newstate();
  if (L==NULL){
    printf("ERR: lua init failed :(\n");
    while (1);
  } 
  luaL_openlibs(L);

  for (int i=0;i<sizeof(functions)/sizeof(functions[0]);i++){
    lua_register(L, functions[i].function_name, functions[i].lua_function);
  }

  flash_init();
  uint8_t id[3];
  flash_read_jedec_id(id);
  printf("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);

  flash_write_script(script);
  printf("wrote script, len: %d\n", (int)strlen(script));

  char *loaded = flash_read_script();
  if (loaded){
    printf("read back %d bytes:\n%s\n", (int)strlen(loaded), loaded);
    if (luaL_dostring(L, loaded)!=0){
      printf("LUA ERR: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);
    }
    free(loaded);
  }
  else {
    printf("flash_read_script returned null, either the read or write failed.\nyou screwed up son\n");
  }

  fill_screen(0x0000);

  // MAIN LOOP
  int64_t last_time = esp_timer_get_time();
  while (1){
    int64_t curr_time = esp_timer_get_time();
    float dt = (curr_time - last_time) / 1000000.0f;
    last_time = curr_time;

    lua_pushnumber(L, dt);
    lua_setglobal(L, "delta_time");

    // get and call update
    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1)){
      if (lua_pcall(L, 0, 0, 0) != LUA_OK){
        printf("ERR CALLING UPDATE: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
      }
    }
    else {
      lua_pop(L, 1);
    }

    // get and call render
    lua_getglobal(L, "render");
    if (lua_isfunction(L, -1)){
      if (lua_pcall(L, 0, 0, 0) != LUA_OK){
        printf("ERR CALLING RENDER: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
      }
    }
    else {
      lua_pop(L, 1);
    }
    
    vTaskDelay(1);
  }
}
