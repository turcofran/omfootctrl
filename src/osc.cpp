/*******************************************************************************

  Copyright (C) 2014 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#include "osc.hpp"

OSC::OSC(const string defoscserv, const int expressiondiv, const bool verb)  throw(ExOSC)
{
  verbose = verb;
  oscAdressDef = defoscserv;
  expressDiv = expressiondiv;
}

// Parse and send OSC command
bool OSC::parseAndSendMess(string arrivedin, cmdmap::command cmdin) throw(ExOSC)
{
  bool sendExpression = false;
  float farg;
  // Build the message to send
  lo_message mess;
  mess = lo_message_new();
  // Check if the cmd has url, else use default value
  lo_address t; 
  if(cmdin.url!="") t = lo_address_new_from_url(cmdin.url.c_str());
  else t = lo_address_new_from_url(oscAdressDef.c_str());
  BOOST_FOREACH(cmdmap::argument arg, cmdin.args){
    switch(arg.type){
      case 's':
        lo_message_add_string(mess, arg.value.c_str());
        break; 
      case 'f':
        //lo_message_add_float(mess, atof(arg.value.c_str()));
        if((arrivedin.size()>1) && arg.range){
          if (arrivedin[1]>=expressDiv) // This is an error condition 
            arrivedin[1]=expressDiv-1;
          farg = (float)arrivedin[1]*(arg.max-arg.min)/(expressDiv-1) + arg.min;
          sendExpression = true;
        }
        else{
          farg = atof(arg.value.c_str());
        } 
        lo_message_add_float(mess, farg);
        break;
      default: break;   
    }
  }  
  // Send and free
  if (lo_send_message(t, cmdin.cmd.c_str(), mess) == -1)
    throw(ExOSC(lo_address_errstr(t))); //lo_address_errno(t)
  lo_address_free(t);
  lo_message_free(mess);
  return sendExpression; 
}

