const int pin_motor_a = 24;
const int pin_motor_b = 25;
const int pin_motor_c = 26;
const int pin_motor_temp = A13;

// examples
//    forward: AcBaCbAcBaCbAcBaCbAcBaCbAc
//    reverse: AbCaBcAbCaBcAbCaBcAbCaBcAbC
const bool trace_transitions = false;

unsigned long a_count = 0;
unsigned long b_count = 0;
unsigned long c_count = 0;
long odometer = 0;

bool a,b,c;
void on_a_change() {
  a = digitalRead(pin_motor_a);
  ++a_count;
  if(a) {
    if(trace_transitions) Serial.print('A');
    if(c) {
      ++odometer;
    } else {
      --odometer;
    }
  }
  else {
    if(b) {
      ++odometer;
    } else {
      --odometer;
    }
    if(trace_transitions) Serial.print('a');
  }
}

void on_b_change() {
  b = digitalRead(pin_motor_b);
  ++b_count;
  if(b) {
    if(trace_transitions) Serial.print('B');
    if(a) {
      ++odometer;
    } else {
      --odometer;
    }
  }
  else {
    if(c) {
      ++odometer;
    } else {
      --odometer;
    }
    if(trace_transitions) Serial.print('b');
  }
}

void on_c_change() {
  c = digitalRead(pin_motor_c);
  ++c_count;
  if(c) {
    if(trace_transitions) Serial.print('C');
    if(b) {
      ++odometer;
    } else {
      --odometer;
    }
  }
  else {
    if(trace_transitions) Serial.print('c');
    if(a) {
      ++odometer;
    } else {
      --odometer;
    }
  }
}


bool every_n_ms(unsigned long last_loop_ms, unsigned long loop_ms, unsigned long ms) {
  return (last_loop_ms % ms) + (loop_ms - last_loop_ms) >= ms;
}


void setup() {
  // put your setup code here, to run once:
  attachInterrupt(digitalPinToInterrupt(pin_motor_a), on_a_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_motor_b), on_b_change, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pin_motor_c), on_c_change, CHANGE);
}
unsigned long loop_time_ms = 0;
unsigned long last_loop_time_ms = 0;

void loop() {
  last_loop_time_ms = loop_time_ms;
  loop_time_ms = millis();

  if (every_n_ms(last_loop_time_ms, loop_time_ms, 1000)) {
    Serial.println((String)"a:" + a_count + " b:" + b_count + " c:" + c_count + " odo:" + odometer + "temp: " + analogRead(pin_motor_temp));
  }
  // put your main code here, to run repeatedly:

}
