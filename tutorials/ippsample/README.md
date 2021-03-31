# Tutorial - Fuzzing ippsample server

Internet Printing Protocol (IPP) is a protocol for communicating between computers and printers (or print servers).
We assume that you have read the AFLNet README.md which includes a detailed tutorial for fuzzing Live555 RTSP server before reading this tutorial.

As described from [RFC 2910](https://tools.ietf.org/html/rfc2910), an IPP operation is inclued in a HTTP message body.
All client requests use the HTTP POST method.
The server can send an intermediate response (HTTP 100 Continue) without IPP data. If the HTTP status code is 200 OK, the response contains the IPP message, and consequently the status-code field (byte 3 and 4 of the IPP message).
IPP response codes can be found [here](https://www.iana.org/assignments/ipp-registrations/ipp-registrations.xhtml).

## Step-0. Server compilation & setup

In this example, we choose to fuzz a [specific version of ippsample](https://github.com/istopwg/ippsample/commit/1ee7bcd4d0ed0e1e49b434c0ab296bb0c9499c0d), largely based upon on CUPS. To compile and setup ippsample for fuzzing, please use the following commands.

```bash
export WORKDIR=$(pwd)
cd $WORKDIR
# Install requirements
sudo apt-get -qq update
sudo apt-get install -y build-essential autoconf avahi-daemon avahi-utils cura-engine libavahi-client-dev libfreetype6-dev libgnutls28-dev libharfbuzz-dev libjbig2dec0-dev libjpeg-dev libmupdf-dev libnss-mdns libopenjp2-7-dev libpng-dev zlib1g-dev net-tools iputils-ping vim avahi-daemon tcpdump man curl git
# Clone ippsample repository
git clone https://github.com/istopwg/ippsample.git ippsample
# Move to the folder
cd ippsample
# Checkout a specific version
git checkout 1ee7bcd4d0ed0e1e49b434c0ab296bb0c9499c0d
# Compile source
CC=$AFLNET/afl-clang ./configure
make clean all
```

Once ippsample source code has been successfully compiled, we should see the server under test (ippserver) in the server folder. We can test the server by running the following commands using the client ipptool (tools folder).
I strongly suggest you to create a RAM disk for the printing spooler.

```bash
# Create a RAM disk
mkdir /tmp/afl-ramdisk
chmod 777 /tmp/afl-ramdisk
sudo mount -t tmpfs -o size=512M tmpfs /tmp/afl-ramdisk
export AFL_TEMP=/tmp/afl-ramdisk
mkdir $AFL_TEMP/spool
# Move to the folder containing ippserver
cd $WORKDIR/ippsample/server
# Run ippserver on port 631 (-p), adding MIME type support for text/plain (-f), spool in RAM disk (-d), with verbose output (-vvvv)
# You may need to run the following command with sudo
# WARNING: if you have cups installed, you should first stop it
./ippserver -p 631 -f text/plain -d $AFL_TEMP/spool -vvvv printerName 
# If you have problem starting ippserver, you should need to start following services:
# sudo service dbus start && sudo service avahi-daemon
# In this example we try to print a txt and to cancel the printing job
# From another terminal, move to the folder containing ipptool (client)
cd $WORKDIR/ippsample/tools
# To get the URI of the printer(s), run ippfind
./ippfind
# Record traffic data with tcpdump
sudo tcpdump -i lo port 631 -w printAndCancelCurrentJobReq.pcap
# Run client to send a print request for the txt file (-f), produce a test report (-t), verbose output (-v), followed by the URI of the printer
./ipptool -f ../examples/testfile.txt -t -v ipp://127.0.0.1:631/ipp/print ../examples/print-job.test 
# Run client to send a cancel job request for the job number 1 (-d), produce a test report (-t), verbose output (-v), followed by the URI of the printer
# You should run the following command immediately after the previus one
# If you aren't fast enough, you can edit the ../examples/print-job.test file to print more copies, for example 100: ATTR integer copies 100
./ipptool -t -v -d job-id=1 ipp://127.0.0.1:631/ipp/print ../examples/cancel-job.test
# If you get an error for job-id 1, stop the server and start it again, or change the job-id value in -d.
```

We should see the outputs showing that it successfully connected to the server, sending the printing job and cancelling it.

## Step-1. Prepare message sequences as seed inputs

We have prepared a seed corpus to fuzz ippserver. If you want to create your own seed corpus, please follow the tutorial for fuzzing Live555 RTSP server included in the main AFLNet README.md.
In this case we have 2 seed inputs, one for the print request and one for the cancel job.

## Step-2. Fuzzing

```bash
cd $WORKDIR/ippsample/server
cp $AFLNET/tutorials/ippsample/ippcleanup.sh ./
chmod +x ippcleanup.sh
# Edit the ippcleanup.sh with the spool directory you choosed (/tmp/afl-ramdisk/spool in this case)
# You may need to run the following command with sudo
afl-fuzz -d -i $AFLNET/tutorials/ippsample/in-ipp/ -o out-ipp/ -N tcp://127.0.0.1/631 -x $AFLNET/tutorials/ippsample/ipp.dict -P IPP -D 100000 -t 2000 -q 3 -s 3 -E -K -R -m 150 -c ippcleanup.sh ./ippserver -p 631 -f text/plain -d /tmp/afl-ramdisk/spool printerName
```
