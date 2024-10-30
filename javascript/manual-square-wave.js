const zcPin=D25;
const psmPin=D27;
pinMode(zcPin, 'input_pullup');
pinMode(psmPin, 'output');
const AMT = 0.001;
const period = 0.008333333333333
const offset = period * 0.9;
const setTime = offset + period;
const resetTime = setTime + AMT;
setWatch(
    function(w,e) { "ram" // ensure it's in RAM and pretokenised
        w(1, e.time+setTime); // turn off before zero crossing
        w(0, e.time+resetTime); // turn on after
    }.bind(null,psmPin.writeAtTime.bind(psmPin)),
    zcPin,
    { repeat: true, edge:'rising', debounce: 0}
)
