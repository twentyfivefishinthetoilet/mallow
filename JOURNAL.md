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

