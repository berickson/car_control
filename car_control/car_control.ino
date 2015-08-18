#include <Servo.h>


/////////////////////////////////////////////////////
// PIN / Wiring
//
// on the micro, interrupts must go on pins 2 and 3
// interrupt 0 = pin 2
// interrupt 1 = pin 3
#define PIN_RX_STEER 2
#define PIN_RX_SPEED 3
#define PIN_SRC_SELECT 4

#define PIN_PING_TRIG 7
#define PIN_PING_ECHO 6
#define PIN_SPEAKER   10

// computer steering and speed
#define PIN_U_STEER 8
#define PIN_U_SPEED 9


struct PwmInput {
  unsigned long last_trigger_us = 0;
  unsigned long pulse_width_us = 0;
  
  // since we are dealing with standard RC, 
  // anthing out of of the below ranges should not occur and is ignored
  unsigned long max_pulse_us = 3000;
  unsigned long min_pulse_us = 500;
  
  unsigned long last_pulse_ms = 0;  // time when last pulse occurred

  
  // milliseconds without a pulse to consider a timeout
  int timeout_ms = 500;
  int pin;
  
  void attach(int pin) {
    this->pin = pin;
    last_trigger_us = 0;
    pulse_width_us = 0;
    pinMode(pin, INPUT);
  }
  
  void handle_change() {
    unsigned long us = micros();
    if(digitalRead(pin)) {
      last_trigger_us = us;
    }
    else {
      unsigned long width = us - last_trigger_us;
      // only accept pulses in acceptable range
      if(width >= min_pulse_us && width <= max_pulse_us) {
        pulse_width_us = width;
        last_pulse_ms = millis();
      }
    }
  }
  
  bool is_valid() {
    return millis() - last_pulse_ms < timeout_ms;
  }
  
  // safe method to return pulse width in microseconds
  // returns 0 if invalid
  int pulse_us() {
    if(is_valid())
      return pulse_width_us;
    else
      return 0;
  }
  
  
  
  void trace() {
    Serial.print(pin);
    Serial.print(" ");
    Serial.print(pulse_width_us);
    if(is_valid())
      Serial.print(" valid");
    else
      Serial.print(" invalid");
  }
};


struct TriangleWave {
  int period_ms = 10000;
  int min_value = 1450;
  int max_value = 1550;
  int origin_ms;
  
  void init() {
    origin_ms = millis();
  }
  
  int value() {
    unsigned long now_ms = millis();
    int range = max_value - min_value;

    double cycle_portion = (( now_ms - origin_ms ) % period_ms) / (double) period_ms;
    if(cycle_portion > 0.5)
        cycle_portion = 1.0-cycle_portion;
    return (2 * cycle_portion * range + min_value);
    
  }
};

/*
enum rx_event {
  uninitialized = 0;
  steer_unknown = 1;
  steer_left = 2;
  steer_center = 4;
  steer_right = 8;
  power_unknown = 16;
  power_reverse = 32;
  power_neutral = 64;
  power_forward = 128;
  
}
*/


struct RxEvents {
  char rx_speed = '0';
  char rx_steer = '0';
  bool new_event = false;
  
  void process_pulses(int steer_us, int speed_us) {
    char new_rx_steer = steer_code(steer_us);
    char new_rx_speed = speed_code(speed_us);
    if (new_rx_steer != rx_steer || new_rx_speed != rx_speed) {
      rx_speed = new_rx_speed;
      rx_steer = new_rx_steer;
      new_event = true;
    }
  }
  
 
   char steer_code(int steer_us) {
    if (steer_us == 0)
      return '?';
    if (steer_us < 1300)
      return 'R';
    if (steer_us > 1700)
      return 'L';
    return 'C';
  }
  
  char speed_code(int speed_us) {
    if (speed_us == 0)
      return '?';
    if (speed_us < 1300)
      return 'F';
    if (speed_us > 1700)
      return 'V';
    return 'N';
  }
  
