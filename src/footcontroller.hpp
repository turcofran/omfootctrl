/*******************************************************************************
	Copyright (C) 2014 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#ifndef FOOTCONTROLLER_H_
#define FOOTCONTROLLER_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <fstream>

#include "ominputdev.hpp"
#include "ocv.hpp"
#include "serialport.hpp"

#include "lo/lo.h"
#include <csignal>
#include "cmdmap.hpp"
#include "osc.hpp"
#include "midi.hpp"
#include <boost/foreach.hpp>

using namespace std;

#define MIDI_CLIENT_NAME "OMBridge"

//exceptions for FootController
class ExFootController: public exception
{
protected:
 const char *expMess;
public:
 ExFootController(const char *mess) throw() : exception() { expMess=mess;}
 virtual const char *what() const throw()
 {
   return expMess;
 };
};

//FootController
class FootController
{  
  
public:
  FootController(const int incamdev, const int  baudrate,  
        const string map, const bool nogui, const int guiport,  
        const string defoscserv, const int expressiondiv, const bool verb)  throw(ExFootController);
  //~ FootController(const string port, const int  baudrate,  
               //~ const string map, const bool nogui, const int guiport,  
               //~ const string defoscserv, const int expressiondiv, const bool verb)  throw(ExFootController);  
  void processInput(void);
  
protected: 
 
private:
 
  bool verbose;
  bool noGUI;
  boost::asio::ip::udp::endpoint guiEndpoint;
  boost::asio::ip::udp::udp::socket * socketGUI;
  //~ SerialPort * serial;
  OMInputDev * inputDev;
  CmdMap * cmap;
  cmdmap::bank * aBank;
  OSC * oscDev;
  MIDI * midiDev;
    
};

#endif 
