#!/usr/bin/env python2.7

import io
import serial
import sys
import datetime
import time
import glob
import os
from select import select

sleep_time = 0.01

def log(l):
  try:
    log_file = open('/var/log/car/output.log','a')
    ts = datetime.datetime.now()
    log_file.write("{0},{1}\n".format(ts,l.strip()))
    log_file.flush()
  except IOError as e:
    print 'Error writing logs: {0}'.format(str(e))
  

class fast_line_reader:
  def __init__(self, path):
    self.leftover =  ''
    self.f = io.open(path,buffering = 1)
    
  def get_lines(self):
    lines = self.f.readlines()
    if len(lines)>0:
    
      if len(self.leftover) > 0:
        lines[0] = self.leftover+lines[0]
        self.leftover = ''
       
      if not lines[-1].endswith('\n'):
        self.leftover = lines[-1]
        del lines[-1]
    for line in lines:
      yield line
  


def get_commands(command_file):
  while True:
    buffer = os.read(command_file,1000).strip()
    if(buffer != ''):
      for l in buffer.split('\n'):
        log('COMMAND,{0}'.format(l))
        yield(l)
    else:
      return
      
def get_output(s):
  while(s.inWaiting() > 0):
    yield s.readline()

      
def run(command_file):
  connected = False
  while True:
    try:
      for usb_path in glob.glob('/dev/ttyACM*'):
        try:
          s = serial.Serial(usb_path)
          f = fast_line_reader(usb_path)
          log('serial connected')
          connected = True
          while True:
            did_work = False
            for c in get_commands(command_file):
              s.write('{0}\n'.format(c))
              did_work = True
            for o in f.get_lines():
              log(o)
              did_work = True
            if did_work == False:
              time.sleep(sleep_time)
        except IOError:
          time.sleep(sleep_time)
          if (connected):
            log('serial disconnected')
            connected = False    
    except OSError:
      if (connected):
        log('serial disconnected')
        connected = False
      
command_file = os.open('/dev/car/command',os.O_RDONLY | os.O_NONBLOCK)

run(command_file)

