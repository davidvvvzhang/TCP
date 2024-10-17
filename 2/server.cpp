#include <iostream>
#include <string>
#include <map>
#include <set>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#define PORT 8080
#define BUFFER_SIZE 1024

std::map<std::string, int> user_sockets;
std::map<std::string, bool> user_roles;  // true 表示管理员, false 表示普通用户

// 广播当前用户列表
void broadcast_user_list() {
    std::string user_list = "当前在线用户: ";
    for (const auto& pair : user_sockets) {
        user_list += pair.first + " ";
    }
    user_list += "\n";
    for (const auto& pair : user_sockets) {
        send(pair.second, user_list.c_str(), user_list.length(), 0);
    }
}

// 处理客户端连接
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    std::string username;
    bool is_admin = false;

    // 接收用户名并区分管理员和普通用户
    long valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (valread > 0) {
        username = buffer;
        if (username == "admin") {
            // 要求输入密码
            send(client_socket, "请输入密码: ", 20, 0);
            memset(buffer, 0, BUFFER_SIZE);
            valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (valread > 0 && std::string(buffer) == "1234") {
                is_admin = true;
                user_roles[username] = true;
                send(client_socket, "管理员登录成功！\n", 40, 0);
            } else {
                send(client_socket, "密码错误，断开连接\n", 40, 0);
                close(client_socket);
                return;
            }
        } else {
            user_roles[username] = false;
            send(client_socket, ("欢迎, " + username + "!\n").c_str(), 40, 0);
        }
        user_sockets[username] = client_socket;
        std::cout << username << " 已连接" << std::endl; // 新增连接信息
        broadcast_user_list();
    }

    // 功能提示
    std::string prompt_msg = is_admin
        ? "功能提示：\n1. 输入 '#群聊名 消息' 发送群聊消息\n2. 输入 '@用户名: 消息' 发送私聊消息\n3. 输入 'exit' 退出聊天\n4. 输入 'kick 用户名' 强制用户下线（仅管理员适用）\n"
        : "功能提示：\n1. 输入 '#群聊名 消息' 发送群聊消息\n2. 输入 '@用户名: 消息' 发送私聊消息\n3. 输入 'exit' 退出聊天\n";
    send(client_socket, prompt_msg.c_str(), prompt_msg.length(), 0);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (valread <= 0) {
            std::cout << username << " 断开连接" << std::endl;
            close(client_socket);
            user_sockets.erase(username);
            user_roles.erase(username);
            broadcast_user_list();
            break;
        }

        std::string message = buffer;

        // 群聊功能
        if (message[0] == '#') {
            std::string group_name = message.substr(1, message.find(' ') - 1);
            std::string group_msg = message.substr(message.find(' ') + 1);
            // 可以在这里处理群聊的逻辑
            send(client_socket, "群聊功能尚未实现\n", 40, 0);
        }
        // 私聊功能
        else if (message[0] == '@') {
            size_t colon_pos = message.find(':');
            if (colon_pos != std::string::npos) {
                std::string target_user = message.substr(1, colon_pos - 1);
                std::string private_msg = message.substr(colon_pos + 2);
                if (user_sockets.find(target_user) != user_sockets.end()) {
                    std::string full_message = username + " (私聊): " + private_msg + "\n";
                    send(user_sockets[target_user], full_message.c_str(), full_message.length(), 0);
                    send(client_socket, ("(你向 " + target_user + " 发送私聊消息): " + private_msg + "\n").c_str(), private_msg.length(), 0);
                } else {
                    send(client_socket, "用户不在线或不存在\n", 40, 0);
                }
            }
        }
        // 管理员踢出用户功能
        else if (is_admin && message.substr(0, 4) == "kick") {
            std::string target_user = message.substr(5);
            if (user_sockets.find(target_user) != user_sockets.end()) {
                send(user_sockets[target_user], "你已被管理员踢出\n", 40, 0);
                close(user_sockets[target_user]);
                user_sockets.erase(target_user);
                user_roles.erase(target_user);
                std::cout << target_user << " 已被管理员踢出" << std::endl; // 打印踢出用户信息
                broadcast_user_list();
            } else {
                send(client_socket, "该用户不存在或不在线\n", 40, 0);
            }
        }
        // 禁止大厅消息
        else {
            send(client_socket, "不允许在大厅发送消息\n", 40, 0);
        }
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 创建 socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket 创建失败" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定端口
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "绑定失败" << std::endl;
        return -1;
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        std::cerr << "监听失败" << std::endl;
        return -1;
    }

    std::cout << "服务器已启动，等待连接..." << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "连接失败" << std::endl;
            return -1;
        }

        // 创建新线程处理客户端
        std::thread(handle_client, new_socket).detach();
    }

    return 0;
}
