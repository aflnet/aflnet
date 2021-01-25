# AFLNet: A Greybox Fuzzer for Network Protocols
AFLNet is a greybox fuzzer for protocol implementations. Unlike existing protocol fuzzers, it takes a mutational approach and uses state-feedback, in addition to code-coverage feedback, to guide the fuzzing process. AFLNet is seeded with a corpus of recorded message exchanges between the server and an actual client. No protocol specification or message grammars are required. It acts as a client and replays variations of the original sequence of messages sent to the server and retains those variations that were effective at increasing the coverage of the code or state space. To identify the server states that are exercised by a message sequence, AFLNet uses the server’s response codes. From this feedback, AFLNet identifies progressive regions in the state space, and systematically steers towards such regions.

# Licences

AFLNet is licensed under [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

AFLNet is an extension of [American Fuzzy Lop](http://lcamtuf.coredump.cx/afl/) written and maintained by Michał Zalewski <<lcamtuf@google.com>>. For details on American Fuzzy Lop, we refer to [README-AFL.md](README-AFL.md).

* **AFL**: [Copyright](https://github.com/aflsmart/aflsmart/blob/master/docs/README) 2013, 2014, 2015, 2016 Google Inc. All rights reserved. Released under terms and conditions of [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0).

# Citing AFLNet
AFLNet has been accepted for publication as a Testing Tool paper at the IEEE International Conference on Software Testing, Verification and Validation (ICST) 2020. 

```
@inproceedings{AFLNet,
author={Van{-}Thuan Pham and Marcel B{\"o}hme and Abhik Roychoudhury},
title={AFLNet: A Greybox Fuzzer for Network Protocols},
booktitle={Proceedings of the 13rd IEEE International Conference on Software Testing, Verification and Validation : Testing Tools Track},
year={2020},}
```

# Installation (Tested on Ubuntu 18.04 & 16.04 64-bit)

## Prerequisites

```bash
# Install clang (as required by AFL/AFLNet to enable llvm_mode)
sudo apt-get install clang
# Install graphviz development
sudo apt-get install graphviz-dev
```

## AFLNet

Download AFLNet and compile it. We have tested AFLNet on Ubuntu 18.04 and Ubuntu 16.04 64-bit and it would also work on all environments that support the vanilla AFL and [graphviz](https://graphviz.org).

```bash
# First, clone this AFLNet repository to a folder named aflnet
git clone <links to the repository> aflnet
# Then move to the source code folder
cd aflnet
make clean all
cd llvm_mode
# The following make command may not work if llvm-config cannot be found
# To fix this issue, just set the LLVM_CONFIG env. variable to the specific llvm-config version on your machine
# On Ubuntu 18.04, it could be llvm-config-6.0 if you have installed clang using apt-get
make
# Move to AFLNet's parent folder
cd ../..
export AFLNET=$(pwd)/aflnet
export WORKDIR=$(pwd)
```

## Setup PATH environment variables

```bash
export PATH=$AFLNET:$PATH
export AFL_PATH=$AFLNET
```

# Usage

AFLNet adds the following options to AFL. Run ```afl-fuzz --help``` to see all options. Please also see the FAQs section for common questions about these AFLNet's options.

- ***-N netinfo***: server information (e.g., tcp://127.0.0.1/8554)

- ***-P protocol***: application protocol to be tested (e.g., RTSP, FTP, DTLS12, DNS, DICOM, SMTP, SSH, TLS, DAAP-HTTP, SIP)

- ***-D usec***: (optional) waiting time (in microseconds) for the server to complete its initialization 

- ***-K*** : (optional) send SIGTERM signal to gracefully terminate the server after consuming all request messages

- ***-E*** : (optional) enable state aware mode

- ***-R*** : (optional) enable region-level mutation operators

- ***-F*** : (optional) enable false negative reduction mode

- ***-c script*** : (optional) name or full path to a script for server cleanup

- ***-q algo***: (optional) state selection algorithm (e.g., 1. RANDOM_SELECTION, 2. ROUND_ROBIN, 3. FAVOR)

- ***-s algo***: (optional) seed selection algorithm (e.g., 1. RANDOM_SELECTION, 2. ROUND_ROBIN, 3. FAVOR)


Example command: 
```bash
afl-fuzz -d -i in -o out -N <server info> -x <dictionary file> -P <protocol> -D 10000 -q 3 -s 3 -E -K -R <executable binary and its arguments (e.g., port number)>
```

# Tutorial - Fuzzing Live555 media streaming server

[Live555 Streaming Media](http://live555.com) is a C++ library for multimedia streaming. The library supports open protocols such as RTP/RTCP and RTSP for streaming. It is used internally by widely-used media players such as [VLC](https://videolan.org) and [MPlayer](http://mplayerhq.hu) and some security cameras & network video recorders (e.g., [DLink D-View Cameras](http://files.dlink.com.au/products/D-ViewCam/REV_A/Manuals/Manual_v3.51/D-ViewCam_DCS-100_B1_Manual_v3.51(WW).pdf), [Senstar Symphony](http://cdn.aimetis.com/public/Library/Senstar%20Symphony%20User%20Guide%20en-US.pdf), [WISENET Video Recorder](https://www.eos.com.au/pub/media/doc/wisenet/Manuals_QRN-410S,QRN-810S,QRN-1610S_180802_EN.pdf)). In this example, we show how AFLNet can be used to fuzz Live555 and discover bugs in its RTSP server reference implementation (testOnDemandRTSPServer). Similar steps would be followed to fuzz servers implementing other protocols (e.g., FTP, SMTP, SSH).

If you want to run some experiments quickly, please take a look at [ProFuzzBench](https://github.com/profuzzbench/profuzzbench). ProFuzzBench includes a suite of representative open-source network servers for popular protocols (e.g., TLS, SSH, SMTP, FTP, SIP), and tools to automate experimentation.

## Step-0. Server and client compilation & setup

The newest source code of Live555 can be downloaded as a tarball at [Live555 public page](http://live555.com/liveMedia/public/). There is also [a mirror of the library](https://github.com/rgaufman/live555) on GitHub. In this example, we choose to fuzz an [old version of Live555](https://github.com/rgaufman/live555/commit/ceeb4f462709695b145852de309d8cd25e2dca01) which was commited to the repository on August 28th, 2018. While fuzzing this specific version of Live555, AFLNet exposed four vulnerabilites in Live555, two of which were zero-day. To compile and setup Live555, please use the following commands.

```bash
cd $WORKDIR
# Clone live555 repository
git clone https://github.com/rgaufman/live555.git
# Move to the folder
cd live555
# Checkout the buggy version of Live555
git checkout ceeb4f4
# Apply a patch. See the detailed explanation for the patch below
patch -p1 < $AFLNET/tutorials/live555/ceeb4f4.patch
# Generate Makefile
./genMakefiles linux
# Compile the source
make clean all
```

As you can see from the commands, we apply a patch to make the server effectively fuzzable. In addition to the changes for generating a Makefile which uses afl-clang-fast++ to do the coverage feedback-enabled instrumentation, we make a small change to disable random session ID generation in Live555. In the unmodified version of Live555, it generates a session ID for each connection and the session ID should be included in subsequent requests sent from the connected client. Otherwise, the requests are quickly rejected by the server and this leads to undeterministic paths while fuzzing. Specifically, the same message sequence could exercise different server paths because the session ID is changing. We handle this specific issue by modifing Live555 in such a way that it always generates the same session ID.

Once Live555 source code has been successfully compiled, we should see the server under test (testOnDemandRTSPServer) and the sample RTSP client (testRTSPClient) placed inside the testProgs folder. We can test the server by running the following commands.

```bash
# Move to the folder keeping the RTSP server and client
cd $WORKDIR/live555/testProgs
# Copy sample media source files to the server folder
cp $AFLNET/tutorials/live555/sample_media_sources/*.* ./
# Run the RTSP server on port 8554
./testOnDemandRTSPServer 8554
# Run the sample client on another screen/terminal
./testRTSPClient rtsp://127.0.0.1:8554/wavAudioTest
```

We should see the outputs from the sample client showing that it successfully connects to the server, sends requests and receives responses including streaming data from the server.

## Step-1. Prepare message sequences as seed inputs

AFLNet takes message sequences as seed inputs so we first capture some sample usage scenarios between the sample client (testRTSPClient) and the server under test (SUT). The following steps show how we prepare a seed input for AFLNet based on a usage scenario in which the server streams an audio file in WAV format to the client upon requests. The same steps can be followed to prepare other seed inputs for other media source files (e.g., WebM, MP3).

We first start the server under test

```bash
cd $WORKDIR/live555/testProgs
./testOnDemandRTSPServer 8554
```

After that, we ask [tcpdump data-network packet analyzer](https://www.tcpdump.org) to capture all traffics through the port opened by the server, which is 8554 in this case. Note that you may need to change the network interface that works for your setup using the ```-i``` option.

```bash
sudo tcpdump -w rtsp.pcap -i lo port 8554
```

Once both the server and tcpdump have been started, we run the sample client

```bash
cd $WORKDIR/live555/testProgs
./testRTSPClient rtsp://127.0.0.1:8554/wavAudioTest
```

When the client completes its execution, we stop tcpdump. All the requests and responses in the communication between the client and the server should be stored in the specified rtsp.pcap file. Now we use [Wireshark network analyzer](https://wireshark.org) to extract only the requests and use the request sequence as a seed input for AFLNet. Please install Wireshark if you haven't done so.

We first open the PCAP file with Wireshark.

```bash
wireshark rtsp.pcap
```

This is a screenshot of Wireshark. It shows packets (requests and responses) in multiple rows, one row for one packet.

![Analyzing the pcap file with Wireshark](tutorials/live555/images/rtsp_wireshark_1.png)

To extract the request sequence, we first do a right-click and choose Follow->TCP Stream.

![Follow TCP Stream](tutorials/live555/images/rtsp_wireshark_2.png)

Wireshark will then display all requests and responses in plain text.

![View requests and responses in plain text](tutorials/live555/images/rtsp_wireshark_3.png)

As we are only interested in the requests for our purpose, we choose incoming traffic to the SUT-opened port by selecting an option from the bottom-left drop-down list. We choose ```127.0.0.1:57998->127.0.0.1:8554``` in this example which askes Wireshark to display all request messages sent to port 8554.

![View requests in plain text](tutorials/live555/images/rtsp_wireshark_4.png)

Finally, we switch the data mode so that we can see the request sequence in raw (i.e., binary) mode. Click "Save as" and save it to a file, say rtsp_requests_wav.raw.

![View and save requests in raw binary](tutorials/live555/images/rtsp_wireshark_5.png)

The newly saved file rtsp_requests_wav.raw can be fed to AFLNet as a seed input. You can follow the above steps to create other seed inputs for AFLNet, say rtsp_requests_mp3.raw and so on. We have prepared a ready-to-use seed corpus in the tutorials/live555/in-rtsp folder.

## Step-2. Make modifications to the server code (optional)

Fuzzing network servers is challenging and in several cases, we may need to slightly modify the server under test to make it (effectively and efficiently) fuzzable. For example, this [blog post](http://www.vegardno.net/2017/03/fuzzing-openssh-daemon-using-afl.html) shows several modifications to OpenSSH server to improve the fuzzing performance including disable encryption, disable MAC and so on. In this tutorial, the RTSP server uses the same response code ```200``` for all successful client requests, no matter what actual server state is. So to make fuzzing more effective, we can apply [this simple patch](tutorials/live555/ceeb4f4_states_decomposed.patch) that decomposes the big state 200 into smaller states. It makes the inferred state machine more fine grained and hence AFLNet has more information to guide the state space exploration.

## Step-3. Fuzzing

```bash
cd $WORKDIR/live555/testProgs
afl-fuzz -d -i $AFLNET/tutorials/live555/in-rtsp -o out-live555 -N tcp://127.0.0.1/8554 -x $AFLNET/tutorials/live555/rtsp.dict -P RTSP -D 10000 -q 3 -s 3 -E -K -R ./testOnDemandRTSPServer 8554
```

Once AFLNet discovers a bug (e.g., a crash or a hang), a test case containing the message sequence that triggers the bug will be stored in ```replayable-crashes``` or ```replayable-hangs``` folder. In the fuzzing process, AFLNet State Machine Learning component keeps inferring the implmented state machine of the SUT and a .dot file (ipsm.dot) is updated accordingly so that the user can view that file (using a .dot viewer like xdot) to monitor the current progress of AFLNet in terms of protocol inferencing. Please read the AFLNet paper for more information.

## Step-4. Reproducing the crashes found

AFLNet has an utility (aflnet-replay) which can replay message sequences stored in crash and hang-triggering files (in ```replayable-crashes``` and ```replayable-hangs``` folders). Each file is structured in such a way that aflnet-replay can extract messages based on their size. aflnet-replay takes three parameters which are 1) the path to the test case generated by AFLNet, 2) the network protocol under test, and 3) the server port number. The following commands reproduce a PoC for [CVE-2019-7314](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2019-7314).

```bash
cd $WORKDIR/live555/testProgs
# Start the server
./testOnDemandRTSPServer 8554
# Run aflnet-replay
aflnet-replay $AFLNET/tutorials/live555/CVE_2019_7314.poc RTSP 8554
```

To get more information about the discovered bug (e.g., crash call stack), you can run the buggy server with [GDB](https://gnu.org/software/gdb) or you can apply the Address Sanitizer-Enabled patch ($AFLNET/tutorials/live555/ceeb4f4_ASAN.patch) and recompile the server before running it. 

# FAQs

## 1. How do I extend AFLNet?

AFLNet has a modular design that makes it easy to be extended.

### 1.1. How do I add support for another protocol?

If you want to support another protocol, all you need is to follow the steps below.

#### Step-1. Implement 2 functions to parse the request and response sequences

You can use the available ```extract_requests_*``` and ```extract_response_codes_*``` functions as references. These functions should be declared and implemented in [aflnet.h](aflnet.h) and [aflnet.c](aflnet.c), respectively. Note that, please use the same function parameters.

#### Step-2. Update main function to support a new protocol

Please update the code that handles the ```-P``` option in the main function to support a new protocol.

### 1.2. How do I implement another search strategy?

It is quite straightforward. You just need to update the two functions ```choose_target_state``` and ```choose_seed```. The function ```update_scores_and_select_next_state``` may need an extension too. 

## 2. What happens if I don't enable the state-aware mode by adding -E option?

If ```-E``` is not enabled, even though AFLNet still manages the requests' boundaries information so it can still follow the sequence diagram of the protocol -- sending a request, waiting for a response and so on, which is not supported by normal networked-enabled AFL. However, in this setup AFLNet will ignore the responses and it does not construct the state machine from the response codes. As a result, AFLNet cannot use the state machine to guide the exploration.

## 3. When I need -c option and what I should write in the cleanup script?

You may need to provide this option to keep network fuzzing more deterministic. For example, when you fuzz a FTP server you need to clear all the files/folders created in the previous fuzzing iteration in the shared folder because if you do not do so, the server will not be able to create a file if it exists. It means that the FTP server will work differently when it receives the same sequence of requests from the client, which is AFLNet in this fuzzing setup. So basically the script should include commands to clean the environment affecting the behaviors of the server and give the server a clean environment to start.

## 4. What is false-negative reduction mode and when I should enable it using -F?

Unlike stateless programs (e.g., image processing libraries like LibPNG), several stateful servers (e.g., the RTSP server in the above tutorial) do not terminate themselves after consuming all requests from the client, which is AFLNet in this fuzzing setup. So AFLNet needs to gracefully terminate the server by sending the SIGTERM signal (when -K is specified). Otherwise, AFLNet will detect normal server executions as hangs. However, the issue is that if AFLNet sends SIGTERM signal too early, say right after all request messages have been sent to the server, the server may be forced to terminate when it is still doing some tasks which may lead to server crashes (i.e., false negatives -- the server crashes are missed). The false-negative reduction mode is designed to handle such situations. However, it could slow down the fuzzing process leading to slower execution speed.


