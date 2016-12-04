/*******************************************************************************

  Copyright (C) 2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#include "ocv.hpp"
#include "ocv_config.hpp"

#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h> 
#include <chrono>

int sLowerbTrackebar = DEF_S_MIN; // TODO this is not coherent!

OCV::OCV(const int incamdev, const string hsvFilterConfFile, const int expressiondiv, const bool verb)  throw(ExOCV)
{
  expressionDiv = expressiondiv;
  lastExLevel=0;
  verbose = verb;
  paused = false;
  
  if (readHSVFilterConf(hsvFilterConfFile)!=0) {
    cerr << "WARNING! Not possible to read configuration file with HSV range. Default bounds will be used." << endl;
    hsvRange.lowerb = Scalar(DEF_H_MIN, DEF_S_MIN, DEF_V_MIN);
    hsvRange.upperb = Scalar(DEF_H_MAX, DEF_S_MAX, DEF_V_MAX);
  }
  
  try
  {
    // Initialize structuring elements for morphological operations
    morphERODE  = getStructuringElement( MORPH_RECT, Size(ERODE_RECT_PIXEL,ERODE_RECT_PIXEL));
    morphDILATE = getStructuringElement( MORPH_RECT, Size(DILATE_RECT_PIXEL,DILATE_RECT_PIXEL));
    trackState = TrackStt::NO_TRACK;

    #ifdef VIDEO_IN
      string camdev = "sample/footsample.avi";
      //~ string camdev = "sample.avi";
    #else
      int camdev = incamdev; 
    #endif
    
    // auto exposure control
    if (disable_exposure_auto_priority(camdev) != 0)
      cerr << "WARNING! Not possible to disable auto priority. The delay in the camera reading may be too much!" << endl;
      //~ throw(ExOCV("Not possible to disable auto priority")); 
        
    // open video to write
    #ifdef VIDEO_OUT
      time_t now;
      time(&now);
      string voname = "vsample"+to_string(now)+".avi";
      videoOut.open(voname, 0, 30.0, Size(FRAME_WIDTH, FRAME_HEIGHT));
      if (!videoOut.isOpened())
        throw(ExOCV("Not possible to open write video")); 
    #endif
    
    // get image for layout and paused state
    if (get_png(layout6x, LAYOUT_PNG_NAME) || get_png(layoutPaused, PAUSED_PNG_NAME))
      throw(ExOCV("Not possible to read controller layout")); 
    
    //open capture object at location zero (default location for webcam)
    videoCap.open(camdev);
    if (!videoCap.isOpened())
      throw(ExOCV("Not possible to open input device or video")); 
    // TODO implement reading the fps by using v4l2 lib
    int frameIntervalUS = read_frame_interval_us(videoCap);
    if (frameIntervalUS == -1) {
      cerr << "WARNING!  Not possible to read the frame interval from device " << camdev << 
              ". Default value will be used" << endl;
      frameIntervalUS = DEF_FRAME_INT_US;
    }
    cerr << "Frame interval us: " << frameIntervalUS << endl;
    
    debouceFrames = (int)((double)DEBOUNCE_TIME_MS*1000/(double)frameIntervalUS);
    debounceCounter = 0;

    //set height and width of capture frame
    videoCap.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
    videoCap.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
    
    namedWindow(W_NAME_FEED); moveWindow(W_NAME_FEED, 10, 10);
    // TODO Implement something more oop
    createTrackbar( "+ light -", W_NAME_FEED, &sLowerbTrackebar, 256, NULL);
    //~ cerr << getBuildInformation() << endl;
    #ifdef SHOW_WIN
      namedWindow(W_NAME_HSV); moveWindow(W_NAME_HSV, 800, 10);
      namedWindow(W_NAME_THRESHOLD); moveWindow(W_NAME_THRESHOLD, 400, 10);
    #endif
    }
  catch (const exception &e)
  {
    throw(ExOCV(e.what())); 
  }
}

void OCV::printCmdNames(const cmdmap::bank * currentBank) {
  layout6x.copyTo(layout6xcmds);
  putText(layout6xcmds, currentBank->cmmds_v[0].shortname , Point(B1_HORI_L+15, BUTT_VER_B+40 ), 1, 1, Scalar(0,0,0), 2);
  putText(layout6xcmds, currentBank->cmmds_v[1].shortname , Point(B2_HORI_L+15, BUTT_VER_B+40 ), 1, 1, Scalar(0,0,0), 2);
  putText(layout6xcmds, currentBank->cmmds_v[2].shortname , Point(B3_HORI_L+15, BUTT_VER_B+40 ), 1, 1, Scalar(0,0,0), 2);
  putText(layout6xcmds, currentBank->cmmds_v[3].shortname , Point(B4_HORI_L+15, BUTT_VER_B+40 ), 1, 1, Scalar(0,0,0), 2);
  putText(layout6xcmds, currentBank->cmmds_v[4].shortname , Point(B5_HORI_L+15, BUTT_VER_T-20 ), 1, 1, Scalar(0,0,0), 2);
  putText(layout6xcmds, currentBank->cmmds_v[5].shortname , Point(B6_HORI_L+15, BUTT_VER_T-20 ), 1, 1, Scalar(0,0,0), 2);
  putText(layout6xcmds, currentBank->cmmds_v[6].shortname , Point(EXP_HORI_L+15, FRAME_HEIGHT/2-20 ), 1, 1, Scalar(0,0,0), 2);
}

// Process input
string OCV::readBLine(void)
{
  auto start = chrono::steady_clock::now();
  string retCmd = "";
  char keyPressed;
  Mat camFeed, procHSV, procThreshold;
  //~ Mat canvas = Mat::zeros(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
  // Get commnds map and it
  if (!videoCap.read(camFeed)) {
    cerr << "VideoCapture is not reading" << endl;
    #ifdef VIDEO_IN
      exit(0);
    #endif
    return retCmd;
  }
  auto tic1 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
  //flip image
  #ifndef VIDEO_IN
    flip(camFeed, camFeed, 1);
  #endif
  // write out
  #ifdef VIDEO_OUT
    videoOut.write(camFeed);
  #endif
  cvtColor(camFeed, procHSV, COLOR_BGR2HSV);
  //~ inRange(procHSV, hsvRange.lowerb, hsvRange.upperb, procThreshold);
  inRange(procHSV, Scalar(hsvRange.lowerb[0], sLowerbTrackebar, hsvRange.lowerb[2]), hsvRange.upperb, procThreshold);
  erodeAndDilate(procThreshold);
  auto tic2 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
  if (!paused)
    retCmd = trackAndEval(procThreshold, camFeed);
  else 
    addWeighted(camFeed, 0.5, layoutPaused, 1, 0.0, camFeed);

  //addWeighted(camFeed, 0.8, layout6x, 0.7, 0.0, camFeed);  // TODO it's nut add the frames each time!
  addWeighted(camFeed, 0.8, layout6xcmds, 0.7, 0.0, camFeed);

  
  imshow(W_NAME_FEED, camFeed);

  //delay so that screen can refresh.
  #ifdef SHOW_WIN // Overwrite this macro with $ cmake -DSHOW_WIN=0 . 
    imshow(W_NAME_HSV, procHSV);
    imshow(W_NAME_THRESHOLD, procThreshold);
  #endif
  //image will not appear without this waitKey() command
  #ifdef VIDEO_IN
    keyPressed = waitKey(DEF_FRAME_INT_US/1000);
  #else  
    keyPressed = waitKey(CV_DELAY_MS);
  #endif
  //~ cout << "keyPressed: " << to_string(keyPressed) << endl;
  switch(keyPressed) {
    case 27: exit(0); break;
    case 32: paused = !paused; break;
    default: break;
  }
  auto tic3 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
  #ifdef DEBUG_TICS
    cout << "processInput: " << tic1.count() << " - " << tic2.count()<< " - " << tic3.count()<< endl;
  #endif
  return retCmd;
}

void OCV::erodeAndDilate(Mat &frame){
  erode(frame, frame, morphERODE, Point(-1,-1), ERODE_DILATE_ITS);
  dilate(frame, frame, morphDILATE, Point(-1,-1), ERODE_DILATE_ITS);
  //TODO maybe a blur filter is faster than this... medianBLur?
}

string OCV::trackAndEval(Mat &threshold, Mat &canvas){
  //~ Mat temp;
  //~ threshold.copyTo(temp);
  //these two vectors needed for output of findContours
  vector< vector<Point> > contours;
  vector<Vec4i> hierarchy;
  //find contours of filtered image using openCV findContours function
  findContours(threshold, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
  //use moments method to find our filtered object
  string retValue = "";
  double area;
  int numObjects = hierarchy.size();
  if (debounceCounter) debounceCounter--;
  if (numObjects > 0) {
    //if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
    if (numObjects==1){
      Moments moment = moments((Mat)contours[0]);
      area = moment.m00;
      Point lastPoint(
        moment.m10/area, // x
        moment.m01/area  // y
      );
      drawObject(area, lastPoint, canvas);
      // Evaluate in which position of the grid the point is
      // state machine
      // TODO CHECk bounding rectangles and contour to evaluate it. Use the layout form PNG image!
      // expression limits
      switch (trackState) {
        case TrackStt::NO_TRACK:
          cout << "TrackStt::NO_TRACK" << endl;
        case TrackStt::UNARMED:
          if (lastPoint.x > EXP_HORI_L) {
            trackState = TrackStt::EXPRESSION;
            //~ cout << "Next state TrackStt::EXPRESSION" << endl; 
          }
          else if (lastPoint.y > BUTT_VER_T && lastPoint.y < BUTT_VER_B) {
            trackState = TrackStt::ARMED;
            cout << "Next state TrackStt::ARMED" << endl;
          }
          else {
            trackState = TrackStt::UNARMED;
          }
          break;
        case TrackStt::ARMED:
          if (lastPoint.x > EXP_HORI_L) {
            trackState = TrackStt::EXPRESSION;
          }
          else if (lastPoint.y > BUTT_VER_B) {
            trackState = TrackStt::DEBOUNCING;
            debounceCounter = debouceFrames;
            if (lastPoint.x < B1_HORI_R) {
              cout << "1" << endl; 
              retValue = "1";
            }
            else if (lastPoint.x < B2_HORI_R) {
              cout << "2" << endl; 
              retValue = "2";
            }
            else if (lastPoint.x < B3_HORI_R) {
              cout << "3" << endl; 
              retValue = "3";
            }
            else if (lastPoint.x < B4_HORI_R) {
              cout << "4" << endl; 
              retValue = "4";
            }
          }
          else if (lastPoint.y < BUTT_VER_T) {
            trackState = TrackStt::DEBOUNCING;
            debounceCounter = debouceFrames;
            if (lastPoint.x < B5_HORI_R) {
              cout << "5" << endl; 
              retValue = "5";
            }
            else if (lastPoint.x > B6_HORI_L && lastPoint.x < B6_HORI_R) {
              cout << "6" << endl; 
              retValue = "6";
            }
          }
          break;
        case TrackStt::DEBOUNCING: 
          //~ cout << "DEBOUNCING" << endl; 
          if (debounceCounter==0) 
            trackState = TrackStt::UNARMED;
          break;
        case TrackStt::EXPRESSION: 
          if (lastPoint.x < EXP_HORI_L) {
              trackState = TrackStt::UNARMED;
          }
          else{ 
            // TODO make a previous level comparition
            int expLevel;
            if (lastPoint.y > EXP_VER_B) 
              expLevel = 0;
            else if (lastPoint.y < EXP_VER_T)
              expLevel = expressionDiv-1;
            else {
              float ylevel = (float)(lastPoint.y-EXP_VER_T)/(float)(EXP_VER_RANGE);
              expLevel = (int)((float)(expressionDiv-1)*(1.0 - ylevel));
            }
            if (expLevel!=lastExLevel) {
              cout << "Expression level:" << expLevel << endl; 
              retValue = "X"+to_string(expLevel);
              lastExLevel = expLevel;
            }
          }
          break;
        default: 
          break;
      } 
      return retValue;
    }
    else {
      if (trackState!=TrackStt::DEBOUNCING) trackState = TrackStt::NO_TRACK;
      //void putText(Mat& img, const string& text, Point org, int fontFace, double fontScale, Scalar color, int thickness=1, int lineType=8, bool bottomLeftOrigin=false )
      putText(canvas, "More than one object detected!", Point(2, FRAME_HEIGHT-10), 1, 0.7, Scalar(0,0,255), 1);
      cout << "More than one object detected! Next state is TrackStt::NO_TRACK" << endl; 
    }
  }
  if (trackState!=TrackStt::DEBOUNCING) trackState = TrackStt::NO_TRACK;
  return retValue;
}


// TODO set the color in function to the state
void OCV::drawObject(int area, Point point, Mat &frame){
  //use some of the openCV drawing functions to draw crosshairs on your tracked image!
  circle(frame,point,4,Scalar(255,255,0),-1);
  
  line(frame, Point(point.x,point.y-10), Point(point.x,point.y+10), Scalar(255,255,0),0.5);
  line(frame, Point(point.x-10,point.y), Point(point.x+10,point.y), Scalar(255,255,0),0.5);
   
  putText(frame, to_string(point.x) + ","+ to_string(point.y), Point(point.x, point.y+30), 1, 1, Scalar(255,255,0),2);  
  putText(frame, to_string(area), Point(point.x, point.y-40), 1, 1, Scalar(255,255,0),2);  
}


/*
void OCV::drawCmdAreas(Mat &frame){
  //~ line(frame, Point(0, FRAME_HEIGHT/3), Point(FRAME_WIDTH, FRAME_HEIGHT/3), Scalar(255,255,0),2);
  //~ line(frame, Point(0, 2*FRAME_HEIGHT/3), Point(FRAME_WIDTH, 2*FRAME_HEIGHT/3), Scalar(255,255,0),2);
  // Middle line
  line(frame, Point(EXP_HORI_LIMIT, BBUTT_VER_LIMIT), Point(FRAME_WIDTH, BBUTT_VER_LIMIT), Scalar(255,255,0),2);
  // A
  line(frame, Point(2*FRAME_WIDTH/4, BBUTT_VER_LIMIT), Point(2*FRAME_WIDTH/4, FRAME_HEIGHT), Scalar(255,255,0),2);
  // B
  line(frame, Point(3*FRAME_WIDTH/4, BBUTT_VER_LIMIT), Point(3*FRAME_WIDTH/4, FRAME_HEIGHT), Scalar(255,255,0),2);
  // Expression lines
  line(frame, Point(EXP_HORI_LIMIT, 0), Point(EXP_HORI_LIMIT, FRAME_HEIGHT), Scalar(255,255,0),2);
  line(frame, Point(0, EXP_VER_LOW), Point(EXP_HORI_LIMIT, EXP_VER_LOW), Scalar(255,255,0),2);
  line(frame, Point(0, EXP_VER_HIGH), Point(EXP_HORI_LIMIT, EXP_VER_HIGH), Scalar(255,255,0),2);
}
*/

