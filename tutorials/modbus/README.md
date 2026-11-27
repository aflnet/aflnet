# Fuzzing libmodbus unit-test-server (Modbus/TCP)

## Download and compile libmodbus v3.2.0
```bash
apt-get install autoconf automake libtool
cd $WORKDIR
git clone https://github.com/stephane/libmodbus.git
cd libmodbus
git checkout v3.2.0
./autogen.sh
CC=afl-gcc ./configure --enable-static --disable-shared
make
```
`CC=afl-gcc` has to be given to `configure`, and `--disable-shared` makes `tests/unit-test-server` a statically linked, instrumented binary instead of a libtool wrapper script.

## Fuzzing
```bash
cd $WORKDIR/libmodbus/tests
afl-fuzz -d -i $AFLNET/tutorials/modbus/in-modbus -o out-modbus -m none -N tcp://127.0.0.1/1502 -P MODBUS -D 10000 -q 3 -s 3 -E -K -R ./unit-test-server tcp
```
The seeds are short sessions that read and write the coils (`0x130`), discrete inputs (`0x1C4`), holding registers (`0x160`) and input registers (`0x108`) mapped by `unit-test-server`.

States are the function codes of the responses. Exception responses keep the exception code, e.g. `0x8302` (33538) is Read Holding Registers -> Illegal Data Address.

## Docker
```bash
# from the root of the AFLNet repository
docker build -f tutorials/modbus/Dockerfile -t aflnet-modbus .
docker run -it aflnet-modbus
# then run the afl-fuzz command above (with $WORKDIR/libmodbus = /opt/libmodbus)
```
