#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

#define PORT 8080
#define BUFFER_SIZE 1024

void receive_messages(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        long valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread > 0) {
            std::cout << buffer;
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return -1;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    // 输入用户名
    std::string username;
    std::cout << "请输入用户名: ";
    std::getline(std::cin, username);
    send(sock, username.c_str(), username.length(), 0);

    // 接收密码（如果是管理员）
    if (username == "admin") {
        recv(sock, buffer, BUFFER_SIZE, 0);
        std::cout << buffer;
        memset(buffer, 0, BUFFER_SIZE);
        std::string password;
        std::getline(std::cin, password);
        send(sock, password.c_str(), password.length(), 0);
    }

    // 开启接收消息的线程
    std::thread(receive_messages, sock).detach();

    // 功能提示
    std::cout << "功能提示：\n";
    std::cout << "1. 输入 '#群聊名 消息' 发送群聊消息\n";
    std::cout << "2. 输入 '@用户名: 消息' 发送私聊消息\n";
    std::cout << "3. 输入 'exit' 退出聊天\n";

    std::string message;
    while (true) {
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        send(sock, message.c_str(), message.length(), 0);
    }

    close(sock);
    return 0;
}

