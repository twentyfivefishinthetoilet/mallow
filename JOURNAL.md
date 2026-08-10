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

<img width="693" height="423" alt="image" src="https://github.com/user-attachments/assets/a38e46a6-6a4a-4355-9469-c5ddee016d1a" />

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

# DAY 4 (8/4/26): DISPLAYING IMAGES
Total time spent: 1:22

today was a short day. i may do more later, but for now, all i did was convert an image to a bitmap, put that in a header file, and display it on the screen. i also got transparent colors working without an alpha channel.

in older consoles, like the gameboy, it had to deal with the small memory that it had. the gameboy only had 4 color channels, index 0 being for "transparency". it handles transparency by skipping that color entirely when drawing an image to the screen. 

BEFORE TRANSPARENCY 
```c
    // clear screen/fill framebuffer
    for (int i = 0; i< 320* 240; i++){
        fb[i] = 0xffff;
    }

    // draws the app icon while swapping the bytes for correct colors
    for (int i = 0; i< 198 * 132; i++){
        fb[i] = __builtin_bswap16(appicons[i]);
    }
    // display fb 
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);
```

<img width="3024" height="3024" alt="20260804_135018" src="https://github.com/user-attachments/assets/3fa45432-8146-4df8-b441-c4b9580ef7f8" />

AFTER TRANSPARENCY 
```c
    // clear screen/fill framebuffer
    for (int i = 0; i< 320* 240; i++){
        fb[i] = 0xffff;
    }

    for (int y = 0; y < ICON_H; y++) {
        for (int x = 0; x < ICON_W; x++) {
            uint16_t px = appicons[y * ICON_W + x];
            if (px != 0xf81f) { // basically saying "if this pixel's color is the one we want to be transparent,
                fb[y * FB_W + x] = __builtin_bswap16(px);  // do this
            }
        }
    }

    // display fb 
    esp_lcd_panel_draw_bitmap(panel, 0, 0, 320, 240, fb);
```
<img width="3024" height="3024" alt="20260804_135458" src="https://github.com/user-attachments/assets/820a5de4-ab52-459a-b275-b900283b7e3c" />

...by the way, this is what happens when you dont configure the screen right. it took me probably 20 minutes just to figure out the right configuration that the screen uses for proper colors. the image of that is below.

<img width="3024" height="3024" alt="20260804_132311" src="https://github.com/user-attachments/assets/2d969000-a3c5-495f-9375-576e61f327dc" />

the new code will be added to breadboard_testing, titled main_804. nothing much changed, though.

next up is animations! 

EDIT @ 16:45 - I GOT ANIMATIONS RUNNING!!!

it was really easy actually, its just drawing bitmaps to the screen with a small 20ms delay. it runs at 50fps.

<img width="480" height="854" alt="20260804_162937" src="https://github.com/user-attachments/assets/937dba7d-b73b-4a7c-97e7-a5ccaca0af6e" />

im proud to say that this isnt sped up. this is amazing compared to the 3 frames-per-whenever i was getting with TFT_eSPI and the Arduino IDE. here's a quick rundown of what i added:
- offset variables for animation positioning on screen (basic math, but im proud of it)
- a frame index to iterate through the frames properly
- new includes for timing 

# DAYS 8/5 -> 8/8: PLAYING CATCH-UP
Total time spent: 6:00

i've been working on my programming over the span of a few days now and i forgot to update my journal. i've set myself a schedule to work on Mallow 2 hours a day, and HOPEFULLY i can stick to that. if i dont, i'll be truthful with my hours logged and try to make up for it on later days i guess.

enough about the boring stuff, let's get into what you're here for. over the past days, it's been a bunch of programming and some soldering. i'll split this part up in 2 sections: hardware and software

**HARDWARE:**

i soldered what i got in from aliexpress onto my test circuit board, mallow rev 2.1. im still waiting on capacitors, buttons, and an fpc(?) connector for the screen. the ominous wires go to a speaker, which im now realizing as i type this that it'll be a pain to get sound out of it without a file to play... hopefully i can just do some beeps and boops through i2s so i dont need an sd card. one thing i think i should have done was at least wire up some through holes to solder to so it can be a breakout board if all else fails.

