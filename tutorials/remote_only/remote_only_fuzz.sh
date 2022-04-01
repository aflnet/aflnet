#!/bin/bash

# SCRIPT FOR AFLNET NET_ONLY MODE

RAMDISKPATH="/tmp/afl-ramdisk"
SERVERSCRIPT="server.py"
AFLNETPATH="aflnet"
IN="reqdump"
OUT="output"
CONSOLE="x-terminal-emulator"
IFACE="lo"
CURIP=$(ifconfig $IFACE | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*')
TARGETPORT="5020"
TARGET="tcp://$CURIP/$TARGETPORT"
RUNSERVSCRIPT="remote_only_runserver.sh"
#TCPDUMPLOGDIR="/tmp"

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

if [ ! -d $AFLNETPATH ]
then
  echo "$AFLNETPATH is not a directory, clone AFLNET to $AFLNETPATH"
  exit
else
  if [ ! -f "$AFLNETPATH/afl-fuzz" ]; then
    CURDIR=$(pwd)
    cd $AFLNETPATH
    su -c "make clean all" $(logname)
    cd $CURDIR
  fi
fi

if [ ! -d $IN ]
then
  echo "Seed directory $IN is not valid"
  exit
fi

if [ -d $RAMDISKPATH ]
then
  if [[ $(mount | grep $RAMDISKPATH) ]]
  then
    umount -f $RAMDISKPATH
  fi
else
  su -c "mkdir -p $RAMDISKPATH && chmod 777 $RAMDISKPATH" $(logname)
fi

mount -t tmpfs -o size=2048M tmpfs $RAMDISKPATH
cp -pr $AFLNETPATH $RAMDISKPATH
cp -pr $IN $RAMDISKPATH
cp -pr $SERVERSCRIPT $RAMDISKPATH
cp -pr $RUNSERVSCRIPT $RAMDISKPATH

cd $RAMDISKPATH
export AFLNET=$(pwd)/aflnet
export WORKDIR=$(pwd)
export PATH=$PATH:$AFLNET
export AFL_PATH=$AFLNET
export AFLNET_DEBUG=1

if [ ! -z "$TCPDUMPLOGDIR" ]
then
  if [ ! -d $TCPDUMPLOGDIR ]
  then
    su -c "mkdir -p $TCPDUMPLOGDIR && chmod 777 $TCPDUMPLOGDIR" $(logname)
  fi
  TCPDUMPFILE="$TCPDUMPLOGDIR/$(date +%s).pcap"
  echo "Dumping trafic to $TCPDUMPFILE"
  nohup tcpdump -i $IFACE -w $TCPDUMPFILE port $TARGETPORT >/dev/null 2>&1 &
  TCPDUMPPID=$!
  echo "Tcpdump running with pid $TCPDUMPPID"
fi

function cleanup {
  if [ -z "$TCPDUMPPID" ]
  then
    kill -9 "$TCPDUMPPID"
  fi
}
trap cleanup EXIT
#sleep 2

su -c "$AFLNET/afl-fuzz -d -i $IN -o $OUT -N $TARGET -P MODBUSTCP -D 10000 -q 3 -s 3 -E -R -r -W 5 -e $WORKDIR/$RUNSERVSCRIPT" $(logname)
