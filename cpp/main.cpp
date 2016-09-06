#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <unistd.h> // for usleep, etc

#include "ackerman.h"
#include "usb.h"
#include "work_queue.h"
#include "dynamics.h"
#include "config.h"
#include "car.h"
#include "fake_car.h"

#include "split.h"
#include "menu.h"
#include "system.h"
#include "car_menu.h"
#include "console_menu.h"
#include "geometry.h"
#include "pi_buttons.h"
#include "route.h"
#include "lookup_table.h"
#include "driver.h"
#include "camera.h"
#include "tracker.h"

using namespace std;




int main(int, char *[]) {
  try {
    run_car_menu();

    //test_ackerman();
    //test_camera();
    //test_car();
    //test_config();
    //test_console_menu();
    //test_driver();
    //test_dynamics();
    //test_fake_car();
    //test_geometry();
    //test_lookup_table();
    //test_menu();
    //test_pi_buttons();
    //test_route();
    //test_split();
    //test_stereo_camera();
    //test_system();
    //test_tracker();
    //test_usb();
    //test_work_queue();

  } catch (string & s) {
    cout << "caught: " << s << endl;
  }
}
