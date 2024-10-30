// Tested - works (usually). Connects to wifi and creates a webserver on the network

var wifi = require("Wifi");

var ssid = 'IP-in-the-hot-tub';
var password = 'everytime';
var port = 80;

function processRequest (req, res) {
  res.writeHead(200);
  res.end('Hello World');
}

wifi.disconnect();

wifi.connect(ssid, {password: password}, function(err){
  if (err) {
    console.log("Failed to connect: " + err);
  } else {
    console.log("Connected to WiFi");
    var http = require('http');
    http.createServer(processRequest).listen(port);
  
    console.log(`Web server running at http://${wifi.getIP().ip}:${port}`)
  }
});

wifi.on("connected", function(details) {
  console.log("WiFi connected:", details);

});

wifi.on("disconnected", function() {
  console.log("WiFi disconnected");
});