
# Open62541 Fuzzing Setup

This repository sets up fuzz testing for the Open62541 OPC UA server using AFLNet. Follow the instructions below to clone, configure, and fuzz test the server.

## Clone the repository

Start by cloning the Open62541 repository and checking out a specific commit:

```bash
git clone https://github.com/open62541/open62541.git
cd open62541
git checkout 46d0395
git submodule update --init --recursive
```

## Copy modified `server_ctt.c` for anonymous user authentication

Replace the default `server_ctt.c` with the modified version that supports anonymous user authentication:

```bash
cp ../server_ctt.c examples/
```

## Build Open62541 with examples

Configure and build Open62541 with AFLNet instrumentation and example servers:

```bash
AFL_HARDEN=1 cmake -DCMAKE_C_COMPILER=afl-clang-fast -DCMAKE_CXX_COMPILER=afl-clang-fast++ -DBUILD_SHARED_LIBS=ON -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_ENCRYPTION=OFF ../open62541
make -j1
make install
```

## Copy the server

Once the build is complete, copy the compiled `server_ctt` binary for fuzzing:

```bash
cd ..
cp open62541/bin/examples/server_ctt .
```

## Fuzzing

Run AFLNet to start fuzzing the `server_ctt` binary:

```bash
afl-fuzz -i in-opcua -o out/<name> -N tcp://127.0.0.1/4840 -P OPCUA -q 3 -s 3 -E -K -R ./server_ctt
```

### Parameters

- `-i in-opcua`: Specifies the input directory containing initial test cases.
- `-o out/<name>`: Specifies the output directory for fuzzing results.
- `-N tcp://127.0.0.1/4840`: Points to the TCP server running on port `4840`.
- `-P OPCUA`: Specifies the protocol (OPC UA) for fuzzing.
- `-q 3`, `-s 3`: AFL fuzzing configuration options.
- `-E`, `-K`, `-R`: Advanced AFLNet options for experimental features, server restarts, and deterministic fuzzing.
