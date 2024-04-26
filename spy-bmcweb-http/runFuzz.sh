KERN=/home/czx/openbmc-workspace/openbmc/build/romulus/tmp/deploy/images/romulus
QEMUSPY=/home/czx/qemu-workspace/qemu-spy
AFLSPY=$QEMUSPY/plugin_spy

# gdbserver 127.0.0.1:1234 \
../afl-fuzz -i ./inputs -o ./outputs -P HTTP -N tcp://127.0.0.1/2443 -m 4096M -QQ \
     -d -q 3 -s 3  -R -W 5 -w 50000 -t 50000\
     -- \
     $QEMUSPY/build/qemu-system-arm \
    -m 256 -machine romulus-bmc \
    -drive file=$KERN/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw\
    -net nic \
    -net user,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:14817-:4817,hostname=qemu \
    -d plugin \
    -plugin $AFLSPY/build/libaflspy.so \
    -D qemu_log.txt \
    -nographic \



