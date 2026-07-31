# DAY 1 (7/26/26) @ 9:16: A FRESH START
Total time spent: 2:47

today i started a new schematic (see below). the schematic contains...
- an ESP32 S3 WROOM 1U
- an LD1117S33TR (5v -> 3.3v voltage regulator)
- a USB-C port
- BOOT and EN buttons
- a sketchy ST7789 setup which i'm not confident in
- a sketchy MAX98357A(ETE+T) setup which i'm also not confident in

<img width="2362" height="1672" alt="SCH_malrev2_72626" src="https://github.com/user-attachments/assets/c8485e01-7afb-4b4e-aefe-bb01da0587b6" />

<hr>

EARLY PCB LAYOUT (created/edited around 1:50PM)

its going to change when i get components in from aliexpress (mainly my screen that i need to take measurements of), which is probably going to take forever. see that fpc connector? thats where the screen ribbon cable will sit.

R5 is a zero ohm resistor, i'll only add that if i want to connect that to a gpio. it looks nicer than me wiring it to that gpio then cutting the trace if i dont want it.

that screw terminal probably isnt going to go there, i just needed the holes and that was the cleanest way to do it. a speaker is probably going to be soldered directly to the board, rather than being placed in the terminal.

<img width="511" height="382" alt="Screenshot 2026-07-26 142528" src="https://github.com/user-attachments/assets/3c158481-c7eb-4586-8742-1c11530e207c" />

<hr>

CURRENT PCB LAYOUT (edited around 6:50PM)

im gonna cave and order this and the parts today. these are all of the core components, i'll add the accessories later (aux jack, etc). 

<img width="801" height="562" alt="Screenshot 2026-07-26 192315" src="https://github.com/user-attachments/assets/ba2cb775-b825-4389-930b-47c17917f6a9" />

<hr>

ONE THING I LEARNED

im going to put a few things here actually. 
- did you know USB-C has dedicated pins for aux accessories? they're pins SBU1 and SBU2.
- i learned how to make a new component in EasyEDA
- learned what decoupling caps are (basically a filter for steady voltage)
- the boot button just shorts the system sort of, basically turning it off and on again when released
- the ESP32-S3's pins are VERY flexable, unlike the Arduino's (which i originally came from)

