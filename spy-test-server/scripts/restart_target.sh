TARGET_PROC=hello-server
TARGET_SERVICE=hello-server.service
CMD="pgrep $TARGET_PROC && (pgrep $TARGET_PROC | xargs kill -9); \
    systemctl reset-failed && \
    systemctl restart $TARGET_SERVICE"

curl 127.0.0.1:14817/execute \
    -d "{\"cmd\": \"$CMD\"}" > /dev/null 2>&1
    # --max-time 2

# (1)Use pgrep to check if the process is running before killing it
# (2)Use systemctl reset-failed to reset the failed status of the service[IMPORTANT]
# (3)Use systemctl start or restart to restart the service, no obvious difference between them
# (4)We Should only use one request instead of sending multiple requests for each command,
#    otherwise the second request will influence the target_ctx, setting the value of target_ctx
#    to context of spy-agent process wrongly, because the kill command have already killed the 
#    target process and the target_ctx have been reset and ready to update.
# (5)Put the comments below in the file to avoid imcomplete reading of the commands due to excessive comments