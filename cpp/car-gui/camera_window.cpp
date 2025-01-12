#include "camera-window.h"
#include "ui-camera-window.h"
#include "route-window.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>
#include "opencv2/videoio.hpp"
#include "opencv2/calib3d.hpp"
#include <QMessageBox>
#include <QDateTime>
#include <QDir>

#include <string>
#include <sstream>
#include "../json.hpp"

using namespace std;


/*
v4l2-ctl --device=/dev/video1 --list-formats-ext
ioctl: VIDIOC_ENUM_FMT
  Index       : 0
  Type        : Video Capture
  Pixel Format: 'MJPG' (compressed)
  Name        : Motion-JPEG
    Size: Discrete 1920x1080
      Interval: Discrete 0.033s (30.000 fps)
    Size: Discrete 1280x720
      Interval: Discrete 0.017s (60.000 fps)
    Size: Discrete 1024x768
      Interval: Discrete 0.033s (30.000 fps)
    Size: Discrete 640x480
      Interval: Discrete 0.008s (120.101 fps)
    Size: Discrete 800x600
      Interval: Discrete 0.017s (60.000 fps)
    Size: Discrete 1280x1024
      Interval: Discrete 0.033s (30.000 fps)
    Size: Discrete 320x240
      Interval: Discrete 0.008s (120.101 fps)

  Index       : 1
  Type        : Video Capture
  Pixel Format: 'YUYV'
  Name        : YUYV 4:2:2
    Size: Discrete 1920x1080
      Interval: Discrete 0.167s (6.000 fps)
    Size: Discrete 1280x720
      Interval: Discrete 0.111s (9.000 fps)
    Size: Discrete 1024x768
      Interval: Discrete 0.167s (6.000 fps)
    Size: Discrete 640x480
      Interval: Discrete 0.033s (30.000 fps)
    Size: Discrete 800x600
      Interval: Discrete 0.050s (20.000 fps)
    Size: Discrete 1280x1024
      Interval: Discrete 0.167s (6.000 fps)
    Size: Discrete 320x240
      Interval: Discrete 0.033s (30.000 fps)
*/

void CameraWindow::set_camera()
{
  frame_grabber.end_grabbing();
  frame_grabber_2.end_grabbing();

  if(cap_1.isOpened()) {
    cap_1.release();
  }

  if(cap_2.isOpened()) {
    cap_2.release();
  }

  string device_name = ui->video_device->currentText().toStdString();
  if(QFile(QString::fromStdString(device_name)).exists()) {
    if(!cap_1.open(device_name)){
      throw (string) "couldn't open camera";
    }
    frame_grabber.begin_grabbing(&cap_1, "");
  }


  string device_name_2 = ui->video_device_2->currentText().toStdString();
  if(QFile(QString::fromStdString(device_name_2)).exists()) {
    if(!cap_2.open(device_name_2)){
      throw (string) "couldn't open camera";
    }
    frame_grabber_2.begin_grabbing(&cap_2, "");
  }
}

CameraWindow::CameraWindow(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::CameraWindow)
{
  ui->setupUi(this);

  for(auto res : supported_resolutions) {

    stringstream ss;
    ss << res.width() << "_" << res.height();
    ui->resolutions_combo_box->addItem(QString::fromStdString(ss.str()));
  }
  ui->resolutions_combo_box->setCurrentIndex(1);


  set_camera();

  ui->brightness_slider->setValue(cap_1.get(cv::CAP_PROP_BRIGHTNESS)*100);
  ui->contrast_slider->setValue(cap_1.get(cv::CAP_PROP_CONTRAST)*100);
  ui->saturation_slider->setValue(cap_1.get(cv::CAP_PROP_SATURATION)*100);


  timer.setSingleShot(false);
  timer.setInterval(100);
  connect(&timer, SIGNAL(timeout()), this, SLOT(process_one_frame()));
  connect(ui->frames_per_second_slider, SIGNAL(valueChanged(int)), this, SLOT(fps_changed(int)));
  timer.start();

}

CameraWindow::~CameraWindow()
{
  frame_grabber.end_grabbing();
  frame_grabber_2.end_grabbing();
  if(cap_1.isOpened())
    cap_1.release();
  if(cap_2.isOpened())
    cap_2.release();
  delete ui;
}

void CameraWindow::on_actionExit_triggered()
{
  this->close();
}


void load_calibration(string camera_name, cv::Mat & camera_matrix, cv::Mat & dist_coefs) {
  stringstream ss;
  ss << QDir::homePath().toStdString() + "/car/camera_calibrations.json";
  string path = ss.str();

  std::ifstream json_file(path);
  nlohmann::json calibration_json = nlohmann::json::parse(json_file);

  auto j = calibration_json.find(camera_name.c_str());
  if(j==calibration_json.end())  {
    throw string("could not find camera in json calibration");
  }
  auto m=j->at("camera_matrix");
  camera_matrix = (cv::Mat1d(3, 3) <<  m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);

  auto d = j->at("dist_coefs");
  dist_coefs = (cv::Mat1d(1, 5) << d[0][0], d[0][1], d[0][2], d[0][3], d[0][4]);
}

