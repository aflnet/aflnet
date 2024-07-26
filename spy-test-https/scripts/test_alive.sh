# Inform aflspy to set the context of the target process based on this request
# --insecure is used to ignore the certificate verification
curl https://127.0.0.1:18084 --insecure --max-time 2 > /dev/null 2>&1
