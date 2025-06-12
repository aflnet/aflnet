# Inform aflspy to set the context of the agent process based on this request
curl 127.0.0.1:14817 --max-time 60 > /dev/null 2>&1

# Disable ASLR
CMD="echo 0 > /proc/sys/kernel/randomize_va_space"
curl 127.0.0.1:14817/execute \
    -d "{\"cmd\": \"$CMD\"}" > /dev/null 2>&1