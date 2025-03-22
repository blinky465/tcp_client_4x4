
#include "a_globals.h"
#include "b_rgb_leds.h"
#include "c_hall_sensors.h"
#include "d_parse_message.h"
#include "e_web_server.h"

// this establishes a connection between the device (client) and the app (server)
// and includes all the functions for handling auto-connect, switching from UDP to
// TCP etc. The actual device code (read the sensors, flash the LEDs etc.) is included
// in separate files

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


// ----------- this is for connecting to the server (app) over TCP -------------------
IPAddress serverIP(192, 168, 1, 216);  // Replace this IP address with the IP address of your server
#define portNumber 1977
#define TCP_TX_PACKET_MAX 32
WiFiClient client_A;

int connect_fail_count = 0;
int const MAX_CONNECT_RETRY_COUNT = 5; //20;
int const MAX_ROUTER_CONNECT_RETRY = 25;
int const TCP_HEARTBEAT_INTERVAL = 3000;
bool router_connected = false;
bool has_server_ip_address = false;
unsigned long tcp_heartbeat = 0;
String incomingMessageTCP = "";

// ------------------- this is for shouting over UDP ----------------------------------
#define portUDP_Out 1975
#define portUDP_In 1976
#define UDP_TX_PACKET_MAX 32
const IPAddress broadcastIP(255,255,255,255);

String acknowledgement = "ACK";
char packetBufferUDP[UDP_TX_PACKET_MAX]; // buffer to hold incoming UDP packet (server response)
char ReplyBufferUDP[] = "????????????";                  
WiFiUDP udp_in;
WiFiUDP udp_out;
int const MAX_BROADCAST_ATTEMPT_COUNT = 100;
int const UDP_BROADCAST_REPEAT_DELAY = 1000;
int const CONNECT_LED_DELAY = 500;


// ----------------- general variables and state machine etc. -------------------------
unsigned long curr_millis = 0;
unsigned long udp_next = 0;
unsigned long connect_next_led = 0;
bool showConnectToRouter = true;
bool showConnectViaTCP = true;


void setup() {
    delay(200); // delay to allow the serial port to purge
    setup_serial_port();
    initLEDs();
    flashPowerUp();
    initSensors();
    get_ssid_from_eeprom();  
    get_device_id_from_eeprom();    
    broadcast_string = "hello " + device_id;
    setup_board_rotation();    
}

void loop() {
    curr_millis = millis();

    if(state == STATE_AP_MODE) { 
      
    } else if(router_connected) {
      // if we already have the server's IP address, we can get on with
      // attempting a TCP connection (if necessary) and servicing any
      // incoming messages from the server - otherwise just start shouting
      // over UDP until the server responds
      
      if(has_server_ip_address) {  
        TCP_Client_A();
      } else { 
        UDP_Client_A();
      }
      
    } else { 
      connect_to_router();
      if(router_connected) {         
        udp_out.begin(portUDP_Out);        
      }
    }
}

void get_ssid_from_eeprom() { 
  // load the ssid and password from eeprom (in case it's changed
  // from the hard-coded default)    
  ssid = readWord(0); 
  if(isEmpty(ssid)) { ssid = SSID_ROUTER; } 
  pass = readWord(50);
  if(isEmpty(pass)) { pass = PWD_ROUTER; }
}

void sendMessageTCP(String msg){ 
  client_A.print(msg + " " + device_id);  
}

void UDP_Client_A() { 
  int packetSize = udp_out.parsePacket();                           // if there’s data available, read a packet
  if (packetSize) {
    Serial.print(F("Received packet of size: "));
    Serial.print(packetSize);
    Serial.print(F(" From remote: "));
    IPAddress remote = udp_out.remoteIP();

    // don't assign straight away - check that this is the correct
    // response message from the server (and a client is not responding
    // to another client or some other broadcast from another device)
    //serverIP = UDP_out.remoteIP();

    for (int k = 0; k < 4; k++)
    {
      Serial.print(remote[k], DEC);
      if (k < 3)
      {
        Serial.print(F("."));
      }
    }
    Serial.print(F(", port: "));
    Serial.print(" UDPremotePort: ");
    Serial.println(udp_out.remotePort());

    udp_out.read(packetBufferUDP, UDP_TX_PACKET_MAX);             // read the packet into packetBufffer
    Serial.print(F("Contents: "));
    Serial.println(packetBufferUDP);

    // if the response is from the server, inviting us to connect,
    // then remember the IP address of the originator (it's possible
    // we might capture hello messages from other clients as they
    // boot up, so only respond to an accept connection message from the server)
    String t = String(packetBufferUDP);
    int i = t.indexOf(" ");
    if(i) { t = t.substring(i+1); }
    Serial.print("t: ");
    Serial.println(t);
    
    if(t.indexOf("connect") >= 0) { 
      serverIP = remote;    

      // send an acknowledgement back
      udp_out.beginPacket(serverIP, portUDP_Out); 
      udp_out.write(acknowledgement.c_str());
      udp_out.endPacket();
      udp_next = curr_millis + UDP_BROADCAST_REPEAT_DELAY;
      
      // kill the udp listener
      // and set up a TCP IP connection
      Serial.println("Now connect via TCP");
      has_server_ip_address = true;
    } else { 
      // ignore this message
    }
  } 

  if(!has_server_ip_address) { 
    if(curr_millis > udp_next) { 
      // broadcast a message onto the network so the server can find us and send an acknowledgement
      Serial.println(broadcast_string);
      udp_out.beginPacket(broadcastIP, portUDP_Out); 
      udp_out.write(broadcast_string.c_str());
      udp_out.endPacket();
      udp_next = curr_millis + UDP_BROADCAST_REPEAT_DELAY;
    }

    if(curr_millis > connect_next_led) { 
      // move on to the next led in the sequence
      if(showConnectToRouter) { 
        nextLEDConnect(1, CONNECT_COLOUR_INDEX);
      }
      connect_next_led = curr_millis + CONNECT_LED_DELAY;
    }
  }
}