<img width="3024" height="3024" alt="20260808_211047" src="https://github.com/user-attachments/assets/679dbde9-2394-41b6-9178-d44807b3dc44" />

i'll probably come back tomorrow if my capacitors decide to show up, solder them on, and tell you what works and what doesn't. 

**SOFTWARE:**

software is a pain in my behind. im just going to go over the major stuff that i added since 8/4. 

VISUALS:

draw_pixel basically does all of the heavy lifting here. it picks a coordinate in the 1d framebuffer, swaps the bytes of it so it presents the right color, and sends it off to the framebuffer.

```c
void draw_pixel(int x, int y, uint16_t color)
{
    if (x < 0 || x >= FB_W || y < 0 || y >= FB_H) return;

    fb[y * FB_W + x] = __builtin_bswap16(color);;
}
```

here's an example of it being used to draw a line.

```c
void draw_line_horizontal(int sx, int dx, int y, uint16_t color){
    for (int i = sx; i<dx; i++){
        draw_pixel(i, y, color);
    }
}
```

other functions i added included were `draw_line_vertical`, `draw_rect`, `draw_rect_filled`, and `fill_screen`. all of these lead back to draw_pixel and some for loops, so that's why im only showing the two functions above.

<img width="3024" height="3024" alt="graphical_functions" src="https://github.com/user-attachments/assets/81d84f0d-9130-44bf-9317-c03a001c0255" />

"GAME" LOOP:

i learned that for games, you should almost always have an `update` and `render` function for organization. im using this for the main system too for some reason, i'll probably remove it later and move it into the game file that's gonna be stored in external flash memory. in short, `update` is used to update variables (ex. `player_x = speed * dt`) and render is to actually display what was updated. there's probably an `init` function that should be used that works like the `setup` function for arduino, but im just doing that in `app_main` right now.

BUTTON INPUTS:

finally, something simple and hardware related. i wired up 2 gigantic buttons to pins 20 and 21 and read the values in code. since im using internal pullups, the buttons are an active LOW (0). alright, back to the software... i made a button class that holds states, the button's name, and the pin number. 

```c
typedef struct {
    int pin; int state; int last_state; char name;
} button;

button buttons[2] = {
    {GREENPIN, 0, 0, 'M'},
    {BLUEPIN, 0, 0, 'W'}
};
```

...then, i made a quick helper function to get a button press.

```c
int get_btn_press(button *b){
    b->last_state = b->state;
    b->state = gpio_get_level(b->pin);
    // below expands to (in lua)...
    // if b.last_state == 1 && b.state == 0 then return 1 else return 0 end
    return (b->last_state == 1 && b->state == 0);
}
```

...which can be called like what i do in `render`.

```c
...
    if (get_btn_press(&buttons[0])==1){
        fill_screen(0x07e0);
    }

    if (get_btn_press(&buttons[1])==1){
        fill_screen(0x001f);
    }
...
```

MISC: 

- if you look at main_808, you'll see an array defined as `large_font_letters large_letters`. i attempted to make my own font, but then quickly backed out of it. i tried doing it on my own, then used claude, then decided to remove it entirely since i didn't want to just copy and paste code i didn't understand. i'll probably come back to making a custom font again later, maybe after i understand sprite sheets and bitmaps a little better.
- rand_offset was also going to be used for the text to give it some character.
- i stopped recording my exact timestamps a while ago because i kept forgetting. i think just putting in what i did for each day (unlike this entry) is enough.
- for buttons, you need to configure gpio pins, just like in the arduino ide.

```c
    // green
    gpio_reset_pin(buttons[0].pin);
    gpio_set_direction(buttons[0].pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttons[0].pin, GPIO_PULLUP_ONLY);// enables internal pullup (active low)
    // blue
    gpio_reset_pin(buttons[1].pin);
    gpio_set_direction(buttons[1].pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttons[1].pin, GPIO_PULLUP_ONLY);// enables internal pullup (active low)
```

