## Download and compile mosquitto v2.0.7  
apt-get install libssl-dev libwebsockets-dev uuid-dev docbook-xsl docbook xsltproc  
cd $WORKDIR  
git clone https://github.com/eclipse/mosquitto.git  
cd mosquitto  
git checkout 2665705  
export AFL_USE_ASAN=1  
CFLAGS="-g -O0 -fsanitize=address -fno-omit-frame-pointer" LDFLAGS="-g -O0 -fsanitize=address -fno-omit-frame-pointer"  CC=afl-gcc make clean all WITH_TLS=no WITH_TLS_PSK:=no WITH_STATIC_LIBRARIES=yes WITH_DOCS=no WITH_CJSON=no WITH_EPOLL:=no  
## Fuzzing  
cd $WORKDIR/mosquitto  
afl-fuzz -d -i $AFLNET/tutorials/mosquitto/in-mqtt -o ./out-mqtt -m none -N tcp://127.0.0.1/1883 -P MQTT -D 10000 -q 3 -s 3 -E -K -R ./src/mosquitto  
## In addition: Seeds generation  
You can refer to the script in this [repository](https://github.com/B1y0nd/mosquitto_seed) to generate more seeds.  
Support mqtt v3.1.1 and v5.  
