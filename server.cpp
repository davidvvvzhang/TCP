//  server.cpp

#include <iostream>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket
#else
    #include <sys/socket.h> // 包含创建和操作套接字的函数，如socket()、bind()、listen()等
    #include <netinet/in.h> // 定义了地址族AF_INET和端口的结构体sockaddr_in
    #include <arpa/inet.h> // 用于处理地址转换的函数，如inet_pton()
    #include <unistd.h> // 包含系统调用，如close()，用于关闭套接字
#endif

#define PORT 8888 // 定义端口号8888，服务器将使用这个端口号监听客户端连接
#define BUFFER_SIZE 1024 // 定义缓冲区大小为1024字节，用于存储接收到的数据

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return -1;
    }
#endif
    
    // 创建服务器端的套接字
    int server_fd, new_socket; // server_fd是服务器的套接字描述符，用于创建套接字，new_socket用于接收客户端的连接
    struct sockaddr_in address; // 定义了一个sockaddr_in结构体，用于存储服务器的IP地址和端口信息
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0}; // 用于存储从客户端接收到的数据

    // 创建套接字
    // socket()：创建一个套接字，使用AF_INET表示IPv4协议，SOCK_STREAM表示使用TCP协议。返回值是一个套接字描述符，如果返回0表示套接字创建失败
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // 设置地址结构
    address.sin_family = AF_INET; // 设置地址族为IPv4
    address.sin_addr.s_addr = INADDR_ANY; // 表示服务器将绑定到本机的所有可用IP地址
    address.sin_port = htons(PORT); // 将端口号8888转换为网络字节序（大端模式），便于跨网络传输

    // 绑定套接字
    // bind()：将服务器套接字与IP地址和端口号绑定，(struct sockaddr*)&address指定绑定的地址信息。返回值小于0表示绑定失败
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(server_fd);
        return -1;
    }

    // 监听端口
    // listen()：将服务器套接字设置为监听状态，3是最大等待连接队列长度。返回值小于0表示监听失败
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "等待连接..." << std::endl;

    // 接受客户端连接
    // accept()：接收客户端的连接，new_socket是为每个客户端创建的新套接字，服务器通过这个套接字与客户端通信。如果返回值小于0表示接受连接失败
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        std::cerr << "Accept failed" << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "客户端连接成功，等待消息..." << std::endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE); // 将缓冲区清空，防止读取到之前的旧数据
        long valread = read(new_socket, buffer, BUFFER_SIZE); // 从客户端读取数据，存储在buffer中。返回值为读取到的数据长度
        if (valread > 0) {
            std::cout << "客户端: " << buffer << std::endl;

            // 发送回复
            std::string response = "服务器已收到: " + std::string(buffer);
            send(new_socket, response.c_str(), response.length(), 0); // 将服务器的回复发送给客户端
        }
    }

    // 关闭套接字
    // 关闭客户端和服务器的套接字，释放资源
    close(new_socket);
    close(server_fd);
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}
