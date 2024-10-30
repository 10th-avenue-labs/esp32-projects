// tested - works. Connects to wifi
var ssid = 'IP-in-the-hot-tub';
var password = 'everytime';
var wifi = require('Wifi');

console.log("Starting...")

wifi.disconnect();

wifi.connect(ssid, {password: password}, function() {
    console.log('Connected to Wifi.  IP address is:', wifi.getIP().ip);
    wifi.save(); // Next reboot will auto-connect
});