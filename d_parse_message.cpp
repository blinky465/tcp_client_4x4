
#include "d_parse_message.h"
#include "a_globals.h"
#include "b_rgb_leds.h"
#include "c_hall_sensors.h"

void sendSettings() { 
  // send the bitmasked settings value back to the server
  Serial.print("Current settings value: ");
  Serial.println(settings_bmask);
  String tcp_out = "S" + settings_bmask;
  sendMessageTCP(tcp_out);  
}

extern void parseMessage(String msg) { 
  int pos = 0;
  int len = msg.length();

  char msg_type = msg[0];
  char c;
  int led_idx;
  int col_idx;
  int k = 0;

  Serial.print("parsing: ");
  Serial.println(msg);

  if(msg == "RESET") { 
    resetWifi();
    return;
  }
  
  if(msg_type == 'W' || msg_type == 'U') { 
    // the next character is the LED index (0-3, 4-7, 8-11, 12-15 etc)
    pos++;
    c = msg[pos];
    led_idx = indexFromHex(c);
    
    // the next character are the flags for this square
    // (e.g. B1000 = flash, B0100 = half-brightness etc.)
    pos++;
    c = msg[pos];
    k = indexFromHex(c);
    k = k << 4;

    // the next character is the colour index (0-F)
    pos++;
    c = msg[pos];
    col_idx = indexFromHex(c);
    
    k = k | col_idx;
  }
  
  switch(msg_type) { 
    case 'W':
      // this is a write-message
      overwriteRGB(led_idx, k);
    break;

    case 'U':
      // this is an update message
      updateRGB(led_idx, k);
    break;

    case 'T':
      // this is a write to an LED string
      clearLEDs();
      led_idx = 0;
      pos = 1;
      while(pos < len) { 
        // each character is a colour index        
        c = msg[pos];
        col_idx = indexFromHex(c);        
        setRGBColour(led_idx, col_idx);
        pos++;
        led_idx++;
      }
      updateLEDs();
    break;  

    case 'C':
      // this is a "start calibrate" request; light up the 
      // corner squares (flashing) then set the "is calibrating"
      // flag to true
      calibrating = true;
      setCalibrateRGB();
    break;

    case 'A':
      // this is an acknowledgement after sending a message to say a 
      // square had been occupied (we may have set the square to purple,
      // to this is to say "put the target square back to it's own colour)
      if(!flashOnMove) { return; }
      c = msg[1];
      col_idx = indexFromHex(c);      
      unHighlightLED(col_idx);
    break;

    case 'X':
      // just turn off all the LEDs
      resetLEDs();
    break;

    case 'Q':
      // query the current state of the board
      queryState();
    break;

    case 'R':
      // set the rotation of the device
      c = msg[1];
      board_rotation = indexFromHex(c);
      if(board_rotation < 0 || board_rotation > 3) { board_rotation = 0; }  
      setBoardRotation(board_rotation);  
    break;

    case 'F':
      // set the flash length/delay (globally, all squares flash at the same rate)
      // (flash delay is multiplied by 100, so flash_delay=5 means 500ms)
      c = msg[1];
      k = indexFromHex(c);
      if(msg.length() > 1) { 
        k = k << 4;
        c = msg[1];
        k += indexFromHex(c);
      }
      flash_timeout = k;
    break;

    case 'S':
      // settings as a hex value/bitmask
      // since this gets decoded when it's read back from eeprom, 
      // we can just write the string straight to eeprom as a word/string
      // and then call the function to read it again
      c = msg[1];
      if(c!='?') { 
        write_settings_to_eeprom(msg.substring(1,3));
        delay(10);
      }
      get_settings_from_eeprom();
      if(c=='?'){
        sendSettings();
      }
    break;

    case 'I':
      // set the device id to the value in the message
      // (typically this is a hex representation of an 8-bit value
      //  but in truth, it's just a two-character string)      
      device_id = msg.substring(1,3);
      Serial.print("new device id: ");
      Serial.println(device_id);
      // write the new device id to eeprom
      writeWord(device_id, 100);
      
    break;
  }      
}
