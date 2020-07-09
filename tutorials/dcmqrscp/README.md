# Tutorial - Fuzzing dcmqrscp server

This assumes that you have read the AFLNet README.md before reading this tutorial. 

This tutorial was tested on Ubuntu 18.04.

## Step-0. Server compilation

You can download the source code of dcmqrscp from the [Official DCMTK GitHub Mirror](https://github.com/DCMTK/dcmtk).
In this example, we will fuzz a [specific version of dcmqrscp](https://github.com/DCMTK/dcmtk/commit/7f8564cf11e5531689dd329523fb16023aeda3ed). To compile and set up this version of `dcmqrscp` for fuzzing, please use the following commands.


```bash
export WORKDIR=$(pwd)
cd $WORKDIR

# Clone a specific version of dcmtk repository
git clone https://github.com/DCMTK/dcmtk
cd dcmtk
git checkout 7f8564c

# Apply a patch to change C/C++ compiler to afl-clang-fast/afl-clang-fast++
patch -p1 < $AFLNET/tutorials/dcmqrscp/7f8564c.patch

# Compile source
mkdir build && cd build
cmake ..
make dcmqrscp
```

When `dcmqrscp` server source code has been successfully compiled, the hardest part starts. You need to setup the server and check that everything works correctly.

## Step-1. Server setup

To setup `dcmqrscp` server follow commands:

- Initialize DICOM database:
    ```bash
    cd $WORKDIR/dcmtk/build/bin

    # Create directory for DICOM database
    mkdir ACME_STORE
    ```

- Create configuration file:
    ```bash
    cp $AFLNET/tutorials/dcmqrscp/dcmqrscp.cfg ./
    ```

    Fix paths to `ACME_STORE` directory in configuration file `dcmqrscp.cfg`(line: 69).

- Start `dcmqrscp`.
    ```bash
    export DCMDICTPATH=$WORKDIR/dcmtk/dcmdata/data/dicom.dic
    ./dcmqrscp
    ```

 - Check that everything works correctly:
    ```bash
    # Open new terminal.
    # Install DICOM toolkit:
    sudo apt install dcmtk

    # Test connection:
    echoscu -v localhost 5158
    storescu -v localhost 5158 $AFLNET/tutorials/dcmqrscp/test.dcm
    findscu -P localhost 5158 $AFLNET/tutorials/dcmqrscp/query.dcm
    getscu localhost 5158 $AFLNET/tutorials/dcmqrscp/query.dcm
    ```

    If everything works correctly, `dcmqrscp` won't write any logs.

## Step-2. Prepare message sequences as seed inputs
You can find various DICOM queries to use as seed inputs in the `aflnet/tutorials/dcmqrscp/in-dicom` directory. If you want to create your own inputs, please follow the tutorial for fuzzing Live555 RTSP server included in the main AFLNet README.md.

## Step-3. Fuzzing

```bash
cd $WORKDIR/dcmtk/build/bin
afl-fuzz -d -i $AFLNET/tutorials/dcmqrscp/in-dicom -o out-dicom -N tcp://127.0.0.1/5158 -P DICOM -D 10000 -E -K -R ./dcmqrscp
```

With this particular version of `dcmqrscp`, you should get a few crashes after waiting long enough.

![ ](dcmqrscp_fuzz.png  "Fuzzing Output" )
