/*******************************************************************************
  Copyright (C) 2011 Francisco Salomón <fsalomon.ing@gmail.com>

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.
  
  Description:
  Class to get command map banks from an XML configuration file. That file must
  have the following format:
  
  <commands_map_banks>
    <bank name=...>
      <command name="...">
        <button>...</button>
        <cmd>...</cmd>
        <type>...</type>
        <args>
          <arg type="...">...</arg>
          ...
        </args>    
        <databytes>
          <databyte>...</databyte>
          ...
        </databytes>
      </command>
      ...
    </bank>
    ...
  <commands_map_banks>
  
  This is an example of how to instanciate a single const "record" command:
  
  const cmdmap::argument RECORD_ARG{'s', false, 0.0, 0.0, "record"};
  const list<cmdmap::argument> RECORD_ARGLIST = {RECORD_ARG};
  const list<unsigned int> RECORD_DATABYTESLIST = {0};
  const cmdmap::command RECORD_CMD{"record", "osc", '1', "", "record", RECORD_ARGLIST, RECORD_DATABYTESLIST};

  
*******************************************************************************/
#ifndef CMD_MAP_H_
#define CMD_MAP_H_

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <list>

using namespace std;

namespace cmdmap{

//~ enum cmdTypeEnum { OSC_TYPE, MIDI_TYPE, INTERNAL_TYPE };

// typedef for arguments
typedef struct{
  char type;
  bool range;
  float min;
  float max;
  string value;
} argument;

// typedef for commands
typedef struct{
  string name;
  //cmdTypeEnum type;
  string type;
  char button;
  string url;
  string cmd;
  list<argument> args;
  list<unsigned int> databytes;
} command;

// typedef for bank
typedef struct{
  string name;
  list<command> cmmds;
} bank;

}

class CmdMap {

private:
  unsigned int uiSelBank;
    
  
public:
  // List of banks 
  list<cmdmap::bank> banks;
     
  // Constructor
  CmdMap(const string &filename){
    uiSelBank = 0;
    using boost::property_tree::ptree;
    ptree pt;
    read_xml(filename, pt);
    // search for each bank
    BOOST_FOREACH(ptree::value_type &b, pt.get_child("commands_map_banks")){
      cmdmap::bank abank;
      if(b.first == "bank"){ 
        // this bank has any name?
        boost::optional<string> bankName = 
            b.second.get_optional<string>("<xmlattr>.name");
        if(bankName) abank.name = *bankName;
        else abank.name = "unnamed";
        // search for each command inside the bank
        BOOST_FOREACH(ptree::value_type &c, b.second){
          if (c.first == "command"){
            cmdmap::command acmd;
            acmd.name = c.second.get<string>("<xmlattr>.name");
            acmd.button = c.second.get<char>("button");
            acmd.cmd = c.second.get<string>("cmd");
            // optional url
            boost::optional<string> op_url = 
              c.second.get_optional<string>("url");
            if(op_url) acmd.url = * op_url;
            // optional type
            boost::optional<string> op_type = 
              c.second.get_optional<string>("type");
            if(op_type) acmd.type = * op_type;
            /*
            if(op_type){
              switch (* op_type){
                case "osc":
                case "midi":
                case "internal":
              }
            } 
            */
            // search for each argument inside the command
            boost::optional<ptree&> op_args_child = c.second.get_child_optional("args");
            if (op_args_child) {
              BOOST_FOREACH(ptree::value_type &s, c.second.get_child("args")){
                  cmdmap::argument aarg;
                  aarg.type = s.second.get<char>("<xmlattr>.type");
                  if (aarg.type=='f') {
                    boost::optional<string> op_min = 
                       s.second.get_optional<string>("<xmlattr>.min");
                    boost::optional<string> op_max = 
                       s.second.get_optional<string>("<xmlattr>.max");
                    if(op_min && op_max){
                      aarg.range = true;
                      aarg.min = atof((* op_min).c_str());
                      aarg.max = atof((* op_max).c_str());
                    }
                    else{
                      aarg.range = false;
                    }                  
                  }
                  aarg.value = s.second.data();
                  acmd.args.push_back(aarg);
              }
            }
            // search for each databyte inside the command (for midi)
            boost::optional<ptree&> op_databyte_child = c.second.get_child_optional("databytes");
            if (op_databyte_child) {
               BOOST_FOREACH(ptree::value_type &s, c.second.get_child("databytes")){
                 string dbs = s.second.data();
                 unsigned int dsui =  atoi(dbs.c_str());
                 acmd.databytes.push_back(dsui);
               }
            }   
            abank.cmmds.push_back(acmd);
          }
        }
        banks.push_back(abank);
      }
    }
  }

  // get next bank
  cmdmap::bank * getNextBank() {
    list<cmdmap::bank>::iterator iterBank =  banks.begin();
    if(uiSelBank < (banks.size()-1))
      advance(iterBank, ++uiSelBank);
    else
      uiSelBank = 0;
    return &*iterBank;
  }
  
  // get previous bank
  cmdmap::bank * getPrevBank() {
    list<cmdmap::bank>::iterator iterBank =  banks.begin();
    if(uiSelBank>0)
      --uiSelBank;
    else
      uiSelBank = banks.size()-1;
    advance(iterBank, uiSelBank);
    return &*iterBank;
  }
    
  // get actual bank
  cmdmap::bank * getFirstBank() {
    return &*banks.begin();
  }
    
  // print banks information
  void printAllBanks(bool verbose = false) {
    BOOST_FOREACH( cmdmap::bank a, banks){
        printBank(a, verbose);
        cout << endl;
    }
  }
  
  // print info about selected bank
  void printSelBank(bool verbose = false){
    list<cmdmap::bank>::iterator iterBank =  banks.begin();
    advance(iterBank, uiSelBank);
    cout << "Bank selected is " << (*iterBank).name << endl; 
    printBank(*iterBank, verbose);
  }
  
  // print selected bank information
  void printBank(cmdmap::bank abank, bool verbose) {
    BOOST_FOREACH( cmdmap::command a, abank.cmmds)
      printCommand(a, verbose);
  }
  
  // print a selected commad information
  void printCommand(cmdmap::command acmd, bool verbose) {
    if(verbose){
      cout << "Command " << acmd.name << endl; 
      cout << " - button : " << acmd.button << endl;
      if(acmd.type!="") cout << " - type : " << acmd.type << endl;
      else cout << " - type : default (osc)" << endl;
      if(acmd.url!="") cout << " - url : " << acmd.url << endl;
      cout << " - cmd : " << acmd.cmd << endl;
      BOOST_FOREACH( cmdmap::argument ar, acmd.args){
        cout << " - arg : " << endl;
        cout << "   arg.type  : " << ar.type << endl;
        if (ar.type=='f' && ar.range) {
          cout << "   arg.min  : " << ar.min << endl;
          cout << "   arg.max  : " << ar.max << endl;
        }
        cout << "   arg.value : " << ar.value << endl;
      }
      BOOST_FOREACH( unsigned int db, acmd.databytes){
        cout << " - databyte : " << endl;
        cout << "   databyte  : " << db << endl;
      }
    }
    else{
      cout << " - " << acmd.button << ": " << acmd.name << endl;
    }
  } 

};
#endif 
