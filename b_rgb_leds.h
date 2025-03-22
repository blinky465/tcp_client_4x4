#ifndef RGB_LEDS_
#define RGB_LEDS_

#include <Arduino.h>  // needed if we use Arduino-like types (such as byte, int, string etc.)
 

// this stuff is for making the RGB LEDs light up
#include <FastLED.h>
#include "a_globals.h"
#define NUM_LEDS 16

extern int flash_delay;
extern CRGBArray<NUM_LEDS> leds;
extern CRGB colour;
extern int flash_timeout;

extern void initLEDs();
extern void highlightLED(int index);
extern void unHighlightLED(int index);
extern void resetLEDs();
extern void clearLEDs();
extern void updateLEDs();
extern CRGB colourFromIndex(int idx);

extern void overwriteRGB(int square_index, int col);
extern void updateRGB(int square_index, int col);
extern void setRGBColour(int led_idx, int led_col);
extern void setCalibrateRGB();
extern void flashConnected();
extern void flashPowerUp();
extern void handleFlashingSquares(unsigned long mils);
extern void nextLEDConnect(int dir, int index);


#endif
