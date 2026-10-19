// cartridge programmer for mallow

// this is a fallback program in case the main board
// cannot load the sd card properly.

// most of it is just taken from my main mallow_rev2.c file,
// except for a few changes that had to be made from the switch to
// an esp32 c3

// GENERAL
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

// ESP-IDF
#include "esp_err.h"
#include "driver/spi_master.h"

// FOR SD CARD
// https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/get-started/case-study/storage-examples/sd-card-examples/sdmmc-example.html
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

// FOR DELAYS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// FOR DELTA TIME
#include "esp_timer.h"

// FOR LUA
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

// pin defs
#define PROG_SPI_HOST SPI2_HOST

#define SPI_SCK  4
#define SPI_MOSI 6
#define SPI_MISO 5

#define FLASH_CS 9  // cartridge
#define SD_CS 10 // sd card

// for the lua script
#define SCRIPT_BASE_ADDR 0x000100
#define SCRIPT_LEN_ADDR 0x000100 // 4 bytes for length
#define SCRIPT_DATA_ADDR 0x000104 // script starts right after
#define MAX_SCRIPT_SIZE (64 * 1024)
// for images
#define DIRECTORY_BASE 0x010100
#define DATA_BASE 0x011000

// SD CARD
// mount path
#define MOUNT_POINT "/SDCARD"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// SHARED SPI BUS (cartridge + sd card)
spi_bus_config_t bus_config = {
    .sclk_io_num = SPI_SCK,
    .mosi_io_num = SPI_MOSI,
    .miso_io_num = SPI_MISO,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000 // 256 is the size of the flash page, but 4000 is the size the sd card driver wants
};

// the driver handles the chip select for us now that it's a real gpio
spi_device_interface_config_t flash_dev_config = {
    .clock_speed_hz = 20 * 1000 * 1000,
    .mode = 0,
    .spics_io_num = FLASH_CS,
    .queue_size = 4,
    .command_bits = 8, // instruction byte
    .address_bits = 24 // the w25q128 uses 24 bit addresses
};

spi_device_handle_t flash_handle;

// stops the program with a message, delay so the watchdog doesnt complain.
static void __attribute__((noreturn)) halt(const char *msg){
    printf("ERR: %s\n", msg);
    while (1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// SD FUNCTIONS

esp_vfs_fat_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
};

// init sd
// attaches the sd card to the shared spi bus. the bus has to be
// initialized (spi_bus_initialize) before calling this.
sdmmc_card_t *card;
bool init_sd(void){
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = PROG_SPI_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS;
    slot_config.host_id = PROG_SPI_HOST;

    esp_err_t ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK){
        printf("failed to mount file system :(\nerror: %s\n", esp_err_to_name(ret));
        return false;
    }
    printf("sd card mounted successfully :)\n");
    return true;
}

// FLASH FUNCTIONS
// commands like 02h are taken from documentation
// link to documentation: https://www.pjrc.com/teensy/W25Q128FV.pdf
esp_err_t flash_init(void){
    esp_err_t err = spi_bus_add_device(PROG_SPI_HOST, &flash_dev_config, &flash_handle);
    printf("spi bus add device: %s\n", esp_err_to_name(err));
    return err;
}

void flash_write_enable(void){
    spi_transaction_ext_t t = {0};
    t.base.cmd = 0x06;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 0;
    spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
}

uint8_t flash_read_status(void){
    spi_transaction_ext_t t = {0};
    t.base.cmd = 0x05;
    t.base.length = 8;
    t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 0;
    spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
    return t.base.rx_data[0];
}

void flash_read_jedec_id(uint8_t output[3]){
    spi_transaction_ext_t t = {0};
    t.base.cmd = 0x9F;
    t.base.length = 24;
    t.base.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 0;
    spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
    memcpy(output, t.base.rx_data, 3);
}

void flash_wait_busy(void){
    while(flash_read_status() & 0x01){
        vTaskDelay(1);
    }
}

void flash_read_data(uint32_t address, uint8_t *buf, size_t len){
    size_t read = 0;
    while (read < len){
        size_t chunk = min((size_t)4000, len-read);
        spi_transaction_t t = {0};
        t.cmd = 0x03;
        t.addr = address + read;
        t.length = chunk * 8; // driver wants bits, not bytes
        t.rx_buffer = buf + read;
        spi_device_polling_transmit(flash_handle, &t);
        read+=chunk;
    }
}

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

