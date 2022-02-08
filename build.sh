#!/bin/bash

TARGET="./"
DEV="/dev/ttyUSB0"

if [ -e /dev/ttyUSB0 ] ; then 
    if [ -e /dev/ttyUSB1 ] ; then 
        if [ -z "$2" ] ; then 
            echo "Error: Multiple USB TTYs found. Please specify."
            usage
            exit -1
        else
            DEV="$2"
        fi
    fi
fi

if [ "$1" == "-p" ] ; then 
    time arduino-cli compile -b esp32:esp32:firebeetle32 -u -p "$DEV" "$TARGET" && echo "OK"
else
    time arduino-cli compile -b esp32:esp32:firebeetle32  && echo "OK"
fi
