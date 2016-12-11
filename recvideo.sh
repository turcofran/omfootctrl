#!/bin/bash
#
# Script example for recording demo video
#
SL_SESSION=jackmaster3loop.slsess
#trap on_exit SIGINT SIGTERM EXIT

# On exit function; kill all launched stuffs
function on_exit(){
  pkill sooperlooper;
  pkill slgui;
  pkill mplayer;
  kill 0;
}

slgui --load-session=$SL_SESSION &
############################################################
# send select next loop 
sleep 2
send_osc 9951 /sl/-2/set select_next_loop 0.0
############################################################
mplayer tv:// -tv driver=v4l2   -geometry 320x240+10+400 &
sleep 3
#recordmydesktop --use-jack=system:capture_1 sooperlooper:common_out_1 --no-wm-check -o /tmp/out.ogv -x 0
recordmydesktop --use-jack=sooperlooper:common_out_1 --no-wm-check -o /home/fran/out.ogv -x 0
wait
exit
