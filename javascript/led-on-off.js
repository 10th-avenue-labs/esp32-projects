// tested and works - will set pin27 on for a sec, then off for a sec

pinMode(D27, 'output'); // Set pin D27 as output

let state = false; // Variable to keep track of the pin state

setInterval(function() {
  state = !state;   // Toggle the state
  digitalWrite(D27, state); // Write the state to D27
}, 1000);  // 1000 milliseconds = 1 second