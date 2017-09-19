#include "car_menu.h"
#include "menu.h"
#include "route.h"
#include "system.h"
#include "console_menu.h"
#include "fake_car.h"
#include "file_names.h"
#include "string_utils.h"
#include <fstream>
#include <sstream>
#include <bitset>
#include "car_ui.h"
#include "driver.h"
#include "run_settings.h"
#include <unistd.h> // usleep
#include "camera.h"
#include "logger.h"

#include <ncurses.h> // sudo apt-get install libncurses5-dev



string run_settings_path = "run_settings.json";

RunSettings run_settings;


string wheel_display_string(const Speedometer & wheel){
  return format(wheel.get_meters_travelled(),7,2)+"m "
         + format(wheel.get_smooth_velocity(),4,1)+"m/s "
         + format(wheel.get_ticks()) + "t";
}

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
  log_info("restarting service based on user request");
  int rv = system("sudo ./run");
  terminate();
  assert0(rv);
}
void update_software() {
  log_info("restarting based on user update request");
  int ignored __attribute__((unused));
  ignored = system("git pull");
  ignored = system("./build");
  ignored = system("sudo ./run");
  log_info("terminating");
  terminate();

}


SubMenu route_selection_menu{};


string get_route_name() {
  return run_settings.route_name;
}

void set_route_name(string s) {
  run_settings.route_name = s;
}

void update_route_selection_menu() {
  route_selection_menu.items.clear();
  vector<string> route_names = FileNames().get_route_names(run_settings.track_name);
  string max_name = "";
  if(route_names.size() > 0) {
      max_name = *max_element(route_names.begin(),route_names.end()) ;
  };
  set_route_name(max_name);
  selection_menu<string>(route_selection_menu, route_names, get_route_name, set_route_name);
}

string get_track_name() {
  return run_settings.track_name;
}

void set_track_name(string s) {
  run_settings.track_name = s;
  update_route_selection_menu();
}


// getters / setters for config
void set_max_accel_lat(double v){run_settings.max_accel_lat = v;}
double get_max_accel_lat(){return run_settings.max_accel_lat;}

void set_max_accel(double v){run_settings.max_accel = v;}
double get_max_accel(){return run_settings.max_accel;}

void set_max_decel(double v){run_settings.max_decel = v;}
double get_max_decel(){return run_settings.max_decel;}

void set_max_v(double v){run_settings.max_v = v;}
double get_max_v(){return run_settings.max_v;}

void set_t_ahead(double v){run_settings.t_ahead = v;}
double get_t_ahead(){return run_settings.t_ahead;}

void set_d_ahead(double v){run_settings.d_ahead = v;}
double get_d_ahead(){return run_settings.d_ahead;}

void set_k_smooth(double v){run_settings.k_smooth = v;}
double get_k_smooth(){return run_settings.k_smooth;}

void set_k_p(double v){run_settings.steering_k_p = v;}
double get_k_p(){return run_settings.steering_k_p;}

void set_k_i(double v){run_settings.steering_k_i = v;}
double get_k_i(){return run_settings.steering_k_i;}

void set_k_d(double v){run_settings.steering_k_d = v;}
double get_k_d(){return run_settings.steering_k_d;}

void set_v_k_p(double v){run_settings.v_k_p = v;}
double get_v_k_p(){return run_settings.v_k_p;}

void set_v_k_i(double v){run_settings.v_k_i = v;}
double get_v_k_i(){return run_settings.v_k_i;}

void set_v_k_d(double v){run_settings.v_k_d = v;}
double get_v_k_d(){return run_settings.v_k_d;}

void set_prune_max(double v){run_settings.prune_max = v;}
double get_prune_max(){return run_settings.prune_max;}

void set_prune_tolerance(double v){run_settings.prune_tolerance = v;}
double get_prune_tolerance(){return run_settings.prune_tolerance;}


void set_slip_rate(double v){run_settings.slip_rate = v;}
double get_slip_rate(){return run_settings.slip_rate;}

void set_slip_slop(double v){run_settings.slip_slop = v;}
double get_slip_slop(){return run_settings.slip_slop;}


void set_capture_video(double v){run_settings.capture_video = v;}
double get_capture_video(){return run_settings.capture_video;}

void set_crash_recovery(double v){run_settings.crash_recovery = v;}
double get_crash_recovery(){return run_settings.crash_recovery;}

void set_optimize_velocity(double v){run_settings.optimize_velocity = v;}
double get_optimize_velocity(){return run_settings.optimize_velocity;}


SubMenu pi_menu {
  MenuItem{"shutdown",shutdown},
  MenuItem{"reboot",reboot},
  //{"restart",restart},
  //{"update software 2",update_software}
};


