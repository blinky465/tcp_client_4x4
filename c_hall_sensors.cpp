
#include "c_hall_sensors.h"
#include "a_globals.h"
#include "b_rgb_leds.h"


const long sensor_timeout = 10;
unsigned long sensorMillis = 0;

// these are for tracking the reading of the array of hall effect sensors
// (count from 0-7 and on even numbers, read the sensors, on odd numbers
// simply prepare the device for the next read)
int sensor_count = 255;
int curr_sensor = 0;
int input_val = 0;
int curr_row = 0;

// we can't just use 1-4 as outputs and 5-8 as inputs bec/ause on the NodeMCU
// D3, D4 and D8 are special pins and we need to keep them clear during boot-up
// (D8 does not have an active internal pull-up because of an external pull-down resistor
//  and D3/D4 connected to hall sensor supply lines cause boot-up to fail)
int row_out[4] = { D8, D1, D2, D5 };
int input_sensor[4] = { D4, D3, D6, D7 };
int input_values[4] = { 0, 0, 0, 0 };

// this is to allow us to rotate the device (so square 0 is usually top-left, but if rotated
// it might top-right, or bottom-right, or bottom-left)
const int square_numbers_0[16] PROGMEM = { 0, 7, 8, 15, 1, 6, 9, 14, 2, 5, 10, 13, 3, 4, 11, 12 };
const int square_numbers_1[16] PROGMEM = { 3, 2, 1, 0, 4, 5, 6, 7, 11, 10, 9, 8, 12, 13, 14, 15 };
const int square_numbers_2[16] PROGMEM = { 12, 11, 4, 3, 13, 10, 5, 2, 14, 9, 6, 1, 15, 8, 7, 0 };
const int square_numbers_3[16] PROGMEM = { 15, 14, 13, 12, 8, 9, 10, 11, 7, 6, 5, 4, 0, 1, 2, 3 };

const int input_numbers_0[16] PROGMEM = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
const int input_numbers_1[16] PROGMEM = { 12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3 };
const int input_numbers_2[16] PROGMEM = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
const int input_numbers_3[16] PROGMEM = { 3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12 };

int h = 0;
int i = 0;
int j = 0;
int k = 0;

// used if we want to flash a square and await confirmation
bool flash_and_confirm = true;

void initSensors() { 
  // make all the sensor row pins output pins
  // make all the sensor read pins input, with pull-up
  for(int i = 0; i <= 3; i++) { 
    pinMode(row_out[i], OUTPUT);
    pinMode(input_sensor[i], INPUT_PULLUP);
  }
}

void setBoardRotation(int rot) { 
  board_rotation = rot;
  // write the new board rotation back to eeprom
  String t = "0";
  if(board_rotation == 1) { t = "1"; }
  if(board_rotation == 2) { t = "2"; }
  if(board_rotation == 3) { t = "3"; }
  //writeWord(t, 200);
}

void calibrateTopLeft(int row, int col) { 
  if(row == 0){ 
    if(col == 0) { setBoardRotation(0); }
    if(col == 3) { setBoardRotation(3); }
  }
  if(row == 3){ 
    if(col == 0) { setBoardRotation(1); }
    if(col == 3) { setBoardRotation(2); }
  }
}

void killAllRows() {
  for (int i = 0; i <= 3; i++) { 
    digitalWrite(row_out[i], LOW);
  }  
}

int getInputIndex(int ix, int rot) { 
  if(rot == 0) { return(ix); }
  int iy = 0;
  for(int iz = 0; iz <= 15; iz++) { 
    if(rot == 1) { 
      if(input_numbers_1[iz] == ix) { 
        iy = iz;
        break;
      }
    }
    if(rot == 2) { 
      if(input_numbers_2[iz] == ix) { 
        iy = iz;
        break;
      }
    }
    if(rot == 3) { 
      if(input_numbers_3[iz] == ix) { 
        iy = iz;
        break;
      }
    }
  }
  return(iy);
}

