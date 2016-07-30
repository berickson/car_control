#include "frame_grabber.h"
#include <map>

using namespace std;
using namespace cv;


void FrameGrabber::begin_grabbing(cv::VideoCapture * _cap)
{
  this->cap = _cap;
  this->grab_thread = std::thread(&FrameGrabber::grab_thread_proc, this);
  grabOn.store(true);
}

void FrameGrabber::end_grabbing() {
  if(grabOn.load()==true) {
    grabOn.store(false);    //stop the grab loop
    grab_thread.join();               //wait for the grab loop

    while (!buffer.empty())    //flushing the buffer
    {
      buffer.pop();
    }
  }
}

bool FrameGrabber::get_one_frame(Mat & frame)
{
  bool rv = false;
  mtxCam.lock();                //lock memory for exclusive access
  if (buffer.size() > 0)              //if some
  {
    //reference to buffer.front() should be valid after
    //pop because of Mat memory reference counting
    //but if content can change after unlock is better to keep a copy
    //an alternative is to unlock after processing (this will lock grabbing)
    buffer.front().copyTo(frame);   //get the oldest grabbed frame (queue=FIFO)
    buffer.pop();            //release the queue item
    rv = true;
  }
  mtxCam.unlock();            //unlock the memory
  return rv;
}

bool FrameGrabber::get_latest_frame(cv::Mat & frame) {
  mtxCam.lock();
  while(buffer.size()>1) {
    buffer.pop();
  }
  mtxCam.unlock();
  return get_one_frame(frame);
}

int FrameGrabber::get_frame_count_grabbed() {
  return frames_grabbed;
}


void FrameGrabber::grab_thread_proc()
{
  Mat tmp;

  //To know how many memory blocks will be allocated to store frames in the queue.
    //Even if you grab N frames and create N x Mat in the queue
    //only few real memory blocks will be allocated
    //thanks to std::queue and cv::Mat memory recycling
    std::map<unsigned char*, int> matMemoryCounter;
    uchar * frameMemoryAddr;

    while (grabOn.load() == true) //this is lock free
    {
        //grab will wait for cam FPS
        //keep grab out of lock so that
        //idle time can be used by other threads
        *cap >> tmp; //this will wait for cam FPS

        if (tmp.empty()) continue;

        //get lock only when we have a frame
        mtxCam.lock();
        ++frames_grabbed;
        //buffer.push(tmp) stores item by reference than avoid
        //this will create a new cv::Mat for each grab
        buffer.push(Mat(tmp.size(), tmp.type()));
        tmp.copyTo(buffer.back());
        frameMemoryAddr = buffer.front().data;
        mtxCam.unlock();
        //count how many times this memory block has been used
        matMemoryCounter[frameMemoryAddr]++;
    }
}