SubMenu max_accel_lat_menu{};
SubMenu max_accel_menu{};
SubMenu max_decel_menu{};
SubMenu max_v_menu{};

SubMenu k_p_menu{};
SubMenu k_i_menu{};
SubMenu k_d_menu{};

SubMenu v_k_p_menu{};
SubMenu v_k_i_menu{};
SubMenu v_k_d_menu{};

SubMenu prune_max_menu{};
SubMenu prune_tolerance_menu{};

SubMenu k_smooth_menu{};
SubMenu t_ahead_menu{};
SubMenu d_ahead_menu{};
SubMenu capture_video_menu{};
SubMenu crash_recovery_menu{};
SubMenu optimize_velocity_menu{};

void go(Car& car) {
  log_entry_exit w("go");
  try {
    run_settings.load_from_file_json(run_settings_path);
    auto f = FileNames();
    string & track_name = run_settings.track_name;
    string & route_name = run_settings.route_name;
    string run_name = f.next_run_name(track_name, route_name);
    string run_folder = f.get_run_folder(track_name,route_name, run_name);
    string runs_folder = f.get_runs_folder(track_name,route_name);
    mkdir(runs_folder);
    mkdir(run_folder);
    run_settings.write_to_file_json(f.config_file_path(track_name, route_name, run_name));
    run_settings.write_to_file_json(run_settings_path);
    car.reset_odometry();
    string input_path = f.path_file_path(track_name,route_name);
    Route rte;
    log_info("loading route: " + input_path);
    rte.load_from_file(input_path);
    log_info("route loaded");
    StereoCamera camera;
    if(run_settings.capture_video) {
      camera.warm_up();
    }


    //ui.clear();
    //ui.print("smoothing route\n");
    //ui.refresh();
    log_info("preparing route");
    rte.smooth(run_settings.k_smooth);
    //ui.print("pruning route\n");
    rte.prune(run_settings.prune_max, run_settings.prune_tolerance);
    //ui.refresh();

    //ui.clear();
    //ui.move(0,0);
    //ui.refresh();
    if(run_settings.optimize_velocity) {
      rte.optimize_velocity(run_settings.max_v, run_settings.max_accel_lat, run_settings.max_accel, run_settings.max_decel);
      //ui.print("optimized velocity\n");
    } else {
      //ui.print("using saved velocities\n");
    }
    //ui.print((string)"max_v calculated at " + format(rte.get_max_velocity(),4,1) + "\n\n");
    //ui.print("[back] [] []  [go]");
    //ui.refresh();
    //if(ui.wait_key()!='4') {
    //  return;
    //}

    //ui.clear();
    //ui.print((string)"playing route with max velocity " + format(rte.get_max_velocity()));
    //ui.refresh();

    if(run_settings.capture_video) {
      vector<string> video_paths = f.stereo_video_file_paths(track_name,route_name,run_name);
      camera.begin_recording(video_paths[0],video_paths[1]);
    }

    string recording_file_path = f.recording_file_path(track_name, route_name, run_name);
    car.begin_recording_input(recording_file_path);
    car.begin_recording_state(f.state_log_path(track_name, route_name, run_name));

    string state_file_path = f.state_log_path(track_name, route_name, run_name);
    car.begin_recording_state(state_file_path);
    std::string error_text = "";
    try {
      Driver d(car,run_settings);
      d.drive_route(rte);
      log_info("back from drive_route");
    } catch (std::string e) {
      error_text = e;
      log_error(e);
    }
    if(run_settings.capture_video) {
      camera.end_recording();
    }
    car.end_recording_input();
    car.end_recording_state();
    //ui.clear();
    //ui.print("making path");
    log_info("making path");

    string path_file_path = f.path_file_path(track_name, route_name, run_name);
    log_info("1");
    write_path_from_recording_file(recording_file_path, path_file_path);
    log_info("2");
    log_info("recording: " + recording_file_path);
    log_info("track: " + track_name);
    log_info("route: " + route_name);
    log_info("run: " + run_name);
    string dynamics_path = f.dynamics_file_path(track_name,route_name,run_name);
    log_info(dynamics_path);
    log_info(recording_file_path);
    write_dynamics_csv_from_recording_file(recording_file_path, dynamics_path);
    rte.write_to_file(f.planned_path_file_path(track_name, route_name, run_name));
    run_settings.write_to_file_json(run_settings_path);

    if(error_text.size()) {
      //ui.clear();
      //ui.print((string) "Error:: \n"+error_text);
      log_error((string) "Error during play route: "+error_text);

      //ui.print("[ok]");
      //ui.refresh();
      //ui.wait_key();
    } else {
      //ui.clear();
      //ui.print("Playback Success [ok]");
      //ui.refresh();
      //ui.wait_key();
    }

  } catch (string s) {
    //ui.clear();
    //ui.move(0,0);
    //ui.print("error: " + s);
    log_error("go caught error:" + s);
    //ui.refresh();
    //ui.wait_key();
  } catch (...) {
    log_error("unknown error caught in go");
  }
}

