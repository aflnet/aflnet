# gdbserver 127.0.0.1:1234 \
../afl-fuzz -d -i ./inputs -o ./outputs -P HTTP -N tcp://127.0.0.1/8080 -m 2048M \
    -D 100000 -q 3 -s 3 -W 5 -w 500000 -t 500000 -K -R -Q  -- ./hello-crow