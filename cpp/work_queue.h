#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <queue>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include "system.h"

using namespace std;
using namespace std::chrono;

struct StampedString {
  StampedString(string s, system_clock::time_point timestamp) : message(s), timestamp(timestamp) {
  }
  StampedString(){}
  
  string to_string() const{
    return (string) time_string(timestamp)+","+message;
  }

  static StampedString from_string(string s) {
    StampedString rv;
    size_t i = s.find(",");
    string time_string = s.substr(0,i-1);
    rv.timestamp = time_from_string(time_string);
    rv.message = s.substr(i);

  }

  system_clock::time_point timestamp;
  string message;
};

template <class T>
class WorkQueue {
  std::mutex q_mutex;
  std::queue<T> q;
  std::condition_variable cv;

public:
  void push(T& s) {
    {
      std::lock_guard<std::mutex> lock(q_mutex);
      q.emplace(s);
    }

    cv.notify_one();
  }

  bool try_pop(T& s, int milliseconds) {
    std::unique_lock<std::mutex> lock(q_mutex);
    if(q.empty()) {
      chrono::milliseconds timeout(milliseconds);
      if (!cv.wait_for(lock, timeout,[this](){return !q.empty(); }))
        return false;
    }
    s = q.front();
    q.pop();

    return true;
  }
};


void test_work_queue();


#endif // WORK_QUEUE_H
