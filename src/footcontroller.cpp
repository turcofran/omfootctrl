/*******************************************************************************

  Copyright (C) 2014-2016 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#include "footcontroller.hpp"

//~ FootController::FootController(const int camdev, const int  baudrate,  
               //~ const string map, const bool nogui, const int guiport,  
               //~ const string defoscserv, const int expressiondiv, const bool verb)  throw(ExFootController)
FootController::FootController(const int incamdev, const int  baudrate,  
        const string map, const bool nogui, const int guiport,  
        const string defoscserv, const int expressiondiv, const bool verb)  throw(ExFootController)
{
  namespace bip = boost::asio::ip;
  verbose = verb;
  noGUI = noGUI;
  try
  {
    // Get serial port
    //inputDev = new SerialPort(port,baudrate);
    inputDev = new OCV(incamdev,"hsvfilter.config", expressiondiv, verb);
    // Get commnds map and its iterator       
    cmap = new CmdMap(map);
    aBank = cmap->getFirstBank(); 
    cmap->printSelBank(verbose);
    // Create midi device
    //midiDev = new MIDI(MIDI_CLIENT_NAME, expressiondiv, verbose);
    // Create osc device
    oscDev = new OSC(defoscserv, expressiondiv, verbose);
    // Create an UDP socket for GUI in localhost
    if (!noGUI){        
      guiEndpoint = bip::udp::endpoint(bip::address::from_string("127.0.0.1"), guiport);
      boost::asio::io_service ioService;
      socketGUI = new bip::udp::socket(ioService);
      socketGUI->open(bip::udp::v4());
    }
  }
  catch (const exception &e)
  {
    throw(ExFootController(e.what())); 
  }
}

// Process input method
void FootController::processInput(void)
{
  string arrivedCmd = inputDev->readBLine();
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
             throw(ExFootController(e.what()));
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

