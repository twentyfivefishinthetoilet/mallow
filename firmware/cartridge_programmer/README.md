# mallow cartridge programmer

## wiring diagram
<img width="840" height="656" alt="image" src="https://github.com/user-attachments/assets/739d6b02-c02e-4be3-9af0-0b9c3baef313" />

## BOM

| Item Name	| Qty	| Price (total) (USD)	| Item link |
| :--- | :--- | :--- | :--- |
| ESP32 C3 SUPERMINI | 1 | $3.05 | https://www.aliexpress.us/item/3256810425066082.html?spm=a2g0o.productlist.main.2.185a1402aw0jOC&algo_pvid=af18fc56-25fc-4e07-ac92-700b821fffbf&algo_exp_id=af18fc56-25fc-4e07-ac92-700b821fffbf-1&pdp_ext_f=%7B%22order%22%3A%2291%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%216.07%213.05%21%21%2140.36%2120.26%21%402101de2517924461357865798e0d1d%2112000052966958299%21sea%21US%216315051454%21X%211%210%21n_tag%3A-29919%3Bd%3A4760036b%3Bm03_new_user%3A-29895&curPageLogUid=clTMfJ0tt8Ts&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005010611380834%7C_p_origin_prod%3A |
| TF/microSD card slot | 1 | $1.93 | https://www.aliexpress.us/item/3256806934665092.html?spm=a2g0o.productlist.main.1.1a7030e4EjzNEm&algo_pvid=8beec59c-e865-4a1c-9e1c-2c0a40b03c1c&algo_exp_id=8beec59c-e865-4a1c-9e1c-2c0a40b03c1c-0&pdp_ext_f=%7B%22order%22%3A%22152%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%211.93%211.93%21%21%211.93%211.93%21%402101e81117924461989696960e0fce%2112000039471802294%21sea%21US%216315051454%21X%211%210%21n_tag%3A-29919%3Bd%3A4760036b%3Bm03_new_user%3A-29895&curPageLogUid=VohZGbMqfrLH&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005007120979844%7C_p_origin_prod%3A |
| Mallow cartridge | 1 | priceless 🥹 | https://github.com/twentyfivefishinthetoilet/mallow/tree/main/pcb/game_cartridge |

## USAGE
first, download everything here as a zip file, or create a new esp-idf project and copy-paste source code from each file. 

then, format your microsd card in this way (flash_update.txt is a blank file. it's not needed for revision 2.):

<img width="418" height="131" alt="image" src="https://github.com/user-attachments/assets/386ae694-0920-4ab1-8f70-0297b610d55e" />

all of your images should be RGB565 compatible in a .bin file format. you can convert a png to a bin file by using the python script below:
```py
from PIL import Image
import struct
import sys
import os

TRANSPARENT_KEY = (255, 0, 255)  # 0xf81f is pure magenta, mallow's transparent color'
SUPPORTED_EXTENSIONS = (".png", ".jpg", ".jpeg", ".bmp", ".gif")


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_image_to_raw(input_path, output_path, use_alpha=True):
    img = Image.open(input_path).convert("RGBA")
    width, height = img.size
    pixels = img.load()

    with open(output_path, "wb") as f:
        for y in range(height):
            for x in range(width):
                r, g, b, a = pixels[x, y]

                # transparency handling
                if use_alpha and a == 0:
                    color = 0xF81F
                elif (r, g, b) == TRANSPARENT_KEY:
                    color = 0xF81F
                else:
                    color = rgb888_to_rgb565(r, g, b)

                f.write(struct.pack("<H", color)) # little-endian uint16

    print(f"converted: {input_path}")
    print(f"size: {width}x{height}")
    print(f"output: {output_path} ({width * height * 2} bytes)")

def convert_directory(input_dir, output_dir, use_alpha=True):
    if not os.path.isdir(input_dir):
        print(f"error: '{input_dir}' is not a directory")
        sys.exit(1)

    os.makedirs(output_dir, exist_ok=True)

    files = sorted(
        f for f in os.listdir(input_dir)
        if f.lower().endswith(SUPPORTED_EXTENSIONS)
        and os.path.isfile(os.path.join(input_dir, f))
    )

    if not files:
        print(f"no supported images found in '{input_dir}'")
        return

    success, failed = 0, 0
    for filename in files:
        input_path = os.path.join(input_dir, filename)
        base_name = os.path.splitext(filename)[0]
        output_path = os.path.join(output_dir, base_name + ".bin")

        try:
            convert_image_to_raw(input_path, output_path, use_alpha)
            success += 1
        except Exception as e:
            print(f"Failed: {input_path} ({e})")
            failed += 1

    print(f"\ndone. {success} converted, {failed} failed.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: python png2raw.py <input_dir> [output_dir]")
        print("if output_dir is omitted, files are written to <input_dir>/raw")
        sys.exit(1)

    input_directory = sys.argv[1]
    output_directory = sys.argv[2] if len(sys.argv) > 2 else os.path.join(input_directory, "raw")

    convert_directory(input_directory, output_directory)
```

if you added any images to flash, then you'd need to edit `cartridge_programmer/main/cartridge_programmer.c` at line 290+, then flash your supermini. if not, dont worry about it.
```c
...
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
...
```

now it's time for your lua file contents (main.lua)! the current builtin draw functions i have are listed here:
```c
static const char *stub_functions[] = {
    "draw_pixel", "draw_line_h", "draw_line_v", "draw_rect", "draw_rect_filled",
    "display_fb", "draw_image", "fill_screen", "draw_circle", "draw_circle_filled"
};
```

...pretty minimal, but you can probably make snake or something. if you want a simple test file, use this! 
```lua
counter = 0
laps = 0
function update()

end

function render()
  if counter >= 2.5 then
    if laps == 0 then
      fill_screen(0xf800)
      laps = 1
    else
      fill_screen(0x001f)
      laps = 0
    end
    display_fb()
    counter = 0;
  end
  counter = counter + delta_time
end
```

okay, you have everything on your sd card. now what? 


plug your microSD card into the slot, wire up your cartridge, and flash `cartridge_programmer.c` to the supermini (`idf.py flash monitor`). 
if the terminal says everything is okay, unplug the power then the cartridge, pop it into mallow, and test it out!

<hr>

happy gaming!
