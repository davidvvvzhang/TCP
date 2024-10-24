#include <iostream>
#include <cstring>
#include <thread>
#include <fstream>
#include <algorithm>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#define PORT 8888
#define BUFFER_SIZE 1024

void receive_file(int socket, const std::string& filename);

void receive_messages(int sock) {
    char buffer[BUFFER_SIZE] = {0};
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        long valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread > 0) {
            buffer[valread] = '\0';  // 确保消息以 \0 结束
            std::string message(buffer);
            std::cout << message << std::endl;  // 打印接收到的消息
            
            // 处理被管理员踢出情况
            if (message.find("你已被管理员踢出") != std::string::npos) {
                std::cout << "你已被踢出服务器。" << std::endl;
                #ifdef _WIN32
                    closesocket(sock);
                #else
                    close(sock);
                #endif
                break;
            }
        } else if (valread == 0) {
            std::cout << "服务器已关闭连接。" << std::endl;
            #ifdef _WIN32
                closesocket(sock);
            #else
                close(sock);
            #endif
            break;
        }
    }
}

void handle_commands(int socket) {
    std::string command;

    while (true) {
        std::getline(std::cin, command); // 读取用户输入

        if (command.substr(0, 10) == "/sendfile ") {
            std::string filename = command.substr(10); // 提取文件名

            // 将文件命令发送给服务器
            send(socket, command.c_str(), command.length(), 0);

            // 调用 receive_file 接收文件
            receive_file(socket, filename);
        }

        if (command == "/exit") {
            break;
        }
    }
}

void receive_file(int socket, const std::string& filename) {
    char buffer[BUFFER_SIZE];
    std::streamsize total_received = 0;
    std::streamsize file_size = 0;

    recv(socket, reinterpret_cast<char*>(&file_size), sizeof(file_size), 0);

    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        std::cerr << "打开文件失败: " << filename << std::endl;
        return;
    }

    while (total_received < file_size) {
        std::streamsize bytes_received = recv(socket, buffer, std::min(static_cast<std::streamsize>(BUFFER_SIZE), file_size - total_received), 0);
        if (bytes_received <= 0) {
            std::cerr << "接收文件数据时出错或连接关闭。" << std::endl;
            break;
        }
        total_received += bytes_received;
        outfile.write(buffer, bytes_received);
    }

    outfile.close();
    std::cout << "文件接收成功: " << filename << std::endl;
}

int main() {
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "Winsock 初始化失败" << std::endl;
            return -1;
        }
    #endif

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket 创建失败" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "无效的地址" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "连接失败" << std::endl;
        return -1;
    }

    std::string username;
    std::cout << "请输入用户名: ";
    std::cin >> username;
    send(sock, username.c_str(), username.length(), 0);

    if (username == "admin") {
        std::string password;
        std::cout << "请输入管理员密码: ";
        std::cin >> password;
        send(sock, password.c_str(), password.length(), 0);
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::thread(receive_messages, sock).detach();

    while (true) {
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit") {
            send(sock, message.c_str(), message.length(), 0);
            break;
        }
        else if (message[0] == '@' && message.find(':') != std::string::npos) {
            send(sock, message.c_str(), message.length(), 0);
        }
        else if (message[0] == '#' && message.find(' ') != std::string::npos) {
            send(sock, message.c_str(), message.length(), 0);
        }
        else if (message.substr(0, 7) == "/create" || message.substr(0, 5) == "/join") {
            send(sock, message.c_str(), message.length(), 0);
        }
        else if (message.substr(0, 7) == "/leave ") {
            send(sock, message.c_str(), message.length(), 0);
        }
        else if (username == "admin") {
            send(sock, message.c_str(), message.length(), 0);
        }
        else if (message.substr(0, 10) == "/sendfile ") {
            std::string filename = message.substr(10);
            send(sock, message.c_str(), message.length(), 0);
            receive_file(sock, filename);
        }
        else {
            std::string error_msg = "不允许在大厅发送消息\n";
            std::cout << error_msg;
        }
    }

    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif

    return 0;
}
