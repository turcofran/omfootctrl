/*******************************************************************************
	Copyright (C) 2014 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************

  Some MIDI messages
  http://www.midi.org/techspecs/midimessages.php

  General Format
  --------------
  Status Byte + 1st Data Byte + 2nd Data Byte + ... 

  Note ON
  -------
  Status byte + Note (1st databyte) + Velocity 0-127 (2nd databyte) 
  - Status byte: 1001XXXX, XXXX: Channel (0->ch1, ... , 15->ch16)
  - Note: [00 - 7F] -> [C1 - G9]. One octave contains 12 notes -> C3 is 12x3=36=0x24
  - Velocity: Most no sensitive devices sends 64

  Note OFF
  --------
  Status byte + Note (1st databyte) + Velocity 0-127 (2nd databyte) 
  - Status byte: 1000XXXX, XXXX: Channel (0->ch1, ...)
  - Note: Idem Note ON
  - Velocity: Idem Note ON

  Control/Mode Change Messages
  ----------------------------
  Status byte + Control number (1st databyte) + Value 0-127 (2nd databyte) 
  - Status byte: 1011XXXX -> 176 (ch1) - 191 (ch16)
  - Control number: Main Volume(7),Foot Controller (4), etc.  
  - Value: continous (0-127) or logic (0<64; 1>=64)

  Program Change Messages
  ----------------------------
  Status byte + Program number (1st databyte) 
  - Status byte: 1100XXXX -> 192 (ch1) - 207 (ch16)
  - Program number: continous (0-127)

*******************************************************************************/
#ifndef MIDI_H_
#define MIDI_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <pthread.h>
#include <fstream>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <csignal>
#include "cmdmap.hpp"
#include <boost/foreach.hpp>

#define MIDI_MAX_LENGTH 16 // System messages has no limit, but are not common
#define MIDI_CC_MIN 176
#define MIDI_CC_MAX (176+15)
#define MIDI_PC_MIN (192)
#define MIDI_PC_MAX (192+15)
#define MIDI_MAX_VALUE 127

using namespace std;

//exceptions for MIDI
class ExMIDI: public exception
{
protected:
 const char *expMess;
public:
 ExMIDI(const char *mess) throw() : exception() { expMess=mess;}
 virtual const char *what() const throw()
 {
   return expMess;
 };
};

//MIDI
class MIDI
{  
  
public:
  MIDI(const string name, const int expressiondiv, const bool verb)  throw(ExMIDI);
  MIDI(const bool verb) {verbose = verb;jMidiClient=NULL;}; // This is just a constructor provided as a mock 
  ~MIDI();
  bool parseAndSendMess(string arrivedin, cmdmap::command cmdin) throw(ExMIDI);
  
protected: 
 
private:
 
  bool verbose;
  int expressDiv;
  string clientName;
  
  unsigned int midiBuffer[MIDI_MAX_LENGTH]; 
  unsigned int midiCDBQtt; // Command and databytes qtt
    
  jack_client_t * jMidiClient;
  jack_port_t * jMidiOutPort;
  int jMidiProcess(jack_nframes_t nframes);
  // static method for jack callback,  makes a bind to member this->process()
  static int _jMidiProcess(jack_nframes_t nframes, void* arg) {
    MIDI * mp = static_cast<MIDI*>(arg); 
    return mp->jMidiProcess(nframes);
  }

};

#endif 
