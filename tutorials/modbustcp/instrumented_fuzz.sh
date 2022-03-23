#!/bin/bash

# SCRIPT FOR TESTING AFLNET MODBUSTCP MODE

RAMDISKPATH="/tmp/afl-ramdisk"
SERVERSOURCE="random-test-server.c"
AFLNETPATH="aflnet"
IN="reqdump"
OUT="output"
IFACE="lo"
CURIP=$(ifconfig $IFACE | grep -Eo 'inet (addr:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*')
TARGETPORT="1502"
TARGET="tcp://$CURIP/$TARGETPORT"
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

apt install -y libmodbus-dev

if [ -f "$SERVERSOURCE" ]; then
  SERVERBIN=${SERVERSOURCE%.*}
  su -c "$AFLNETPATH/afl-gcc $SERVERSOURCE -o $SERVERBIN -I/usr/include/modbus/ -lmodbus" $(logname)
else
  echo "Server source not found"
  exit
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
cp -pr $SERVERBIN $RAMDISKPATH

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

su -c "$AFLNET/afl-fuzz -d -i $IN -o $OUT -N $TARGET -P MODBUSTCP -D 10000 -q 3 -s 3 -E -R -K $RAMDISKPATH/$SERVERBIN" $(logname)