void record(Car& car) {
  log_entry_exit w("record");
  run_settings.load_from_file_json(run_settings_path);  
  car.reset_odometry();
  FileNames f;
  string track_name = run_settings.track_name;
  string route_name = f.next_route_name(track_name);
  log_info((string)"recording route name: " + route_name);
  mkdir(f.get_routes_folder(track_name));
  mkdir(f.get_route_folder(track_name,route_name));

  StereoCamera camera;
  if(run_settings.capture_video) {
    camera.warm_up();
    vector<string> video_paths = f.stereo_video_file_paths(track_name,route_name);
    camera.begin_recording(video_paths[0],video_paths[1]);
  }

  string recording_path = f.recording_file_path(track_name,route_name);
  car.begin_recording_input(recording_path);
  car.begin_recording_state(f.state_log_path(track_name,route_name));

  while(car.command_from_socket == "record") {
    usleep(30000);
  }

  car.end_recording_input();
  car.end_recording_state();

  if(run_settings.capture_video){
    camera.end_recording();
  }

  log_info("done recording - making path");

  string path_file_path = f.path_file_path(track_name, route_name);
  write_path_from_recording_file(recording_path, path_file_path);
  write_dynamics_csv_from_recording_file(recording_path, f.dynamics_file_path(track_name, route_name));

  run_settings.route_name = route_name;
  update_route_selection_menu();
  run_settings.write_to_file_json(run_settings_path);
  return;
}


string calibration_string(int a) {
  std::bitset<8> x(a);
  stringstream ss;
   ss << x;
  return ss.str();
}

void run_car_socket() {
  log_entry_exit w("run_car_socket");
  try {
    Car car;
    run_settings.load_from_file_json(run_settings_path);
    
    while(true) {
     
      if(car.command_from_socket == "go") {
        try {
           go(car);
        } catch (...) {
           log_error("exception caught in run_car_socket go");
        }
      }
      if(car.command_from_socket == "record") {
        run_settings.track_name = "desk";
        try {
           record(car);
        } catch (...) {
           log_error("exception caught in run_car_socket record");
        }
      }
      car.command_from_socket = "";
      usleep(30000);
    } 
  }
  catch (...) {
    log_error("exception caught in run_car_socket");
  }
}

