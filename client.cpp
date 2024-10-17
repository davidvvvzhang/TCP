//  client.cpp

#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
    #define inet_pton(a, b, c) (*(c) = inet_addr(b))  // 使用inet_addr替代inet_pton
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#define PORT 8888
#define BUFFER_SIZE 1024

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }
#endif
    
    int sock = 0; // 客户端的套接字描述符，用于创建套接字
    struct sockaddr_in serv_addr; // 定义服务器地址的结构体，存储服务器的IP地址和端口号
    char buffer[BUFFER_SIZE] = {0}; // 用于存储服务器返回的数据

    // 创建套接字
    // socket()：创建一个套接字，和服务器端的用法相同
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // 设置服务器地址
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将服务器地址转换为二进制形式
    // inet_pton()：将点分十进制的IP地址（例如"127.0.0.1"）转换为二进制的网络字节序，便于进行网络传输
#ifdef _WIN32
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid address" << std::endl;
        return -1;
    }
#else
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return -1;
    }
#endif

    // 连接服务器
    // connect()：连接服务器，sock是客户端的套接字，serv_addr是服务器的地址结构体。返回值小于0表示连接失败
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }

    std::cout << "连接到服务器，输入消息并发送：" << std::endl;

    while (true) {
        std::string input;
        std::cout << "你: ";
        std::getline(std::cin, input); // 获取用户的输入，用于发送消息到服务器

        // 发送消息
        send(sock, input.c_str(), input.length(), 0); // 将输入的消息发送给服务器，参数包括套接字、消息内容、消息长度

        // 接收服务器的回复
        memset(buffer, 0, BUFFER_SIZE);
        long valread = read(sock, buffer, BUFFER_SIZE); // 从服务器接收数据，并输出服务器的回复
        if (valread > 0) {
            std::cout << "服务器: " << buffer << std::endl;
        }

        // 退出条件
        if (input == "exit") {
            std::cout << "断开连接" << std::endl;
            break;
        }
    }

    // 关闭套接字
    close(sock);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