void sendSensorMessage(int row, int col, int dir) { 
  bool b = false;
  int idx = 0;

  // we could do fancy bit shifting and then turn int to hex
  // (like row_col = row << 4 | col, for example) 
  // or we just send the values as a concatenated string 
  // edit: send as a single square number (0-15) but allow for
  // the board being rotated 

  idx = (row * 4) + col; // this is the same as input_numbers_0[idx]  
  idx = getInputIndex(idx, board_rotation);        
  sendMessageTCP("S" + valToHex(idx) + String(dir));
}

void readSensors(int idx) { 
  // if this is an odd numbered bit, simply activate a single row
  // but don't do anything else (we'll read it next time around, so
  // it's had time to settle)
  if(idx == 1 || idx == 3 || idx == 5 || idx == 7) { 
    killAllRows();
    if(idx == 1) { digitalWrite(row_out[0], HIGH); }
    if(idx == 3) { digitalWrite(row_out[1], HIGH); }
    if(idx == 5) { digitalWrite(row_out[2], HIGH); }
    if(idx == 7) { digitalWrite(row_out[3], HIGH); }    
  } else { 
    // read the column(s) and calculate any differences compared to the last value(s)
    curr_row = idx/2;
    input_val = 0;
    for(int i = 0; i <= 3; i++) {            
      input_val = input_val << 1;
      k = digitalRead(input_sensor[i]);
      if(k == LOW) { 
        input_val = input_val | 1;        
      } 
    }
    
    // our input value is now a binary representation of the input sensors    
    j = input_val ^ input_values[curr_row]; // ^ is XOR
    k = input_val;
       
    // remember the current state of this row for next time
    input_values[curr_row] = input_val;
      
    if(j != 0) {                    
      // something has changed since last time; 
      // we need to interrogate the previous and current values to decide
      // if a sensor has just gone low-to-high or high-to-low
      // (interrogate the LSB of the XOR result and where it is one, this 
      // indicates a change on that bit has occurred; check the LSB of the 
      // input value to determine whether it is now high or low

      bool flash_it = false;
      for(int i=0; i<=3; i++) { 
          h = j & 1;
          if(h != 0) { 
            // this bit has just changed; check the LSB of the input value
            h = k & 1;

            if(h == 1 && calibrating == true) { 
              calibrating = false;
              resetLEDs();
              calibrateTopLeft(curr_row, i);
              
            } else {
              // h is now zero if this bit has just gone high to low              
              // or one if this bit has just gone low to high
              sendSensorMessage(curr_row, i, h);

              if(h == 1 && flash_and_confirm) { 
                // flash the square we've just put a piece down on
                idx = (curr_row * 4) + i;
                idx = square_numbers_0[idx];

                highlightLED(idx);
                flash_it = true;              
              }
            }
          }
          // bit-shift the XOR result and the input value one place to the right
          j = j >> 1;
          k = k >> 1;          
      }            
    }
  }
}

void handleSensorInput(unsigned long currMillis){
  if (currMillis > sensorMillis) { 
    sensor_count ++;    
    if(sensor_count >= 8) { sensor_count = 0; }     
    sensorMillis = currMillis + sensor_timeout;
    readSensors(sensor_count);  
  }
}

void queryState() { 
  // read all the sensors, then send a message back to the host
  // to say which squares are currently occupied
  String tcp_out = "Q";  
  int ik = 0;
  int ij = 0;
  int ih = 0;
  int idx = 0;
  String t = "";
  
  for(int i = 0; i <= 3; i++) { 
    ik = input_values[i];   
    
    
    for(ij = 0; ij <= 3; ij++) { 
       ih = ik & 1;       
       if(ih > 0) { 
         // this square is occupied, so add it to the list of occupied squares
         idx = (i*4) + ij;         
         // turn the idx into a square number, based on the rotation of the board
         idx = getInputIndex(idx, board_rotation);
         
         // now turn that number into a hex value, to send to the host
         t = valToHex(idx);         
         if(t!="") { tcp_out += t; }              
       }
       ik = ik >> 1;
    }    
  }

  Serial.print("board state: ");    
  Serial.println(tcp_out);
  sendMessageTCP(tcp_out);
  
}
