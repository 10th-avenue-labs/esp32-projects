#![no_std]
#![no_main]

use esp_backtrace as _;
use esp_hal::{
    delay::Delay,
    gpio::{Io, Level, Output},
    prelude::*
};

#[entry]
fn main() -> ! {
    #[allow(unused)]
    let peripherals = esp_hal::init(esp_hal::Config::default());
    let delay = Delay::new();

    esp_println::logger::init_logger_from_env();

    // Set GPIO25 as an output
    let io = Io::new(peripherals.GPIO, peripherals.IO_MUX);
    let mut led = Output::new(io.pins.gpio25, Level::High);

    loop {
        log::info!("Setting LED HIGH");
        led.set_high();
        delay.delay(500.millis());
        log::info!("Setting LED LOW");
        led.set_low();
        delay.delay(500.millis());
    }
}