RESOURCES:
- i can't exactly remember where i got the schematic from. i was looking off of an older project's schematics (mallow rev.1) as a reference. once i find the youtube tutorial i used, i'll update it here.
- [ESP32 S3 datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [ST7789 / GMT020-02 datasheet](https://www.makerguides.com/wp-content/uploads/2025/01/GMT020-02-Datasheet.pdf) (referenced for the 8 bit wiring and display pinout)

<hr>

# DAY 2 (7/27/26): PROGRAMMING
Total time spent: 1:32

while i wait for my circuit board to come in, i decided to get started on the dreaded programming. today was just me learning how to use the ESP-IDF, flash it to an ESP32S3, and display something on the ST7789. all of those components are going to be what im going to use on the circuit board when it comes in. find the program in mallow/breadboard_testing/main_727.c. there's timestamps and some explanations about what the code does. 

i decided to use the esp idf this time instead of TFT_eSPI because, even though there's more lines of code to write, it's supposedly faster. besides, i like a challenge (and i like C more than C++).

<hr>

let's go line by line with the code. this'll be probably my longest journal entry, so strap in. i gotta prove i understand what im writing somehow!

**INCLUDES**:
```c
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
```
USAGE:
| INCLUDE NAME | USAGE |
| --- | --- |
| stdio.h | standard input and output, everyone knows what that is |
| esp_lcd_panel_ops.h | handles display init, screen orientation, resetting, displaying bitmaps, etc |
| esp_lcd_panel_io.h | i still actually have no clue what this one does |
| esp_lcd_panel_vender.h | i think it just includes headers for a bunch of different display chips. |
| spi_master.h | a header for all spi needs (controls slave/secondary devices, multithreading(?), etc.) |
| gpio.h | this one's an easy one. controls the direction of gpio pins (ex. gpio_config(...) or gpio_set_level(...)) |

*i googled what each library is used for after uploading the code to github*

**CONFIGURAIONS**:
```c
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
```
WHAT IT DOES:

sets up configurations for 3 things: the io library, the panel itself, and the spi bus. these configurations hold pin numbers and other library based configurations, like the max bytes the transfers will be (320 * 240 * 2). TFT_eSPI for the arduino ide did all the work for me before, so this is all new to me. so sorry if i dont explain this right. the way ive been setting this up before was just with a simple `TFT_eSPI tft = TFT_eSPI();`.

oh, and by the way, the panel variable is what gets used the most. for example, esp_lcd_panel_init(panel) to initialize it, esp_lcd_panel_reset(panel) to reset it, another function to draw to it, etc.

**FRAMEBUFFER**:
```c
// framebuffer and drawing a color to the screen (7/27/26 @ 15:24)
uint16_t fb[240 * 320];
```
WHAT IT DOES:

defines a framebuffer for showing visuals on the display. a framebuffer is a block of memory that contains color data before it's shown on screen. it's the size of the screen (320 * 240) because that's how many pixels that can be shown. 

**APP_MAIN/MAIN FUNCTION**:
```c
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
```
most of the stuff is explained in the code comments, so i'll write stuff i didn't touch on. 
- DMA is direct memory access, meaning the RAM bypasses the CPU entirely when sending data to the screen.
- where it says "continued", most of those are just initializing the variables i defined before (ex. panelconf)
- one thing i noticed was that framebuffers dont need to be 2d to work. it's basically the same as writing fb\[240]\[320], just different syntax.
- espidf doesnt need a return value for app_main, since it doesnt have an os to return to.

<hr>


IMAGE 1: wiring
<img width="3024" height="3024" alt="1000035585" src="https://github.com/user-attachments/assets/2f82a37c-fb35-46c2-8112-4a268b318465" />
for any of those wondering, my wiring is as follows:

```c
#define PIN_CS 10
#define PIN_RESET 4
#define PIN_DC 5
#define PIN_SDA_MOSI 11 // the st7789 uses weird pin labels. why SDA for MOSI?
#define PIN_SCL_SCK 12
```

IMAGE 2: IT WORKS!
<img width="3024" height="3024" alt="1000035586" src="https://github.com/user-attachments/assets/b8f43974-edcb-43e2-8668-b8fe862b7a90" />

ONE THING I LEARNED:

again, im going to put a few things here.

- a general idea of how SPI works (i'd love to rant about it to someone, ask me about it sometime)
- how to use esp-idf
- the st7789 has weird pin names (i2c names for SPI? what gives?)

# DAY 3 (7/30/26): SPI STUDY SESSION 

Time time spent: 1:30 

in order to learn how to flash my game cartridges, i need to actually know how SPI and the flash chip work.


SPI:

here are the common pins for 4 pin spi.

| PIN NAME | USAGE | 
| --- | --- |
| CS (CHIP SELECT) | active LOW, this pin is only active when data is being transferred. marks the end of a transaction when it goes HIGH. |
| SCK (CLOCK) | a clock signal generated by the MCU. provides timing for data lines so the mcu knows how to interpret the data. |
| MOSI (MASTER OUT, SLAVE IN) | data out. ex, the mcu sends a command to a display to turn it on. |
| MISO (MASTER IN, SLAVE OUT) | data in. ex, the display needs to send a touch coordinate to the screen.

note that data can only be transferred when CS is active, or else the MCU wouldnt know where to send the data. theres no set naming convention for spi: for example, SDI can mean MOSI--they're interchangeable. each device defines what each command does. for example, the command 0x3c means write memory continue on the ili9341, but it can mean something completely different for a temperature sensor. 

HOW DATA WORKS: different registers are set to different functions. the controller knows what function to use by the stream of data that gets send, usually in 8 bytes, the first byte being read or write mode, while the other 7 are for which register or instruction you want to use. for example, according to the [W25Q128 FLASH MEMORY DOCUMENTATION](https://www.pjrc.com/teensy/W25Q128FV.pdf) im looking at, it says that sending 0000101 (05h) through SDI is the read status register command. it's basically binary, which i find cool

sending another 8 bits sends a parameter for that instruction, if its an instruction that takes in a parameter. somehow the slave device knows how many clock pulses to be waiting until turning off the line, i'll look into that more. the process goes from CONTROL BYTE -> DATA BYTE -> CONTROL BYTE -> DATA BYTE...

i think libraries do all of the data processing under the hood. sending control byte after control byte after control byte would take up a lot of space in code, so thank god for files. heres what sending an ili9341 each and every init command would look like manually:

```c
void ili9341_init()
{
    lcd_reset();

    // SOFTWARE RESET
    lcd_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(5));

    // TURN DISPLAY OFF
    lcd_cmd(0x28);

    uint8_t data[5];

    // EXTENDED (0xEF)
    lcd_cmd(0xEF);
    data[0] = 0x03; data[1] = 0x80; data[2] = 0x02;
    lcd_data(data, 3);

    // POWER CONTROL B (0xCF)
    lcd_cmd(0xCF);
    data[0] = 0x00; data[1] = 0xC1; data[2] = 0x30;
    lcd_data(data, 3);

    // POWER SEQUENCE C (0xED)
    lcd_cmd(0xED);
    data[0] = 0x64; data[1] = 0x03; data[2] = 0x12; data[3] = 0x81;
    lcd_data(data, 4);

    // TIMING A (0xE8)
    lcd_cmd(0xE8);
    data[0] = 0x85; data[1] = 0x00; data[2] = 0x78;
    lcd_data(data, 3);

    // POWER CONTROL A (0xCB)
    lcd_cmd(0xCB);
    data[0] = 0x39; data[1] = 0x2C; data[2] = 0x00; data[3] = 0x34; data[4] = 0x02;
    lcd_data(data, 5);

    // PUMP RATIO (0xF7)
    lcd_cmd(0xF7);
    data[0] = 0x20;
    lcd_data(data, 1);

    // TIMING B (0xEA)
    lcd_cmd(0xEA);
    data[0] = 0x00; data[1] = 0x00;
    lcd_data(data, 2);

    // POWER CONTROL 1 (0xC0)
    lcd_cmd(0xC0);
    data[0] = 0x23;
    lcd_data(data, 1);

    // POWER CONTROL 2 (0xC1)
    lcd_cmd(0xC1);
    data[0] = 0x10;
    lcd_data(data, 1);

    // VCOM CONTROL 1 (0xC5)
    lcd_cmd(0xC5);
    data[0] = 0x3E; data[1] = 0x28;
    lcd_data(data, 2);

    // VCOM CONTROL 2 (0xC7)
    lcd_cmd(0xC7);
    data[0] = 0x86;
    lcd_data(data, 1);

    // PIXEL FORMAT
    lcd_cmd(0x3A);
    data[0] = 0x55;
    lcd_data(data, 1);

    // MADCTL / ORIENTATION
    lcd_cmd(0x36);
    data[0] = 0xE8;
    lcd_data(data, 1);

    // SLEEP OUT
    lcd_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));

    // DISPLAY ON
    lcd_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(20));
}
```
what a pain! it's a good way to learn the commands and how spi work though, i might do this for my flash memory programmer...

now that im throwing code into this log, i might as well show what data and cmd look like.

```c
// send command
// cmd: tells the slave what operation to preform
void lcd_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .length = 8, // the ili9341 runs off of 8 bit chunks
        .tx_buffer = &cmd // a pointer for data to be sent
    };

    gpio_set_level(PIN_NUM_DC, 0); // set the display to command mode (dc off)
    spi_device_transmit(spi, &t);
}
```
this doesnt really apply to what im working with, given i dont have a dc pin on the flash memory.  i thought i'd include it anyways, it makes me look smart. 


this is what a reset looks like...

```c
void lcd_reset()
{
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}
```
alright, that's enough code, this is a hardware project.

TO RECAP FROM MY RAMBLINGS:

spi sends data through binary, 1 and 0 pulses at the rate of the clock speed, SCK. 

