#include "Arduino.h"
#include "Task.h"
#include "Fsm.h"
#include "ManualMode.h"
#include "RemoteMode.h"
#include "CommandInterpreter.h"

const int pin_motor_a = 24;
const int pin_motor_b = 25;
const int pin_motor_c = 26;
const int pin_motor_temp = A13;

const int pin_odo_fl_a = 0;
const int pin_odo_fl_b = 1;
const int pin_odo_fr_a = 3;
const int pin_odo_fr_b = 2;

const int pin_str = 8;
const int pin_esc = 9;
const int pin_esc_aux = 10;
const int pin_rx_str = 11;
const int pin_rx_esc = 12;

const int pin_mpu_interrupt = 17;

// all these ugly pushes are because the 9150 has a lot of warnings
// the .h file must be included in one time in a source file
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-value"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include <MPU9150_9Axis_MotionApps41.h>
#pragma GCC diagnostic pop

#include "Mpu9150.h"

Mpu9150 mpu9150;

#include "QuadratureEncoder.h"
#include "MotorEncoder.h"

#include "Servo2.h"
#include "PwmInput.h"

///////////////////////////////////////////////
// helpers

#define count_of(a) (sizeof(a)/sizeof(a[0]))


///////////////////////////////////////////////
// globals

QuadratureEncoder odo_fl(pin_odo_fl_a, pin_odo_fl_b);
QuadratureEncoder odo_fr(pin_odo_fr_a, pin_odo_fr_b);

PwmInput rx_str;
PwmInput rx_esc;

Servo2 str;
Servo2 esc;

MotorEncoder motor(pin_motor_a, pin_motor_b, pin_motor_c);

///////////////////////////////////////////////
// modes

ManualMode manual_mode;
RemoteMode remote_mode;

Task * tasks[] = {&manual_mode, &remote_mode};

Fsm::Edge edges[] = {
  {"manual", "remote", "remote"},
  {"remote", "manual", "manual"},
  {"remote", "non-neutral", "manual"},
  {"remote", "done", "manual"}
};

Fsm modes(tasks, count_of(tasks), edges, count_of(edges));

///////////////////////////////////////////////
// commands

CommandInterpreter interpreter;

void command_pulse_steer_and_esc() {
  String & args = interpreter.command_args;
  log(LOG_TRACE,"pse args" + args);
  int i = args.indexOf(",");
  if(i == -1) {
    log(LOG_ERROR,"invalid args to pse " + args);
    return;
  }
  String s_str = args.substring(0, i);
  String s_esc = args.substring(i+1);

  remote_mode.command_steer_and_esc(s_str.toInt(),  s_esc.toInt());
}

void trace_mpu_on() {
  TRACE_MPU = true;
}

void trace_mpu_off() {
  TRACE_MPU = false;
}

void trace_loop_speed_on() {
  TRACE_LOOP_SPEED = true;
}

void trace_loop_speed_off() {
  TRACE_LOOP_SPEED = false;
}

void trace_dynamics_on() {
  TD = true;
}

void trace_dynamics_off() {
  TD = false;
}




void command_manual() {
  modes.set_event("manual");
}

void command_remote_control() {
  modes.set_event("remote");
  Serial.println("remote event");
}

void calibrate_mpu() {
  mpu9150.start_calibrate_at_rest(0, 60);
}

void help(); // forward decl for commands

const Command commands[] = {
  {"?", "help", help},
  {"help", "help", help},
  {"calibrate", "calibrate at rest mpu", calibrate_mpu},
  {"td+", "trace dynamics on", trace_dynamics_on},
  {"td-", "trace dynamics off", trace_dynamics_off},
//  {"tp+", "trace ping on", trace_ping_on},
//  {"tp-", "trace ping off", trace_ping_off},
  {"tm+", "trace mpu on", trace_mpu_on},
  {"tm-", "trace mpu off", trace_mpu_off},
  {"tl+", "trace loop speed on", trace_loop_speed_on},
  {"tl-", "trace loop speed off", trace_loop_speed_off},
//  {"c", "circle", command_circle},
  {"m", "manual", command_manual},
//  {"f", "follow", command_follow},
  {"rc", "remote control", command_remote_control},
  {"pse", "pulse steer, esc", command_pulse_steer_and_esc},
//  {"beep", "beep", command_beep},
  
};

void help() {
  for(unsigned int i = 0; i < count_of(commands); i++) {
    const Command &c = commands[i];
    Serial.println(String(c.name)+ " - " + c.description);
  }
}



///////////////////////////////////////////////
// Interrupt handlers

void rx_str_handler() {
  rx_str.handle_change();
}

void rx_esc_handler() {
  rx_esc.handle_change();
}


void odo_fl_a_changed() {
  odo_fl.sensor_a_changed();
}

void odo_fl_b_changed() {
  odo_fl.sensor_b_changed();
}

