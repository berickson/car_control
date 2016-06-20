#include "car.h"
#include "config.h"

#include <iostream>
#include "lookup_table.h"
#include "split.h"
using namespace std;

Car::Car(bool online) {
  read_configuration(config_path);
  this->online = online;
  reset_odometry();
  if(online) {
    connect_usb();
  }
}

Car::~Car() {
  quit=true;
  if(usb_thread.joinable())
    usb_thread.join();
}

void Car::connect_usb() {
  usb_thread = thread(&Car::usb_thread_start, this);
}

void Car::usb_thread_start() {
  usb.add_line_listener(&usb_queue);
  usb.write_on_connect("\ntd+\n");
  usb.run();
  while(!quit) {
    string line;
    if(usb_queue.try_pop(line,1)) {
      process_line_from_log(line);
    }
  }
}

void Car::process_line_from_log(string line) {
  if(input_recording_file.is_open()) {
    input_recording_file << line << endl; //todo: make non-blocking
  }
  if(split(line)[1]!="TD") {
    return;
  }
  Dynamics d;
  bool ok = Dynamics::from_log_string(d,line);
  if(ok) {
    apply_dynamics(d);
    for(auto listener:listeners) {
      listener->push(std::move(d));
    }
  }
  else {
    ++usb_error_count;
    cerr << "dynamics not ok for " << line << endl;
  }
}

void Car::send_command(string command) {
  usb.write_line(command);
}

void Car::set_rc_mode() {
  send_command("rc");

}

void Car::set_manual_mode() {
  send_command("m");
}

void Car::set_esc_and_str(unsigned esc, unsigned str)
{
  send_command((string)"pse "+to_string(str)+","+to_string(esc));
}


void Car::begin_recording_input(string path) {
  end_recording_input();
  input_recording_file.open(path, ios_base::out);
}

void Car::end_recording_input() {
  if(input_recording_file.is_open())
    input_recording_file.flush();
    input_recording_file.close();
}

void Car::apply_dynamics(Dynamics & d) {
  // set all the dynamics variables
  reading_count++;

  previous_dynamics = current_dynamics;
  current_dynamics = d;
  if(reading_count == 1) {
    original_dynamics = d;
    return;
  }

  // update state
  Dynamics & current = current_dynamics;
  Dynamics & previous = previous_dynamics;

  // correct heading with adjustment factor
  auto temp = (current.yaw - previous.yaw);
  Angle adjustment = temp * (1. - gyro_adjustment_factor) ;
  heading_adjustment += adjustment;

  // if wheels have moved, update ackerman
  double wheel_distance_meters = (current.odometer_front_left-previous.odometer_front_left)  * meters_per_odometer_tick;

  if (fabs(wheel_distance_meters) > 0.) {
    double outside_wheel_angle = radians(angle_for_steering(previous.str));
    ackerman.move_left_wheel(outside_wheel_angle, wheel_distance_meters, get_heading_radians());
  }


  // update velocity
  if (current.odometer_front_left_last_us != previous.odometer_front_left_last_us) {
    double elapsed_seconds = (current.odometer_front_left_last_us - previous.odometer_front_left_last_us) / 1000000.;
    velocity = wheel_distance_meters / elapsed_seconds;
    last_verified_velocity = velocity;
  } else {
    // no tick this time, how long has it been?
    double seconds_since_tick = ( current.us - current.odometer_front_left_last_us) / 1000000.;
    if (seconds_since_tick > 0.1){
      // it's been a long time, let's call velocity zero
      velocity = 0.0;
    } else {
      // we've had a tick recently, fastest possible speed is when a tick is about to happen
      // let's use smaller of that and previously certain velocity
      double  max_possible = meters_per_odometer_tick / seconds_since_tick;
      if(max_possible > fabs(last_verified_velocity)){
        velocity = last_verified_velocity;
      } else {
        if(last_verified_velocity > 0)
          velocity = max_possible;
        else
          velocity = -max_possible;
      }
    }
  }

}

void Car::add_listener(WorkQueue<Dynamics>* listener) {
  listeners.push_back(listener);
}

