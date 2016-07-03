#ifndef RUN_SETTINGS_H
#define RUN_SETTINGS_H

#include <string>

using namespace std;

struct RunSettings{
  RunSettings();

  string track_name = "";
  string route_name;// = FileNames().get_route_names(track_name)[0];
  double max_a = 0.25;
  double max_v = 1.0;
  double t_ahead = 0.3;
  double d_ahead = 0.05;
  double k_smooth = 0.4;
  bool capture_video = false;
  void write_to_file(string path);
};


#endif // RUN_SETTINGS_H