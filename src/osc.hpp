/*******************************************************************************
	Copyright (C) 2014 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.
*******************************************************************************/
#ifndef OSC_H_
#define OSC_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <fstream>

#include "lo/lo.h"
#include <csignal>
#include "cmdmap.hpp"
#include <boost/foreach.hpp>

using namespace std;

//exceptions for OSC
class ExOSC: public exception
{
protected:
 const char *expMess;
public:
 ExOSC(const char *mess) throw() : exception() { expMess=mess;}
 virtual const char *what() const throw()
 {
   return expMess;
 };
};

//OSC
class OSC
{  
  
public:
  OSC(const string defoscserv, const int expressiondiv, const bool verb) throw(ExOSC);  
  bool parseAndSendMess(const string, const cmdmap::command) throw(ExOSC);
  
protected: 
 
private:
 
  bool verbose;
  int expressDiv;
  string oscAdressDef;
  
};

#endif 
