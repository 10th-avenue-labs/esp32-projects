use esp_idf_svc::hal::peripherals::Peripherals;
use esp_idf_svc::hal::timer::config::Config;
use esp_idf_svc::hal::timer::TimerDriver;

static INTERRUPT_FLAG: core::sync::atomic::AtomicBool = core::sync::atomic::AtomicBool::new(false);


fn alarm_callback() {
    // Alarm triggered
    INTERRUPT_FLAG.store(true, core::sync::atomic::Ordering::Relaxed);
}


fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_svc::sys::link_patches();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    // Take ownership of the peripherals
    let peripherals = Peripherals::take().unwrap();

    // Configure and Initialize Timer Drivers
    let config = Config::new();
    let mut timer = TimerDriver::new(peripherals.timer00, &config).unwrap();

    // Set Counter Start Value to Zero
    timer.set_counter(0_u64).unwrap();

    // Set an alarm to trigger after a number of us
    timer.set_alarm(1 * 1000 * 1000 * 10).unwrap();

    // Subscribe to the timer with a callback
    unsafe { timer.subscribe(alarm_callback).unwrap(); }

    // Enable the alarm
    timer.enable_alarm(true).unwrap();

    // Enable interrupts
    timer.enable_interrupt().unwrap();

    // Enable Counter. Enabling also starts the timer
    timer.enable(true).unwrap();

    loop {
        if INTERRUPT_FLAG.load(core::sync::atomic::Ordering::Relaxed) {
            log::info!("Interrupt detected!");
            INTERRUPT_FLAG.store(false, core::sync::atomic::Ordering::Relaxed);
        }

        std::thread::sleep(std::time::Duration::from_secs(1));
    }
}