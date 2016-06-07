#!/usr/bin/perl  
#
#  Copyright (C) 2011 Francisco Salomón <fsalomon.ing@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify it under
#  the terms of the GNU General Public License as published by the Free Software
#  Foundation, version 2 of the License.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more 
#  details.
#
#  You should have received a copy of the GNU General Public License along with 
#  this program; if not, write to the Free Software Foundation, Inc., 675 Mass 
#  Ave, Cambridge, MA 02139, USA.
#
#  Simple GUI fo OSC Foot Controller Bridge 
#
#  Description:
#  This script opens an UDP port and listens messages from the OSCBridge. For
#  each button pressed, the corresponding button's figure blinks, and check the 
#  selected command bank. If selected bank has changed, the text in the buttons 
#  is updated.
#
################################################################################
use IO::Socket;
use strict;
use warnings;
use Switch;
# use '-init' is equal to use Gtk->init in the body
use Gtk2 '-init';
use Glib  qw(TRUE FALSE);
use Gtk2::Helper; 
use Time::HiRes qw ( alarm );
use Getopt::Long;
use Data::Dumper;
use XML::LibXML; #libxml-libxml-perl

# Variables
my $help = '';
my $verbose = '';
my $MAXLEN = 256;
my $PORTNO = 5151;
my $cmd_map_file = '../maps/map_expression.xml';
#~ my $logo_file = "/home/fran/Proyectos/OSC-MIDI/OMFootCtrl/software/src/logo.png";
my $logo_file = "./logo.png";
my @BANKS; # List of banks { NAME (string), CMDS (array of {NAME, BUTTON}) }
my $SEL_BANK = 0; # Selected bank
my $EXPREESS_DIV = 12; 

# Get program options
GetOptions("help" => \$help ,"verbose" => \$verbose, "map=s" => \$cmd_map_file, "port=i" => \$PORTNO);

# If requested, print help message and exit
printHelpMess($help);

# Read commands map file
# Info about Records: http://docstore.mik.ua/orelly/perl/prog3/ch09_06.htm
my $parser = XML::LibXML->new();
my $map    = $parser->parse_file($cmd_map_file);
my $indexBank = 0; 
foreach my $bank ($map->findnodes('commands_map_banks/bank')) {
  my $bname = $bank->getAttribute('name');
  my @cmds;
  my $indexCmd = 0;
  foreach my $command ($bank->findnodes('./command')) {
    my $cname = $command->getAttribute('name');
    my($cbutton) = $command->findnodes('./button');
    $cmds[$indexCmd] = {NAME => $cname, BUTTON => $cbutton->to_literal}; 
    $indexCmd = $indexCmd+1;
  }
  $BANKS[$indexBank] = {NAME => $bname, CMDS => [ @cmds ]};
  $indexBank = $indexBank+1;
}
#print Dumper(@BANKS);

# Create socket and add watch
# http://gtk2-perl.sourceforge.net/doc/gtk2-perl-study-guide/x3661.html
my $sock = new IO::Socket::INET->new(LocalPort => $PORTNO, Proto => 'udp') or die "socket: $@";
my $sock_watcher = Gtk2::Helper->add_watch(fileno($sock), 'in', \&watch_insocket_callback, $sock);

# Colors table
my $red = Gtk2::Gdk::Color->new (0xFFFF,0,0);
my $white = Gtk2::Gdk::Color->new (0xFFFF,0xFFFF,0xFFFF);
my $black = Gtk2::Gdk::Color->new (0,0,0);
#my $slblue = Gtk2::Gdk::Color->new (0x3700, 0x4e00, 0x7200);
my $slblue = Gtk2::Gdk::Color->new (0x4f00, 0x6500, 0xa600); 

