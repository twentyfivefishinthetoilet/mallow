// TODO:
// FLASH MEMORY [-] - https://www.pjrc.com/teensy/W25Q128FV.pdf
//    i need to deal with two flash memory chips here. one for the game 
//    cartridge, one for the controller (which isnt important, maybe skip?)

// ESP_LCD [x]

// DRAW FUNCTIONS/VISUALS [-]
//    should be easy to implement, right? maybe a draw function for circles 
//    and a triangle, then i'll mark it as complete for now.

// LUA INTEGRATION [-]
//    i dont remember why this isnt marked off as complete. probably because
//    i didnt finish the game, or rather, ANY game. 

// BUTTONS [x]

// SD CARD [ ]
//    for the sd card, i need to edit the functions because the cs pin is
//    now connected to an mcp23017 pin. i'd need to do raw spi commands,
//    like what i did for flash memory. cant be that hard, right? 
//    at least it'll get me hours logged...
//
//    i dont think i necessarily need the sd card for my shipped project.
//    all that's doing right now is flashing code onto the game cartridge,
//    which i can just use my breadboard revision for. 

// GENERAL
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <dirent.h>

// no cartridge inserted message
#include "warning.h"

// ESP-IDF DRIVERS
#include "driver/gpio.h" // handles gpio
#include "driver/spi_master.h" // spi driver
#include "esp_log.h"

// FOR ST7789
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

// FOR SD CARD
// https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/get-started/case-study/storage-examples/sd-card-examples/sdmmc-example.html
#include "esp_vfs_fat.h" // "provides mounting, reading and writing,
//  and virtual file system management functions of the fat system"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
// power regulation
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

// FOR GPIO EXPANSION/MCP23017
#include "driver/i2c_master.h"
#include "i2cdev.h"
#include "mcp23x17.h"

// FOR LUA
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

// FOR SLEEP FUNCTIONS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// FOR DELTA TIME
#include "esp_timer.h"

// visuals/graphics
#include "draw_functions/draw_functions.h"
//#include "frames.h" // <== no more need for carbuncle dancing animations....... for now

// i dont want to make a seprate file for this, retyping all of this would be a pain. 
// for now, im using #ifdef for the different designs: BREADBOARD and PCB 
#define PCB // uncomment for pcb design
// #define BREADBOARD // uncomment for bb design.
                      // should i call it revision 2.0? idk

// PIN DEFINITIONS
// SCREEN
#ifdef BREADBOARD
  #define LCD_CS 10
  #define LCD_RESET 4
  #define LCD_DC 5
  #define LCD_MOSI 11
  #define LCD_SCK 12
#endif

#ifdef PCB
  #define LCD_CS 4
  #define LCD_RESET 9
  #define LCD_DC 6
  #define LCD_MOSI 7
  #define LCD_MISO 8
  #define LCD_SCK 5
#endif

// FLASH
#ifdef BREADBOARD
  #define FLASH_CS 8
  #define FLASH_MOSI 18
  #define FLASH_MISO 17
  #define FLASH_SCK 48
#endif

#ifdef PCB
  #define FLASH_CS 11 // this is GPB3 of the gpio MCP gpio expander. 
                      // NOT a gpio number!!!!

  #define CONTROLLER_FLASH_CS 10 // GPB2
  #define FLASH_MOSI 47
  #define FLASH_MISO 48
  #define FLASH_SCK 39
#endif

// MCP23017 pins
#ifdef BREADBOARD
  #define MCP_SDA 21
  #define MCP_SCL 38
#endif

#ifdef PCB 
  #define MCP_SDA 16
  #define MCP_SCL 15
#endif 

// for the lua script
#define SCRIPT_BASE_ADDR 0x000100
#define SCRIPT_LEN_ADDR 0x000100 // 4 bytes for length
#define SCRIPT_DATA_ADDR 0x000104 // script starts right after
// for images
#define DIRECTORY_BASE 0x010100
#define DATA_BASE 0x011000

