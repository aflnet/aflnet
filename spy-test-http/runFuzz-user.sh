# gdbserver 127.0.0.1:1234 \
../afl-fuzz -d -i ./inputs -o ./outputs -P HTTP -N tcp://127.0.0.1/8080 -m none \
    -D 10000 -K -q 3 -s 3 -t 10000 -R -Q   -- ./hello-crow