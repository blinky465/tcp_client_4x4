
#include "d_parse_message.h"
#include "a_globals.h"
#include "b_rgb_leds.h"
#include "c_hall_sensors.h"

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

  }  
    

}
