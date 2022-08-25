# Tutorial - Using network namespaces

A network namespace is a logical copy of the network stack from the host system. Each namespace has its own IP addresses, network interfaces, routing tables, and so forth.
You can ask AFLNet to run your target server inside a such namespace to isolate it from the rest of the system and operate completly independently.


## Step-0. Network namespace setup

There is a good [article](ifeanyi.co/posts/linux-namespaces-part-4/) by [Ifeanyi Ubah](https://github.com/iffyio) explaining how to work with linux namespaces.
Here's the short version:

```
┌────────────────────────────────────────────┐
│                                            │
│ ┌──────────────────────┐                   │
│ │                      │                   │
│ │                ┌─────┤             ┌─────┤
│ │                │     │             │     │
│ │                │veth1│◄───────────►│veth0│
│ │                │     │             │     │
│ │                └─────┤             └─────┤
│ │              nspce   │                   │
│ └──────────────────────┘                   │
│                                            │
│                              Host namespace│
└────────────────────────────────────────────┘
```

```bash
# create network namespace
sudo ip netns add nspce
# Create a veth pair
sudo ip link add veth0 type veth peer name veth1
# Move veth1 to the new namespace
sudo ip link set veth1 netns nspce
# Bring loopback interface up
sudo ip netns exec nscpe ip link set dev lo up
# Assign ip to interface in the new namespace
sudo ip netns exec nscpe ip addr add 10.0.0.2/24 dev veth1
sudo ip netns exec nscpe ip link set dev veth1 up
# Assign ip to interface in the host namespace
sudo ip addr add 10.0.0.1/24 dev veth0
sudo ip link set dev veth0 up
```
Now we should be able to do an inter-namespace communication between two processes running in both namespaces.

## Step-1. Fuzzing

That's all preparations required. Fuzz servers as usual.

You can use `-e <netns name>` option to specify network namespace to run the server in.

Considering the abovementioned setup:

```bash
./afl-fuzz -i ~/paht/to/seed -o ~/path/to/fuzzer/findings -N tcp://10.0.0.2/port -P protocol -e nspce ~/path/to/server args
```