// SD CARD
// buffer size: used to specify the max length of char arrays
#define MAX_CHAR_SIZE 128
// mount path
#define MOUNT_POINT "/SDCARD"
// speed mode judgement: used to judge whether the sd card is in high speed
// mode in the program, so as to adjust the initialization parameters or read
// and write strategies. (again, from documentation, i didnt write this)
#define EXAMPLE_IS_UHS1 (CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50 || CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50)

// pins
// #define SD_MISO 17
// #define SD_CLK 20
// #define SD_MOSI 21
// ^^^ just going to be using the SPI3_HOST/flash spi line for the sd card
#ifdef BREADBOOARD
  #define SD_CS 47
#endif

#ifdef PCB 
  // gpio expander pin (GB0)
  #define SD_CS 8
#endif

// BUTTON PINS
// M = GREEN
// W = BLUE
// these are going to be the naming conventions for mallow.
// i plan on adding 6 total buttons (UP DOWN LEFT RIGHT + W AND M)
// pin definition names might change since controllers are going to be modular.

// TODO: add buttons and change pin numbers when pcb arrives

#ifdef BREADBOARD
  #define M_PIN 3
  #define W_PIN 2
#endif 

#ifdef PCB 
  // all of these are mcp gpio expander pins.
  // again, NOT GPIO!!!!!!!!!!!
  // todo: write code to actually handle button presses with 
  //       mcp pins wired up to the buttons rather than gpio pins.
  //       pretty sure i did something like this in init_gpio_expander
  //       but i never initialized the special and home buttons.
  //       it should be an easy fix, probably just going to use a for loop
  //       decreasing from 9 and skipping 8 (no button assigned to it) 
  #define UP_PIN 7
  #define DOWN_PIN 6
  #define LEFT_PIN 5
  #define RIGHT_PIN 4
  
  #define M_PIN 3
  #define W_PIN 2

  #define SPECIAL_PIN 1
  // v GPB1 v
  #define HOME_PIN 9
#endif 

// OTHER
#define LCD_WIDTH 320
#define LCD_HEIGHT 240

// ternary operator / if statement
// syntax: condition ? true : false;
// used for defining variables or macros quicker
// example / what min expands to
/*
 * if (a < b){
 *  return a;
 * }
 * else {
 *  return b
 * }
 */
#define min(a, b) ((a) < (b) ? (a) : (b))

// button struct for organizing variables.
// i should probably do this for the drawing functions...
typedef struct {
  int pin;
  uint32_t state;
  uint32_t last_state;
  char *name;
} Button;

// setting the default states to 1 since the buttons are going to be an
// active low. i dont know if this does anything, but it's worth a shot
Button buttons[2] = {{M_PIN, 1, 1, "M"}, {W_PIN, 1, 1, "W"}};

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
  .max_transfer_sz = 4000 // 256 is the size of the flash page, but 4000 is the side of the sd card
};

