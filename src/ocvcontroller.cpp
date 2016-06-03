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
#include <time.h> 
#include <chrono>

#define VIDEO_IN
//~ #define VIDEO_OUT

OCVController::OCVController(const int incamdev, const int  baudrate,  
               const string map, const bool nogui, const int guiport,  
               const string defoscserv, const int expressiondiv, const bool verb)  throw(ExOCVController)
{
  namespace bip = boost::asio::ip;
  expressionDiv = expressiondiv;
  verbose = verb;
  noGUI = nogui;
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
      //~ throw(ExOCVController("Not possible to disable auto priority")); 
        
    // open video to write
    #ifdef VIDEO_OUT
      time_t now;
      time(&now);
      string voname = "vsample"+to_string(now)+".avi";
      videoOut.open(voname, 0, 30.0, Size(FRAME_WIDTH, FRAME_HEIGHT));
      if (!videoOut.isOpened())
        throw(ExOCVController("Not possible to open write video")); 
    #endif

    //open capture object at location zero (default location for webcam)
    videoCap.open(camdev);
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
    namedWindow(W_NAME_FEED); moveWindow(W_NAME_FEED, 10, 10);
    namedWindow(W_NAME_THRESHOLD); moveWindow(W_NAME_THRESHOLD, 400, 10);
    namedWindow(W_NAME_CANVAS); moveWindow(W_NAME_CANVAS, 800, 10);
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
    #ifdef VIDEO_IN
      exit(0);
    #endif
    return;
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
    
    
  processCmd(trackAndEval(procThreshold, canvas));
    
  //delay so that screen can refresh.
  #ifdef SHOW_WIN
    imshow(W_NAME_FEED, camFeed);
    imshow(W_NAME_THRESHOLD, procThreshold);
  #endif
  drawCmdAreas(canvas);
  imshow(W_NAME_CANVAS, canvas);
  //image will not appear without this waitKey() command
  #ifdef VIDEO_IN
    if (waitKey(DEF_FRAME_INT_US/1000)!=-1) exit(0);
  #else  
    if (waitKey(CV_DELAY_MS)!=-1) exit(0);
  #endif
  auto tic3 = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
  if (DEBUB_TICS)
    cout << "processInput: " << tic1.count() << " - " << tic2.count()<< " - " << tic3.count()<< endl;
}

// TODO set the color in function to the state
void OCVController::drawObject(int area, Point point, Mat &frame){
  //use some of the openCV drawing functions to draw crosshairs on your tracked image!
  circle(frame,point,4,Scalar(0,255,0),-1);
  
  line(frame, Point(point.x,point.y-10), Point(point.x,point.y+10), Scalar(0,255,0),0.5);
  line(frame, Point(point.x-10,point.y), Point(point.x+10,point.y), Scalar(0,255,0),0.5);
   
  putText(frame, to_string(point.x) + ","+ to_string(point.y) + "\n" + to_string(area), 
          Point(point.x, point.y+30), 1, 1, Scalar(0,255,0),2);  
}


