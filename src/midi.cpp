/*******************************************************************************

	Copyright (C) 2014 Francisco Salomón <fsalomon.ing@gmail.com>
  
  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation, version 2 of the License.

*******************************************************************************/
#include "midi.hpp"

MIDI::MIDI(const string name, const int expressiondiv, const bool verb)  throw(ExMIDI)
{
  verbose = verb;
  clientName = name;
  expressDiv = expressiondiv;
  try
  {
    midiCDBQtt=0;
    if((jMidiClient = jack_client_open(clientName.c_str(), JackNoStartServer, NULL)) == 0){
      throw(ExMIDI("MIDI client cannot be created, JACK server not running?\n"));
    }
    jack_set_process_callback(jMidiClient, _jMidiProcess, this);
    jMidiOutPort = jack_port_register(jMidiClient, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    if (jack_activate(jMidiClient)){
      throw(ExMIDI("Cannot activate MIDI client"));
    }
  }
  catch (const exception &e)
  {
    throw(ExMIDI(e.what())); 
  }
}

// Destructor closes midi client
MIDI::~MIDI(void)
{
  jack_client_close(jMidiClient);
}

// Parse and send Midi command
bool MIDI::parseAndSendMess(string arrivedin, cmdmap::command cmdin) throw(ExMIDI)
{  
  if(jMidiClient == NULL){
    cout << "The command " << cmdin.name << " won't be processes since midi client is NULL" << endl;
    return false;
  }
  bool sendExpression = false;
  if(!midiCDBQtt){
    // Get command and databytes
    int cdbqtt = 0;
    midiBuffer[cdbqtt++] = atoi(cmdin.cmd.c_str());
    if(arrivedin.size()>1){
      // Control/Mode change message
      if ((midiBuffer[0] >= MIDI_CC_MIN) && (midiBuffer[0] <= MIDI_CC_MAX)){
        if (cmdin.databytes.size() != 2) 
           throw(ExMIDI("Bad formed CC MIDI message"));
        midiBuffer[cdbqtt++] = cmdin.databytes.front(); // controller number
        cmdin.databytes.pop_front();
        //~ if (arrivedin[1]>=expressDiv) // This is an error condition 
           //~ arrivedin[1]=expressDiv-1;
        //~ midiBuffer[cdbqtt++] = arrivedin[1]*MIDI_MAX_VALUE/(expressDiv-1); //value 
        int value = atoi(arrivedin.substr(1, arrivedin.size() - 1).c_str());
        if (value  >= expressDiv) // This is an error condition 
          value=expressDiv-1;
        midiBuffer[cdbqtt++] = value*MIDI_MAX_VALUE/(expressDiv-1); //value 
        sendExpression = true;        
      }
      // Program change message
      else if ((midiBuffer[0] >= MIDI_PC_MIN) && (midiBuffer[0] <= MIDI_PC_MAX)){
        if (cmdin.databytes.size() != 1) 
           throw(ExMIDI("Bad formed PC MIDI message"));
        midiBuffer[cdbqtt++] = cmdin.databytes.front(); // program number
        cmdin.databytes.pop_front();
      }
    }
    else{
      BOOST_FOREACH(unsigned int uidb, cmdin.databytes){
        midiBuffer[cdbqtt++] = uidb;
      }
    }
    if (cdbqtt < MIDI_MAX_LENGTH) midiCDBQtt =  cdbqtt;
    else throw(ExMIDI("MIDI command longer than permitted"));
  }
  else{
    throw(ExMIDI("MIDI command cannot be sent, other sending on progress"));
  }
  return sendExpression;
}


int  MIDI::jMidiProcess(jack_nframes_t nframes)
{
  void* port_buf = jack_port_get_buffer(jMidiOutPort, nframes);
  unsigned char* buffer;
  jack_midi_clear_buffer(port_buf);
 
  if (midiCDBQtt) {
     // cout << "Se tiene un commando MIDI pa enviar con esta cantidad de db: " << midiCDBQtt << endl;
     // for (unsigned int i=0; i<midiCDBQtt; i++) cout << midiBuffer[i] << endl;
     buffer = jack_midi_event_reserve(port_buf, 1, midiCDBQtt);
     if ( buffer != 0 ) {
       for (unsigned int i=0; i<midiCDBQtt; i++) buffer[i] = (unsigned char)midiBuffer[i];
        midiCDBQtt=0;  
     }
     else {
       cout << "jack_midi_event_reserve returned NULL buffer" << endl;
     }
  }
  return 0;
}