the above code is in my `app_main` function. `gpio_reset_pin` resets the pin to its default state. `gpio_set_pin_direction` defines whether it's an input or output, and the other one is enabling the pullup resistors. i feel like this all could have been self explanatory, now that im typing all of this out.

when i add more buttons, im probably going to do something similar to what i did for mallow rev 1...
```cpp
  for (const Button &btn : Buttons) {
    pinMode(btn.pin, INPUT_PULLUP);
  }
```

since that's c++, it'll probably be a little weirder syntax-wise to write in C, but it's doable. probably just going to do something like the following.

```c
for (int i=0;i<=MAX_BUTTONS;i++){
    gpio_reset_pin(buttons[i].pin);
    gpio_set_direction(buttons[i].pin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(buttons[i].pin, GPIO_PULLUP_ONLY);// enables internal pullup (active low)
}
```
**WHAT'S NEXT?**

well, i finished my initial goals ("roadmap") that i set for myself in my code, but im far from done. i need to still do the following...

- read flash memory (read the game file on it)
- write to flash memory (write the game file to it)
- make a gui in raylib to make writing to the flash easier for people to use (gui in C is the only way i know how to do it, i refuse to learn any other way) 
- hook up a micro SD card reader, read directories from that (i hope there's a library for that)
- get my circuit board working, design a new circuit board, etc

**ONE THING I LEARNED**

POINTERS ARE ACTUALLY IMPORTANT!

# DAY 9 - 8/9/26: GAME CARTRIDGE BASICS
Total time spent: 3:30

as the title states, ive been focusing on how the game cartridges are going to work today. the workflow of the game cartridge will be the same as revision 1's: 

the game cartridge has a flash memory chip on it that holds the game files. the game file gets read through spi by the esp32 s3. program-wise (yup, another programming post...) and getting a little more in depth, the lua file gets ran by the lua virtual machine / C code and will be ran constantly with render and update functions, sort of like how LOVE2D does it. 

example: 

lua/game file
```lua
counter = 0
function update
counter = counter + 1
end

function render
draw.text(counter)
end
```
C (app_main)

```c
...
// GET AND CALL UPDATE
lua_getglobal(L, "update");
if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        tft.println(lua_tostring(L, -1));
        lua_pop(L, 1);
      }
} else {
    lua_pop(L, 1);
}
// GET AND CALL RENDER
sprite.fillSprite(0xffff);
lua_getglobal(L, "render");
if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        tft.println(lua_tostring(L, -1));
        lua_pop(L, 1);
     }
} else {
  lua_pop(L, 1);
}
...
```

...i just stole the C code from my revision 1 code. i'll explain what the lua part of it means in that section of this log.

this log will be split into two parts again: flash memory and lua.

FLASH MEMORY

after writing all of this, i found out that littefs exists. im just going to stick to raw bytes for the moment, since it's more low level and interesting.  i'll go line by line of the flash part and explain what it does.

```c
// flash macros
#define FLASH_CS   8
#define FLASH_SCK  48
#define FLASH_MOSI 18
#define FLASH_MISO 17

#define SCRIPT_BASE_ADDR 0x000000
#define SCRIPT_LEN_ADDR  0x000000 // 4 bytes for length
#define SCRIPT_DATA_ADDR 0x000004 // script starts right after
```
two sets of macros here: pin definitions and addresses for memory. script len address stores the length of the script, as you probably could have guessed. this is necessary because the w25q128 just stores raw bytes, it doesn't know when a string ends. flash cs, sck, mosi, and miso macros are just pin definitions. 

```c
spi_device_interface_config_t flash_dev_conf = {
    .clock_speed_hz = 20 * 1000 * 1000,
    .mode = 0, // what other spi modes are there? beats me
    .spics_io_num = FLASH_CS,
    .queue_size = 4,
    .command_bits = 8,   // the instruction byte 
    .address_bits = 24,  // the w25q128 uses 24 bit addresses
};
```
sets up the flash spi line

```c
// sets up and attaches flash memory to the bus
void flash_init(void){
    esp_err_t err;
    err = spi_bus_initialize(SPI3_HOST, &flash_bus_conf, SPI_DMA_CH_AUTO);
    printf("spi_bus_initialize: %s\n", esp_err_to_name(err));
    err = spi_bus_add_device(SPI3_HOST, &flash_dev_conf, &flash_handle);
    printf("spi_bus_add_device: %s\n", esp_err_to_name(err));
}
```
error handling was added here because spi wasnt working right, though i probably should have added it in the first place. 

```c
void flash_write_enable(void){
    spi_transaction_ext_t t = {0};
    t.base.cmd = 0x06;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 0;
    spi_device_polling_transmit(flash_handle, (spi_transaction_t*)&t);
}
```
i'm going to explain this once. for the rest of the functions, i'll just be going over what command does what (or what it claims to do in documentation). 

the `SPI_TRANS_VARIABLE_ADDR` flag tells the program to ignore the original 24 address bits defined in `flash_dev_conf` and use the following 0 bits defined the line after. the write enable command 06h doesn't have any "parameters", so it's just that singular byte that's called. 

`spi_device_polling_transmit` actually transmits the data that was just defined. 

as for the actual write command, it's defined in the documentation as so:

```
8.2.1 Write Enable (06h)
The Write Enable instruction (Figure 5) sets the Write Enable Latch (WEL) bit in the Status Register to a 1.
The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase, Block Erase, Chip Erase, Write Status Register and Erase/Program Security Registers instruction.
```

here's a little todo for me: come back and explain the rest. the other main commands are 05h, 9Fh, 02h and 20h. cut me some slack, it's 10:10 at night as im typing this and im tired.

LUA

this one's a little more fun. lua uses a stack (and also indexes starting at 1 for some reason) and it's interesting to work with the pop functions. it reminds me of a simpler version of assembly.

| func. | usage | ex. |
| :--- | :--- | :--- |
| `luaL_newstate()` | creates the interpreter | `lua_State* L = luaL_newstate();` | 
| `luaL_openlibs(lua_State* L)` | loads std libraries (math, string, tables, etc) | `luaL_openlibs(L);` |
| `luaL_dofile(lua_State* L, char *path)` | executes a lua script | `luaL_dofile(L, path.c_str());` |
| `luaL_dostring(lua_State* L, char *cmd)` | executes a line of lua code directly | `int r = luaL_dostring(L, cmd.c_str());` |
| `lua_register(lua_State* L, char *lua_function, function c_function);` | exposes C++ function to Lua code so lua can call lua_function | `lua_register(L, "drawText", lua_drawText);` |  
| `lua_pcall(lua_State* L, int expected_args, int expected_return, int error_handling)` | executes a single function with the arguments provided | `lua_pcall(L, 2, 1, 0)`|

that table is from revision 1 notes, still applies here... just excuse the C++ notation for the examples (ex.)

remember how i said i'll explain what my lua functions do in the flash section? i sure didnt. let me explain what each function does without repeating myself from what i wrote in the table. 

```c
// GET AND CALL UPDATE
lua_getglobal(L, "update");
if (lua_isfunction(L, -1)) {
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        tft.println(lua_tostring(L, -1));
        lua_pop(L, 1);
      }
} else {
    lua_pop(L, 1);
}
```

the workflow is this: C calls a lua function (in this case, "update") -> checks if "update" is a function -> error checks while calling update/pcall. if the function is ran without any issues, it pops/removes it from the stack. if not, it prints an error then removes it from the stack. 

it's a lot to take in at once! this guy can do a much better job at explaining it than me, it's who i watched to learn this anyways: https://youtu.be/4l5HdmPoynw?si=0h_c781tNLyucszX

whatever that guy didnt explain, i googled. 

MISC:
- lua indexes start at 1
- -1 index is also the top of the stack

