#ifndef GLOBALS_
#define GLOBALS_

#include <Arduino.h>  // needed if we use Arduino-like types (such as byte, int, string etc.)
#include <EEPROM.h>
 
#define NUM_LEDS 16
#define DATA_PIN D0

#define STATE_NONE 0
#define STATE_CONNECTING_TO_ROUTER 2
#define STATE_AP_MODE 3
#define STATE_RESET_DEVICE 999

// ------------------- edit these to match your own router -----------------

    #define SSID_ROUTER "PLUSNET-FSCFWN"
    #define PWD_ROUTER "pfJdakNqMX7vMu"

// -------------------------------------------------------------------------

extern int board_rotation;
extern bool calibrating;
extern const String firmware_version;
extern String device_id;
extern String broadcast_string;
extern int state;

extern String ssid;
extern String pass;

extern int indexFromHex(char c);
extern String valToHex(int i);
extern void writeWord(String myWord, int addr);
extern String readWord(int addr);
extern bool isEmpty(String s);  


#endif
