# Pin interrupts

Create a timer interrupt.

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
I (450) main_task: Started on CPU0
I (460) main_task: Calling app_main()
I (11460) timer_interrupts: Interrupt detected!
```