################################################################################
# Window structure
# ################
#                
#             ----------------------------------------------------
# hboxTop    |                        (empty)                     | 
#             ----------------------------------------------------    
#            |   hboxMidLT (logo, vboxMidL)     |                 |
# hdoxMid    |                                  |   (pbarX)       |       
#            |   hboxMidLB (eboxA-E, vboxMidL)  |                 | 
#             ----------------------------------------------------
# hboxBot    |                        (labels)                    |
#             ----------------------------------------------------
#
################################################################################
## Window size
#my($W_W, $W_T);
#$W_W = 1000;
#$W_T = 180;
## Event Boxes size (Buttons)
#my($EB_W, $EB_T);
#$EB_W = 140;
#$EB_T = 50;
## Progress bar size (Expression)
#my($PB_W, $PB_T); 
#$PB_W = $EB_W-25;
#$PB_T = $EB_T*2.5;
# Window size
my($W_W, $W_T);
$W_W = 900;
$W_T = 70;
# Event Boxes size (Buttons)
my($EB_W, $EB_T);
$EB_W = 150;
$EB_T = 20;
# Progress bar size (Expression)
my($PB_W, $PB_T); 
$PB_W = $EB_W-25;
$PB_T = $EB_T*2.5;
# Create icon
my $navIco = Gtk2::Gdk::Pixbuf->new_from_file("./logo.ico");
# Create main windows 
my $window = Gtk2::Window->new( 'toplevel' );
$window->set_default_icon($navIco); 
$window->set_title("OM Foot Controller GUI");
$window->signal_connect(delete_event => sub { Gtk2->main_quit; FALSE; });
#$window->set_border_width( 10 );
$window->set_border_width( 10 );
$window->modify_bg('normal', $black);
$window->set_size_request($W_W, $W_T);
$window->set_resizable(FALSE); 
$window->set_decorated(FALSE);
$window->move(300, 980);

# Create event boxes for buttons
# Button 1
my $eboxA = Gtk2::EventBox->new();
$eboxA->modify_bg('normal', $slblue);
$eboxA->set_size_request($EB_W,$EB_T);
my $labelA =  Gtk2::Label->new;
setBoldText($labelA, getCmd('1'));
$eboxA->add($labelA);
# Button 2
my $eboxB = Gtk2::EventBox->new();
$eboxB->modify_bg('normal', $slblue);
$eboxB->set_size_request($EB_W,$EB_T);
my $labelB =  Gtk2::Label->new;
setBoldText($labelB, getCmd('2'));
$eboxB->add($labelB);
# Button 3
my $eboxC = Gtk2::EventBox->new();
$eboxC->modify_bg('normal', $slblue);
$eboxC->set_size_request($EB_W,$EB_T);
my $labelC =  Gtk2::Label->new;
setBoldText($labelC, getCmd('3'));
$eboxC->add($labelC);
# Button 4
my $eboxD = Gtk2::EventBox->new();
$eboxD->modify_bg('normal', $slblue);
$eboxD->set_size_request($EB_W,$EB_T);
my $labelD =  Gtk2::Label->new;
setBoldText($labelD, getCmd('4'));
$eboxD->add($labelD);
# Button 5
my $eboxE = Gtk2::EventBox->new();
$eboxE->modify_bg('normal', $slblue);
$eboxE->set_size_request($EB_W,$EB_T);
my $labelE =  Gtk2::Label->new;
setBoldText($labelE, getCmd('5'));
$eboxE->add($labelE);
# Button 6
my $eboxF = Gtk2::EventBox->new();
$eboxF->modify_bg('normal', $slblue);
$eboxF->set_size_request($EB_W,$EB_T);
my $labelF =  Gtk2::Label->new;
setBoldText($labelF, getCmd('6'));
$eboxF->add($labelF);
# Progress bar X
my $pbarX = Gtk2::ProgressBar->new();
#$pbarX->set_size_request($EB_W,$EB_T);
$pbarX->set_size_request($PB_W,$PB_T);
$pbarX->set_text(getCmd('X'));
$pbarX->set_fraction(1.0);
#$pbarX->set_orientation("bottom_to_top");
$pbarX->set_orientation("top_to_bottom");
$pbarX->modify_bg('normal', $red);
$pbarX->modify_fg('normal', $slblue);
my $font_desc = Gtk2::Pango::FontDescription->from_string("Serif Bold SC + 12"); 
$font_desc->set_weight ('heavy');
$pbarX->modify_font($font_desc);

# Top horizontal box container
my $hboxTop = Gtk2::HBox->new( FALSE, 1 );
$hboxTop->set_size_request($W_W, $EB_T/3);

# Top horizontal box container
my $hboxMidLT = Gtk2::HBox->new( FALSE, 4 );
$hboxMidLT->set_size_request($W_W-$PB_W-30, $EB_T);
my $logo =  Gtk2::Image->new_from_file($logo_file);
#$hboxMidLT->pack_start( $logo, TRUE, FALSE, FALSE );
$hboxMidLT->pack_start( $eboxE, TRUE, FALSE, FALSE );
$hboxMidLT->pack_start( $eboxF, TRUE, FALSE, FALSE );