void TCP_Client_A() {
    
  if (!client_A.connected()) {
    if (client_A.connect(serverIP, portNumber)) {                                         // Connects to the server
      Serial.print("Connected to Gateway IP = "); Serial.println(serverIP);
      connect_fail_count = 0;
      flashConnected();      
      
    } else {
      Serial.print("Could NOT connect to Gateway IP = "); Serial.println(serverIP);
      delay(500);
      connect_fail_count++;
      if(connect_fail_count >= MAX_CONNECT_RETRY_COUNT) { 
        // do we want to abort - it appears that the server has gone away?
        // or should we just keep retrying, so when the server reappears 
        // we'll just magically reconnect and everything will be golden?
        // there's no guarantee that restarting the server will put it on the
        // same IP address, so maybe we should go back to broadcast UDP mode here?
        has_server_ip_address = false;
      }

      if(curr_millis > connect_next_led) { 
        // move on to the next led in the sequence
        if(showConnectViaTCP) {           
          nextLEDConnect(1, ROUTER_COLOUR_INDEX);
        }
        connect_next_led = curr_millis + CONNECT_LED_DELAY;
      }
      
    }
  } else {
    // Receives data from the server and processes as necessary
    while (client_A.available()) { 
      char c = client_A.read();  // Read one character at a time
      if(c == 0x0a) { 
        // this is an end of message marker, send to the message parser
        parseMessage(incomingMessageTCP);
        // reset the incoming message
        incomingMessageTCP = "";
        
      } else if(c >= 0x20) {
        // ignore any non-printable characters, and append anything else 
        // to the incoming buffer
        incomingMessageTCP += c;      // Append to string  
      }
      
    } 
        
    // we need to send a heartbeat back to the client every few seconds
    if(curr_millis > tcp_heartbeat) { 
      sendMessageTCP("hb");
      tcp_heartbeat = curr_millis + TCP_HEARTBEAT_INTERVAL;
    }

    // if we're connected, we might want to check the sensor states
    handleSensorInput(curr_millis);  

    // and handle any flashing squares
    handleFlashingSquares(curr_millis);
    
  }
}

// ------------ general purpose functions --------------------------
void(* resetFunc) (void) = 0; //declare reset function at address 0


void resetWifi() { 
  // set the ssid/password to "blank" (not an empty string else
  // we'll just pick up the hard-coded SSID/PWD combo)
  // and reset the device
  writeWord("blank", 0);
  writeWord("blank", 50);
  Serial.println("Resetting device");
  resetLEDs();
  resetFunc();  
}


//---------------------- Setup functions ---------------------------

void setup_serial_port() {
  uint32_t baudrate = 9600;
  Serial.begin(baudrate);
  Serial.println("");
  Serial.print("Serial port connected: ");
  Serial.println(baudrate);
}

void setup_ap_mode() { 
  Serial.println("Entering AP mode...");
  startWebServer();
}

void setup_udp(int port_number) { 
  int connect_attempt = 0;
  bool udp_success = false;

  WiFiUDP udp;
  if(port_number == portUDP_Out) { udp = udp_out; }
  if(port_number == portUDP_In) { udp = udp_in; }
  
  while(udp_success == false) {
    while (!udp.begin(port_number) && connect_attempt < MAX_BROADCAST_ATTEMPT_COUNT) {
      Serial.print("+");
      yield();   
      delay(50);
      connect_attempt++; 
    }     
    if(connect_attempt >= MAX_BROADCAST_ATTEMPT_COUNT) { 
      // unable to contact the server - maybe the app isn't running?
      // you may want to add some logic here to change modes?
      // for now, we'll just keep trying to shout into the ether
      connect_attempt = 0; 
    } else { 
      udp_success = true;
    }
  }
  Serial.print("UDP init successful on port ");
  Serial.println(port_number);
}

void connect_to_router() {
  uint8_t wait = 0;
  int retry_count = 0;
  state = STATE_CONNECTING_TO_ROUTER;
  
  Serial.println("");
  Serial.print("Connecting to router ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED && retry_count < MAX_ROUTER_CONNECT_RETRY) {
    Serial.print(".");
    if(retry_count == 0){
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, pass);
    }
    delay(500);
    nextLEDConnect(1, ROUTER_COLOUR_INDEX); // use -1 to run leds in reverse order
    retry_count++;    
  }
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    router_connected = true;
  } else { 
    Serial.println("Cannot connect to router");
     // go into AP access mode so we can connect to this device
     // from our app and provide updated SSID and password details
     state = STATE_AP_MODE;
     setup_ap_mode();
  }
}

void get_device_id_from_eeprom() { 
  // this gets the device id and saves it to the global variable device_id  
  device_id = readWord(100);
  if(device_id.length() > 2 || device_id.length() < 1){ device_id = "FF"; }  
}

void setup_board_rotation() { 
  board_rotation = 0;
  
  // load the previous board rotation from eeprom
  String rotation_id = readWord(200);
  if(rotation_id.length() > 1){ rotation_id = "0"; }
  
  if(rotation_id == "1") { board_rotation = 1; } 
  if(rotation_id == "2") { board_rotation = 2; } 
  if(rotation_id == "3") { board_rotation = 3; } 
  if(board_rotation < 0 || board_rotation > 3){ board_rotation = 0; }

}
