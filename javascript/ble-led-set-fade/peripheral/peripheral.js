// tested - works. Will act as a BLE peripheral and provide functionality for the brightness of an LED to be set via an available characteristic.

// Define the GPIO pin for PWM output
const pwmPin = D27; // Change this to the pin you want to use

// Configure the pin as a PWM output
pinMode(pwmPin, 'output');

// Set the PWM frequency
const pwmFrequency = 1000; // 1 kHz

// Led service
// Led characteristic for the LED service
let state = false;
pinMode(D27, 'output');
const ledService = {
    '12345697-1234-5678-1234-56789abcdef0': {
        value: [0],
        maxLen: 1,
        writable: true,
        onWrite: (evt) => {
            console.log("Writing Led", evt);
            var brightness = evt.data[0] / 100;
            analogWrite(pwmPin, brightness, pwmFrequency);
        }
    }
}

// 128-bit UUID's should be of the form "ABCDABCD-ABCD-ABCD-ABCD-ABCDABCDABCD"
// You should always generate your own UUID - just search online for a UUID generator (or on Linux type date | md5sum into the terminal). Then keep that same UUID and replace the 5th to 8th characters with 0001, 0002, etc for all the different services and characteristics you need - this is efficient in BLE as the 128 bit UUID only has to be sent once.
// NOTE: 16 bit UUIDs are actually 128 bit UUIDs of the form 0000xxxx-0000-1000-8000-00805F9B34FB where xxxx is the 16 bit UUID.
// If you absolutely require two or more 128 bit UUIDs then you will have to specify your own raw advertising data packets with NRF.setAdvertising



NRF.setServices({
    '12345600-1234-5678-1234-56789abcdef0': ledService
}, { uart: false })

console.log("Advertising...")
NRF.setAdvertising({}, {name:"ble-esp"});
console.log("Advertised")