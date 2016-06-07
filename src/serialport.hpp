/*******************************************************************************
  Copyright (C) 2011 Francisco Salomón <fsalomon.ing@gmail.com>

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#ifndef SERIAL_PORT_H_
#define SERIAL_PORT_H_

#include "ominputdev.hpp"

#include <boost/asio.hpp>

class SerialPort: public OMInputDev{
public:
  // Constructor method
  SerialPort(std::string port, unsigned int baud_rate): io(), serial(io,port){
    serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
  }
 
  // Reading char method
  char readChar(){
    char c;
    std::string result;
    boost::asio::read(serial,boost::asio::buffer(&c,1));
    return c;
  }

  //Read line until '\' char 
  std::string readBLine(){
    unsigned char c;
    std::string result;
    for(;;){
      boost::asio::read(serial,boost::asio::buffer(&c,1));
      switch(c){
        case 0x5C:
            return result;
        default:
            result+=c;
      }
    }
  } 

private:
    boost::asio::io_service io;
    boost::asio::serial_port serial;
};
#endif 