# Middle horizontal box container
my $hboxMidLB = Gtk2::HBox->new( FALSE, 5 );
#$hboxMidLB->set_size_request($W_W-$PB_W-30, $EB_T);
$hboxMidLB->set_size_request($W_W-$PB_W, $EB_T);
#pack_start(child, expand=True, fill=True, padding=0)
$hboxMidLB->pack_start( $eboxA, TRUE, FALSE, FALSE );
$hboxMidLB->pack_start( $eboxB, TRUE, FALSE, FALSE );
$hboxMidLB->pack_start( $eboxC, TRUE, FALSE, FALSE );
$hboxMidLB->pack_start( $eboxD, TRUE, FALSE, FALSE );
#$hboxMidLB->pack_start( $eboxE, TRUE, FALSE, FALSE );
#$hboxMidLB->pack_start( $eboxF, TRUE, FALSE, FALSE );

# Create a vertical box container
my $vboxMidL = Gtk2::VBox->new( TRUE, 5 );
$vboxMidL->pack_start( $hboxMidLT, TRUE, FALSE, FALSE );
$vboxMidL->pack_start( $hboxMidLB, TRUE, FALSE, FALSE );

# Middle horizontal box container
my $hboxMid = Gtk2::HBox->new( FALSE, 5 );
#>>> $hboxMid->set_size_request($W_W, $PB_T);
$hboxMid->pack_start( $vboxMidL, TRUE, FALSE, FALSE );
$hboxMid->pack_start( $pbarX, TRUE, FALSE, FALSE );

# Bottom horizontal box container
my $hboxBot = Gtk2::HBox->new( FALSE, 5 );
#$hboxBot->set_size_request($W_W, $EB_T/3);
#$hboxBot->set_size_request($W_W, $EB_T/2);
#my $labelBA =  Gtk2::Label->new;
#$labelBA->set_size_request($EB_W,$EB_T/2);
#setBoldText($labelBA, "A");
#my $labelBB =  Gtk2::Label->new;
#$labelBB->set_size_request($EB_W,$EB_T/2);
#setBoldText($labelBB, "B");
#my $labelBC =  Gtk2::Label->new;
#$labelBC->set_size_request($EB_W,$EB_T/2);
#setBoldText($labelBC, "C");
#my $labelBD =  Gtk2::Label->new;
#$labelBD->set_size_request($EB_W,$EB_T/2);
#setBoldText($labelBD, "D");
#my $labelBE =  Gtk2::Label->new;
#$labelBE->set_size_request($EB_W,$EB_T/2);
#setBoldText($labelBE, "E");
#my $labelBX =  Gtk2::Label->new;
#$labelBX->set_size_request($PB_W,$EB_T/2);
#setBoldText($labelBX, "X");
#$hboxBot->pack_start( $labelBA, TRUE, FALSE, FALSE );
#$hboxBot->pack_start( $labelBB, TRUE, FALSE, FALSE );
#$hboxBot->pack_start( $labelBC, TRUE, FALSE, FALSE );
#$hboxBot->pack_start( $labelBD, TRUE, FALSE, FALSE );
#$hboxBot->pack_start( $labelBE, TRUE, FALSE, FALSE );
#$hboxBot->pack_start( $labelBX, TRUE, FALSE, FALSE );

# Create a vertical box container
my $vbox = Gtk2::VBox->new( FALSE, 4 );
#>>> $vbox->pack_start( $hboxTop, FALSE, FALSE, FALSE );
$vbox->pack_start( $hboxMid, FALSE, FALSE, FALSE );
#>>> $vbox->pack_start( $hboxBot, FALSE, FALSE, FALSE );

# Add conteiner, show window and run main loop
$window->add($vbox);
$window->show_all;
Gtk2->main;

################################################################################
# Subs
################################################################################

