#!/bin/bash

THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

##### cd ${THIS_DIR}/../GLOBAL-TEMP-DISPLAY.d/SERVER-BACKEND.d

if [ $# == "1" ]; then
    argPort=$1
else
    argPort=5678
fi

##### while [ 1 ]; do

   echo
   echo Will be listening on port $argPort
   echo
   echo
   ${THIS_DIR}/anomaly.exe -G 4 -M ${THIS_DIR}/v4_ABC_airports.inv -P $argPort -B 1 -S 5:5 -W ${THIS_DIR}/v4_raw.dat ${THIS_DIR}/v4_adj.dat

   echo
   echo The backend server program died...
   echo Probable cause -- could not listen on port $argPort
   echo
   echo

#####    echo -n "Enter another port number (1025-32767):  "
#####    read argPort
#####    echo
#####    echo Will try to set up shop on port $argPort
#####    echo
##### done 

   
