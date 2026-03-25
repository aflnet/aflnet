# Tutorial - Fuzzing Modbus Server (libmodbus)

This tutorial guides you through the process of fuzzing a Modbus server using AFLNet. We will use the `unit-test-server` from the popular `libmodbus` library as our target.

## Step-0. Server Compilation & Setup

First, we need to download and compile `libmodbus` and its test server.

```bash
# Set working directory
export WORKDIR=$(pwd)
cd $WORKDIR

# Clone libmodbus repository
git clone https://github.com/stephane/libmodbus.git
cd libmodbus

# Build libmodbus and the test server
./autogen.sh
# Note: Use CC=afl-clang-fast or CC=afl-gcc to correctly instrument the target
CC=afl-clang-fast ./configure --enable-static --disable-shared
make

# The server executable is located at tests/unit-test-server
# We will use it for fuzzing.
```

## Step-1. Prepare Seed Inputs

AFLNet requires a set of valid requests (seed corpus) to start fuzzing. We have prepared a Python script to generate some basic Modbus/TCP packets.

1.  Make sure you have Python installed.
2.  Create a directory for the seeds:
    ```bash
    mkdir $WORKDIR/in-modbus
    ```
3.  Run the seed generator script (provided in this tutorial folder):
    ```bash
    python3 $AFLNET/tutorials/modbus/create_seed.py $WORKDIR/in-modbus
    ```

This will generate several `.raw` files in the `in-modbus` directory, each containing a valid Modbus/TCP request.

## Step-2. Fuzzing

Now we are ready to run AFLNet.

```bash
# Move to the tests directory where the server is located
cd $WORKDIR/libmodbus/tests

# Run afl-fuzz
# -P MODBUS: Specify the protocol
# -D 10000: Wait time (us)
# -q 3 -s 3: Queue and state selection strategies (optional but recommended)
# -E -R: Enable evolution and mutation strategies
# tcp://127.0.0.1/1502: The target address and port (unit-test-server listens on 1502 by default)

afl-fuzz -d -i $WORKDIR/in-modbus -o out-modbus -N tcp://127.0.0.1/1502 -P MODBUS -D 10000 -q 3 -s 3 -E -R ./unit-test-server tcp
```

**Note:** The `unit-test-server` takes an argument `tcp` to start in TCP mode. It listens on `127.0.0.1:1502` by default.


## Docker Support

We have provided a `Dockerfile` to simplify the process of setting up the environment.

1.  **Build the Docker image** (from the root of the repository):
    ```bash
    docker build -f tutorials/modbus/Dockerfile -t aflnet-modbus .
    ```

2.  **Run the fuzzer**:
    ```bash
    docker run -it aflnet-modbus
    ```

    This will automatically start fuzzing the Modbus server with the generated seeds.

## Cleanup

To clean up the generated files and directories (if running locally without Docker), you can use the provided script:

```bash
$AFLNET/tutorials/modbus/clean.sh
```
