# DAY 1 (7/26/26) @ 9:16: A FRESH START
Total time spent: 1:32

today i started a new schematic (see below). the schematic contains...
- an ESP32 S3 WROOM 1U
- an LD1117S33TR (5v -> 3.3v voltage regulator)
- a USB-C port
- BOOT and EN buttons
- a sketchy ST7789 setup which i'm not confident in  

<img width="2362" height="1672" alt="SCH_malrev2_72626" src="https://github.com/user-attachments/assets/32142d66-a870-4dc0-903d-c5ad61fa42f2" />

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
