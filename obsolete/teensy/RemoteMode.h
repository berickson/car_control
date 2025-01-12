#pragma once

#include "Task.h"

class RemoteMode : public Task {
public:
  virtual void begin();
  virtual void end();
  virtual void execute();
  
  void command_steer_and_esc(unsigned int steer_us, unsigned int esc_us);
private:
  void update_pulses();
  unsigned long last_command_ms = 0;
  unsigned int steer_us = 1500;
  unsigned int esc_us = 1500;
};