spi_device_interface_config_t flash_dev_config = {
  .clock_speed_hz = 20 * 1000 * 1000,
  .mode = 0,
  .spics_io_num = -1, // no gpio controlled chip select
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

// MCP23017 setup
mcp23x17_t dev;

Button expansion_buttons[6] = {
  {7, 1, 1, "U"}, {6, 1, 1, "D"},
  {5, 1, 1, "L"}, {4, 1, 1, "R"},
  {3, 1, 1, "M"}, {2, 1, 1, "W"}
};

void init_gpio_expander(void){
  i2cdev_init();
  memset(&dev, 0, sizeof(mcp23x17_t)); // clear garbage before init

  mcp23x17_init_desc(&dev, MCP23X17_ADDR_BASE, I2C_NUM_0, MCP_SDA, MCP_SCL);

  // set button pins
  // GPA7: UP
  // GPA6: DOWN
  // GPA5: LEFT
  // GPA4: RIGHT
  // GPA3: M
  // GPA2: W
  for (int i=2;i<=7;i++){
    mcp23x17_set_mode(&dev, i, MCP23X17_GPIO_INPUT);
    mcp23x17_set_pullup(&dev, i, true);
  }

  // setting game cartridge chip select to output
  mcp23x17_set_mode(&dev, FLASH_CS, MCP23X17_GPIO_OUTPUT);
  mcp23x17_set_level(&dev, FLASH_CS, 1); 
}

// active low
// turns off the chip select manually since it's 
// connected to a mcp23x pin
void game_flash_cs_select(void) {
  mcp23x17_set_level(&dev, FLASH_CS, 0);
}

// deselects the cs pin (turns it high manually)
void game_flash_cs_deselect(void) {
  mcp23x17_set_level(&dev, FLASH_CS, 1);
}

int get_ex_button_press(Button *b){
  b->last_state = b->state;
  mcp23x17_get_level(&dev, b->pin, &b->state);
  return (b->state == 0);
  // return (b->last_state == 1 && b->state == 0);
}

// SD FUNCTIONS

esp_vfs_fat_mount_config_t mount_config = {
  .format_if_mount_failed = false,
  .max_files = 5,
  .allocation_unit_size = 16 * 1024
};

// init sd
// attaches sd to flash spi line, etc
sdmmc_card_t *card;
void init_sd(void){
  esp_err_t ret;

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI3_HOST;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_CS;
  slot_config.host_id = SPI3_HOST;

  ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK){
    printf("failed to mount file system :(\nerror: %s\n", esp_err_to_name(ret));
    while (1);
  }
  printf("sd card mounted successfully :)\n");
}

// write to card
static esp_err_t write_file(const char *path, char *data){
  printf("write file: OPENING FILE \"%s\"\n", path);
  FILE *f = fopen(path, "w");
  if (f==NULL){
    printf("write file: ERR: failed to open file for writing :(\n");
    return ESP_FAIL;
  }
  fprintf(f, "%s", data);
  fclose(f);
  printf("write file: file written :)\n");
  esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
  return ESP_OK;
}

// FLASH FUNCTIONS
// commands like 02h are taken from documentation
// link to documentation: https://www.pjrc.com/teensy/W25Q128FV.pdf

// sets up and attaches flash memory to the bus
void flash_init(void){
  esp_err_t err; // used for error handling. ive learned that this makes your life
  // so much easier when debugging.
  err = spi_bus_add_device(SPI3_HOST, &flash_dev_config, &flash_handle);
  printf("spi bus add device: %s\n", esp_err_to_name(err));
}

/*
quick references so i dont need to go to the website every time
// spi_transaction_ext_t/other spi info found here: 
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html

public members (spi_transaction_ext_t):
- base: transaction data, converts it into spi_transactionext_t
base is the only one i care about right now. address bits just
the transaction length. 

// spi_transaction_t members:

- flags: how a transaction is handled
- cmd: command data (length set in command_bits)
*/

/*
 * // enables writing to the chip.
 * // this write enable (06h) command needs to be called before every
 * // page program or erase call.
 *
 *
 * from documentation, 8.2.1 (page 30 of pdf):
 *
 * The Write Enable instruction is entered by driving /CS low,
 * shifting the instruction code “06h” into the Data Input (DI)
 * pin on the rising edge of CLK, and then driving /CS high.
 */
void flash_write_enable(void){
  spi_transaction_ext_t t = {0};
  t.base.cmd = 0x06;
  t.base.flags = SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  game_flash_cs_select();
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
  game_flash_cs_deselect();
}

/*
 * read status register
 *
 * command: 0x05
 *
 * from documentation, 8.2.4 (page 31 of pdf):
 *
 * The Read Status Register instructions allow the 8-bit Status Registers to be read
 * ...
 * The Read Status Register instruction may be used at any time, even while a Program, Erase or Write
 * Status Register cycle is in progress
 */
uint8_t flash_read_status(void){
  spi_transaction_ext_t t = {0};
  t.base.cmd = 0x05;
  t.base.length = 8;
  t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  game_flash_cs_select();
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
  game_flash_cs_deselect();
  return t.base.rx_data[0];
}

