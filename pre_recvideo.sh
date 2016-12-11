#!/bin/bash
#
# Script example for pre recording demo video
#

hydrogen &
amsynth &
cd src; ./cvOM -d 1 
wait
cd ..
exit
