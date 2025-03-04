#ifndef HALL_SENSORS_
#define HALL_SENSORS_
#include <Arduino.h>  // needed if we use Arduino-like types (such as byte, int, string etc.)


extern void initSensors();
extern void handleSensorInput(unsigned long currMillis);
extern void queryState();
extern void setBoardRotation(int rot);

void sendMessageTCP(String msg);  // this is part of the main .ino file

#endif