  // returns true if new event received since last call
  bool get_event() {
    bool rv = new_event;
    new_event = false;
    return rv;
  }
  
  void trace() {
    Serial.write(rx_steer);
    Serial.write(rx_speed);
  }
};

//////////////////////////
// Globals

Servo steering;
Servo speed;

PwmInput rx_steer;
PwmInput rx_speed;
RxEvents rx_events;
TriangleWave steering_wave;
TriangleWave speed_wave;

enum {
  mode_starting,
  mode_connected,
  mode_running
} mode;


///////////////////////////////////////////////
// Interrupt handlers

void rx_str_handler() {
  rx_steer.handle_change();
}

void rx_spd_handler() {
  rx_speed.handle_change();
}


double ping_inches() {
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  digitalWrite(PIN_PING_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_PING_TRIG, LOW);
  const int timeout_us = 10000;
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  int microseconds = pulseIn(PIN_PING_ECHO, HIGH, timeout_us);

  // convert the time into a distance
  double inches = (double) microseconds / 148.;
  return inches;
}


void setup() {
  steering.attach(PIN_U_STEER);
  speed.attach(PIN_U_SPEED);
  
  rx_steer.attach(PIN_RX_STEER);
  rx_speed.attach(PIN_RX_SPEED);
  
  pinMode(PIN_SRC_SELECT, OUTPUT);
  digitalWrite(PIN_SRC_SELECT, LOW);
  
  attachInterrupt(0, rx_str_handler, CHANGE);
  attachInterrupt(1, rx_spd_handler, CHANGE);

  pinMode(PIN_PING_TRIG, OUTPUT);
  pinMode(PIN_PING_ECHO, INPUT);

  digitalWrite(PIN_PING_TRIG, LOW);
  
  
  
  speed_wave.min_value = 1400;
  speed_wave.max_value = 1650;
  speed_wave.period_ms = 30000;
  speed_wave.init();
  steering_wave.min_value = 1200;
  steering_wave.max_value = 1800;
  steering_wave.init();
  
  mode = mode_starting;
  
  tone(PIN_SPEAKER, 440, 200);

  Serial.begin(9600);
  Serial.println("hello, steering!");
}


double speed_for_ping_inches(double inches) {
  
  // get closer if far
  if (inches > 20.) 
    return 1390;
  // back up if too close
  if (inches < 10.)
    return 1620;
    
  return 1500;

  
}

void loop() {
  
  int loop_time_millis = millis();
  rx_events.process_pulses(rx_steer.pulse_us(), rx_speed.pulse_us());
  double inches = ping_inches();

  if(rx_events.get_event()) {
    rx_events.trace();
    Serial.println();
  }
  
  switch (mode) {
    case mode_starting:
      steering.writeMicroseconds(1500);
      speed.writeMicroseconds(1500);
      digitalWrite(PIN_SRC_SELECT, LOW);
      if (rx_events.rx_steer == 'R' && rx_events.rx_speed == 'N') {
        mode = mode_running;
        Serial.print("switched to running");
      }
      break;
      
    case mode_running:
//      steering.writeMicroseconds(steering_wave.value());
      digitalWrite(PIN_SRC_SELECT, HIGH);
      steering.writeMicroseconds(1500);
      speed.writeMicroseconds(speed_for_ping_inches(inches));
      if (rx_events.rx_steer == 'L' && rx_events.rx_speed == 'N') {
        mode = mode_starting;
        Serial.print("switched to starting - user initiated");
      }
      if (rx_events.rx_steer == '?' || rx_events.rx_speed == '?') {
        mode = mode_starting;
        Serial.print("switched to starting - no coms");
      }
      break;

  }


  // state tracing
  if (0) {
    rx_steer.trace();
    Serial.print(",");
    rx_speed.trace();
    Serial.print(",");
    Serial.print("Speed: ");
    Serial.println(speed_wave.value());
  }
}

