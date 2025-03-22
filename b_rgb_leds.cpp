
#include "b_rgb_leds.h"
#include "a_globals.h"

static int led[NUM_LEDS];
static int led_prev[NUM_LEDS];
CRGBArray<NUM_LEDS> leds;
CRGB colour;

const int square_numbers_0[16] PROGMEM = { 0, 7, 8, 15, 1, 6, 9, 14, 2, 5, 10, 13, 3, 4, 11, 12 };
const int square_numbers_1[16] PROGMEM = { 3, 2, 1, 0, 4, 5, 6, 7, 11, 10, 9, 8, 12, 13, 14, 15 };
const int square_numbers_2[16] PROGMEM = { 12, 11, 4, 3, 13, 10, 5, 2, 14, 9, 6, 1, 15, 8, 7, 0 };
const int square_numbers_3[16] PROGMEM = { 15, 14, 13, 12, 8, 9, 10, 11, 7, 6, 5, 4, 0, 1, 2, 3 };

const int connecting_leds[12] PROGMEM = { 0, 7, 8, 15, 14, 13, 12, 11, 4, 3, 2, 1 };
int connect_led_index = 0;

unsigned long flashMillis = 0;
bool flash_flag = false;
int flash_timeout = 5;

void initLEDs() {
  // set all leds to OFF by default
  FastLED.addLeds<NEOPIXEL,DATA_PIN>(leds, NUM_LEDS);     
  resetLEDs();
}

void clearLEDs() { 
  for(int i=0; i < NUM_LEDS; i++) {
    led[i] = 0;
  }  
}

void resetLEDs() { 
  clearLEDs();
  updateLEDs();
}
  
CRGB colourFromIndex(int idx) { 
  if(idx == 0) { return CRGB::Black; }
  if(idx == 1) { return CRGB::Red; }
  if(idx == 2) { return CRGB::Green; }
  if(idx == 3) { return CRGB::Blue; }
  if(idx == 4) { return CRGB::Yellow; }
  if(idx == 5) { return CRGB::Magenta; }
  if(idx == 6) { return CRGB::Cyan; }
  if(idx == 7) { return CRGB::LightSalmon; }
  if(idx == 8) { return CRGB::LightSlateGray; }
  if(idx == 9) { return CRGB::Orange; }
  if(idx == 10) { return CRGB::Sienna; } 
  if(idx == 14) { return CRGB::Magenta; } // this is the colour "blip" when you put a piece down 
  if(idx == 15) { return CRGB::White; }
  return CRGB::Black;
}


void updateLEDs() {   
  for(int i=0; i < NUM_LEDS; i++) {
    int led_col = led[i];
    int ih = led_col & B11110000;  // 0xF0
    int ij = led_col & B00001111;  // 0x0F 
    if(ij > 0) {      
      colour = colourFromIndex(ij);
      leds[i] = colour;
    } else { 
      leds[i] = CRGB::Black;
    }       

    if(ij > 0) {   
      if((ih & B01000000) == 0) { 
        leds[i].maximizeBrightness();
      } else { 
        leds[i].fadeLightBy(128);
      }  
    }     
  }  
  FastLED.show();
  FastLED.show();    
}


int applyRotation(int index) { 
  int led_idx = index;
  if(board_rotation == 0) { led_idx = square_numbers_0[led_idx]; }
  if(board_rotation == 1) { led_idx = square_numbers_1[led_idx]; }
  if(board_rotation == 2) { led_idx = square_numbers_2[led_idx]; }
  if(board_rotation == 3) { led_idx = square_numbers_3[led_idx]; }  
  return(led_idx);
}


void unHighlightLED(int index) {
  int idx = applyRotation(index);
  led[idx] = led_prev[idx];
  updateLEDs();
}

void highlightLED(int index) { 
  // here we record the current LED colour (if it's not the highlight colour)
  // then make this LED the highlight colour
  // (the idea is that when we remove the highlight, we return to the colour
  //  that the LED was showing, and don't turn it out to black)
   
  // led index 14 is the flash colour so if the led is
  // any other colour, remember it so we can restore to 
  // it after receiving an acknowledgement
  if(led[index] != 14) { 
    led_prev[index] = led[index];
  }
  led[index] = 14;
  updateLEDs();
}

void setRGBColour(int led_idx, int led_col) { 
  int led_index = led_idx;
  // now the led_index is sent as 0-3 across the top
  // (with 4-7 on the next row down, 8-11 on the next
  // and the bottom row being 12-15) but the LEDs are
  // laid out in a snake pattern - and if the board is
  // rotated, this pattern could get messed up, so decode
  // to get the actual led_index
  if(board_rotation == 0) { led_index = square_numbers_0[led_index]; }
  if(board_rotation == 1) { led_index = square_numbers_1[led_index]; }
  if(board_rotation == 2) { led_index = square_numbers_2[led_index]; }
  if(board_rotation == 3) { led_index = square_numbers_3[led_index]; }
         
  led[led_index] = led_col;                        
}

void overwriteRGB(int square_index, int col) { 
  resetLEDs();
  setRGBColour(square_index, col);
  updateLEDs(); 
}

void updateRGB(int square_index, int col) { 
  setRGBColour(square_index, col);
  updateLEDs(); 
}

void setCalibrateRGB() { 
  resetLEDs();
  led[0] = 9 | B10000000; // flash orange
  led[3] = 9 | B10000000;
  led[15] = 9 | B10000000;
  led[12] = 9 | B10000000;
  updateLEDs();  
}

void flashConnected() { 
  resetLEDs();
  led[0] = 4;
  led[3] = 4;
  led[15] = 4;
  led[12] = 4;
  updateLEDs();
  delay(100);
  resetLEDs();

  led[5] = 4;
  led[6] = 4;
  led[9] = 4;
  led[10] = 4;
  updateLEDs();
  delay(100);
  resetLEDs();
  
}

void flashPowerUp(){ 
  resetLEDs();
  led[0] = 2;
  led[3] = 2;
  led[15] = 2;
  led[12] = 2;
  updateLEDs(); 
  delay(100);
  resetLEDs(); 
}

void flashLEDs() { 
  // loop through the leds array - anything with a flash flag set
  // needs turning on/off then we may need to call update
  int do_flash_update = 0;
  int led_col;
  int j;
  CRGB colour;

  flash_flag = !flash_flag;
  for(int i = 0; i < NUM_LEDS; i++) { 
    led_col = led[i];       

    if ((led_col & B10000000) > 0) { 
      // this LED needs to flash  
      if(flash_flag) { 
        leds[i] = CRGB::Black;        
      } else { 
        j = led_col & B00001111;
        colour = colourFromIndex(j);          
        leds[i] =  colour;
      }
      
      if(led_col > 0) {
        if((led_col & B01000000) == 0) { 
          leds[i].maximizeBrightness();
        } else { 
          leds[i].fadeLightBy(128);
        }
      }
      
      do_flash_update = 1;
    }    
  }    
  
  if(do_flash_update == 1) {     
    FastLED.show();    
    FastLED.show();    
  }
}

void handleFlashingSquares(unsigned long mils) { 
  bool do_flash = false;
  
  if (mils > flashMillis) {     
    flashLEDs();
    flashMillis = mils + (flash_timeout * 100);     
  }
}

void nextLEDConnect(int dir, int col_index) { 
  connect_led_index += dir;
  if(connect_led_index < 0)  { connect_led_index = 11; }
  if(connect_led_index > 11) { connect_led_index = 0; }
  
  clearLEDs();  
  int k = connecting_leds[connect_led_index];  
  led[k] = col_index;
  updateLEDs();
}
