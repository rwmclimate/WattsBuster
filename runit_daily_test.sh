#!/bin/bash

trap 'echo "[${BASH_SOURCE}:${LINENO}] $BASH_COMMAND"; read -p "Continue?"' DEBUG

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
   ${THIS_DIR}/anomaly.exe -D 2:0:15 -M ${THIS_DIR}/ghcndaily_metadata.inv -P $argPort -B 1 -s -S 5:5 -W /home/eoslab/TEMP-DATA.d/GHCN.d/DAILY.d/ghcnd_all_tmin_tmax.dly

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

   
