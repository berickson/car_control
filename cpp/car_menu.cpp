#include "car_menu.h"
#include "menu.h"
#include "system.h"
#include "console_menu.h"
#include "fake_car.h"
#include "filenames.h"


struct {
  string track_name = "desk";
  string route_name = FileNames().get_route_names(track_name)[0];
  double max_a = 0.25;
  double max_v = 1.0;
  double t_ahead = 0.3;
  double d_ahead = 0.05;
  double k_smooth = 0.4;
  bool capture_video = false;
} car_settings;


void assert0(int n) {
  if(n!=0) {
    throw (string) "expected zero, got" + to_string(n);
  }

}


void shutdown(){
  int rv =  system("sudo shutdown now");
  assert0(rv);
}
void reboot(){
  int rv = system("sudo shutdown -r now");
  assert0(rv);

}
void restart(){
  int rv = system("sudo service car restart");
  assert0(rv);
}


SubMenu route_selection_menu{};


string get_route_name() {
  return car_settings.route_name;
}

void set_route_name(string s) {
  car_settings.route_name = s;
}

void update_route_selection_menu() {
  route_selection_menu.items.clear();
  vector<string> route_names = FileNames().get_route_names(car_settings.track_name);
  selection_menu<string>(route_selection_menu, route_names, get_route_name, set_route_name);
}


string get_track_name() {
  return car_settings.track_name;
}
void set_track_name(string s) {
  car_settings.track_name = s;
  update_route_selection_menu();
}





SubMenu pi_menu {
  {"shutdown",shutdown},
  {"reboot",reboot},
  {"restart",restart}
};



CarMenu::CarMenu() {
}

void run_car_menu() {
  FakeCar car;



  SubMenu track_selection_menu{};
  vector<string> track_names = FileNames().get_track_names();
  selection_menu<string>(track_selection_menu, track_names, get_track_name, set_track_name);

  update_route_selection_menu();

  SubMenu route_menu {
    {[](){return (string)"track ["+car_settings.track_name+"]";},&track_selection_menu},
    {[](){return (string)"route ["+car_settings.route_name+"]";},&route_selection_menu}

  };

  SubMenu mid_menu {
    {"routes",&route_menu},
    {"pi",&pi_menu}
  };

  SubMenu car_menu {
    {[&car](){return get_first_ip_address();}, &mid_menu},
    {[&car](){return "v: " + to_string(car.get_voltage());}},
    {[&car](){return "front: " + to_string(car.get_front_position());}},
    {[&car](){return "usb errors: " + to_string(car .get_usb_error_count());}},
    {[&car](){return "heading: " + to_string(car.get_heading_degrees());}},
    {[&car](){return "rear: " + to_string(car.get_rear_position());}},
    {[&car](){return "odo_fl: " + to_string(car.get_odometer_front_left());}},
    {[&car](){return "odo_fr: " + to_string(car.get_odometer_front_right());}},
    {[&car](){return "odo_bl: " + to_string(car.get_odometer_back_left());}},
    {[&car](){return "odo_br: " + to_string(car.get_odometer_back_right());}}

  };

  ConsoleMenu menu(&car_menu);
  menu.run();
}

void test_car_menu() {
  run_car_menu();
}