void OCVController::drawCmdAreas(Mat &frame){
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


void OCVController::erodeAndDilate(Mat &frame){
  erode(frame, frame, morphERODE, Point(-1,-1), ERODE_DILATE_ITS);
  //~ erode(procThreshold, procThreshold, erodeElement);
  //~ dilate(procThreshold, procThreshold, dilateElement);
  dilate(frame, frame, morphDILATE, Point(-1,-1), ERODE_DILATE_ITS);
  //TODO maybe a blur filter is faster than this... medianBLur?
}


string OCVController::trackAndEval(Mat &threshold, Mat &canvas){
  Mat temp;
  threshold.copyTo(temp);
  //these two vectors needed for output of findContours
  vector< vector<Point> > contours;
  vector<Vec4i> hierarchy;
  //find contours of filtered image using openCV findContours function
  findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
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
      #ifdef SHOW_WIN
        drawObject(area, lastPoint, canvas);
      #endif 
      // Evaluate in which position of the grid the point is
      // state machine
      // TOD CHECH bounding rectangles and contour to check the area!!!!
      switch (trackState) {
        case TrackStt::NO_TRACK:
        case TrackStt::UNARMED:
          if (lastPoint.x < EXP_HORI_LIMIT) {
            trackState = TrackStt::EXPRESSION;
            cout << "Next state TrackStt::EXPRESSION" << endl; 
          }
          else if (lastPoint.y < BBUTT_VER_LIMIT) {
            trackState = TrackStt::ARMED;
            cout << "Next state TrackStt::ARMED" << endl;
          }
          else {
            trackState = TrackStt::UNARMED;
          }
          break;
        case TrackStt::ARMED:
          if (lastPoint.x < EXP_HORI_LIMIT) {
            trackState = TrackStt::EXPRESSION;
          }
          else if (lastPoint.y > BBUTT_VER_LIMIT) {
            trackState = TrackStt::DEBOUNCING;
            debounceCounter = debouceFrames;
            if (lastPoint.x < 2*FRAME_WIDTH/4) {
              cout << "1" << endl; 
              retValue = "1";
            }
            else if (lastPoint.x < 3*FRAME_WIDTH/4) {
              cout << "2" << endl; 
              retValue = "2";
            }
            else {
              cout << "3" << endl; 
              retValue = "3";
            }
          }
          break;
        case TrackStt::DEBOUNCING: 
          cout << "DEBOUNCING" << endl; 
          if (debounceCounter==0) 
            trackState = TrackStt::UNARMED;
          break;
        case TrackStt::EXPRESSION: 
          if (lastPoint.x > EXP_HORI_LIMIT) {
              trackState = TrackStt::UNARMED;
          }
          else{ 
            int expLevel;
            if (lastPoint.y > EXP_VER_HIGH) 
              expLevel = 0;
            else if (lastPoint.y < EXP_VER_LOW)
              expLevel = expressionDiv;
            else {
              float ylevel = (float)(lastPoint.y-EXP_VER_LOW)/(float)(EXP_VER_RANGE);
              expLevel = (int)((float)expressionDiv*(1.0 - ylevel));
            }
            cout << "Expression level:" << expLevel << endl; 
            retValue = "X"+to_string(expLevel);
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
      putText(canvas, "More than one object detected!", Point(2, FRAME_HEIGHT-10), 1, 0.5, Scalar(0,0,255), 1);
    }
  }
  if (trackState!=TrackStt::DEBOUNCING) trackState = TrackStt::NO_TRACK;
  return retValue;
}


int OCVController::disable_exposure_auto_priority(const int dev) 
{
  string camdev = "/dev/video" + to_string(dev);
  return disable_exposure_auto_priority(camdev);
}


int OCVController::disable_exposure_auto_priority(const string dev) 
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
int OCVController::read_frame_interval_us(const int dev)
{
  return -1;
}

// TODO Implement
int OCVController::read_frame_interval_us(VideoCapture cap)
{
  return -1;
//  return (int)(1000000.0/(double)(cap.get(CV_CAP_PROP_FPS)));
}; 

// Process message from ocv
void OCVController::processCmd(const string arrivedCmd)
{
  if (arrivedCmd=="") return;
  bool sendExpression2GUI = false;
  try
  {
    BOOST_FOREACH(cmdmap::command cmapIt, aBank->cmmds){
      if(cmapIt.button == arrivedCmd[0]){  
        // Internal command
        if(cmapIt.type=="internal"){
          if(verbose) cout << "Send INTERNAL command: " << cmapIt.name << endl;
          // Select next commands map bank
          if(cmapIt.name=="sel_next_bank"){   
              aBank = cmap->getNextBank();
              cmap->printSelBank(verbose);
          }
          // Select previous commands map bank
          else if(cmapIt.name=="sel_prev_bank"){ 
              aBank = cmap->getPrevBank();
              cmap->printSelBank(verbose);
          }
        } 
        // Midi command
        if(cmapIt.type=="midi"){
          if(verbose) cout << "Send MIDI command: " << cmapIt.name << endl;
          sendExpression2GUI = midiDev->parseAndSendMess(arrivedCmd, cmapIt);
        }
        // OSC command
        else{   
          if(verbose) cout << "Send OSC command: " << cmapIt.name << endl;
          sendExpression2GUI = oscDev->parseAndSendMess(arrivedCmd, cmapIt);
        }
        if(!noGUI){
          // Notify to GUI about the command arrived by sending an string  
          // with format "button_pressed,actual_bank,expression" if(expression)
          try{
              string sTempCmd("");
              sTempCmd += cmapIt.button;
              sTempCmd += "," ;
              sTempCmd += aBank->name;
              if(sendExpression2GUI){
                char cExpressVal[3];
                sprintf(cExpressVal, "%d", arrivedCmd[1]);
                //itoa(sendExpressValue, cExpressVal, 10);
                sTempCmd += "," ;
                sTempCmd += cExpressVal;
              }
              socketGUI->send_to(boost::asio::buffer(sTempCmd.c_str(), sTempCmd.length()), guiEndpoint);
          } catch (exception& e) {
             throw(ExOCVController(e.what()));
          }
        }
        break;
      }
    }
  } catch(ExMIDI& e) { // In this instance, exceptions are not thrown
      cerr<< e.what() <<endl;
  } catch(ExOSC& e) {
      cerr<< e.what() <<endl;
  }
}
