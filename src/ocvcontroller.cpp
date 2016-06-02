/*******************************************************************************

  Copyright (C) 2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#include "ocvcontroller.hpp"
#include "main_config.hpp"

#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <chrono>

OCVController::OCVController(const int camdev, const int  baudrate,  
               const string map, const bool nogui, const int guiport,  
               const string defoscserv, const int expressiondiv, const bool verb)  throw(ExOCVController)
{
  namespace bip = boost::asio::ip;
  verbose = verb;
  noGUI = noGUI;
  try
  {
    // Initialize structuring elements for morphological operations
    morphERODE  = getStructuringElement( MORPH_RECT, Size(ERODE_RECT_PIXEL,ERODE_RECT_PIXEL));
    morphDILATE = getStructuringElement( MORPH_RECT, Size(DILATE_RECT_PIXEL,DILATE_RECT_PIXEL));
    trackState = TrackStt::NO_TRACK;
    // auto exposure control
    if (disable_exposure_auto_priority(camdev) != 0)
      throw(ExOCVController("Not possible to disable auto priority")); 
    // TODO implement reading the fps
    int frameIntervalUS = read_frame_interval_us(camdev);
    if (frameIntervalUS == -1) {
      cerr << "Not possible to read the frame interval from device " << ". Default value will be used: " << DEF_FRAME_INT_US << endl;
      frameIntervalUS = DEF_FRAME_INT_US;
    }
    debouceFrames = (int)((double)DEBOUNCE_TIME_MS*1000/(double)frameIntervalUS);
    debounceCounter = 0;
    
    // open video to write
    //~ videoOut.open("sample.raw", CV_FOURCC('V','P','8','0'), 30.0, Size(FRAME_HEIGHT, FRAME_WIDTH), true);
    videoOut.open("sample.avi", 0,  30.0, Size(FRAME_WIDTH, FRAME_HEIGHT));
    if (!videoOut.isOpened())
      throw(ExOCVController("Not possible to open write video")); 

    //open capture object at location zero (default location for webcam)
    videoCap.open(camdev);
    cb_tpoints.set_capacity(CB_CAPACITY); // =  new boost::circular_buffer<Point>(CB_CAPACITY);

    //set height and width of capture frame
    videoCap.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
    videoCap.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
    // Get commnds map and its iterator       
    cmap = new CmdMap(map);
    aBank = cmap->getFirstBank(); 
    cmap->printSelBank(verbose);
    // Create midi device
    // TODO reenable midi
    //midiDev = new MIDI(MIDI_CLIENT_NAME, expressiondiv, verbose);
    // Create osc device
    oscDev = new OSC(defoscserv, expressiondiv, verbose);
    // Create an UDP socket for GUI in localhost
    //~ if (!noGUI){        
      //~ guiEndpoint = bip::udp::endpoint(bip::address::from_string("127.0.0.1"), guiport);
      //~ boost::asio::io_service ioService;
      //~ socketGUI = new bip::udp::socket(ioService);
      //~ socketGUI->open(bip::udp::v4());
    //~ }
  }
  catch (const exception &e)
  {
    throw(ExOCVController(e.what())); 
  }
}

// Process input method
void OCVController::processInput(void)
{
  auto start = chrono::steady_clock::now();
  bool sendExpression2GUI = false;
  Mat camFeed, procHSV, procThreshold;
  //~ Mat canvas(FRAME_HEIGHT, FRAME_WIDTH, CV_8U); 
  Mat canvas = Mat::zeros(FRAME_HEIGHT, FRAME_WIDTH, CV_8UC3);
    // Get commnds map and it
  if (!videoCap.read(camFeed)) {
    cerr << "VideoCapture is not reading" << endl;
    return;
  }
  auto tic1 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
  //flip image
  flip(camFeed, camFeed, 1);
  //videoOut.write(camFeed);
  //convert frame from BGR to HSV colorspace
  cvtColor(camFeed, procHSV, COLOR_BGR2HSV);
  //filter HSV image between values and store filtered image to threshold matrix
  inRange(procHSV,Scalar(H_MIN,S_MIN,V_MIN),Scalar(H_MAX,S_MAX,V_MAX),procThreshold);
  //perform morphological operations on thresholded image to eliminate noise and emphasize the filtered object(s)
  erodeAndDilate(procThreshold);
  //pass in thresholded frame to our object tracking function
  //this function will return the x and y coordinates of the
  //filtered object
  auto tic2 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
    
  if (trackAndEval(procThreshold, canvas)) {
    try {
      // OSC command
      if(verbose) cout << "Send OSC command: " << RECORD_CMD.name << endl;
      sendExpression2GUI = oscDev->parseAndSendMess("1", RECORD_CMD);
    } catch(ExOSC& e) {
      cerr<< e.what() <<endl;
    }
  } 
  //delay so that screen can refresh.
  #ifdef SHOW_WIN
    imshow("OM OpenCV - feed", camFeed);
    imshow("OM OpenCV - threshold", procThreshold);
  #endif
  drawCmdAreas(canvas);
  imshow("OM OpenCV - Canvas", canvas);
  //image will not appear without this waitKey() command
  waitKey(CV_DELAY_MS);
  auto tic3 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
  if (DEBUB_TICS)
    cout << "processInput: " << tic1.count() << " - " << tic2.count()<< " - " << tic3.count()<< endl;
}

// TODO set the color in function to the state
void OCVController::drawObject(int area, Point point, Mat &frame){
  //use some of the openCV drawing functions to draw crosshairs on your tracked image!
  circle(frame,point,4,Scalar(0,255,0),-1);
  //circle(frame,Point(x,y),20,Scalar(0,255,0),2);
  //~ if(point.y-25>0) line(frame, point, Point(point.x,point.y-25), Scalar(0,255,0),2);
  //~ else line(frame, point, Point(point.x, 0), Scalar(0,255,0),2);
  //~ 
  //~ if(point.y+25<FRAME_HEIGHT) line(frame, point, Point(point.x, point.y+25), Scalar(0,255,0),2);
  //~ else line(frame, point, Point(x, FRAME_HEIGHT), Scalar(0,255,0),2);
  //~ 
  //~ if(point.x-25>0) line(frame, point, Point(point.x-25,point.y), Scalar(0,255,0),2);
  //~ else line(frame, point, Point(0,point.y), Scalar(0,255,0),2);
  //~ 
  //~ if(point.x+25<FRAME_WIDTH) line(frame, point, Point(point.x+25, point.y), Scalar(0,255,0),2);
  //~ else line(frame, point, Point(FRAME_WIDTH, point.y), Scalar(0,255,0),2);
//~ 
  putText(frame, to_string(point.x) + ","+ to_string(point.y) + "\n" + to_string(area), 
          Point(point.x, point.y+30), 1, 1, Scalar(0,255,0),2);  
}


void OCVController::drawCmdAreas(Mat &frame){
  //~ line(frame, Point(0, FRAME_HEIGHT/3), Point(FRAME_WIDTH, FRAME_HEIGHT/3), Scalar(255,255,0),2);
  //~ line(frame, Point(0, 2*FRAME_HEIGHT/3), Point(FRAME_WIDTH, 2*FRAME_HEIGHT/3), Scalar(255,255,0),2);

  // Middle line
  line(frame, Point(FRAME_WIDTH/4, 2*FRAME_HEIGHT/3), Point(FRAME_WIDTH, 2*FRAME_HEIGHT/3), Scalar(255,255,0),2);
  // A
  line(frame, Point(2*FRAME_WIDTH/4, 2*FRAME_HEIGHT/3), Point(2*FRAME_WIDTH/4, FRAME_HEIGHT), Scalar(255,255,0),2);
  // B
  line(frame, Point(3*FRAME_WIDTH/4, 2*FRAME_HEIGHT/3), Point(3*FRAME_WIDTH/4, FRAME_HEIGHT), Scalar(255,255,0),2);
  
  // Expression line
  line(frame, Point(FRAME_WIDTH/4, 0), Point(FRAME_WIDTH/4, FRAME_HEIGHT), Scalar(255,255,0),2);
}


void OCVController::erodeAndDilate(Mat &frame){
  erode(frame, frame, morphERODE, Point(-1,-1), ERODE_DILATE_ITS);
  //~ erode(procThreshold, procThreshold, erodeElement);
  //~ dilate(procThreshold, procThreshold, dilateElement);
  dilate(frame, frame, morphDILATE, Point(-1,-1), ERODE_DILATE_ITS);
  //TODO maybe a blur filter is faster than this... medianBLur?
}


bool OCVController::trackAndEval(Mat &threshold, Mat &canvas){
  Mat temp;
  threshold.copyTo(temp);
  //these two vectors needed for output of findContours
  vector< vector<Point> > contours;
  vector<Vec4i> hierarchy;
  //find contours of filtered image using openCV findContours function
  findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
  //use moments method to find our filtered object
  bool retValue = false;
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
      #ifdef SHOW_WIN
        drawObject(area, lastPoint, canvas);
      #endif 
      // Evaluate in which position of the grid the point is
      // state machine
      // TOD CHECH bounding rectangles and contour to check the area!!!!
      switch (trackState) {
        case TrackStt::NO_TRACK:
        case TrackStt::UNARMED:
          if (lastPoint.x < FRAME_WIDTH/4) {
            trackState = TrackStt::EXPRESSION;
            cout << "Next state TrackStt::EXPRESSION" << endl; 
          }
          else if (lastPoint.y < 2*FRAME_HEIGHT/3) {
            trackState = TrackStt::ARMED;
            cout << "Next state TrackStt::ARMED" << endl;
          }
          else {
            trackState = TrackStt::UNARMED;
          }
          break;
        case TrackStt::ARMED:
          if (lastPoint.x < FRAME_WIDTH/4) {
            trackState = TrackStt::EXPRESSION;
          }
          else if (lastPoint.y > 2*FRAME_HEIGHT/3) {
            trackState = TrackStt::DEBOUNCING;
            debounceCounter = debouceFrames;
            if (lastPoint.x < 2*FRAME_WIDTH/4) {
              cout << "A" << endl; 
            }
            else if (lastPoint.x < 3*FRAME_WIDTH/4) {
              cout << "B" << endl; 
            }
            else {
              cout << "C" << endl; 
            }
          }
          break;
        case TrackStt::DEBOUNCING: 
          cout << "DEBOUNCING" << endl; 
          if (debounceCounter==0) 
            trackState = TrackStt::UNARMED;
          break;
        case TrackStt::EXPRESSION: 
          if (lastPoint.x > FRAME_WIDTH/4) {
              trackState = TrackStt::UNARMED;
          }
          break;
        default: 
          break;
      } 
      return retValue;
    }
    else {
      trackState = TrackStt::NO_TRACK;
      //void putText(Mat& img, const string& text, Point org, int fontFace, double fontScale, Scalar color, int thickness=1, int lineType=8, bool bottomLeftOrigin=false )
      putText(canvas, "More than one object detected!", Point(2, FRAME_HEIGHT-10), 1, 0.5, Scalar(0,0,255), 1);
    }
  }
  trackState = TrackStt::NO_TRACK;
  return retValue;
}


int OCVController::disable_exposure_auto_priority(const int dev) 
{
  string camdev = "/dev/video" + to_string(dev);
  int descriptor = v4l2_open(camdev.c_str(), O_RDWR);

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

int OCVController::read_frame_interval_us(const int dev)
{
  return -1;
}

