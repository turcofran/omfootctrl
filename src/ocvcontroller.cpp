/*******************************************************************************

  Copyright (C) 2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#include "ocvcontroller.hpp"
#include "main_config.hpp"

OCVController::OCVController(const string port, const int  baudrate,  
               const string map, const bool nogui, const int guiport,  
               const string defoscserv, const int expressiondiv, const bool verb)  throw(ExOCVController)
{
  namespace bip = boost::asio::ip;
  verbose = verb;
  noGUI = noGUI;
  try
  {
    //open capture object at location zero (default location for webcam)
    capture.open(0);
    cb_tpoints.set_capacity(CB_CAPACITY); // =  new boost::circular_buffer<Point>(CB_CAPACITY);

    //set height and width of capture frame
    capture.set(CV_CAP_PROP_FRAME_WIDTH,FRAME_WIDTH);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT,FRAME_HEIGHT);
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
  bool sendExpression2GUI = false;
  //store image to matrix
  // TODO read return value
  capture.read(cameraFeed);
  //convert frame from BGR to HSV colorspace
  cvtColor(cameraFeed,HSV,COLOR_BGR2HSV);
  //filter HSV image between values and store filtered image to threshold matrix
  inRange(HSV,Scalar(H_MIN,S_MIN,V_MIN),Scalar(H_MAX,S_MAX,V_MAX),threshold);
  //perform morphological operations on thresholded image to eliminate noise and emphasize the filtered object(s)
  morphOps(threshold);
  //pass in thresholded frame to our object tracking function
  //this function will return the x and y coordinates of the
  //filtered object
  if (trackFilteredObject(threshold, cameraFeed)) {
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
    imshow("OM OpenCV",cameraFeed);
  #endif
  //image will not appear without this waitKey() command
  waitKey(CV_DELAY_MS);
}

void OCVController::drawObject(int area, Point point, Mat &frame){
  //use some of the openCV drawing functions to draw crosshairs on your tracked image!
  circle(frame,point,20,Scalar(0,255,0),2);
  if(point.y-25>0) line(frame, point, Point(point.x,point.y-25), Scalar(0,255,0),2);
  else line(frame, point, Point(point.x, 0), Scalar(0,255,0),2);
  
  if(point.y+25<FRAME_HEIGHT) line(frame, point, Point(point.x, point.y+25), Scalar(0,255,0),2);
  else line(frame, point, Point(x, FRAME_HEIGHT), Scalar(0,255,0),2);
  
  if(point.x-25>0) line(frame, point, Point(point.x-25,point.y), Scalar(0,255,0),2);
  else line(frame, point, Point(0,point.y), Scalar(0,255,0),2);
  
  if(point.x+25<FRAME_WIDTH) line(frame, point, Point(point.x+25, point.y), Scalar(0,255,0),2);
  else line(frame, point, Point(FRAME_WIDTH, point.y), Scalar(0,255,0),2);

  //  putText(frame,intToString(x)+","+intToString(y)+"\n"+area,Point(x,y+30),1,1,Scalar(0,255,0),2);
  putText(frame, to_string(point.x) + ","+ to_string(point.y) + "\n" + to_string(area), Point(point.x, point.y+30), 1, 1, Scalar(0,255,0),2);
}

void OCVController::morphOps(Mat &thresh){
  //create structuring element that will be used to "dilate" and "erode" image.
  //the element chosen here is a 3px by 3px rectangle
  Mat erodeElement = getStructuringElement( MORPH_RECT, Size(3,3));
  //dilate with larger element so make sure object is nicely visible
  Mat dilateElement = getStructuringElement( MORPH_RECT, Size(8,8));
  erode(thresh,thresh,erodeElement);
  erode(thresh,thresh,erodeElement);
  dilate(thresh,thresh,dilateElement);
  dilate(thresh,thresh,dilateElement);
  //TODO maybe a blur filter is faster than this...
}


bool OCVController::trackFilteredObject(Mat &threshold, Mat &cameraFeed){
  Mat temp;
  threshold.copyTo(temp);
  //these two vectors needed for output of findContours
  vector< vector<Point> > contours;
  vector<Vec4i> hierarchy;
  //find contours of filtered image using openCV findContours function
  findContours(temp,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );
  //use moments method to find our filtered object
  bool retValue = false;
  double refArea = 0;
  double area;
  bool objectFound = false;
  int numObjects = hierarchy.size();
  if (numObjects > 0) {
    //if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
    if (numObjects<MAX_NUM_OBJECTS){
      for (int index = 0; index >= 0; index = hierarchy[index][0]) {
        Moments moment = moments((Mat)contours[index]);
        area = moment.m00;
        //if the area is less than 20 px by 20px then it is probably just noise
        //if the area is the same as the 3/2 of the image size, probably just a bad filter
        //we only want the object with the largest area so we safe a reference area each
        //iteration and compare it to the area in the next iteration.
        if (area>MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea){ // TODO Evaluate this restictions
          Point lastPoint(
            moment.m10/area, // x
            moment.m01/area  // y
          );
          objectFound = true;
          refArea = area;
          cb_tpoints.push_back(lastPoint);
          /* TODO 
           * 1- Llenar un buffer con info de los últimos puntos trackeados.
           * 2- Analizar la info en cada pasada: Cuantos puntos válidos hay? Hubo movimiento? Filtrar algo?
           * 3- Si hubo movimiento, retornar true, setear un inhibicion despues de detectar evento. Timer? 
           * 4- Levantar inhibicion y arrancar de nuevo a evaluar
           */
          #ifdef SHOW_WIN
            drawObject(area, lastPoint, cameraFeed);
          #endif          
          // 
          // !!!!!!!!!!       TODO ver phase corr !!!! puede ser la solución, analizando dos thresholdeadas!!!!!!!!!!
          // 
          if (cb_tpoints.full()) {
            // analize expresion!
            // dtf() <- how to analize that! remember is an array of points!
            // http://docs.opencv.org/2.4/modules/imgproc/doc/motion_analysis_and_object_tracking.html
            vector<double> speed;
            //cout << "HOla, llene el buffer" << endl;
            for (int i=1; i<CB_CAPACITY; i++) {
              //speed.push_back(());
            }
            //~ if () {
              //~ cb_tpoints.clear(); // reset buffer
              //~ retValue = true;
            //~ }
          }
        }
        else 
          cb_tpoints.clear(); // reset buffer
      }
    }
    else {
      cb_tpoints.clear();
      putText(cameraFeed,"TOO MUCH NOISE! ADJUST FILTER",Point(0,50),1,2,Scalar(0,0,255),2);
    }
    return retValue;
  }
}

