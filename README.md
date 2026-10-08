# mallow

<img width="250" height="250" alt="front" src="https://github.com/user-attachments/assets/edd5493a-8dd6-4076-bb8c-ac835d93225c" />

mallow is an open source handheld game console that runs off of real cartridges you've designed. mallow isn't an emulator, it's supposed to inspire you to make your own games without having to jump through so many hoops to get it running on real hardware. 

mallow has a detatchable controller, leaving you open to customizing your own controllers with a custom button layout. if you wanted to get more advanced, add a sensor! since it's open source, you can edit the source code to do so. 

i tried adding a bunch of comments in my source code so everyone knows where everything is, as well as how it works. 

## stardance judges, PLEASE READ!!!!!!!!!!!
- mallow_bom.csv should probably be opened in excel or google sheets. it's not pretty when you view it as a text file, but it'll get the job done. 
- worked on easyeda (pro) for the PCB designs, hence the .epro2 project file in the hardware folder.
- programs/FINAL is where my final program/firmware/c file will be. i uploaded many different files for backups and keeping track of progress 

## current features/components
mallow has 3 parts: the cartridge, the controller, and the main board

### main board
- 2.8" ST7789 IPS display 
    - i honestly didnt know what ips meant before buying this, but it turns out it's a good thing: ips offers a better viewing angle than the ili9341 (or any tft display) would
- SD card slot
    - dont know if it'll work when it gets shipped for stardance, but it's there!
- GPIO expander 
    - expands... gpios. seriously though, it opens up slots for more buttons or maybe a sensor or two. right now (rev 2.4), it's only using half of the extra slots. 
- 2x10 cartridge + controller slots
    - im grasping at straws here
- other/notes
    - mcu: esp32 s3 wroom 1u (n16r8)
### controller 
- x6 6mm by 6mm push buttons (UP, DOWN, LEFT, RIGHT, M, W)
- x2 3mm by 6mm push buttons (SPECIAL, HOME)
- onboard W25Q128 flash memory
    - remember how you could transfer Miis from one Wii console to another by sending it to the Wiimote, then connecting that Wiimote to the desired console? that's the idea here. at some point i'm going to program it to save game data, but it can really be used to save whatever if it's programmed right.

### game cartridge
i have to admit, this one is pretty bare board. it has the same W25Q128 flash memory chip on it that holds a "Lua file". that "Lua file" is read by the esp32 s3 by using the lua vm and runs update() and render() functions, sort of like LOVE2D. 

oh, it also has an smd led to tell you there's power. that's about it. 

## workflow 
the cartridge has a lua file stored on the chip. the esp32 s3 reads that file and runs it constantly in a loop, as well as checking for button inputs from the controller.

## gallery 
this is the first ever mallow board. 

<img width="500" height="350" alt="Screenshot_20260404_194022" src="https://github.com/user-attachments/assets/f9a3ef72-b7df-41bc-97d2-bf6f67db48fd" />

originally, it was going to have detachable controllers, just less accessible. you'd have to connect it via flex cable. i changed that now to rows of female header pins. the only issue with this change is that i dont have the side controller "slots" anymore. 

**a little todo note for me. i still need to add new pcb photos, photos of it working, export actual design files and bom. https://stardance.hackclub.com/resources/shipping-hardware**

