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

