// #![no_std]
// #![no_main]

// use core::cell::RefCell;

// use critical_section::Mutex;
// use esp_backtrace as _;
// use esp_hal::{
//     delay::Delay,
//     gpio::{Event, Input, Io, Pull},
//     macros::ram,
//     prelude::*,
// };

// /**
//  * We need to create a static variable here.
//  * 
//  * After the interrupt is triggered, we need to clear that interrupt so that it can fire again.
//  * The interrupt handler cannot accept any arguments, and we need some way to pass the reference
//  * to the interrupt pin to the handler, hence needing this static optional variable.
//  * 
//  * We wrap the static optional in a Mutex<Refcell> to make accessing the variable safe.
// **/
// static INTERRUPT_PIN: Mutex<RefCell<Option<Input>>> = Mutex::new(RefCell::new(None));

// static INTERRUPT_SET: Mutex<RefCell<bool>> = Mutex::new(RefCell::new(false));

// #[entry]
// fn main() -> ! {
//     // Initialize the peripherals
//     let peripherals = esp_hal::init(esp_hal::Config::default());

//     // Initialize the delay functionality
//     let delay = Delay::new();

//     // Initialize the logger
//     esp_println::logger::init_logger_from_env();

//     // Initialize the IO functionality
//     let mut io = Io::new(peripherals.GPIO, peripherals.IO_MUX);

//     // Set the interrupt handler
//     io.set_interrupt_handler(handler);

//     // Initialize the interrupt pin
//     let mut interrupt_pin = Input::new(io.pins.gpio25, Pull::Down);

//     critical_section::with(|cs| {
//         // Listen to the rising edge
//         interrupt_pin.listen(Event::RisingEdge);

//         // Transfer ownership of the interrupt pin to the global variable
//         INTERRUPT_PIN.borrow_ref_mut(cs).replace(interrupt_pin)
//     });

//     loop {
//         // Check if the interrupt flag has been set
//         if critical_section::with(|cs| {
//             *INTERRUPT_SET.borrow_ref(cs)
//         }) {
//             log::info!("Interrupt set");

//             // Reset the interrupt flag
//             critical_section::with(|cs| {
//                 *INTERRUPT_SET.borrow_ref_mut(cs) = false;
//             });
//         }

//         // Delay
//         delay.delay_millis(500);
//     }
// }

// #[handler]
// #[ram]
// fn handler() {
//     critical_section::with(|cs| {
//         // Check if the interrupt is set
//         if INTERRUPT_PIN
//             .borrow_ref_mut(cs)
//             .as_mut()
//             .unwrap()
//             .is_interrupt_set()
//         {
//             // The interrupt pin was the source of the interrupt

//             // Set the interrupt flag
//             critical_section::with(|cs| {
//                 *INTERRUPT_SET.borrow_ref_mut(cs) = true;
//             });
//         }

//         // Clear the interrupt
//         INTERRUPT_PIN
//             .borrow_ref_mut(cs)
//             .as_mut()
//             .unwrap()
//             .clear_interrupt();
//     });
// }

#![no_std]
#![no_main]

use core::cell::RefCell;
use critical_section::Mutex;
use esp_backtrace as _;
use esp_hal::{
    delay::Delay,
    gpio::{Event, Input, Io, Level, Output, Pull},
    prelude::*,
};
use esp_println::println;

static BUTTON: Mutex<RefCell<Option<Input>>> = Mutex::new(RefCell::new(None));

#[entry]
fn main() -> ! {
    let peripherals = esp_hal::init(esp_hal::Config::default());

    println!("Hello world!");

    let mut io = Io::new(peripherals.GPIO, peripherals.IO_MUX);
    // Set the interrupt handler for GPIO interrupts.
    io.set_interrupt_handler(handler);
    // Set GPIO7 as an output, and set its state high initially.
    let mut led = Output::new(io.pins.gpio32, Level::Low);

    // Set GPIO9 as an input
    let mut button = Input::new(io.pins.gpio25, Pull::Up);

    // ANCHOR: critical_section
    critical_section::with(|cs| {
        button.listen(Event::FallingEdge);
        BUTTON.borrow_ref_mut(cs).replace(button)
    });
    // ANCHOR_END: critical_section

    let delay = Delay::new();
    loop {
        led.toggle();
        delay.delay_millis(500u32);
    }
}

#[handler]
fn handler() {
    critical_section::with(|cs| {
        println!("GPIO interrupt");
        BUTTON
            .borrow_ref_mut(cs)
            .as_mut()
            .unwrap()
            .clear_interrupt();
    });
}