/*
 * read jedec id
 *
 * command: (0x9F)
 *
 * returns C8 40 18 for my flash chip.
 * if it returns FF FF FF or 00 00 00, something is wired up
 * wrong or broken.
 *
 * with a quick google search, i found that a jedec id is an
 * identification code returned by flash chips when you try to
 * do anything with it with a microcontroller. it tells the
 * chip manufacturer, memory type, and capacity respectively
 */
void flash_read_jedec_id(uint8_t output[3]){
  spi_transaction_ext_t t = {0};
  t.base.cmd = 0x9F;
  t.base.length = 24;
  t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  game_flash_cs_select();
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
  game_flash_cs_deselect();
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
 * flash read data
 *
 * command : 0x03
 *
 * from documentation, 8.2.6 (page 35)
 *
 * The Read Data instruction allows one or more data bytes to be sequentially
 * read from the memory. The instruction is initiated by driving the /CS pin low
 * and then shifting the instruction code “03h” followed by a 24-bit address (A23-A0)
 * into the [MOSI] pin
 */
void flash_read_data(uint32_t address, uint8_t *buf, size_t len){
  size_t read = 0;
  while (read < len){
    size_t chunk = min((size_t)4000, len-read);
    spi_transaction_t t = {0};
    t.cmd = 0x03;
    t.addr = address + read;
    t.length = chunk * 8; // driver wants bits, not bytes
    t.rx_buffer = buf + read;
    game_flash_cs_select();
    spi_device_polling_transmit(flash_handle, &t);
    game_flash_cs_deselect();
    read+=chunk;
  }
}

/*
 * page program
 *
 * command: 0x02
 *
 * from documentation, 8.2.15 (page 50 of pdf):
 * The Page Program instruction allows from one byte to 256 bytes (a page) of data to be programmed at
 * previously erased (FFh) memory locations. A Write Enable instruction must be executed before the
 * device will accept the Page Program Instruction (Status Register bit WEL= 1).
 *
 * something else to keep in mind (from the same page of documentation)
 *
 * If an entire 256 byte page is to be programmed, the last address byte (the 8 least significant address bits)
 * should be set to 0. If the last address byte is not zero, and the number of clocks exceeds the remaining
 * page length, the addressing will wrap to the beginning of the page
 */
void flash_page_program(uint32_t address, const uint8_t *buf, size_t len){
  flash_write_enable();
  spi_transaction_t t = {0};
  t.cmd = 0x02;
  t.addr = address;
  t.length = len * 8;
  t.tx_buffer = buf;
  game_flash_cs_select();
  spi_device_polling_transmit(flash_handle, &t);
  game_flash_cs_deselect();
  flash_wait_busy();
}

/*
 * flash sector erase
 *
 * comand: 0x20
 *
 * from documentation, 8.2.17 (page 53 of pdf)
 *
 * The Sector Erase instruction sets all memory within a specified sector (4K-bytes)
 * to the erased state of all 1s (FFh).
 */
void flash_sector_erase(uint32_t address){
  flash_write_enable();
  spi_transaction_t t = {0};
  t.cmd = 0x20;
  t.addr = address;
  game_flash_cs_select();
  spi_device_polling_transmit(flash_handle, &t);
  game_flash_cs_deselect();
  flash_wait_busy();
}

void flash_erase_range(uint32_t address, size_t len){
  uint32_t start = address & ~(0xFFF); // round down to 4kb boundry
  uint32_t end = (address + len + 0xFFF) & ~(0xFFF);
  for (uint32_t a = start; a < end; a+=4096){
    flash_sector_erase(a);
  }
}

/*
 * erase the whole flash chip
 *
 * from documentation 8.2.20 (page 56 of pdf)
 *
 * The Chip Erase instruction sets all memory within the device to the erased state of all 1s (FFh). A Write
 * Enable instruction must be executed before the device will accept the Chip Erase Instruction (Status
 * Register bit WEL must equal 1). The instruction is initiated by driving the /CS pin low and shifting the
 * instruction code “C7h” or “60h”.
 */
void flash_erase_all(void){
  flash_write_enable();

  spi_transaction_ext_t t = {0};
  t.base.cmd = 0xC7;
  t.base.length = 16;
  t.base.flags = SPI_TRANS_VARIABLE_ADDR;
  t.address_bits = 0;
  game_flash_cs_select();
  spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
  game_flash_cs_deselect();
  flash_wait_busy();
}

/*
 * flash write data - this is where stuff gets a little tricky because there's no documentation to guide anyone through this
 * (im sure there is somewhere, just not in the chip's datasheet)
 *
 * flash data can only be programmed 256 bytes, or a page, at a time.
 * this function keeps track of a starting address, the data you want to write (buf) and how many bytes to write total (len).
 * an example of this code being called is in flash_write_script, which writes the lua script to flash:
 *
 *      flash_write_data(SCRIPT_DATA_ADDR, (const uint8_t*)script, len);
 *
 * size_t written keeps track of how many bytes that have been written so far. keeps the while loop going until all data has been written (written == len).
 * uint32_t current_address is where the next block should be written
 * size_t space_in_page calculates how many bytes are left in the page
 * size_t chunk checks how many bytes are left in total to write. the following if statement locks the chunk to the space left in the page
 *
 * chunk gets added to written at the end, then it repeats until all the data has been transmitted
 */
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

/*
 * flash write script
 *
 * writes the lua script (or any script provided) to flash by using the flash write data function.
 * first it gets the length of the script, erases the past script, the lua file's header's data,
 * then the actual script.
 *
 * (len >> #) & 0xFF shifts the uint32_t len a number of bits and leaves only a uint8_t length left.
 *  ex. (0x12345678 >> 24) & 0xFF leaves you with 0x12
 *
 * stores data in big endian order. used for organization
 */
void flash_write_script(const char *script){
  uint32_t len = strlen(script);
  // erase enough sectors to cover the header and the script itself
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
  if (len == 0xFFFFFFFF || len > 64 * 1024 || len == 0x00000000){
    return NULL;
  }

  char *buf = malloc(len + 1);
  if (!buf) return NULL;

  flash_read_data(SCRIPT_DATA_ADDR, (uint8_t*)buf, len);
  buf[len] = '\0'; // terminates the string
  return buf;
}

// FLASH IMAGE FILES

// describes where an image is located in flash memory
// the image isnt inside here, it's kind of a pointer to it
typedef struct {
  char name[25];
  uint32_t addr; // where on flash the data starts
  uint32_t size; // bytes
  uint16_t w; // image weight
  uint16_t h; // image height
} FFileEntry;

#define MAX_FILES 64

/*
 * another complicated function: send_images_to_flash
 *
 * this kind of works the same way the write script to flash function does:
 * it gets the file size then ships it to flash.
 *
 * static FFileEntry entries[MAX_FILES] = {0}; creates a little makeshift file
 * directory system. entry index 0 would hold one image's data (w, h, name, etc), entry 1
 * would hold another's.
 *
 * FFileEntry (flash file entry) stores the "path/directory" (address) of where to find the
 * image entry i could be looking for. the size is how big the file is in bytes. width and height
 * explains itself.
 *
 * the size of the file and starting address both are important because you need to know where
 * the image file starts and ends, and where the next one begins. current_addr += chunk moves the
 * address forward.
 *
 * the name is just an identifier for lua, it makes it easier so i dont need to call address 0x123456.
 * it gets called from flash_find_entry, which works by loading in the max file of images at once and
 * then iterating through them to see if the name provided matches a valid entry's name. if it does,
 * it fills the out buffer with the valid entry.
 *
 * it loads the image to the screen by finding that entry, malloc-ing the size of the file to a buffer,
 * then finds the actual image data at whatever address "flash_read_data(entry.addr, buffer, entry.size);"
 * is at.
 *
 * i dont know if any of that made sense, but who reads these things anyways?
 */

bool send_images_to_flash(void){
  typedef struct {
    const char *path; const char* name; uint16_t w; uint16_t h;
  } Files;

  Files files[] = {
    {"/SDCARD/images/0.bin", "0.bin", 220, 220},
    {"/SDCARD/images/1.bin", "1.bin", 220, 220},
    {"/SDCARD/images/2.bin", "2.bin", 220, 220},
    {"/SDCARD/images/3.bin", "3.bin", 220, 220},
    {"/SDCARD/images/4.bin", "4.bin", 220, 220},
    {"/SDCARD/images/5.bin", "5.bin", 220, 220},
    {"/SDCARD/images/6.bin", "6.bin", 220, 220},
    {"/SDCARD/images/7.bin", "7.bin", 220, 220},
    {"/SDCARD/images/8.bin", "8.bin", 220, 220},
    {"/SDCARD/images/9.bin", "9.bin", 220, 220},
    {"/SDCARD/images/10.bin", "10.bin", 220, 220},
    {"/SDCARD/images/11.bin", "11.bin", 220, 220},
    {"/SDCARD/images/12.bin", "12.bin", 220, 220},
    {"/SDCARD/images/13.bin", "13.bin", 220, 220},
    {"/SDCARD/images/14.bin", "14.bin", 220, 220},
    {"/SDCARD/images/15.bin", "15.bin", 220, 220},
  };
  int file_count = sizeof(files) / sizeof(files[0]);

  flash_erase_all(); // wipe everything first

  static FFileEntry entries[MAX_FILES] = {0};
  uint32_t current_addr = DATA_BASE;

  // write image data and record addresses
  for (int i = 0; i<file_count; i++){
    FILE *f = fopen(files[i].path, "rb"); // needs a mode, "rb" = read binary
    if (f==NULL){
      printf("send images to flash: failed to open \"%s\" (%s)\n", files[i].name, files[i].path);
      return false;
    }

    strncpy(entries[i].name, files[i].name, sizeof(entries[i].name) - 1);
    entries[i].name[sizeof(entries[i].name) - 1] = '\0';
    entries[i].addr = current_addr;

    // checking file size
    if (fseek(f, 0, SEEK_END) != 0){
      fclose(f);
      printf("send images to flash: something went wrong when trying to check the file size :(\n");
      return false;
    }

    long size = ftell(f);
    if (size < 0){
      fclose(f);
      printf("send images to flash: ftell failed on \"%s\" :(\n", files[i].name);
      return false;
    }
    rewind(f); // apparently theres a function to reset the pointer to the start of the file
    entries[i].size = (uint32_t)size;
    //----------------------

    entries[i].w = files[i].w;
    entries[i].h = files[i].h;

    uint8_t buffer[256];
    size_t remaining = (size_t)size;
    while (remaining > 0){
      size_t chunk = min((size_t)256, remaining);
      size_t bytes_read = fread(buffer, 1, chunk, f);
      if (bytes_read != chunk){
        printf("send images to flash: fread came up short on \"%s\" :(\n", files[i].name);
        fclose(f);
        return false;
      }

      flash_write_data(current_addr, buffer, chunk);

      current_addr += chunk;
      remaining -= chunk;
    }
    fclose(f);
  }

  // write image directory base
  flash_write_data(DIRECTORY_BASE, (uint8_t*)entries, sizeof(entries));

  printf("send images to flash: success :)\n");
  return true;
}

bool flash_find_entry(const char *name, FFileEntry *out){
  static FFileEntry entries[MAX_FILES];
  flash_read_data(DIRECTORY_BASE, (uint8_t*)entries, sizeof(entries));

  for (int i=0;i<MAX_FILES; i++){
    if (entries[i].size == 0 || entries[i].size == 0xFFFFFFFF) continue; // empty slot
    if (strcmp(entries[i].name, name) == 0){
      *out = entries[i];
      return true;
    }
  }
  return false;
}

// loads rgb565 image by name from flash memory
#define IMG_CHUNK 4000   // bytes which match the transfer size
#define TRANSPARENT_KEY 0xf81f // transparent color

bool draw_image_from_flash(const char *name, int x, int y){
  FFileEntry entry;
  if (!flash_find_entry(name, &entry)) return false;

  static uint8_t chunk[IMG_CHUNK]; // fixed size, independent of image dimensions

  uint32_t total = entry.size; // bytes in image
  uint32_t done = 0; // bytes consumed so far
  int px = 0, py = 0; // position within the image

  while (done < total){
    uint32_t n = min((uint32_t)IMG_CHUNK, total - done);
    n &= ~1u; // only whole pixels!!!!!!!!!!!
    if (n == 0) break;

    flash_read_data(entry.addr + done, chunk, n);

    for (uint32_t i = 0; i < n; i += 2){
      uint16_t color = chunk[i] | (chunk[i + 1] << 8);

      if (color != TRANSPARENT_KEY){
        draw_pixel(x + px, y + py, color);   // draw_pixel clips and bswaps
      }

      if (++px == entry.w){
        px = 0;
        py++;
      }
    }
    done += n;
  }
  return true;
}
// LUA SETUP

// lua C api documentation:
// https://www.lua.org/pil/24.html

// defining the variable that will be used for
// the lua interpreter. this variable is used
// in almost every lua function call
lua_State* L;

// i'll probably need to store the length of the script somewhere.
// a script struct is probably going to be used for organization
// char *script = "function update() if U ~= 0 then print(\"UP PRESSED!\") end if D ~= 0 then print(\"DOWN PRESSED!\") end if L ~= 0 then print(\"LEFT PRESSED!\") end if R ~= 0 then print(\"RIGHT PRESSED!\") end end\nfunction render() if U ~= 0 then fill_screen(0xf81f) end if D ~= 0 then fill_screen(0x001f)  end if L ~= 0 then fill_screen(0xf800)  end if R ~= 0 then fill_screen(0x07e0) end display_fb()\nend";
char *script = "";
char *load_script_sd(const char *path){
  FILE *file = fopen(path, "rb");
  if (file == NULL){
    printf("load script: ERR: error opening %s\n", path);
    while (1);
  }

  // check file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET); // set pointer back to start of file

  char *buffer = malloc(file_size + 1); // +1 for '\0'
  if (buffer == NULL){
    printf("load script: ERR: malloc failed :(\n");
    fclose(file);
    while(1);
  }

  size_t bytes_read = fread(buffer, 1, file_size, file);
  if (bytes_read < file_size && ferror(file)){
    printf("load script: ERR: somehow, there was an error reading the script file\nlike, it exists, but you still couldn't read from it. that sucks.\n");
    free(buffer);
    fclose(file);
    while(1);
  }

  buffer[bytes_read] = '\0'; // cant forget that null terminator
  fclose(file);

  return buffer;
}

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

int lua_fill_screen(lua_State* L){
  lua_Integer color = lua_tointeger(L, 1);
  fill_screen(color);
  return 0;
}
// lua display fb
// params: none
int lua_display_fb(lua_State* L){
  esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);
  return 0;
}

