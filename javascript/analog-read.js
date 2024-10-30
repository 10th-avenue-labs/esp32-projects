// Tested - works. read and print an analog value to the console

// Define the GPIO pin for analog read 
const readPin = D32; // Change this to the pin you want to use

// Configure the pin as an input
pinMode(readPin, 'input');

var state = 1
setInterval(function() {
    const value = analogRead(readPin);
    const rounded = value.toFixed(2);
    console.log(`Value is: ${rounded}`);
  }, 100);  // 1000 milliseconds = 1 second