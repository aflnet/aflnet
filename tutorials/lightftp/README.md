# Tutorial - Fuzzing LightFTP server

We assume that you have read the AFLNet README.md which includes a detailed tutorial for fuzzing Live555 RTSP server before reading this tutorial.

## Step-0. Server compilation & setup

The newest source code of LightFTP can be downloaded as a tarball at [LightFTP github repository](https://github.com/hfiref0x/LightFTP). In this example, we choose to fuzz a [specific version of LightFTP](https://github.com/hfiref0x/LightFTP/commit/5980ea1a0ee0e5c3015275f93445626f8c25c83a). To compile and setup LightFTP for fuzzing, please use the following commands.

```bash
export WORKDIR=$(pwd)
cd $WORKDIR
# Install gnutls as required by LightFTP
sudo apt-get install -y libgnutls-dev
# Clone LightFTP repository
git clone https://github.com/hfiref0x/LightFTP.git
# Move to the folder
cd LightFTP
# Checkout a specific version
git checkout 5980ea1
# Apply a patch to handle multi-threaded server
patch -p1 < $AFLNET/tutorials/lightftp/5980ea1.patch
# Move to the source folder
cd Source/Release
# Compile source
CC=afl-clang-fast make clean all
# Copy configuration file
cp $AFLNET/tutorials/lightftp/fftp.conf ./
# Setup certificate and shared folder
# In this tutorial, we have created ready-to-use certificate files
# Please look at the LightFTP document if you want to create these files yourself
cp -r $AFLNET/tutorials/lightftp/certificate ~/
# We assume that the home folder is /home/ubuntu
# If you want to use another folder, you need to update the fftp.conf file accordingly
mkdir ~/ftpshare
```

Once LightFTP source code has been successfully compiled, we should see the server under test (fftp). We can test the server by running the following commands using the default ftp client installed on Ubuntu or using Telnet.

```bash
# Move to the folder keeping fftp server
cd $WORKDIR/LightFTP/Source/Release
# Run LightFTP on port 2200
./fftp fftp.conf 2200
# Telnet to port 2200 (on another terminal)
# And use FTP commands (USER, PASS, ...) to login and do other stuffs
# The default user name and password are both ubuntu
telnet 127.0.0.1 2200
```

We should see the outputs from Telnet showing that it successfully connects to the server.

## Step-1. Prepare message sequences as seed inputs

We have prepared a seed corpus to fuzz LightFTP. If you want to create your own seed corpus, please follow the tutorial for fuzzing Live555 RTSP server included in the main AFLNet README.md.
## Step-2. Fuzzing

```bash
cd $WORKDIR/LightFTP/Source/Release
cp $AFLNET/tutorials/lightftp/ftpclean.sh ./
afl-fuzz -d -i $AFLNET/tutorials/lightftp/in-ftp -o out-lightftp -N tcp://127.0.0.1/2200 -x $AFLNET/tutorials/lightftp/ftp.dict -P FTP -D 10000 -q 3 -s 3 -E -R -c ./ftpclean.sh ./fftp fftp.conf 2200
```

