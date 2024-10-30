use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering;

use esp_idf_svc::hal::gpio::*;
use esp_idf_svc::hal::prelude::*;
use esp_idf_svc::hal::timer::TimerDriver;
use esp_idf_svc::hal::timer::config::Config;


static ZC_FLAG: AtomicBool = AtomicBool::new(false);
static ALARM_FLAG: AtomicBool = AtomicBool::new(false);

fn alarm_interrupt_handler() {
    ALARM_FLAG.store(true, Ordering::Relaxed);
    // // Stop the timer
    // timer.enable(false).unwrap();

    // // Busy pulse the psm pin
    // psm_pin.set_high().unwrap();
    // psm_pin.set_low().unwrap();
}

fn zero_crossing_interrupt_handler() {
    // Set the zero-cross flag
    ZC_FLAG.store(true, Ordering::Relaxed);

    /*
     * Ideally, we would take care of the timer functions here rather than bouncing
     * back to the main loop but the FreeRTOS likely has some restrictions in the
     * interrupt with locks. And due to ownership stuff we can't pass the mutable
     * reference here either.
     */
}

fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_svc::sys::link_patches();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    // Take ownership of peripherals
    let peripherals = Peripherals::take().unwrap();

    // Initialize the GPIO pins
    let mut zc_pin = PinDriver::input(peripherals.pins.gpio25).unwrap();
    let mut psm_pin = PinDriver::output(peripherals.pins.gpio32).unwrap();

    // TODO: Move the pin values to a const somehow at the top of the file

    // TODO: Move the zc_pin configuration steps to a separate function
    // Set the pull mode for the zc pin
    zc_pin.set_pull(Pull::Up).unwrap();

    // Set an interrupt to trigger on positive edge (rising edge)
    zc_pin.set_interrupt_type(InterruptType::PosEdge).unwrap();

    // Create the timer config
    let config = Config::new();

    // Create the timer
    let mut timer = TimerDriver::new(peripherals.timer00, &config).unwrap();

    // Attach the interrupt callback for the zero crossing pin
    unsafe {
        zc_pin.subscribe(zero_crossing_interrupt_handler).unwrap();
    }

    // Enable the interrupt
    zc_pin.enable_interrupt().unwrap();

    // Main loop
    let mut duty = 0;
    loop {
        if ZC_FLAG.load(Ordering::Relaxed) {
            // log::info!("ZC triggered");
            // Reset the ZC flag
            ZC_FLAG.store(false, Ordering::Relaxed);

            // Set Counter Start Value to Zero
            timer.set_counter(0_u64).unwrap();

            // Set an alarm to trigger after a number of us
            timer.set_alarm(8 * 1000).unwrap();

            // Subscribe to the timer with a callback
            unsafe { timer.subscribe(alarm_interrupt_handler).unwrap(); }

            // Enable the alarm
            timer.enable_alarm(true).unwrap();

            // Enable interrupts
            timer.enable_interrupt().unwrap();

            // Re-enable the zero-cross interrupt
            zc_pin.enable_interrupt().unwrap();

            // Start the timer
            timer.enable(true).unwrap();
        }

        if ALARM_FLAG.load(Ordering::Relaxed) {
            ALARM_FLAG.store(false, Ordering::Relaxed);
            // log::info!("Alarm triggered");

            // Busy pulse the psm pin
            psm_pin.set_high().unwrap();
            std::thread::sleep(std::time::Duration::from_micros(50));
            psm_pin.set_low().unwrap();
        }

        // Sleep briefly to prevent the watchdog from triggering
        std::thread::sleep(std::time::Duration::from_micros(10000));
        duty+=1;
        if duty > 100 {
            duty = 0;
        }
    }
}
