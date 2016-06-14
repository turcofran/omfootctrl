/*******************************************************************************
	Copyright (C) 2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#ifndef OCV_H_
#define OCV_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <fstream>

#include "ominputdev.hpp"

#include <csignal>
#include "cmdmap.hpp"
#include "osc.hpp"
#include "midi.hpp"
#include <boost/foreach.hpp>
#include <boost/circular_buffer.hpp>

#include <opencv/highgui.h>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include <opencv/cv.h>

using namespace std;
using namespace cv;

struct hsv_rage_t {
  Scalar lowerb;
  Scalar upperb;
};
// TODO calibration stage
// HSV limits 
// Paper
const int DEF_H_MIN = 95;
const int DEF_S_MIN = 180;
const int DEF_V_MIN = 0;
const int DEF_H_MAX = 128;
const int DEF_S_MAX = 256;
const int DEF_V_MAX = 256;
// Card
//~ const int H_MIN = 85;
//~ const int H_MAX = 128;
//~ const int S_MIN = 109;
//~ const int S_MAX = 256;
//~ const int V_MIN = 166;
//~ const int V_MAX = 256;
// Window Layout
//default capture width and height
const int FRAME_WIDTH = 640/2;
const int FRAME_HEIGHT = 480/2;
// expression limits
const int EXP_VER_T = 50;
const int EXP_VER_B = 210;
const int EXP_VER_RANGE = EXP_VER_B-EXP_VER_T;
const int EXP_HORI_L = 260;
const int EXP_HORI_R = 320;
// Buttons limits
const int BUTT_VER_B = 160; // bottom
const int BUTT_VER_T = 80; // top 
const int B1_HORI_L = 0;
const int B1_HORI_R = 65;
const int B2_HORI_L = 65;
const int B2_HORI_R = 130;
const int B3_HORI_L = 130;
const int B3_HORI_R = 195;
const int B4_HORI_L = 195;
const int B4_HORI_R = 260;
const int B5_HORI_L = 0;
const int B5_HORI_R = 65;
const int B6_HORI_L = 195;
const int B6_HORI_R = 260;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS=1;
//delay between frames, in ms
const int DEF_FRAME_INT_US=33333;
const int DEBOUNCE_TIME_MS=500;
const int CV_DELAY_MS=1;
//Circular buffer CAPACITY
const int CB_CAPACITY=10;
const int ERODE_RECT_PIXEL=4;
const int DILATE_RECT_PIXEL=12;
const int ERODE_DILATE_ITS=2;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;
//names that will appear at the top of each window
const string W_NAME_FEED = "OM OpenCV - Feed";
const string W_NAME_THRESHOLD = "Thresholded Image";
const string W_NAME_HSV = "OM OpenCV - HSV";
const string LAYOUT_PNG_NAME = "img/layout6x.png";
const string PAUSED_PNG_NAME = "img/paused.png";

enum class TrackStt {NO_TRACK, UNARMED, ARMED, DEBOUNCING, TRIGGER, EXPRESSION};

//exceptions for OCV
class ExOCV: public exception
{
protected:
 const char *expMess;
public:
 ExOCV(const char *mess) throw() : exception() { expMess=mess;}
 virtual const char *what() const throw()
 {
   return expMess;
 };
};

//OCV
class OCV: public OMInputDev
{  
  
public:
  OCV(const int incamdev, const string hsvRangeFile, const int expressiondiv, const bool verb)  throw(ExOCV);  
  string readBLine(void);
  static int get_png(Mat & inmat, string filename);
  
protected: 

  void drawObject(int area, Point point, Mat &frame);
  void erodeAndDilate(Mat &frame);
  string trackAndEval(Mat & threshold, Mat &canvas);

  int readHSVFilterConf(const string hsvFilterConfFile);

  void drawCmdAreas(Mat &frame);
  int disable_exposure_auto_priority(const int dev);
  int disable_exposure_auto_priority(const string dev);
  int read_frame_interval_us(const int dev);
  int read_frame_interval_us(VideoCapture cap);

private:
 
  bool verbose;
  int expressionDiv, lastExLevel;
  
  // OpenCV realted objects
  VideoCapture videoCap;
  VideoWriter  videoOut;
  int debouceFrames, debounceCounter;
  int frameIntervalUS;
  TrackStt trackState;
  hsv_rage_t hsvRange;
  bool paused;

  //~ static void on_slowerb_trackbar( int, void* );
  //~ static int sLowerbTrackebar;
  // Canvas matrix
  //~ Mat canvas;
  // Structuring elements for morphological operations
  Mat layout6x, layoutPaused;
  Mat morphERODE, morphDILATE;
  //x and y values for the location of the object
  //~ int x=0, y=0;
  // Circular buffer for the tracked points
  //~ boost::circular_buffer<Point> cb_tpoints; //(CB_CAPACITY);

};

#endif 
