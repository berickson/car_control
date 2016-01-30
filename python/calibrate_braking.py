#!/usr/bin/env python2.7
# coding: utf-8
from __future__ import print_function

from car import Car
import time
from route import reverse_route
from play_route import play_route
import Queue
import numpy as np


total_track_length = 6.
stop_track_length = 2. # required distance to slow at the end, todo: calculate based on max_speed
run_track_length = total_track_length - stop_track_length

max_speed = 2.
test_esc = 1400


car = Car()
time.sleep(0.5)
car.zero_odometer()
car.set_rc_mode();

goal_heading = car.heading_degrees()

# accelerate in straight line until you get to desired speed
print ('accelerating to  speed of '+str(max_speed))
v = 0.0 # assume we start at zero speed
aborted = False
while v < max_speed:
  if car.odometer_meters() > run_track_length:
    aborted = True
    abort_reason = "ran out of track"
    break;
  esc = car.esc_for_velocity(v+0.5)
  steer = car.steering_for_goal_heading_degrees(goal_heading)
  car.set_esc_and_str(esc,steer)
  time.sleep(0.02)
  v = car.get_velocity_meters_per_second()

print('done accelerating at {:4.1f} meters'.format(car.odometer_meters()))


print('testing braking at esc: {}'.format(test_esc))
queue = Queue.Queue()
car.add_listener(queue)

data = []
# set the esc to given braking speed
while True:
  # go straight and record data until one of three things happens
  esc = test_esc
  steer = car.steering_for_goal_heading_degrees(goal_heading)
  car.set_esc_and_str(esc,steer)
  message = queue.get(block=True, timeout = 0.05)
  data.append(message)
 
  #
  # 1. You come to a full stop
  v = car.get_velocity_meters_per_second()
  if v <= 0.:
    print('test concluded at velocity zero')
    break
  # 2. Your speed stabilizes
  # 3. You reach distance limit
  if car.odometer_meters()  > run_track_length:
    print('test length exceeded, allowing room to stop')
    break
  # 4. You detect that you are skidding
car.remove_listener(queue)

if v > 0:
  print('applying safety brake at {} meters and velocity {}'.format(car.odometer_meters(), v))
  # put on "safe level" of brakes until your speed reaches zero (or negative)
  while v > 0:
    esc = 1350
    steer = car.steering_for_goal_heading_degrees(goal_heading)
    car.set_esc_and_str(esc,steer)
    time.sleep(0.02)
    v = car.get_velocity_meters_per_second()
  
# set everything to neutral
print('final distance: {} meters'.format(car.odometer_meters()))
car.set_manual_mode()

# save the results of braking to two files
# car dynamics in one file
# test settings in another file

# do a quick analysis of data and print results
f = open('braking_results.csv', 'w')
print ('seconds,meters,us,esc,odometer_ticks,ax,spur_delta_us,spur_odo',file=f)
for p in data:
  p.seconds = (p.us-data[0].us)/1000000.
  p.meters = (p.odometer_ticks-data[0].odometer_ticks)*car.meters_per_odometer_tick
  fields = [p.seconds,p.meters,p.us,p.esc,p.odometer_ticks,p.ax,p.spur_delta_us,p.spur_odo]
  print(",".join([str(field) for field in fields]), file=f)
print('np polyfit 2 result',np.polyfit([p.seconds for p in data], [p.meters for p in data], 2))
print('np polyfit 3 result',np.polyfit([p.seconds for p in data], [p.meters for p in data], 3))


# distance to stop from speed, distance left in track
# whether skidding occurred
# estimate of acceleration using linear fit
# estimate of acceleration using polynomial fit (TBD: model for this, probably adding k*v^2 term for wind resistance)

car.reset_odometry()
time.sleep(0.01)
route = reverse_route(car.odometer_meters(), max_a=0.5, max_v=2.)
play_route(route, car = car)
car.set_manual_mode()

