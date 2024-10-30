# Hello World

Log 'hello world' to the attached console from the esp32

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
I (446) main_task: Started on CPU0
I (456) main_task: Calling app_main()
I (456) hello_world: Hello, world!
I (456) main_task: Returned from app_main()
```
