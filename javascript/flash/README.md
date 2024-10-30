# ESP32 stuff

## Esptool

Esptool documentation: https://docs.espressif.com/projects/esptool/en/latest/esp32/

How to install: https://docs.espressif.com/projects/esptool/en/latest/esp32/installation.html#installation

## Flashing Espurino to dev board

Run the flash.sh script in this directory

## Espurino CLI

Documentation: https://www.espruino.com/Programming#espruino-command-line-tool

More verbose doc: https://www.npmjs.com/package/espruino

https://www.espruino.com/Reference#software

espruino --list


## Repl

minicom -D /dev/ttyUSB0 -b 115200

espruino --board esp32 -b 115200 --port /dev/ttyUSB0

## Run a file

espruino --board esp32 -b 115200 --port /dev/ttyUSB0 test2.js

to watch the file output, connect minicom and make sure the baud is the same

## Copy files over to device

Write mycode.js to the first Bangle.js device found as a Storage file named app.js
espruino -d Bangle.js mycode.js --storage app.js:-


espruino --board esp32 -b 115200 --port /dev/ttyUSB0 --storage .bootPowerOn:- led-pwm.js

As above, but also write app_image.bin to the device as a Storage file named app.img
espruino -d Bangle.js mycode.js --storage app.js:- --storage app.img:app_image.bin

Connect to Bluetooth device address c6:a8:1a:1f:87:16 and download setting.json from Storage to a local file
bin/espruino-cli.js -p c6:a8:1a:1f:87:16 --download setting.json

# Upload a file named led-pwm.js to run on boot
espruino --board esp32 -b 115200 --port /dev/ttyUSB0 --storage .boot0:led-pwm.js
espruino --board esp32 -b 115200 --port /dev/ttyUSB0 --storage .boot0:ble-peripheral-led-toggle.js


espruino --board esp32 -b 115200 --port /dev/ttyUSB0 --storage c_functions.bin:- build/