void Car::remove_listener(WorkQueue<Dynamics>*listener) {
  listeners.remove(listener);
}

void Car::read_configuration(string path){
  Config config;
  config.load_from_file(path);

  // odometery
  meters_per_odometer_tick = config.get_double("meters_per_odometer_tick");
  gyro_adjustment_factor = config.get_double("gyro_adjustment_factor");

  // esc and steering
  center_steering_us = config.get_double("center_steering_us");
  min_forward_esc = config.get_int("min_forward_esc");
  min_reverse_esc = config.get_int("min_reverse_esc");
  reverse_center_steering_us = config.get_int("reverse_center_steering_us");

  // car dimensions
  front_wheelbase_width_in_meters = config.get_double("front_wheelbase_width_in_meters");
  rear_wheelbase_width_in_meters = config.get_double("rear_wheelbase_width_in_meters");
  wheelbase_length_in_meters = config.get_double("wheelbase_length_in_meters");

  // infer general dimensions
  length = wheelbase_length_in_meters;
  width = front_wheelbase_width_in_meters; // ?! why front?
}

void Car::reset_odometry() {
  //dynamics = Dynamics();
  original_dynamics = current_dynamics;

  velocity = 0.0;
  last_velocity = 0.0;
  heading_adjustment = Angle::degrees(0.);
  odometer_front_left_start = 0;
  odometer_front_right_start = 0;
  odometer_back_left_start = 0;
  odometer_back_right_start = 0;
  ackerman = Ackerman(
    front_wheelbase_width_in_meters,
    wheelbase_length_in_meters);
}


Angle Car::get_heading() {
  Angle a = (
        current_dynamics.yaw
        - original_dynamics.yaw
        + heading_adjustment);
  a.standardize();
  return a;
}

double Car::get_heading_degrees() {
  return get_heading().degrees();
}

double Car::get_heading_radians() {
  return get_heading().radians();
}

int Car::steering_for_angle(Angle theta)
{
  static const LookupTable t({
    {30,1000},
    {25,1104},
    {20,1189},
    {15,1235},
    {10,1268},
    {5, 1390},
    {0, 1450},
    {-5, 1528},
    {-10, 1607},
    {-15,1688},
    {-20, 1723},
    {-25, 1768},
    {-30, 1858}});
  return (int) t.lookup(theta.degrees());
}



double Car::angle_for_steering(int str) {

  const int data[13][2] = {
    {30,1000},
    {25,1104},
    {20,1189},
    {15,1235},
    {10,1268},
    {5, 1390},
    {0, 1450},
    {-5, 1528},
    {-10, 1607},
    {-15,1688},
    {-20, 1723},
    {-25, 1768},
    {-30, 1858}};

  int last = 12;
  if (str <= data[0][1]){
    return data[0][0];
  }
  if (str >= data[last][1]){
    return data[last][0];
  }
  for(int i=0;i<last;i++){
    if (str <= data[i+1][1]){
      return interpolate(
            str, data[i][1], data[i][0], data[i+1][1], data[i+1][0]);
    }
  }

  return NAN;
}

#include <vector>
#include <array>
#include "lookup_table.h"

int Car::esc_for_velocity(double v)
{
  LookupTable t(
  {
    {-2., 1200},
    {-1., 1250},
    {0.0,  1500},
    {0.1, 1645},
    {0.34, 1659},
    {0.85, 1679},
    {1.2, 1699},
    {1.71, 1719},
    {1.88, 1739},
    {2.22, 1759},
    {2.6, 1779},
    {3.0, 1799},
    {14.0, 2000}
  });
  return t.lookup(v);

}

void test_car() {
  Car car;
  cout << "front_wheelbase_width_in_meters: " << car.front_wheelbase_width_in_meters << endl;

  WorkQueue<Dynamics> listener;
  car.add_listener(&listener);
  int message_count = 10000;
  for(int i=0;i<message_count;i++) {
    Dynamics d;
    if(listener.try_pop(d,100)) {
      cout << "." << flush;//cout << d.to_string() << endl;
    } else {
      cout << "timed out waiting for dynamics" << endl;
    }
  }
  car.remove_listener(&listener);
}
