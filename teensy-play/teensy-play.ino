#include "Arduino.h"

const uint8_t pin_led = 13;
void setup(){
  pinMode(pin_led, OUTPUT);
}

void loop() {
  digitalWrite(pin_led,micros()%1000000<100);
}