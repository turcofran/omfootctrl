/*******************************************************************************
  Copyright (C) 2011 Francisco Salomón <fsalomon.ing@gmail.com>

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
  details.
  
  You should have received a copy of the GNU General Public License along with 
  this program; if not, write to the Free Software Foundation, Inc., 675 Mass 
  Ave, Cambridge, MA 02139, USA.
 
  OSC Foot Controller Bridge 

  Description:
  This software captures the commands arrived from USB and passes them to the 
  OSC server open by the looper software as specified in the command map 
  configuration file.   
  
  Compilation:
  To compile this software are required Boost Library (www.boost.org) and Liblo 
  (liblo.sourceforge.net), both in Debian oficial repositories. With these 
  dependencies installed in your system, run: 
    $ make 
  Replace the value of BOOST_PATH variable of Makefile for the correct path in 
  your system. 
   
  Run: 
  To run this bridge, just use:
    $ ./oscbridge
  To get help, use:
    $ ./oscbridge -h

*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <csignal>

using namespace std;

#include <boost/program_options.hpp>
#include "ocvcontroller.hpp"
#include "main_config.hpp"


int main(int ac, char* av[])
{
  const int cCAMDEV = 0;
  const string cDEF_OSC_SERVER_ADDR = "osc.udp://localhost:9951/";
  const int cBAUD_RATE  = 9600;
  const int cGUI_PORT  = 5151;
  //~ const string cVERSION = "v1.0.0";
  const string cMAP_FILE = "../maps/map_expression.xml"; //"../maps/map.xml";
  const int cEXPRESSION_DIV = 12;
  
  cout <<"OM Controller with OpenCV " << CVOM_VERSION_MAJOR << "." << CVOM_VERSION_MINOR << endl;
  cout <<"Copyright 2016 Francisco Salomon" << endl;
  cout <<"This is free software. You are welcome to redistribute it." << endl;

  int camdev = cCAMDEV;
  int gui_port = cGUI_PORT;
  string osc_adress_def = cDEF_OSC_SERVER_ADDR;
  string map = cMAP_FILE;
  bool verbose = false;
  bool noGUI = true;
    
  namespace po = boost::program_options;
  po::options_description desc("Options for OM Controller with OpenCV");
  desc.add_options()
    ("help,h", "Show this help")
    ("version,V", "Show version number")
    ("in-camdev,d", po::value(&camdev), "Set camera device number")
    ("gui-port,g", po::value(&gui_port), "Set graphic user interface udp port")
    ("no-gui,n", "Does not notify to any graphic user interface")
    ("osc-addr,a", po::value(&osc_adress_def), "Set OSC server default address")
    ("map-file,m", po::value(&map), "Set commands map file")
    ("verbose,v", "Set verbose mode")
  ;
  po::variables_map vm;        
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);    
  if(vm.count("help")){
    cout << desc << endl;
    return EXIT_SUCCESS;
  }
  if(vm.count("version")){
    cout << "Version: " << CVOM_VERSION_MAJOR << "." << CVOM_VERSION_MINOR << endl;
    return EXIT_SUCCESS;
  }
  if(vm.count("verbose")){
    verbose=true;
  }  
  if(vm.count("no-gui")){
    noGUI=true;
  }
  
  try {
    if(verbose){
      cout << "Bridge open with following parameters:" << endl;
      cout << " - Input camera dev: " << camdev << endl;
      cout << " - Default OSC server address: " << osc_adress_def << endl;
      if(!noGUI){
        cout << " - GUI at UDP port: " << gui_port << endl;
      }
      cout << " - Commands map file: " << map << endl;
    }
    // Get  controller
    OCVController ocv(camdev, cBAUD_RATE, map, noGUI, gui_port, osc_adress_def, cEXPRESSION_DIV, verbose);
    // Main loop
    for(;;){
      ocv.processInput();
    }
  } catch(ExOCVController& e)  {
      cerr<< e.what() <<endl;
      return EXIT_FAILURE;
  } catch(boost::system::system_error& e)  {
      cerr<< e.what() <<endl;
      return EXIT_FAILURE;
  }
}
