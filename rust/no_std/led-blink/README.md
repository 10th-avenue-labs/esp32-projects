# LED Blink

Toggle an LED connected to pin 25 on/off.

## Commands

Build

`cargo build`

Build, flash, and open serial monitor

`cargo run`

## Code

Entry point:

./src/main.rs

## Output

LED on pin 25 toggles on/off ever half second.

```
...
I (158) boot: Loaded app from partition at offset 0x10000
I (158) boot: Disabling RNG early entropy source...
INFO - Setting LED HIGH
INFO - Setting LED LOW
```