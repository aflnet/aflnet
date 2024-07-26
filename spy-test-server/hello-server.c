#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!"

int serverSocket;

void handleCtrlC(int signal) {
    printf("Ctrl+C received. Exiting...\n");
    close(serverSocket);
    // Perform any necessary cleanup or shutdown operations here
    exit(0);
}

int main() {
    // atexit(close_server);
    signal(SIGINT, handleCtrlC);
    int clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientLength;
    char buffer[BUFFER_SIZE];

    while (1) {
        // 创建套接字
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            perror("Failed to create socket");
            exit(1);
        }

        // 设置服务器地址
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(8080);

        // 绑定套接字到指定地址和端口
        if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
            perror("Failed to bind socket");
            exit(1);
        }

        // 监听来自客户端的连接
        if (listen(serverSocket, 500) < 0) {
            perror("Failed to listen on socket");
            exit(1);
        }

        printf("Server started. Listening on port 8080...\n");
        usleep(1000);
        // 接受客户端连接
        clientLength = sizeof(clientAddress);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLength);
        if (clientSocket < 0) {
            perror("Failed to accept client connection");
            exit(1);
        } else {
            printf("Accepted client connection\n");
        }

        // 读取请求
        memset(buffer, 0, BUFFER_SIZE);
        if (read(clientSocket, buffer, BUFFER_SIZE - 1) < 0) {
            perror("Failed to read from socket");
            exit(1);
        } else {
            printf("Received request\n");
        }

        // 发送响应
        if (write(clientSocket, RESPONSE, strlen(RESPONSE)) < 0) {
            perror("Failed to write to socket");
            exit(1);
        } else {
            printf("Sent response\n");
        }

        // 关闭客户端连接
        close(clientSocket);
        printf("Closed client connection\n");
        // 关闭服务器套接字
        close(serverSocket);
    }

    

    return 0;
}