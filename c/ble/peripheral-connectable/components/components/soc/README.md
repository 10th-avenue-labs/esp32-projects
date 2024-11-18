## `soc` ##

The `soc` component provides hardware description for targets supported by ESP-IDF.

    - `xxx_reg.h`   - defines registers related to the hardware
    - `xxx_struct.h` - hardware description in C `struct`
    - `xxx_channel.h` - definitions for hardware with multiple channels
    - `xxx_caps.h`  - features/capabilities of the hardware
    - `xxx_pins.h`  - pin definitions
    - `xxx_periph.h/*.c`  - includes all headers related to a peripheral; declaration and definition of IO mapping for that hardware

Specially, the `xxx_reg.h` and `xxx_struct.h` headers that generated by script are under `register/soc` folder. Please DO NOT **add** other manual coded files under this folder.

For other soc headers that are used as wrapper, definition, signaling, mapping or manual coded registers, please add them under `include/soc` folder.