void flash_erase_all(void){
    flash_write_enable();

    spi_transaction_ext_t t = {0};
    t.base.cmd = 0xC7;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 0;
    spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
    flash_wait_busy();
}


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
    if (len == 0xFFFFFFFF || len > MAX_SCRIPT_SIZE || len == 0x00000000){
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

bool send_images_to_flash(void){
    typedef struct {
        const char *path; const char* name; uint16_t w; uint16_t h;
    } Files;

    Files files[] = {
        // example of sending images to the cartridge
        // make sure you have your path right as well as your image sizes
        // const char* name is what you refer to your file as in your lua script
        // {"/SDCARD/images/0.bin", "0.bin", 220, 220},
        // {"/SDCARD/images/1.bin", "1.bin", 220, 220},
        // {"/SDCARD/images/2.bin", "2.bin", 220, 220},
        // {"/SDCARD/images/3.bin", "3.bin", 220, 220},
        // {"/SDCARD/images/4.bin", "4.bin", 220, 220},
        // {"/SDCARD/images/5.bin", "5.bin", 220, 220},
        // {"/SDCARD/images/6.bin", "6.bin", 220, 220},
        // {"/SDCARD/images/7.bin", "7.bin", 220, 220},
        // {"/SDCARD/images/8.bin", "8.bin", 220, 220},
        // {"/SDCARD/images/9.bin", "9.bin", 220, 220},
        // {"/SDCARD/images/10.bin", "10.bin", 220, 220},
        // {"/SDCARD/images/11.bin", "11.bin", 220, 220},
        // {"/SDCARD/images/12.bin", "12.bin", 220, 220},
        // {"/SDCARD/images/13.bin", "13.bin", 220, 220},
        // {"/SDCARD/images/14.bin", "14.bin", 220, 220},
        // {"/SDCARD/images/15.bin", "15.bin", 220, 220},
    };
    int file_count = sizeof(files) / sizeof(files[0]);

    printf("send images to flash: erasing whole chip, this can take a while...\n");
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
        printf("send images to flash: wrote \"%s\" (%ld bytes)\n", files[i].name, size);
    }

    // write image directory base
    flash_write_data(DIRECTORY_BASE, (uint8_t*)entries, sizeof(entries));

    printf("send images to flash: success :)\n");
    return true;
}

// SCRIPT LOADING
char *load_script_sd(const char *path){
    FILE *file = fopen(path, "rb");
    if (file == NULL){
        printf("load script: couldn't open %s\n", path);
        return NULL;
    }

    // check file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0 || file_size > MAX_SCRIPT_SIZE){
        printf("load script: %s is empty or bigger than %d bytes\n", path, MAX_SCRIPT_SIZE);
        fclose(file);
        return NULL;
    }

    char *buffer = malloc(file_size + 1); // +1 for '\0'
    if (buffer == NULL){
        printf("load script: malloc failed :(\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read < (size_t)file_size && ferror(file)){
        printf("load script: error reading the script file\n");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0'; // cant forget that null terminator
    fclose(file);

    return buffer;
}

static bool dir_exists(const char *path){
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int lua_noop(lua_State *L){
    return 0;
}

static const char *stub_functions[] = {
    "draw_pixel", "draw_line_h", "draw_line_v", "draw_rect", "draw_rect_filled",
    "display_fb", "draw_image", "fill_screen", "draw_circle", "draw_circle_filled"
};

// button globals
static const char *button_names[] = {"U", "D", "L", "R", "M", "W"};

void app_main(void) {
    // SHARED SPI BUS INIT (cartridge and sd card)
    esp_err_t err = spi_bus_initialize(PROG_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    printf("spi bus init: %s\n", esp_err_to_name(err));
    if (err != ESP_OK) halt("couldn't initialize the spi bus");

    if (flash_init() != ESP_OK) halt("couldn't attach the cartridge to the bus");

    // CARTRIDGE CHECK
    uint8_t id[3];
    flash_read_jedec_id(id);
    printf("JEDEC ID: %02X %02X %02X\n", id[0], id[1], id[2]);
    if ((id[0] == 0xFF && id[1] == 0xFF && id[2] == 0xFF) || (id[0] == 0x00 && id[1] == 0x00 && id[2] == 0x00)){ halt("no cartridge detected (jedec id is all FF or all 00), check the wiring"); }

    // SD CARD
    if (!init_sd()) halt("couldn't mount the sd card");

    // WHAT TO PROGRAM
    bool have_images = dir_exists(MOUNT_POINT "/images");
    char *script = load_script_sd(MOUNT_POINT "/main.lua");

    if (!have_images && script == NULL){
        halt("nothing to program: put main.lua and/or an images folder on the sd card");
    }

    if (have_images){
        if (!send_images_to_flash()) halt("sending images to the cartridge failed");
    }

    if (script != NULL){
        flash_write_script(script);
        printf("wrote script, len: %d\n", (int)strlen(script));

        char *loaded = flash_read_script();
        bool ok = (loaded != NULL) && (strcmp(loaded, script) == 0);
        free(loaded);
        free(script);
        if (!ok) halt("script verify failed :(\tread-back doesn't match what was written");
        printf("script verified :)\n");
    }

    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    printf("programming complete! running the script from the cartridge\n");

    char *loaded = flash_read_script();
    if (loaded == NULL){
        printf("flash_read_script returned null, either the read or write failed.\n");
        return;
    }
    printf("read back %d bytes:\n%s\n", (int)strlen(loaded), loaded);

    lua_State *L = luaL_newstate();
    if (L == NULL){
        free(loaded);
        halt("lua init failed :(");
    }
    luaL_openlibs(L);

    for (int i = 0; i < sizeof(stub_functions)/sizeof(stub_functions[0]); i++){
        lua_register(L, stub_functions[i], lua_noop);
    }
    for (int i = 0; i < sizeof(button_names)/sizeof(button_names[0]); i++){
        lua_pushnumber(L, 0);
        lua_setglobal(L, button_names[i]);
    }

    if (luaL_dostring(L, loaded) != 0){
        printf("LUA ERR: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    free(loaded);

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