// lua draw image from memory
int lua_draw_image(lua_State* L){
  size_t len;
  const char *path = lua_tolstring(L, 1, &len);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  if (!draw_image_from_flash(path, x, y)){
    printf("can't load image, is \"%s\" a valid directory?\n", path);
  }
  return 0;
}

int lua_draw_circle(lua_State* L){
  int xc = luaL_checkinteger(L, 1);
  int yc = luaL_checkinteger(L, 2);
  int r = luaL_checkinteger(L, 3);
  lua_Integer color = lua_tointeger(L, 4);
  draw_circle(xc, yc, r, (uint16_t)color);
  return 0;
}

int lua_draw_circle_filled(lua_State* L){
  int xc = luaL_checkinteger(L, 1);
  int yc = luaL_checkinteger(L, 2);
  int r = luaL_checkinteger(L, 3);
  lua_Integer color = lua_tointeger(L, 4);
  draw_circle_filled(xc, yc, r, (uint16_t)color);
  return 0;
}

LuaFunction functions[10] = {
  {lua_draw_pixel, "draw_pixel"},
  {lua_draw_line_h, "draw_line_h"},
  {lua_draw_line_v, "draw_line_v"},
  {lua_draw_rect, "draw_rect"},
  {lua_draw_rect_filled, "draw_rect_filled"},
  {lua_display_fb, "display_fb"},
  {lua_draw_image, "draw_image"},
  {lua_fill_screen, "fill_screen"},
  {lua_draw_circle, "draw_circle"},
  {lua_draw_circle_filled, "draw_circle_filled"},
};