void odo_fr_a_changed() {
  odo_fr.sensor_a_changed();
}

void odo_fr_b_changed() {
  odo_fr.sensor_b_changed();
}

void motor_a_changed() {
  motor.on_a_change();
}

void motor_b_changed() {
  motor.on_b_change();
}

void motor_c_changed() {
  motor.on_c_change();
}


bool every_n_ms(unsigned long last_loop_ms, unsigned long loop_ms, unsigned long ms) {
  return (last_loop_ms % ms) + (loop_ms - last_loop_ms) >= ms;
}


void setup() {

  Serial.begin(250000);
  while(!Serial); // wait for serial to become ready

  manual_mode.name = "manual";
  // follow_mode.name = "follow";
  remote_mode.name = "remote";
  modes.begin();

  //delay(1000);
  interpreter.init(commands, count_of(commands));
  // put your setup code here, to run once:
  rx_str.attach(pin_rx_str);
  rx_esc.attach(pin_rx_esc);
  str.attach(pin_str);
  esc.attach(pin_esc);

  attachInterrupt(pin_motor_a, motor_a_changed, CHANGE);
  attachInterrupt(pin_motor_b, motor_b_changed, CHANGE);
  attachInterrupt(pin_motor_c, motor_c_changed, CHANGE);

  attachInterrupt(pin_odo_fl_a, odo_fl_a_changed, CHANGE);
  attachInterrupt(pin_odo_fl_b, odo_fl_b_changed, CHANGE);
  attachInterrupt(pin_odo_fr_a, odo_fr_a_changed, CHANGE);
  attachInterrupt(pin_odo_fr_b, odo_fr_b_changed, CHANGE);

  // pinMode(pin_motor_temp, INPUT_PULLUP);

  attachInterrupt(pin_rx_str, rx_str_handler, CHANGE);
  attachInterrupt(pin_rx_esc, rx_esc_handler, CHANGE);

  log(LOG_TRACE,"starting wire");
  Wire.begin();

  log(LOG_TRACE, "starting mpu");

  mpu9150.enable_interrupts(pin_mpu_interrupt);
  log(LOG_INFO, "Interrupts enabled for mpu9150");
  mpu9150.setup();
  
  mpu9150.ax_bias = 0; // 7724.52;
  mpu9150.ay_bias = 0; // -1458.47;
  mpu9150.az_bias = 0; // 715.62;
  mpu9150.rest_a_mag = 0; // 7893.51;
  mpu9150.zero_adjust = Quaternion(0.0, 0.0, 0.0, 1);// Quaternion(-0.07, 0.67, -0.07, 0.73);
  // was ((-13.823402+4.9) / (1000 * 60 * 60)) * PI/180;
  mpu9150.yaw_slope_rads_per_ms  = -0.0000000680;// (2.7 / (10 * 60 * 1000)) * PI / 180;
  mpu9150.yaw_actual_per_raw = 1; //(3600. / (3600 - 29.0 )); //1.0; // (360.*10.)/(360.*10.-328);// 1.00; // 1.004826221;

  mpu9150.zero_heading();
  log(LOG_INFO, "setup complete");
  

}

// diagnostics for reporting loop speeds
unsigned long loop_count = 0;
unsigned long loop_time_ms = 0;
unsigned long last_loop_time_ms = 0;
unsigned long last_report_ms = 0;
unsigned long last_report_loop_count = 0;

void loop() {
  loop_count++;
  last_loop_time_ms = loop_time_ms;
  loop_time_ms = millis();

  bool every_second = every_n_ms(last_loop_time_ms, loop_time_ms, 1000);
  bool every_10_ms = every_n_ms(last_loop_time_ms, loop_time_ms, 10);


  mpu9150.execute();
  interpreter.execute();

  if (every_10_ms) {
    modes.execute();
  }


  if (every_10_ms && TD) {
      Serial.println(
        (String) "a: " + motor.a_count 
        + " b: " + motor.b_count 
        + " c: " + motor.c_count 
        + " odo:" + motor.odometer 
        + " temp: " + analogRead(pin_motor_temp)
        + " heading: " + mpu9150.heading()
        + " fl: (" + odo_fl.odometer_a + ","+odo_fl.odometer_b+")"
        + " fr: (" + odo_fr.odometer_a + ","+odo_fr.odometer_b+")"
      );
  }

  if(every_second && TRACE_LOOP_SPEED) {
    unsigned long loops_since_report = loop_count - last_report_loop_count;
    float seconds_since_report =  (loop_time_ms - last_report_ms) / 1000.;

    log(TRACE_LOOP_SPEED, "loops per second: "+ (loops_since_report / seconds_since_report ) + " microseconds per loop "+ (1E6 * seconds_since_report / loops_since_report) );

    // remember stats for next report
    last_report_ms = loop_time_ms;
    last_report_loop_count = loop_count;
  }
}