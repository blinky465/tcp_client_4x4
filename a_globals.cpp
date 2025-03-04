
#include "a_globals.h"

int board_rotation = 0;
bool calibrating = false;
const String firmware_version = "v1.01";
String device_id = "";
String broadcast_string = "";

// --------------- this is for connecting to the router ------------------------------
String ssid =  SSID_ROUTER;  
String pass =  PWD_ROUTER;  

// -------------------- these are for accessing eeprom --------------------------------
String myString = "";
char myChar[64] =  "";

// -------------------- state handler (tcp_client and web server) ---------------------
int state = STATE_NONE;

int indexFromHex(char c) { 
  if(c==49) { return(1); }
  if(c==50) { return(2); }
  if(c==51) { return(3); }
  if(c==52) { return(4); }
  if(c==53) { return(5); }
  if(c==54) { return(6); }
  if(c==55) { return(7); }
  if(c==56) { return(8); }
  if(c==57) { return(9); }
  if(c==65 || c == 97) { return(10); }
  if(c==66 || c == 98) { return(11); }
  if(c==67 || c == 99) { return(12); }
  if(c==68 || c == 100) { return(13); }
  if(c==69 || c == 101) { return(14); }
  if(c==70 || c == 102) { return(15); }  
  return(0);  
}

String valToHex(int ixk) {
  String tv = "";   
   if(ixk <= 9) { 
      tv = String(ixk);
   } else { 
      if(ixk == 10) { tv = "A"; }
      if(ixk == 11) { tv = "B"; }
      if(ixk == 12) { tv = "C"; }
      if(ixk == 13) { tv = "D"; }
      if(ixk == 14) { tv = "E"; }
      if(ixk == 15) { tv = "F"; }
   }
   return tv;
}


void writeWord(String myWord, int addr) {
  EEPROM.begin(512);
  strcpy(myChar, myWord.c_str());
  EEPROM.put(addr, myChar );
  EEPROM.commit();
}

String readWord(int addr) {
  EEPROM.begin(512);
  EEPROM.get(addr, myChar);
  myString = myChar;
  return myString;
}

bool isEmpty(String s) { 
  // empty strings begin with a non-printable character
  // (e.g. if you read from eeprom and put 0xff into a character array
  //  you might still end up with a string of non-zero length, but 
  //  it's filled with garbage)
  char c = s[0];
  bool b = false;
  if(c < 0x20 || c > 128) { b = true; }
  return(b);
}
