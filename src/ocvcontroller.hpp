/*******************************************************************************
	Copyright (C) 2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#ifndef OCVCONTROLLER_H_
#define OCVCONTROLLER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <fstream>

#include "serialport.hpp"
#include "lo/lo.h"
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
//using namespace boost;
//using namespace boost::asio;
//using namespace boost::asio::ip;

#define MIDI_CLIENT_NAME "OMBridge"

// TODO set at calibration stage
// HSV limits 
const int H_MIN = 95;
const int H_MAX = 128;
const int S_MIN = 180;
const int S_MAX = 256;
const int V_MIN = 60;
const int V_MAX = 256;
//default capture width and height
const int FRAME_WIDTH = 640/2;
const int FRAME_HEIGHT = 480/2;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS=2;
//delay between frames, in ms
const int CV_DELAY_MS=1;
//Circular buffer CAPACITY
const int CB_CAPACITY=10;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;
//names that will appear at the top of each window
const string W_NAME_ORIG = "Original Image";
const string W_NAME_HSV = "HSV Image";
const string W_NAME_THRESHOLD = "Thresholded Image";
const string W_NAME_AFTERMORPHO = "After Morphological Operations";

const cmdmap::argument RECORD_ARG{'s', false, 0.0, 0.0, "record"};
const list<cmdmap::argument> RECORD_ARGLIST = {RECORD_ARG};
const list<unsigned int> RECORD_DATABYTESLIST = {0};

const cmdmap::command RECORD_CMD{"record", "osc", '1', "", "record", RECORD_ARGLIST, RECORD_DATABYTESLIST};

//exceptions for OCVController
class ExOCVController: public exception
{
protected:
 const char *expMess;
public:
 ExOCVController(const char *mess) throw() : exception() { expMess=mess;}
 virtual const char *what() const throw()
 {
   return expMess;
 };
};

//OCVController
class OCVController
{  
  
public:
  OCVController(const string port, const int  baudrate,  
               const string map, const bool nogui, const int guiport,  
               const string defoscserv, const int expressiondiv, const bool verb)  throw(ExOCVController);  
  void processInput(void);
  
  // TODO these are not public?

  void drawObject(int area, Point point, Mat &frame);

  void morphOps(Mat &thresh);
  
  bool trackFilteredObject(Mat & threshold, Mat &cameraFeed);
  
protected: 

 
private:
 
  bool verbose;
  bool noGUI;
  boost::asio::ip::udp::endpoint guiEndpoint;
  boost::asio::ip::udp::udp::socket * socketGUI;
  SerialPort * serial;
  CmdMap * cmap;
  cmdmap::bank * aBank;
  OSC * oscDev;
  MIDI * midiDev;
  
  // OpenCV realted objects
  Mat cameraFeed;
  //matrix storage for HSV image
  Mat HSV;
  //matrix storage for binary threshold image
  Mat threshold;
  //video capture object to acquire webcam feed
  VideoCapture capture;
  //x and y values for the location of the object
  int x=0, y=0;
  // Circular buffer for the tracked points
  boost::circular_buffer<Point> cb_tpoints; //(CB_CAPACITY);

};

#endif 
