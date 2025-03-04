
#ifndef PARSE_MESSAGE_
#define PARSE_MESSAGE_
#include <Arduino.h>  // needed if we use Arduino-like types (such as byte, int, string etc.)

void parseMessage(String msg);

void resetWifi();  // this is part of the main .ino file

#endif