void app_main(void) {
  // BUTTON GPIO INIT
  // sets all buttons to it's original state.
  // improved from the last script. insetad of doing a call for each
  // index in the array, i just have a for loop do it for me.
  // for (int i = 0; i < 2; i++) {
  //   gpio_reset_pin(buttons[i].pin);
  //   gpio_set_direction(buttons[i].pin, GPIO_MODE_INPUT);
  //   gpio_set_pull_mode(buttons[i].pin, GPIO_PULLUP_ONLY);
  // }

  gpio_reset_pin(FLASH_CS);
  gpio_set_direction(FLASH_CS, GPIO_MODE_OUTPUT);
  gpio_set_level(FLASH_CS, 1);

  // LCD SPI BUS INIT
  spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO); // tells the spi line to use DMA, direct memory access.
  // since it bypasses something, i forgot, it writes
  // directly to memory. this lack of delay makes it
  // display stuff faster
  esp_lcd_new_panel_io_spi(SPI2_HOST, &lcd_io_config, &lcd_io_handle);
  esp_lcd_new_panel_st7789(lcd_io_handle, &panel_config, &panel);

  // LCD INIT COMMANDS
  esp_lcd_panel_reset(panel);
  esp_lcd_panel_init(panel);
  esp_lcd_panel_disp_on_off(panel, true);
  esp_lcd_panel_invert_color(panel, true); // i dont know why but my screen needs to be inverted with
  // the bits swapped for the colors to look right.
  // panel rotation
  // todo: make a better panel rotation function for the lua api
  // it's not a priority but idk somebody might want it
  esp_lcd_panel_swap_xy(panel, true);
  esp_lcd_panel_mirror(panel, true, false);

  fill_screen(0xf800);
  esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);

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

  // init spi3 bus
  esp_err_t err = spi_bus_initialize(SPI3_HOST, &flash_bus_config, SPI_DMA_CH_AUTO);
  printf("spi3 bus init: %s\n", esp_err_to_name(err));

  // init_sd();
  // write_file("/SDCARD/hi.txt", "wrote file from esp-idf :-)");
  flash_init();
  init_gpio_expander();

  uint8_t id[3];
  flash_read_jedec_id(id);
  printf("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);

  // FILE *flash_update_file = fopen("/SDCARD/flash_update.txt", "r");
  // if (flash_update_file != NULL){
  //   fclose(flash_update_file);
  //   uint8_t id[3];
  //   flash_read_jedec_id(id);
  //   printf("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);

  //   // if (!send_images_to_flash()){
  //   //   printf("something went wrong sending images to flash (app_main)\n");
  //   //   while(1);
  //   // }

  //   script = load_script_sd("/SDCARD/main.lua");
  //   flash_write_script(script);
  //   printf("wrote script, len: %d\n", (int)strlen(script));
  //   remove("/SDCARD/flash_update.txt");
  // }

  // flash_write_script(script);
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
    //draw_pixel(int x, int y, uint16_t color)
    for (int p = 0; p < 320 * 240; p++) {
      fb[p] = __builtin_bswap16(epd_bitmap_nocartridgeinserted[p]);
    }
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);
    while(1) vTaskDelay(1);
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

    for (int i=0;i<sizeof(expansion_buttons)/sizeof(expansion_buttons[0]);i++){
      lua_pushnumber(L, get_ex_button_press(&expansion_buttons[i]));
      lua_setglobal(L, expansion_buttons[i].name);
    }

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
