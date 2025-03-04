
#include "e_web_server.h"
#include <ESPAsyncWebServer.h>


// ------------------------ if the device cannot connect to a router, it will revert --------------------------
// --------- to access point mode (AP) and provide a basic web interface for entering ssid/password -----------


// Create AsyncWebServer object on port 80 (default HTTP port)
AsyncWebServer server(80);

// ------------------------- set the IP address for your web interface here -----------------------------------
// --------- The default IP address for a NodeMCU acting as an access point (AP) is usually 192.168.4.1 -------
IPAddress    apIP(10, 0, 0, 1);   // Private network address: local & gateway
IPAddress    apGateway(0,0,0,0);  // Gateway not needed for internet access
IPAddress    apSubnet(255, 255, 255, 0);
IPAddress    apDns(1, 1, 1, 1); 

String tmp = "";


// ------- this is the text of the single web page -------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Connected Device</h2>
  <p>
    <span class="dht-labels">SSID</span> 
    <span id="humidity"><input type="text" name="ssid" id="ssid" value="%T_SSID%" /></span>    
  </p> 
  <p>
    <span class="dht-labels">Password</span> 
    <span id="temperature"><input type="text" name="pwd" id="pwd" value="%T_PWD%" /></span>    
  </p>  
  <p>
    <span class="dht-labels">Device ID</span> 
    <span id="temperature"><input type="text" name="d_id" id="d_id" value="%T_ID%" length="2" /></span>    
  </p>  
  <p>
    <input type="button" value=" update " onClick="updateSSID();" />
  </p>
  <p>
    <input type="button" value=" reset " onClick="resetDevice();" />
  </p>
</body>
<script>
function updateSSID ( ) {
  var xhttp = new XMLHttpRequest();  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {      
      alert('Update successful');
    }
  };
  var s = document.getElementById("ssid").value;
  var p = document.getElementById("pwd").value;
  var i = document.getElementById("d_id").value;
  var params = "ssid=" + s + "&pwd=" + p + "&id=" + i;
  xhttp.open("POST", "/update", true);
  xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhttp.send(params);
}

function resetDevice() { 
  var xhttp = new XMLHttpRequest();  
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {      
      alert('Device reset');
    }
  };
  xhttp.open("GET", "/reset", true);
  xhttp.send();
}
</script>
</html>)rawliteral";


String processor(const String& var){  
  if(var == "T_PWD"){
    return pass;
  }
  else if(var == "T_SSID"){
    return ssid;
  } else if(var == "T_ID"){ 
    return device_id;
  }
  return "";
}

const char* ssid_ap  = "ConnectedGame";
const char* password = "12345678";


void startWebServer() { 

  Serial.println("Setting AP (Access Point)...");
  
  //WiFi.mode(WIFI_AP_STA);  //need both to serve the webpage and take commands via tcp
  //delay(300);
  WiFi.config(apIP, apGateway, apSubnet, apDns);
  delay(300);
  
  // this sets the IP address of the web server once AP mode is running
  WiFi.softAPConfig(apIP, apGateway, apSubnet);   // subnet FF FF FF 00  
  delay(300);
  
  // Remove the password parameter, if you want the AP (Access Point) to be open
  // (Apple devices don't always show open/password-less access points so you might
  //  want to add a simple password to get it to show up)
    
  WiFi.softAP(ssid_ap, password);
  //WiFi.softAP(ssid_ap);

  delay(1000);
  Serial.println("AP initialised");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // show we're in AP mode by setting an LED orange
  // state = 4;

  // Route for root / web page (get)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Route for /reset (get)
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    tmp = "reset";
    request->send_P(200, "text/plain", tmp.c_str());
    Serial.println("reset device");
    state = STATE_RESET_DEVICE;
  });

  // Route for /update (post)
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){    
    //List all parameters
    int params = request->params();
    Serial.println("update saved SSID values");
    for(int i=0; i < params; i++){
      const AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // k is the key for the parameter key/value pair, v is the value
        String k = p->name().c_str();
        String v = p->value().c_str();
        if (k == "ssid") { 
          // store the ssid in eeprom
          ssid = v;
          Serial.print("SSID: ");
          Serial.println(ssid);
          writeWord(v, 0);       
        } else if (k == "pwd") { 
          // store the password in eeprom
          pass = v;
          Serial.print("Password: ");
          Serial.println(pass);
          writeWord(v, 50);
        } else if (k == "id") { 
          // store the device id in eeprom
          device_id = v;
          Serial.print("Device id: ");
          Serial.println(device_id);
          writeWord(v, 100);
          // update the broadcast message 
          broadcast_string = "hello " + device_id;  
        }
      }
    }    
    delay(200);
    tmp = ssid + "+" + pass + "+" + device_id;
    request->send_P(200, "text/plain", tmp.c_str());
  });  
  
  // Start the server
  server.begin();
}
