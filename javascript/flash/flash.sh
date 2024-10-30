ESPTOOL_DIR="/home/montana/repositories/esptool"
# ESPURINO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/resources"/home/montana/Downloads/ESP32/espruino_esp32.bin

# Nightly Firmware
ESPURINO_DIR="/home/montana/Downloads/ESP32/espruino_2v24.78_esp32/"
# Custom Firmware
# ESP_FIRMWARE="/home/montana/Downloads/something/espruino_esp32.bin"

echo "****************************************"
echo "*             ESP Flasher              *"
echo "****************************************"
echo

echo "Creating virtual environment"
echo "****************************************"
python3 -m venv $ESPTOOL_DIR
source $ESPTOOL_DIR/bin/activate

echo "Erasing current flash and reset board"
echo "****************************************"
python3 $ESPTOOL_DIR/bin/esptool.py     \
        --chip esp32                    \
        --port /dev/ttyUSB0             \
        erase_flash

echo "Flashing device"
echo "****************************************"
python3 $ESPTOOL_DIR/bin/esptool.py                     \
        --chip esp32                                    \
        --port "/dev/ttyUSB0"                           \
        --baud 921600                                   \
        write_flash                                     \
        -z                                              \
        --flash_mode "dio"                              \
        --flash_freq "40m"                              \
        --flash_size "8MB"                              \
        0x1000 $ESPURINO_DIR/bootloader.bin             \
        0x8000 $ESPURINO_DIR/partitions_espruino.bin    \
        0x10000 $ESPURINO_DIR/espruino_esp32.bin        \
        0x200000 /home/montana/esp/tests/add-two-numbers/build/add-two-numbers.bin
