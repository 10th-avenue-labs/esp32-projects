#![no_std]
#![no_main]

use esp_hal::{delay::Ets, gpio::PinDriver, peripherals::Peripherals};
use esp_sys as _;
use esp_sys::{esp, gptimer_alarm_config_t, gptimer_config_t, gptimer_event_callbacks_t, gptimer_handle_t, gptimer_new_timer, gptimer_set_alarm_action, gpio_set_level, ESP_OK};

use core::ptr;

// Define GPIO pin for the LED
const BLINK_GPIO: i32 = 25;
static mut STATE: bool = false;

// Timer callback function to toggle the LED
unsafe extern "C" fn callback_function(
    _timer: gptimer_handle_t,
    _edata: *const esp_sys::gptimer_alarm_event_data_t,
    _user_data: *mut core::ffi::c_void,
) -> bool {
    gpio_set_level(BLINK_GPIO as i32, 1);
    Ets::delay_us(50);
    gpio_set_level(BLINK_GPIO as i32, 0);
    STATE = !STATE;
    true
}

#[no_mangle]
fn app_main() {
    let peripherals = Peripherals::take().unwrap();
    let pins = peripherals.pins;

    // Configure GPIO pin as output
    let mut led_pin = PinDriver::output(pins.gpio25).unwrap();
    led_pin.set_high().unwrap(); // Initial state
    
    unsafe {
        // Configure the timer
        let mut timer: gptimer_handle_t = ptr::null_mut();
        let timer_config = gptimer_config_t {
            clk_src: 0,
            direction: 0,
            resolution_hz: 1_000_000, // 1 MHz
            ..Default::default()
        };

        // Create a new timer
        assert_eq!(gptimer_new_timer(&timer_config, &mut timer), ESP_OK);

        // Register the callback
        let callback = gptimer_event_callbacks_t {
            on_alarm: Some(callback_function),
            ..Default::default()
        };
        esp!(esp_sys::gptimer_register_event_callbacks(
            timer,
            &callback,
            ptr::null_mut()
        )).unwrap();

        // Enable the timer
        esp!(esp_sys::gptimer_enable(timer)).unwrap();

        // Set up the alarm configuration
        let alarm_config = gptimer_alarm_config_t {
            alarm_count: 8333, // Trigger every ~8.3 ms
            flags: esp_hal::sys::gptimer_alarm_config_t__bindgen_ty_1 {
                _bitfield_1: esp_hal::sys::gptimer_alarm_config_t__bindgen_ty_1::new_bitfield_1(1), // Set auto_reload_on_alarm to true (1)
                _bitfield_align_1: [],
                __bindgen_padding_0: [0; 3],
            },
            reload_count: 0,
        };

        // Set the alarm action
        esp!(gptimer_set_alarm_action(timer, &alarm_config)).unwrap();

        // Start the timer
        esp!(esp_sys::gptimer_start(timer)).unwrap();
    }

    // Main loop
    loop {
        Ets::delay_ms(1000);
    }
}
