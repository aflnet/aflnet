#!/bin/bash

# Reset script for net_only mode
# Handle server each time run_target is called
# reset if exist? keep running?

RAMDISKPATH="/tmp/afl-ramdisk"
SERVERSCRIPT="server.py"
CONSOLE="x-terminal-emulator"
IFACE="lo"
TARGETPORT="5020"

nohup python $RAMDISKPATH/$SERVERSCRIPT $IFACE $TARGETPORT >/dev/null 2>&1 &
PID=$!

while ! kill -0 $PID 2>/dev/null
do
  nohup python $RAMDISKPATH/$SERVERSCRIPT $IFACE $TARGETPORT >/dev/null 2>&1 &
  PID=$!
done
