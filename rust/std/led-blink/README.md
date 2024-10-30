# LED Blink

Repeatedly blink an LED attached to GPIO pin 32

## Commands

Build

`cargo build`

Build, flash, and open serial monitor

`cargo run`

## Code

Entry point:

./src/main.rs

## Output

GPIO blinks an LED on pin 32 on and off for a second each time.

```
...
I (460) main_task: Started on CPU0
I (470) main_task: Calling app_main()
I (470) gpio: GPIO[32]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (470) led_blink: Beginning LED loop
I (480) led_blink: Setting LED HIGH
I (1480) led_blink: Setting LED LOW
I (2480) led_blink: Setting LED HIGH
I (3480) led_blink: Setting LED LOW
I (4480) led_blink: Setting LED HIGH
```