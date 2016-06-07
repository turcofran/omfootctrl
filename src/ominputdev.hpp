/*******************************************************************************

  Copyright (C) 2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#ifndef OMINPUTDEV_H_
#define OMINPUTDEV_H_

class OMInputDev {
 public:
  virtual std::string readBLine()=0;
};

#endif 
