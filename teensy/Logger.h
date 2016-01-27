#pragma once


extern bool LOG_ERROR;
extern bool LOG_INFO;
extern bool LOG_TRACE;
extern bool TRACE_RX;
extern bool TRACE_PINGS;
extern bool TRACE_ESC;
extern bool TRACE_MPU;
extern bool TRACE_LOOP_SPEED;
extern bool TD;



#define log(flag,s)    \
if(flag) {             \
    Serial.println((String) #flag + "," + s); \
} 

#include "Arduino.h"

// returns float as string to overcome bug in Arduino casting
String ftos(float f);
