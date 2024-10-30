// Tested - works. sets the pwm output based on the analog read input

// Define the required pins
const dutyPin = D32;
const watchPin = D34;
const pwmPin = D27;

// Configure the pins
pinMode(dutyPin, 'input');
pinMode(watchPin, 'input_pulldown');
pinMode(pwmPin, 'output');

function isNumber(value) {
    return typeof value === 'number' && !isNaN(value);
}

const lastTimeDeltas = [];
const timeDeltaDepth = 100;
function addTimeDelta(timeDelta) {
    if (lastTimeDeltas.length >= timeDeltaDepth) return;

    // Verify the input
    if (!isNumber(timeDelta)) return;

    // Push the new value
    lastTimeDeltas.push(timeDelta);

    return lastTimeDeltas.length == timeDeltaDepth;
}

var average;
function getAverageTimeDelta() {
  return (
    average ??
    lastTimeDeltas.reduce((partialSum, item) => partialSum + item, 0) /
    lastTimeDeltas.length
  );
}

function repeatWatch(times, watchPin, func, next) {
    var count = 0;
    const watchId = setWatch(
        function (state) {
            count++;
            if (count > times) {
                clearWatch(watchId);
                next();
                return;
            }
            func(state)
        },
        watchPin,
        { repeat: true, edge:'rising', debounce: 0 }
    )
}

// Get the average wave length
const waveLengths = []
const averageDepth = 5;
var averageWavelength;
// repeatWatch(averageDepth, watchPin,
//     function(state) {
//         // Record the wavelength
//         const waveLength = state.time - state.lastTime;
//         if (!isNumber(waveLength)) return;
//         waveLengths.push(waveLength);
//     },
//     function() {
//         // Get the average wavelength
//         averageWavelength = waveLengths.reduce((partialSum, item) => partialSum + item, 0) /
//         waveLengths.length

//         setWatch(
//             function(state) {
//                 digitalPulse(pwmPin, true, 1)
//             },
//             watchPin,
//             { repeat: true, edge:'rising', debounce: 0 }
//         )
//     }
// )


const duty = 50;
var watchNumber = -1;
setWatch(
    function(state) {
        digitalPulse(pwmPin, false, [4, 1])
    },
    watchPin,
    { repeat: true, edge:'rising', debounce: 0 }
)

setInterval(function() {

    console.log(`Average: ${averageWavelength}, length: ${lastTimeDeltas.length}, hz: ${1 / averageWavelength}`);
  }, 1000);  // 1000 milliseconds = 1 second
