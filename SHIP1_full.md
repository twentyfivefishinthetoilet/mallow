my text got cut short during my first ship, so i thought i'd paste the whole thing here.
<hr>

**hello everyone! it's finally time to ship!**

i made a handheld gaming device inspired by Nintendo's Game Boy that runs off of custom cartridges made by YOU! my goals were to make it affordable, run custom games, and learn a lot in the process, and i'd say i met all of them!

the most challenging part of this project was definitely the programming, though i learned a lot in the process of doing so:
- it's important to read documentation, even though it's boring. i dont care whether it's hardware or software, you gotta do it.
- SPI lines are sick! it's just a bunch of timing with 1s and 0s.
- what ground planes are, as well as stitching vias.
- advanced C syntax (did you know structs can hold functions? neither did i)

im really proud of how far mallow has come since June when I brought it to Open Sauce. it has a better refresh rate, a better chassis, and a rechargable battery. 

**WHAT TO KNOW BEFORE MAKING MALLOW (REV 2.4) YOURSELF!!!**
- you'll need the ESP-IDF on your system to flash firmware. create a new project (`idf.py create-project <name>`), copy files from the github repo and paste them into that project (make sure the C file in `main` is the same name as <name>, as well as your CMakeLists.txt files), set the target chip (`idf.py set-target esp32s3`), then flash and monitor (`idf.py flash monitor`)
- read through cartridge_programmer/README.md and source code to fully understand how flashing your cartridge works. 
<hr>
thanks for reading, and thank you for everyone who supported mallow - revision 2.
