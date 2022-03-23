This is quick instructin for using remote only mode of AFLNET
For those that familar with AFL structure, this is basically AFL dumb_mode, only somewhat guided by AFLNET algorithm

Use \-r for net only mode, example for simple modbustcp server:

```
afl-fuzz -d -i in -o out -N tcp://127.0.0.1 -P MODBUSTCP -D 10000 -q 3 -s 3 -E -R -r -W 5 -e remote_only_runserver.sh
```

For quick testing, copy the content of this directory to aflnet parent directory then run remote_only_fuzz.sh as root.

Mind other options:

- \-e: Server monitoring script: this script is responsible for monitor(restarting?) target server each fuzz cycle. Define its behavior depend on what you are going to fuzz. Its usually required unless your server can self-restart.
- \-D, \-W, \-w: Since afl can no longer monitor binary state. Manage your timeout so that server have time to restart in case of crashes. Else expects errors and false positives.

Problems need future improvement:

- Current server monitoring script / state reset script using library call to system(). This cause lots of fork and slow down the fuzzer about 5x.