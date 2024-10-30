# Pin interrupts

Take an action when GPIO pin 25 experiences a rising edge.

## Commands

Build

`cargo build`

Build, flash, and open serial monitor

`cargo run`

## Code

Entry point:

./src/main.rs

## Output

```
...
I (462) main_task: Started on CPU0
I (472) main_task: Calling app_main()
I (472) gpio: GPIO[25]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (36872) pin_interrupts: Interrupt detected! 1
I (37072) pin_interrupts: Interrupt detected! 2
I (37672) pin_interrupts: Interrupt detected! 3
I (38472) pin_interrupts: Interrupt detected! 4
I (39072) pin_interrupts: Interrupt detected! 5
I (39872) pin_interrupts: Interrupt detected! 6
```