# Print help message and exit
sub printHelpMess{
  my($helpreq) = @_;
  #GetOptions("help" => \$help ,"verbose" => \$verbose, "map=s" => \$cmd_map_file, "port=i" => \$PORTNO);
  if($helpreq){
    print "Simple GUI for OSC Foot Controller Bridge\n";
    print "Copyright 2011 Francisco Salomón\n";
    print "This is free software. You are welcome to redistribute it.\n";
    print "Options:\n";
    print "[help | h]       Show this help\n";
    print "[vervose | v]    Set verbose mode\n";
    print "[map | m] arg    Set commands map file (default: $cmd_map_file)\n";
    print "[port | p] arg   Set UDP input port (default: $PORTNO)\n";
    exit(0);
  }
}

# Callback for input socket
sub watch_insocket_callback {
  my ($fd,$condition,$sockin) = @_;
  my $newmsg;
  my $value = 1;
  my $TOOGLE_TIME = 0.2;
  if($sockin->recv($newmsg, $MAXLEN)){
    if($verbose){
      print "Data arrived from UDP: $newmsg\n";
    }
    my @cmdArrived = split(',',$newmsg);
    my ($button, $bank_name);
    $button = $cmdArrived[0];
    $bank_name = $cmdArrived[1];
    if(scalar(@cmdArrived)== 3){
      $value = $cmdArrived[2];
    }
    switch ($button) {
      case '1' { 
        toogle_ebox($eboxA, $TOOGLE_TIME);
      }
      case '2' { 
        toogle_ebox($eboxB, $TOOGLE_TIME);
      }
      case '3' { 
        toogle_ebox($eboxC, $TOOGLE_TIME);
      }
      case '4' { 
        toogle_ebox($eboxD, $TOOGLE_TIME);
      }
      case '5' { 
        toogle_ebox($eboxE, $TOOGLE_TIME);
      }
      case '6' { 
        toogle_ebox($eboxF, $TOOGLE_TIME);
      }
      case 'X' { 
        setvalue_pbar($pbarX, $value);
      }
    }
    if($bank_name ne getBankName()){
      if(!setSelBank($bank_name)){
        print "Bank selected is not in configured map file! Forced exit.\n";
        Gtk2->main_quit; 
        return FALSE;
      }
      else{
        if($verbose){
          print "Commands bank updated\n";
        }
        updateButLabs();
        $window->set_keep_above(TRUE);
      }
    }
  }
  return TRUE;
}

# Tooble button
sub toogle_ebox{
  my($ebox, $seconds) = @_;
  my $i = 0;
  $ebox->modify_bg('normal', $red);
  $SIG{ALRM} = sub { 
    $ebox->modify_bg('normal', $slblue);
  };
  alarm($seconds);
  return TRUE;
}

# Set Value button
sub setvalue_pbar{
  my($pbar, $value) = @_;
  #~ print "Value arrived to setvalue_pbar: $value.\n";
  $pbar->set_fraction(1 - $value/($EXPREESS_DIV-1));
  return TRUE;
}

# Get the name of the selected bank
sub getBankName{
  my $selected_bank = $BANKS[$SEL_BANK];  
  return $selected_bank->{NAME};
}

# Get the command name from the button name
sub getCmd{
  my($button)= @_;
  my $selected_bank = $BANKS[$SEL_BANK];  
  foreach (@{ $selected_bank->{CMDS} }){
    if($_->{BUTTON} eq $button){
      return $_->{NAME};
    } 
  }
}

# Get the index number of a bank from its name
sub setSelBank{
  my($bname)= @_;
  my $bindex=0;
  foreach (@BANKS){
    if($_->{NAME} eq $bname){
      $SEL_BANK = $bindex; 
      return TRUE;
    }
    $bindex=$bindex+1;
  }
  return FALSE;
}

# Set a label text in bold font
sub setBoldText{
  my($label, $text)= @_;
  #$label->set_markup("<b>$text</b>");
  my $font_desc = Gtk2::Pango::FontDescription->from_string("Serif Bold SC + 12"); 
  $font_desc->set_weight ('heavy');
  $label->modify_font($font_desc);
  $label->set_markup("<span color='white'><b>$text</b></span>");
  #$label->set_text($text);

}  

# Update all button's labels
sub updateButLabs{
  setBoldText($labelA, getCmd('1'));
  setBoldText($labelB, getCmd('2'));
  setBoldText($labelC, getCmd('3'));
  setBoldText($labelD, getCmd('4'));
  setBoldText($labelE, getCmd('5'));
  setBoldText($labelF, getCmd('6'));
  $pbarX->set_text(getCmd('X'));
}