int OCV::disable_exposure_auto_priority(const int dev) 
{
  string camdev = "/dev/video" + to_string(dev);
  return disable_exposure_auto_priority(camdev);
}


int OCV::disable_exposure_auto_priority(const string dev) 
{
  int descriptor = v4l2_open(dev.c_str(), O_RDWR);

  v4l2_control c;   // auto exposure control to aperture priority 
  c.id = V4L2_CID_EXPOSURE_AUTO;
  c.value = V4L2_EXPOSURE_APERTURE_PRIORITY; 
  if (v4l2_ioctl(descriptor, VIDIOC_S_CTRL, &c)!=0)
    return -1;
  
  c.id = V4L2_CID_EXPOSURE_AUTO_PRIORITY; // auto priority control to false
  c.value = 0;
  if (v4l2_ioctl(descriptor, VIDIOC_S_CTRL, &c)!=0)
    return -1;
  
  v4l2_close(descriptor);
  return 0;
}


// TODO Implement
int OCV::read_frame_interval_us(const int dev)
{
  return -1;
}


// TODO Implement
int OCV::read_frame_interval_us(VideoCapture cap)
{
  return -1;
//  return (int)(1000000.0/(double)(cap.get(CV_CAP_PROP_FPS)));
}; 


int OCV::readHSVFilterConf(const string hsvFilterConfFile) {
  try 
  {
    ifstream infile(hsvFilterConfFile);
    int tmpBounds[6];
    for (int i=0; i<6;i++){
      int tmp;
      infile >> tmp;
      if (i == 0 || i ==3) {
        if (tmp>179 || tmp <0) return -1;
      }
      else {
        if (tmp>255 || tmp <0) return -1;
      }
      tmpBounds[i] = tmp;
    }
    hsvRange.lowerb = Scalar(tmpBounds[0], tmpBounds[1], tmpBounds[2]);
    hsvRange.upperb = Scalar(tmpBounds[3], tmpBounds[4], tmpBounds[5]);
    return 0;
  }
  catch (const exception &e)
  {
    cerr << "Exception reading color range file" << endl;
    return -1;
  }
}

int OCV::get_png(Mat & inmat, string filename){
  Mat _alpha = imread(filename, -1);
  Mat _imgNoalpha = imread(filename);
  if (!_imgNoalpha.data || !_alpha.data)
    return -1; 
  vector<Mat> ch;
  split(_alpha, ch);
  inmat = Mat::zeros(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
  _imgNoalpha.copyTo(inmat, ch[3]);
  return 0;
}
