// server.cpp
#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <algorithm>
#include <string>

#ifdef _WIN32  // Windows
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#define close closesocket
#else  // Linux/Mac
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define PORT 8888
#define BUFFER_SIZE 1024

std::map<std::string, int> user_sockets;  // 存储用户名和对应的socket
struct Group {
    std::set<int> members;         // 群聊成员
    std::string admin;             // 群聊管理员
};

std::map<std::string, Group> groups;  // 存储群聊名与群聊信息

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    std::string username;
    bool is_admin = false;

    // 接收用户名
    long valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    username = buffer;
    username = username.substr(0, username.find('\n'));  // 去除换行符
    username.erase(0, username.find_first_not_of(" "));  // 去掉前导空格
    username.erase(username.find_last_not_of(" ") + 1);  // 去掉尾随空格

    // 检查是否为管理员
    if (username == "admin") {
        char buffer[BUFFER_SIZE];
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        std::string password = buffer;
        password = password.substr(0, password.find('\n'));  // 去除换行符
        password.erase(0, password.find_first_not_of(" "));  // 去掉前导空格
        password.erase(password.find_last_not_of(" ") + 1);  // 去掉尾随空格
        
        if (password == "1234") {
            is_admin = true;  // 标记为管理员
            send(client_socket, "管理员登录成功\n", 21, 0);
        } else {
            send(client_socket, "管理员密码错误\n", 22, 0);
        }
    }

    user_sockets[username] = client_socket;

    std::string welcome_msg = "欢迎, " + username + "!";
    send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);
    
    // 发送功能提示
    std::string prompt_msg = "功能提示：\n";
    prompt_msg += "1. 输入 '@用户名: 消息' 发送私聊消息\n";
    prompt_msg += "2. 输入 '/create 群聊名' 创建群聊\n";
    prompt_msg += "3. 输入 '#群聊名 消息' 发送群聊消息\n";
    prompt_msg += "4. 输入 '/join 群聊名' 加入群聊\n";
    prompt_msg += "5. 输入 '/leave 群聊名' 退出群聊\n";
    prompt_msg += "6. 输入 'exit' 退出聊天\n";
    prompt_msg += "7. 输入 '/sendfile 文件名' 发送文件\n";
    if (is_admin) {
        prompt_msg += "8. 输入 'kick 用户名' 强制用户下线（仅管理员适用）\n";
    }
    send(client_socket, prompt_msg.c_str(), prompt_msg.length(), 0);

    // 广播在线用户列表
    std::string online_users = "当前在线用户: ";
    for (const auto& user : user_sockets) {
        online_users += user.first + " ";
    }
    for (const auto& user : user_sockets) {
        send(user.second, online_users.c_str(), online_users.length(), 0);
    }

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            std::cout << username << " 断开连接" << std::endl;
            close(client_socket);
            user_sockets.erase(username);
            break;
        }

        std::string message = buffer;
        message = message.substr(0, message.find('\n'));  // 去除换行符
        
        // 文件传输功能
        if (message.substr(0, 10) == "/sendfile ") {
            std::string filename = message.substr(10);
            filename.erase(0, filename.find_first_not_of(" "));  // 去掉前导空格
            filename.erase(filename.find_last_not_of(" ") + 1);  // 去掉尾随空格

            // 读取文件内容
            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                send(client_socket, "文件打开失败\n", 15, 0);
                continue;
            }

            // 发送文件大小
            file.seekg(0, std::ios::end);
            std::streamsize file_size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::string size_msg = std::to_string(file_size);
            send(client_socket, size_msg.c_str(), size_msg.length(), 0);

            // 发送文件内容
            char file_buffer[BUFFER_SIZE];
            while (file.read(file_buffer, sizeof(file_buffer))) {
                // 发送数据并确保发送成功
                ssize_t bytes_sent;
                do {
                    bytes_sent = send(client_socket, file_buffer, file.gcount(), 0);
                } while (bytes_sent < 0); // 如果发送失败则重试
            }
            if (file.gcount() > 0) {
                ssize_t bytes_sent;
                do {
                    bytes_sent = send(client_socket, file_buffer, file.gcount(), 0);
                } while (bytes_sent < 0); // 如果发送失败则重试
            }
            file.close();
            send(client_socket, "传输完成\n", 15, 0);
        }
        
        // 群聊功能
        if (message.substr(0, 8) == "/create ") {
            std::string group_name = message.substr(8); // 提取群聊名
            group_name.erase(0, group_name.find_first_not_of(" "));  // 去掉前导空格
            group_name.erase(group_name.find_last_not_of(" ") + 1);  // 去掉尾随空格

            // 创建群聊
            if (groups.find(group_name) == groups.end()) {
                Group new_group;
                new_group.members.insert(client_socket);
                new_group.admin = username;  // 创建者成为管理员
                groups[group_name] = new_group;

                std::string success_msg = "群聊 " + group_name + " 创建成功，并已加入，您是管理员\n";
                send(client_socket, success_msg.c_str(), success_msg.length(), 0);
            }
        }
        // 加入群聊功能
        else if (message.substr(0, 6) == "/join ") {
            std::string group_name = message.substr(6);  // 提取群聊名
            group_name.erase(0, group_name.find_first_not_of(" "));  // 去掉前导空格
            group_name.erase(group_name.find_last_not_of(" ") + 1);  // 去掉尾随空格

            if (groups.find(group_name) != groups.end()) {
                groups[group_name].members.insert(client_socket);  // 将用户加入群聊
                std::string success_msg = "成功加入群聊 " + group_name + "\n";
                send(client_socket, success_msg.c_str(), success_msg.length(), 0);
            } else {
                std::string error_msg = "群聊 " + group_name + " 不存在\n";
                send(client_socket, error_msg.c_str(), error_msg.length(), 0);
            }
        }
        // 退出群聊
        else if (message.substr(0, 7) == "/leave ") {
            std::string group_name = message.substr(7);

            if (groups.find(group_name) != groups.end()) {
                Group& group = groups[group_name];
                if (group.members.find(client_socket) != group.members.end()) {
                    group.members.erase(client_socket);  // 移除成员
                    std::string exit_msg = "你已退出群聊 " + group_name + "\n";
                    send(client_socket, exit_msg.c_str(), exit_msg.length(), 0);
                } else {
                    send(client_socket, "你不在该群聊中\n", 18, 0);
                }
            } else {
                send(client_socket, "群聊不存在\n", 15, 0);
            }
        }
        // 群聊消息广播
        else if (message[0] == '#') {
            std::string group_name = message.substr(1, message.find(' ') - 1);
            std::string group_msg = message.substr(message.find(' ') + 1);

            if (groups.find(group_name) != groups.end()) {
                Group& group = groups[group_name];
                for (int sock : group.members) {
                    std::string formatted_msg = username + " (群聊 " + group_name + "): " + group_msg;
                    send(sock, formatted_msg.c_str(), formatted_msg.length(), 0);
                }
            }
        }
        // 私聊功能
        else if (message[0] == '@') {
            std::string target_user = message.substr(1, message.find(':') - 1);
            std::string private_msg = message.substr(message.find(':') + 2);

            if (user_sockets.find(target_user) != user_sockets.end()) {
                std::string formatted_msg = username + " (私聊): " + private_msg;
                send(user_sockets[target_user], formatted_msg.c_str(), formatted_msg.length(), 0);
            } else {
                std::string error_msg = "用户 " + target_user + " 不在线或不存在";
                send(client_socket, error_msg.c_str(), error_msg.length(), 0);
            }
        }
        // 处理踢出用户
        else if (is_admin && message.substr(0, 5) == "kick ") {
            std::cout << "管理员尝试踢出用户: " << message << std::endl;  // 调试信息
            std::string target_user = message.substr(5);
            target_user.erase(0, target_user.find_first_not_of(" "));  // 去掉前导空格
            target_user.erase(target_user.find_last_not_of(" ") + 1);  // 去掉尾随空格
            std::cout << "目标用户: " << target_user << std::endl;  // 调试信息
            if (user_sockets.find(target_user) != user_sockets.end()) {
                std::cout << "找到了用户: " << target_user << std::endl;  // 调试信息
                send(user_sockets[target_user], "你已被管理员踢出\n", 18, 0);
                close(user_sockets[target_user]);
                user_sockets.erase(target_user);

                // 更新在线用户列表
                std::string online_users = "当前在线用户: ";
                for (const auto& user : user_sockets) {
                    online_users += user.first + " ";
                }
                for (const auto& user : user_sockets) {
                    send(user.second, online_users.c_str(), online_users.length(), 0);
                }
            } else {
                std::cout << "用户 " << target_user << " 不存在或不在线" << std::endl;  // 调试信息
                std::string error_msg = "用户 " + target_user + " 不存在";
                send(client_socket, error_msg.c_str(), error_msg.length(), 0);
            }
        }
        // 处理退出
        else if (message == "exit") {
            std::string exit_msg = username + " 退出了聊天";
            send(client_socket, exit_msg.c_str(), exit_msg.length(), 0);
            close(client_socket);
            user_sockets.erase(username);
            break;
        }
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
#endif

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // 创建套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // 绑定端口
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt SO_REUSEADDR failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    // 侦听
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "服务器已启动，等待连接..." << std::endl;

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        std::thread client_thread(handle_client, new_socket);
        client_thread.detach();  // 分离线程
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
