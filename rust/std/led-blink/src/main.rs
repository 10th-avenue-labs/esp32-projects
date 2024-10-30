use esp_idf_svc::hal::prelude::Peripherals;
use esp_idf_svc::hal::gpio::PinDriver;


fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_svc::sys::link_patches();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    // Take ownership of the available peripherals (e.g., GPIO, timers) and handle the result safely
    let peripherals = Peripherals::take().unwrap();

    // Configure GPIO32 as an output pin to control the LED
    let mut led = PinDriver::output(peripherals.pins.gpio32).unwrap();

    log::info!("Beginning LED loop");
    loop {
        // Set the LED high for 1 second
        log::info!("Setting LED HIGH");
        led.set_high().unwrap();
        std::thread::sleep(std::time::Duration::from_secs(1));

        // Set the LED low for 1 second
        log::info!("Setting LED LOW");
        led.set_low().unwrap();
        std::thread::sleep(std::time::Duration::from_secs(1));
    }
}