void CameraWindow::process_one_frame()
{
  ui->cam_frame_count_text->setText(QString::number(frame_grabber.get_frame_count_grabbed()));
  if(!cap_1.isOpened()){
     return;
  }
  if(!frame_grabber.get_latest_frame(original_frame)) {
    return;
  }
  cv::flip(original_frame,original_frame,-1);
  original_frame.copyTo(frame);
  if(cap_2.isOpened()) {
    if(!frame_grabber_2.get_latest_frame(original_frame_2)) {
      return;
    }
    cv::flip(original_frame_2,original_frame_2,-1);
    original_frame_2.copyTo(frame_2);
  }

  if(ui->undistort_checkbox->isChecked()) {
    try {
      if(cap_1.isOpened()) {
        string camera_name =
            ui->camera_name->currentText().toStdString()
            + "_"
            + ui->resolutions_combo_box->currentText().toStdString();

        cv::Mat camera_matrix, dist_coefs;
        load_calibration(camera_name, camera_matrix, dist_coefs);

        cv::Mat undistorted;
        cv::undistort(frame, undistorted, camera_matrix, dist_coefs);
        undistorted.copyTo(frame);
      }

      if(cap_2.isOpened()) {
        string camera_name =
            ui->camera_name_2->currentText().toStdString()
            + "_"
            + ui->resolutions_combo_box->currentText().toStdString();

        cv::Mat camera_matrix, dist_coefs;
        load_calibration(camera_name, camera_matrix, dist_coefs);

        cv::Mat undistorted;
        cv::undistort(frame_2, undistorted, camera_matrix, dist_coefs);
        undistorted.copyTo(frame_2);
      }

    } catch (...) {
      ;
    }
  }

  cv::Size chessboard_size(6,9);

  if(cap_1.isOpened()) {
    vector<cv::Point2f> corners;
    bool found = cv::findChessboardCorners(frame, chessboard_size, corners);
    cv::drawChessboardCorners(frame, chessboard_size, corners, found);
  }

  if(cap_2.isOpened()) {
    vector<cv::Point2f> corners_2;
    bool found_2 = cv::findChessboardCorners(frame_2, chessboard_size, corners_2);
    cv::drawChessboardCorners(frame_2, chessboard_size, corners_2, found_2);
  }


  if(ui->find_correspondences_checkbox->isChecked()) {
    tracker.process_image(frame);
  }
  ++frame_count;

  ui->frame_count_text->setText(QString::number(frame_count));


  std::stringstream ss;
  ss << tracker.homography;
  ui->homography_text->setText(QString::fromStdString(ss.str()));
  //cv::putText(frame, ss.str(), cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 1, red, 3);

  if(ui->show_image_checkbox->checkState()==Qt::CheckState::Checked) {

    cv::cvtColor(frame,frame,cv::COLOR_BGR2RGB);
    QImage imdisplay((uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);

    ui->display_image->setPixmap(QPixmap::fromImage(imdisplay));
    //ui->display_image->setFixedSize(imdisplay.width(),imdisplay.height());

    if(cap_2.isOpened()) {
      cv::cvtColor(frame_2, frame_2, cv::COLOR_BGR2RGB);
      QImage imdisplay_2((uchar*)frame_2.data, frame_2.cols, frame_2.rows, frame_2.step, QImage::Format_RGB888);
      //ui->display_image_2->setFixedSize( imdisplay_2.size());
      ui->display_image_2->setPixmap(QPixmap::fromImage(imdisplay_2));
      //ui->display_image_2->setGeometry(imdisplay.width(),0,imdisplay_2.width(),imdisplay_2.height());
    }


    //ui->image_container->setGeometry(QRect(0,0,frame.cols*2, frame.rows));
  }
}



void CameraWindow::fps_changed(int fps)
{
  timer.setInterval((1./fps)*1000);

}
void CameraWindow::on_webcamButton_clicked()
{

  std::vector<std::string> prop_names =
        { "CAP_PROP_POS_MSEC",      //0
          "CAP_PROP_POS_FRAMES",    //1
          "CAP_PROP_POS_AVI_RATIO", //2 };
          "CAP_PROP_FRAME_WIDTH",   //3
          "CAP_PROP_FRAME_HEIGHT",  //4
          "CAP_PROP_FPS",           //5
          "CAP_PROP_FOURCC",        //6
          "CAP_PROP_FRAME_COUNT",   //7
          "CAP_PROP_FORMAT",        //8
          "CAP_PROP_MODE",          //9
          "CAP_PROP_BRIGHTNESS",   //10
          "CAP_PROP_CONTRAST",     //11
          "CAP_PROP_SATURATION",   //12
          "CAP_PROP_HUE",          //13
          "CAP_PROP_GAIN",         //14
          "CAP_PROP_EXPOSURE",     //15
          "CAP_PROP_CONVERT_RGB",  //16
          "CAP_PROP_WHITE_BALANCE_BLUE_U",//17
          "CAP_PROP_RECTIFICATION",//18
          "CAP_PROP_MONOCHROME",   //19
          "CAP_PROP_SHARPNESS",    //20
          "CAP_PROP_AUTO_EXPOSURE", //21 DC1394: exposure control done by camera, user can adjust refernce level using this feature
          "CAP_PROP_GAMMA",        //22
          "CAP_PROP_TEMPERATURE",  //23
          "CAP_PROP_TRIGGER",      //24
          "CAP_PROP_TRIGGER_DELAY",//25
          "CAP_PROP_WHITE_BALANCE_RED_V",//26
          "CAP_PROP_ZOOM",         //27
          "CAP_PROP_FOCUS",        //28
          "CAP_PROP_GUID",         //29
          "CAP_PROP_ISO_SPEED",    //30
          "CAP_PROP_BACKLIGHT",    //32
          "CAP_PROP_PAN",          //33
          "CAP_PROP_TILT",         //34
          "CAP_PROP_ROLL",         //35
          "CAP_PROP_IRIS",         //36
          "CAP_PROP_SETTINGS",     //37
          "CAP_PROP_BUFFERSIZE",   //38
          "CAP_PROP_AUTOFOCUS"    //39
        };

  stringstream ss;
  for(int i=0;i<=39;i++) {
    if(i==cv::CAP_PROP_BUFFERSIZE) break; // reading this cause bug?

    ss << "CAP_PROP " << i << ":";
    ss << prop_names[i] << " ";
    try{
      ss << cap_1.get(i);

    } catch (...) {
      ss << "ERROR";
    }
    ss << endl;
  }
  QMessageBox dialog(this);
  dialog.setText(QString::fromStdString(ss.str()));
  dialog.exec();
}

void CameraWindow::on_brightness_slider_valueChanged(int value)
{
    cap_1.set(cv::CAP_PROP_BRIGHTNESS,value/100.);
}

void CameraWindow::on_contrast_slider_valueChanged(int value)
{
    cap_1.set(cv::CAP_PROP_CONTRAST,value/100.);
}

void CameraWindow::on_hue_slider_valueChanged(int value)
{
  cap_1.set(cv::CAP_PROP_HUE,value/100.);

}

void CameraWindow::on_saturation_slider_valueChanged(int value)
{
  cap_1.set(cv::CAP_PROP_SATURATION,value/100.);
}


void CameraWindow::on_resolutions_combo_box_currentIndexChanged(int )
{
    QSize resolution = supported_resolutions[ui->resolutions_combo_box->currentIndex()];
    cap_1.set(cv::CAP_PROP_FRAME_WIDTH, resolution.width());
    cap_1.set(cv::CAP_PROP_FRAME_HEIGHT, resolution.height());
    cap_2.set(cv::CAP_PROP_FRAME_WIDTH, resolution.width());
    cap_2.set(cv::CAP_PROP_FRAME_HEIGHT, resolution.height());
    ui->scroll_area_contents->setFixedSize(resolution.width()*2, resolution.height());
    ui->display_image->setFixedSize(resolution.width(), resolution.height());
    ui->display_image->setFixedSize(resolution.width(), resolution.height());
}

void CameraWindow::on_video_device_currentTextChanged(const QString &)
{
  set_camera();
}

void CameraWindow::on_take_picture_button_clicked()
{
  string time_string = QDateTime::currentDateTime().toString("yyyy-MM-ddThh.mm.ss.zzz").toStdString();
  string path = QDir::homePath().toStdString();

  if(cap_1.isOpened() && cap_2.isOpened()) {
    {
      stringstream ss;
      ss << path << "/car/data/cap_"
           << time_string
           << "_left.png";
      string path = ss.str();
      cv::imwrite(path, original_frame);
    }

    {
    stringstream ss;
    ss << path << "/car/data/cap_"
         << time_string
         << "_right.png";
    string path = ss.str();
    cv::imwrite(path, original_frame_2);
    }

    return;
  }

  if(cap_1.isOpened()) {
    stringstream ss;
    ss << QDir::homePath().toStdString() + "/car/data/capture_"
         << time_string
         << ".png";
    string path = ss.str();

    cv::imwrite(path, original_frame);
  }
  if(cap_2.isOpened()) {
    stringstream ss;
    ss << QDir::homePath().toStdString() + "/car/data/capture2_"
         << time_string
         << ".png";
    string path = ss.str();

    cv::imwrite(path, original_frame_2);
  }
}

void CameraWindow::on_video_device_2_currentTextChanged(const QString &)
{
  set_camera();
}
