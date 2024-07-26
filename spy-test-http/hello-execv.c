#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main() {
    char *target_path = "/home/czx/qemu-workspace/qemu-spy/build/qemu-system-arm";
    char *args[] = {
        target_path,
        "-m",
        "256M",
        "-M",
        "romulus-bmc",
        "-drive",
        "file=/home/czx/openbmc-workspace/openbmc/build/romulus/tmp/deploy/images/romulus/obmc-phosphor-image-romulus.static.mtd,if=mtd,format=raw",
        "-net",
        "nic",
        "-net",
        "user,hostfwd=:127.0.0.1:2222-:22,hostfwd=:127.0.0.1:2443-:443,hostfwd=:127.0.0.1:18080-:8080,hostfwd=udp:127.0.0.1:2623-:623,hostname=qemu",
        "-d",
        "plugin",
        "-plugin",
        "/home/czx/qemu-workspace/qemu-spy/plugin_spy/build/libaflspy.so",
        "-D",
        "qemu_log.txt",
        "-nographic",
        NULL
    };

    int pid = fork();
    if (pid == 0) {
        // Child process
        execv(target_path, args);
        // execv only returns if an error occurs
        perror("execv");
    } else if (pid > 0) {
        // Parent process
        // Optionally, you can add code here to perform actions in the parent process
        // while the child process is running.
        wait(NULL);
        printf("Child process finished\n");
    } else {
        // Error occurred
        perror("fork");
        return 1;
    }

    return 0;
}