# gdbserver 127.0.0.1:1234 \
../afl-fuzz -d -i ./inputs -o ./outputs -P HTTP -N tcp://127.0.0.1/8080 -m 2048M \
    -D 100000 -K -q 3 -s 3 -t 50000  -R -- ./hello-server