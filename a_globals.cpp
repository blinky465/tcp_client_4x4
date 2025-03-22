
#include "a_globals.h"

int board_rotation = 0;
bool calibrating = false;
const String firmware_version = "v1.01";
String device_id = "";
String broadcast_string = "";
String settings_bmask = "";
int settings_bitmask = 0;
bool showConnectToRouter = true;
bool showConnectViaTCP = true;
bool showWhenConnected = true;

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


void get_settings_from_eeprom() { 
  settings_bmask = readWord(110);
  if(settings_bmask.length() > 2 || settings_bmask.length() < 1){ settings_bmask = "00"; }  
  
  // turn the settings hex string into an int value so we can apply bitmasking correctly
  char c = settings_bmask[0];
  int i = indexFromHex(c);
  i = i << 4;
  c = settings_bmask[1];
  int k = indexFromHex(c);  
  settings_bitmask = i + k;

  if ((settings_bitmask & B00000001) > 0 ) {
    showConnectToRouter = true;
  } else { 
    showConnectToRouter = false;
  }
  if ((settings_bitmask & B00000010) > 0 ) {
    showConnectViaTCP = true;
  } else { 
    showConnectViaTCP = false;
  } 
  if ((settings_bitmask & B00000100) > 0 ) {
    showWhenConnected = true;
  } else { 
    showWhenConnected = false;
  } 
  Serial.print("settings as string: ");
  Serial.println(settings_bmask);
  Serial.print("settings as int value: ");
  Serial.println(settings_bitmask);
}

void write_settings_to_eeprom(String hex) { 
  Serial.print("write value to eeprom: ");
  Serial.println(hex);
  writeWord(hex, 110);
}
