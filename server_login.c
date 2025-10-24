/**
 * @file server_login.c
 * @brief 服务器登录认证模块实现文件
 * @details 实现用户注册和登录功能
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mysql/mysql.h>
#include "database.h"
#include "server_login.h"

/**
 * @brief 用户注册功能
 * @param client_socket 客户端socket文件描述符
 * @param msg 消息结构体，包含用户名和密码
 * @param con MySQL数据库连接句柄
 * @note 注册成功返回cmd=1001，失败返回cmd=-1
 */
void Reg(int client_socket, struct Msg msg, MYSQL *con) {
    printf ("%s 进行注册\n", msg.name);
    
    // 将用户信息保存到数据库
    if(data_insert(con, msg.name, msg.password)){
        msg.cmd = 1001;  // 注册成功
        // 注册成功后自动设置为在线状态
        data_insert_online(con, msg.name, "online");
    } else {
        msg.cmd = -1;    // 数据库中已有该账号
    }
    
    // 将注册结果返回给客户端
    write (client_socket, &msg, sizeof(msg));
}

/**
 * @brief 用户登录功能
 * @param client_socket 客户端socket文件描述符
 * @param msg 消息结构体，包含用户名和密码
 * @param con MySQL数据库连接句柄
 * @note 登录成功返回cmd=1002，失败返回cmd=-1
 */
void Entry(int client_socket, struct Msg msg, MYSQL *con)
{
    // 检测数据库中是否有该账号且密码是否正确
    if(!data_judge(con, msg.name, msg.password)){
        // 登录失败
        msg.cmd = -1;
    } else {
        // 登录成功，更新在线状态
        data_delete_online(con, msg.name);
        data_insert_online(con, msg.name, "online");
        msg.cmd = 1002;  // 登录成功，进入聊天室
    }
    
    // 将登录结果返回给客户端
    write (client_socket, &msg, sizeof(msg));
}