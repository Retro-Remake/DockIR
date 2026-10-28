# DockIR

Superdock IR remote receiver program.

## Current mappings

| Remote Button | Key Mapping | Description |
| ------------- | ----------- | ----------- |
| Power | Unassigned | |
| Contrast + | M | Menu for MiSTer\_DVD |
| Contrast - | T | Title Menu for MiSTer\_DVD |
| TV/AV | D | Display for MiSTer\_DVD |
| ⏩ | F | Fast Fwd for MiSTer\_DVD |
| ⏪ | R | Rewind for MiSTer\_DVD |
| P.N.S | V | |
| Vol + | Volume up | |
| Vol - | Volume down | |
| Mute | Mute Volume | |
| ⏮️ | N | Next Chapter for MiSTer\_DVD |
| ⏭️ | P | Previous Chapter for MiSTer\_DVD |
| P Up | Page Up | Previous Chapter for MiSTer\_DVD |
| P Down | Page Down | Next Chapter for MiSTer\_DVD |
| Menu | F12 | Brings up MiSTer OSD |
| L/R | Tab | Navigates back up a level in the MiSTer OSD |
| Cancel | Backspace | |
| Exit | Escape | Closes MiSTer OSD |
| Direction buttons | Up/down/left/right | |
| OK | Enter | |
| Zoom | G | Angle for MiSTer\_DVD |
| ⏸️ | Space | Pause in MiSTer\_DVD, skip button assignment in controller mapping |
| ▶️ | A | |
| ⏹️ | B | |
| ⏏️ | C | |
| Info | S | Subtitle for MiSTer\_DVD |
| 🕒 | Unassigned | |
| Numpad keys 0 through 9 | Numbers 0 through 9 | |
| 🔁 | Unassigned | |
| 10+ | - | |

## Updating DockIR firmware

1. Goto to [releases](releases) and download the `.uf2` file from the latest release
2. Connect your Superdock to your PC with your SS1 powered off
3. Inside the NVMe slot there are two small buttons, hold down the `BOOTSEL` button
   then the tap `MCU RST` button. Note: you may need to remove your NVMe drive
   to access these buttons
4. It should show up as a drive on your PC (it should be labelled as `RPI RP2` or
   something similar)
5. Copy the the `.uf2` file onto the drive. Once it has flashed it will auto-eject
   itself and your DockIR firmware will be updated

## To compile DockIR

1. Set up your PC to point to use the Raspberry Pi Pico SDK, follow [README](https://github.com/raspberrypi/pico-sdk/blob/master/README.md) in [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).
1. Clone this GitHub repo
      ```
      $ git clone https://github.com/Retro-Remake/DockIR.git
      ```
1. Change to DockIR
      ```
      $ cd DockIR
      ```
1. Setup a CMake build directory.
      For example, if not using an IDE:
      ```
      $ mkdir build
      $ cd build
      $ cmake ..
      ```
1. Make your target from the build directory you created.
      ```
      $ make -j4
      ```
1. You now have `pico_ir_keyboard.elf` to load via a debugger, or `pico_ir_keyboard.uf2` that can be installed and run on your Raspberry Pi Pico via drag and drop.

### Note

- `IR rx pin`, `IR command` (or) `Keyboard keycode` can be customised in `pico-ir-keyboard/src/main.c`.
- By default `GP27` is set as `INPUT`.

## Back story

- It all started with adding IR remote support for [Raspberry Pi 3 Model B+](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/) with [custom embedded Linux](https://buildroot.org/) for Web Kiosk ( I know [Raspberry Pi GPIO](https://www.raspberrypi.org/documentation/usage/gpio/) can decode IR. I have also decoded [NEC protocol](https://www.renesas.com/us/en/document/apn/using-infrared-remote-controller-transmission-and-reception?language=en) with [Python](https://www.python.org/) in [Raspberry Pi OS](https://www.raspberrypi.org/software/operating-systems/#raspberry-pi-os-32-bit) ).
- I have no patience in [recompiling the OS](https://buildroot.org/downloads/manual/manual.html#_cross_compilation_toolchain) with [GPIO](https://www.raspberrypi.org/documentation/usage/gpio/) and [Python](https://www.python.org/).
- I thought it would be easy to add [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/) to emulate the keyboard (USB HID). As the [website](http://visionchart.github.io/) already has keyboard support.
- I also found a product by [Adafruit](https://www.adafruit.com) [pIRkey](https://www.adafruit.com/product/3364) which does the same, but they discontinued it.

## Adding HID

 &numsp;✔️ It's a relatively simple and easy process available with [TinyUSB](https://github.com/hathach/tinyusb), via [CircuitPython](https://circuitpython.org/),&nbsp;[Arduino IDE](https://www.arduino.cc/en/software) (or) [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).
 
 &numsp;❌ but not [MicroPython](https://micropython.org/).

## Decoding IR with pico

1. Try old python code.
    - Moving on, I realised that using [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/) to decode [IR NEC protocol](https://www.renesas.com/us/en/document/apn/using-infrared-remote-controller-transmission-and-reception?language=en) and emulate a keyboard is not that easy.
    - The previous program I used to decode [IR NEC protocol](https://www.renesas.com/us/en/document/apn/using-infrared-remote-controller-transmission-and-reception?language=en) in [Raspberry Pi 3 Model B+](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/) [Python](https://www.python.org/) did not work in either [MicroPython](https://micropython.org/) or [CircuitPython](https://circuitpython.org/).
    - In both time differences between two pulses returned zero.
2. Search for Pico specific code.
    - I found [micropython_ir](https://github.com/peterhinch/micropython_ir) by [peterhinch](https://github.com/peterhinch) in [MicroPython](https://micropython.org/) it :tada: works great in decoding [IR NEC protocol](https://www.renesas.com/us/en/document/apn/using-infrared-remote-controller-transmission-and-reception?language=en).
    - It uses `IRQ`, so no chance of running it in [CircuitPython](https://circuitpython.org/).
    - By default, [MicroPython](https://micropython.org/) doesn't support HID for [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/).
    - I tried including HID in the [MicroPython source](https://github.com/micropython/micropython) and [compile](https://github.com/micropython/micropython/tree/master/ports/rp2) it but had no luck; I should try that in the future.
3. Changing the old code to C.
    - As USB HID support is available in [Arduino IDE](https://www.arduino.cc/en/software), I convert that old [Python](https://www.python.org/) code to C.
    - It didn't work either.
4. Trying with PIO.
    - So, I decided to convert that [Python](https://www.python.org/) code into [PIO assembly language](https://www.raspberrypi.org/blog/what-is-pio/) to include it in [CircuitPython](https://circuitpython.org/).
    - After some searching, I found the same in a pending [pull request](https://github.com/raspberrypi/pico-examples/pull/129) by [mjcross](https://github.com/mjcross) with [PIO](https://www.raspberrypi.org/blog/what-is-pio/) in [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) Great!
    - Then decided to go with [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk).

## Merging two

- Merging is simple with the [device examples provided in TinyUSB](https://github.com/hathach/tinyusb/tree/master/examples/device) and from the [pending GitHub pull request](https://github.com/raspberrypi/pico-examples/pull/129).