/*
void run_car_menu() {
#ifdef RASPBERRY_PI
  Car car;
#else
  FakeCar car;
#endif

  if(file_exists(run_settings_path)) {
      run_settings.load_from_file_json(run_settings_path);
  }

  selection_menu<double>(max_accel_lat_menu, linspace(0.25,10,0.25), get_max_accel_lat, set_max_accel_lat );
  selection_menu<double>(max_accel_menu, linspace(0.25,10,0.25), get_max_accel, set_max_accel );
  selection_menu<double>(max_decel_menu, linspace(0.25,10,0.25), get_max_decel, set_max_decel );
  selection_menu<double>(max_v_menu, linspace(0.5,20,0.5), get_max_v, set_max_v );
  selection_menu<double>(k_p_menu, linspace(0.,300,10), get_k_p, set_k_p );
  selection_menu<double>(k_i_menu, linspace(0.,30,0.5), get_k_i, set_k_i );
  selection_menu<double>(k_d_menu, linspace(0.,300,5), get_k_d, set_k_d );

  selection_menu<double>(v_k_p_menu, linspace(0.,15,0.5), get_v_k_p, set_v_k_p );
  selection_menu<double>(v_k_i_menu, linspace(0.,30,1.), get_v_k_i, set_v_k_i );
  selection_menu<double>(v_k_d_menu, linspace(0.,3,0.25), get_v_k_d, set_v_k_d );

  selection_menu<double>(prune_max_menu, linspace(0.0 ,3.0, 0.1), get_prune_max, set_prune_max );
  selection_menu<double>(prune_tolerance_menu, linspace(0.0, 0.2, 0.01), get_prune_tolerance, set_prune_tolerance);


  selection_menu<double>(k_smooth_menu, linspace(0.,1,0.1), get_k_smooth, set_k_smooth );
  selection_menu<double>(t_ahead_menu, linspace(0.,1,0.1), get_t_ahead, set_t_ahead );
  selection_menu<double>(d_ahead_menu, linspace(0.,.1,0.01), get_d_ahead, set_d_ahead );
  selection_menu<double>(capture_video_menu, {0,1}, get_capture_video, set_capture_video );
  selection_menu<double>(crash_recovery_menu, {0,1}, get_crash_recovery, set_crash_recovery );
  selection_menu<double>(optimize_velocity_menu, {0,1}, get_optimize_velocity, set_optimize_velocity );


  SubMenu track_selection_menu{};
  vector<string> track_names = FileNames().get_track_names();
  selection_menu<string>(track_selection_menu, track_names, get_track_name, set_track_name);

  update_route_selection_menu();

  SubMenu route_menu {
    {[](){return (string)"track ["+run_settings.track_name+"]";},&track_selection_menu},
    {[](){return (string)"route ["+run_settings.route_name+"]";},&route_selection_menu},
    MenuItem("go...",[&car,&ui](){go(car);}),
    {[&car](){
        return (string) calibration_string(car.current_dynamics.calibration_status)
            + " " + format(car.get_heading().degrees(),5,1) + "° "
            + format(car.get_reading_count(),6,0)
            + " " + format(car.current_dynamics.battery_voltage,4,1)+"v"; }},
    MenuItem("record",[&car,&ui](){record(car,ui);}),
    {[](){return (string)"optimize velocity ["+format(run_settings.optimize_velocity)+"]";},&optimize_velocity_menu},
    {[](){return (string)"max_accel_lat ["+format(run_settings.max_accel_lat)+"]";},&max_accel_lat_menu},
    {[](){return (string)"max_accel ["+format(run_settings.max_accel)+"]";},&max_accel_menu},
    {[](){return (string)"max_decel ["+format(run_settings.max_decel)+"]";},&max_decel_menu},
    {[](){return (string)"max_v ["+format(run_settings.max_v)+"]";},&max_v_menu},

    {[](){return (string)"crash recovery ["+format(run_settings.crash_recovery)+"]";},&crash_recovery_menu},
    {[](){return (string)"k_p ["+format(run_settings.steering_k_p)+"]";},&k_p_menu},
    {[](){return (string)"k_i ["+format(run_settings.steering_k_i)+"]";},&k_i_menu},
    {[](){return (string)"k_d ["+format(run_settings.steering_k_d)+"]";},&k_d_menu},

    {[](){return (string)"v_k_p ["+format(run_settings.v_k_p)+"]";},&v_k_p_menu},
    {[](){return (string)"v_k_i ["+format(run_settings.v_k_i)+"]";},&v_k_i_menu},
    {[](){return (string)"v_k_d ["+format(run_settings.v_k_d)+"]";},&v_k_d_menu},


    {[](){return (string)"prune tol ["+format(run_settings.prune_tolerance)+"]";},&prune_tolerance_menu},
    {[](){return (string)"prune max ["+format(run_settings.prune_max)+"]";},&prune_max_menu},

    {[](){return (string)"k_smooth ["+format(run_settings.k_smooth)+"]";},&k_smooth_menu},
    {[](){return (string)"t_ahead ["+format(run_settings.t_ahead)+"]";},&t_ahead_menu},
    {[](){return (string)"d_ahead ["+format(run_settings.d_ahead)+"]";},&d_ahead_menu},
    {[](){return (string)"cap video ["+format(run_settings.capture_video)+"]";},&capture_video_menu}

  };

  SubMenu mid_menu {
    {"routes",&route_menu},
    {"pi",&pi_menu}
  };


  SubMenu car_menu {
    {[&car](){return get_first_ip_address();}, &mid_menu},
    {[&car](){return "v: " + format(car.get_voltage());}},
    {[&car](){return "front: " + to_string(car.get_front_position());}},
    {[&car](){return "usb readings: " + format(car.get_reading_count());}},
    {[&car](){return "usb errors: " + format(car .get_usb_error_count());}},
    {[&car](){return "reset odo ";}, [&car]() {car.reset_odometry();}},
    {[&car](){return "heading: " + format(car.get_heading().degrees());}},
    {[&car](){return "heading_adj: " + format(car.get_zero_heading().degrees());}},
    {[&car](){return "rear: " + to_string(car.get_rear_position());}},
    {[&car](){return "spur: " + format(car.get_spur_odo());}},
    {[&car](){return "fl: " + wheel_display_string(car.get_front_left_wheel());}},
    {[&car](){return "fr: " + wheel_display_string(car.get_front_right_wheel());}},
    {[&car](){return "bl: " + wheel_display_string(car.get_back_left_wheel());}},
    {[&car](){return "br: " + wheel_display_string(car.get_back_right_wheel());}},
    {[&car](){return "str: " + format(car.get_str());}},
    {[&car](){return "esc: " + format(car.get_esc());}}

  };

  ConsoleMenu menu(&car_menu);
  menu.run();
}

void test_car_menu() {
  run_car_menu();
}
*/
