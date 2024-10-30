use esp_idf_svc::hal::prelude::*;
use esp_idf_svc::hal::gpio::*;

// Global flag to track interrupts
static INTERRUPT_FLAG: core::sync::atomic::AtomicBool = core::sync::atomic::AtomicBool::new(false);

// Interrupt callback function
fn interrupt_callback() {
    INTERRUPT_FLAG.store(true, core::sync::atomic::Ordering::Relaxed);
}

fn main() {
    esp_idf_svc::sys::link_patches();

    // Initialize logger
    esp_idf_svc::log::EspLogger::initialize_default();

    // Take ownership of peripherals
    let peripherals = Peripherals::take().unwrap();

    // Set GPIO25 as an input pin with an internal pull-up
    let mut interrupt_pin = PinDriver::input(peripherals.pins.gpio25).unwrap();

    // Set the resister to pull down. This mean the pin will be set low and the interrupt will be triggered when receiving a high signal
    interrupt_pin.set_pull(Pull::Down).unwrap();

    // Set interrupt to trigger on positive edge (rising edge)
    interrupt_pin.set_interrupt_type(InterruptType::PosEdge).unwrap();

    // Attach the interrupt callback
    unsafe { interrupt_pin.subscribe(interrupt_callback).unwrap(); }

    // Enable the interrupt
    interrupt_pin.enable_interrupt().unwrap();

    let mut count = 0;
    loop {
        // Check if interrupt flag was set
        if INTERRUPT_FLAG.load(core::sync::atomic::Ordering::Relaxed) {
            count+=1;
            log::info!("Interrupt detected! {count}");
            INTERRUPT_FLAG.store(false, core::sync::atomic::Ordering::Relaxed);

            // Re-enable the interrupt, as they get disabled after every trigger
            interrupt_pin.enable_interrupt().unwrap();
        }

        // Sleep briefly to prevent the watchdog from triggering
        std::thread::sleep(std::time::Duration::from_millis(100));
    }
}
