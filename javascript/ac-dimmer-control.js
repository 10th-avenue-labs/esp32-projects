// Tested - works. sets the pwm output based on the analog read input

// Define the required pins
const dutyPin = D32;
const frequencyPin = D34;
const pwmPin = D27;

// Configure the pins
pinMode(dutyPin, 'input');
pinMode(frequencyPin, 'input');
pinMode(pwmPin, 'output');

var state = 1
while(1){
    const duty = analogRead(dutyPin);
    const roundedDuty = duty.toFixed(2);
    const frequency = analogRead(frequencyPin);
    const normalizedFrequency = frequency.toFixed(3) * 121
    console.log(`Duty: ${roundedDuty}, Frequency: ${normalizedFrequency}`);
    analogWrite(pwmPin, roundedDuty, { freq: normalizedFrequency, forceSoft: true, soft: true });
}