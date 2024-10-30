// Tested - works. Causes an LED to fade out and then restart

// Define the GPIO pin for PWM output
const pwmPin = D27; // Change this to the pin you want to use

// Configure the pin as a PWM output
pinMode(pwmPin, 'output');

// Set the PWM frequency (in Hz) and duty cycle (0-1023)
const pwmFrequency = 50; // 1 kHz

var state = 1
setInterval(function() {
    console.log(`Setting state to: ${state}`);
    analogWrite(pwmPin, state, { freq: 100 });
    state-=0.01;
    if (state < 0)
        state = 1;
  }, 100);  // 1000 milliseconds = 1 second